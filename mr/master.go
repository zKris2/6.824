package mr

import (
	"fmt"
	"log"
	"net"
	"net/http"
	"net/rpc"
	"os"
	"strconv"
	"strings"
	"sync"
	"time"
)

var mutex sync.Mutex

// 任务类型
type TaskType int

// 任务状态
type TaskState int

// MapReduce执行到的阶段
type Phase int

// 枚举任务类型
const (
	MapTask TaskType = iota
	ReduceTask
)

// 枚举任务状态
const (
	Working TaskState = iota // 任务正在进行
	Waiting                  // 任务正在等待执行
	Finshed                  // 任务已经完成
)

// 枚举MapReduce执行阶段
const (
	MapPhase    Phase = iota // Map阶段
	ReducePhase              // Reduce阶段
	AllDone                  // 全部完成
)

type Task struct {
	TaskId     int       // 任务id（唯一），通过Master生成，从0开始
	TaskType   TaskType  // 任务类型
	TaskState  TaskState // 任务状态
	InputFile  []string  // 输入文件名切片，map情况切片里面就一个分配给worker的文件,reduce情况下是worker需要选取的一些需要聚合到一起的中间文件
	ReducerNum int       // 传入的reducer的数量，用于hash
	ReducerKth int       // 表明reducer的序号（最终输出文件命名时需要），不等于taskId，只有为reduce任务时才有意义
	StartTime  time.Time // 任务被分配给worker后开始执行的时间（用于crash判断）
}

type Master struct {
	CurrentPhase      Phase         // 当前MapReduce处于什么阶段
	TaskIdForGen      int           // 用于生成任务唯一TaskId
	MapTaskChannel    chan *Task    // 管理待执行的Map任务，使用chan保证并发安全
	ReduceTaskChannel chan *Task    // 管理待执行的Reduce任务，使用chan保证并发安全
	TaskMap           map[int]*Task // 记录所有Task信息，方便Master管理任务，获得任务进行情况（当任务分配后更新）
	MapperNum         int           // mapper的数量，由于每个文件分给一个mapper做，因此就等于输入文件的个数
	ReducerNum        int           // 传入的reducer的数量，用于hash
}

// Master在判断当前阶段工作已完成后转入下一阶段
func (m *Master) toNextPhase() {
	switch m.CurrentPhase {
	case MapPhase:
		m.CurrentPhase = ReducePhase // 若当前为Map阶段则下一阶段为Reduce阶段
		m.MakeReduceTask()           // Master开始生成reduce任务
	case ReducePhase:
		m.CurrentPhase = AllDone // 若当前为Reduce阶段则下一阶段为AllDone阶段
	}
}

// Master检查reduce阶段任务是否全部完成，若是则工作完成转入AllDone阶段，准备结束程序
func (m *Master) checkReduceTaskDone() bool {
	var (
		reduceDoneNum   = 0
		reduceUnDoneNum = 0
	)

	// 遍历TaskMap
	for _, v := range m.TaskMap {
		if v.TaskType == ReduceTask { // reduce任务
			if v.TaskState == Finshed {
				reduceDoneNum++
			} else {
				reduceUnDoneNum++
			}
		}
	}

	if reduceDoneNum == m.ReducerNum && reduceUnDoneNum == 0 {
		return true
	}

	return false
}

// Master检查map阶段任务是否全部完成，若是则转入reduce阶段
func (m *Master) checkMapTaskDone() bool {
	var (
		mapDoneNum   = 0
		mapUnDoneNum = 0
	)

	// 遍历TaskMap
	for _, v := range m.TaskMap {
		if v.TaskType == MapTask { // map任务
			if v.TaskState == Finshed {
				mapDoneNum++
			} else {
				mapUnDoneNum++
			}
		}
	}

	if mapDoneNum == m.MapperNum && mapUnDoneNum == 0 { // 当map阶段任务全做完才开始reduce阶段
		return true
	}

	return false
}

// Master给Worker分配任务的函数
func (m *Master) AssignTask(args *TaskRequest, reply *TaskResponse) error {
	// 分配任务需要上锁，防止多个worker竞争，保证并发安全，并用defer回退解锁
	mutex.Lock()
	defer mutex.Unlock()
	fmt.Println("Master gets a request from worker:")

	// Master根据当前执行阶段来判断执行什么操作
	switch m.CurrentPhase {
	case MapPhase:
		// 先检查MapTaskChannel里是否还有未分配的任务
		if len(m.MapTaskChannel) > 0 { // 还有未分配的map任务
			taskp := <-m.MapTaskChannel     // 取出的是指针
			if taskp.TaskState == Waiting { // 若此map任务正等待执行
				//*reply = TaskReply{*taskp}
				reply.Answer = TaskGetted // 请求到了map任务
				reply.Task = *taskp
				taskp.TaskState = Working          // 将任务状态改为Working，代表已被分配给了worker
				taskp.StartTime = time.Now()       // 更新任务开始执行的时间
				m.TaskMap[(*taskp).TaskId] = taskp // 分配任务的同时将其加入TaskMap
				fmt.Printf("Task[%d] has been assigned.\n", taskp.TaskId)
			}
		} else { // 没有未分配的map任务，检查map是否都执行完毕了，若是则Master转向reduce阶段
			reply.Answer = WaitPlz // 本次请求暂无任务分配
			// 检查map是否都执行完毕了
			if m.checkMapTaskDone() { // map阶段任务都执行完了
				m.toNextPhase() // 转向下一阶段
			}
			return nil
		}
	case ReducePhase:
		// 先检查ReduceTaskChannel里是否还有未分配的任务
		if len(m.ReduceTaskChannel) > 0 { // 还有未分配的reduce任务
			taskp := <-m.ReduceTaskChannel  // 取出的是指针
			if taskp.TaskState == Waiting { // 若此reduce任务正等待执行
				//*reply = TaskReply{*taskp}
				reply.Answer = TaskGetted // 请求到了reduce任务
				reply.Task = *taskp
				taskp.TaskState = Working          // 将任务状态改为Working，代表已被分配给了worker
				taskp.StartTime = time.Now()       // 更新任务开始执行的时间
				m.TaskMap[(*taskp).TaskId] = taskp // 分配任务的同时将其加入TaskMap
				fmt.Printf("Task[%d] has been assigned.\n", taskp.TaskId)
			}
		} else { // 没有未分配的reduce任务，检查reduce是否都执行完毕了，若是则Master转向AllDone阶段
			reply.Answer = WaitPlz // 本次请求暂无任务分配
			// 检查map是否都执行完毕了
			if m.checkReduceTaskDone() { // reduce阶段任务都执行完了
				m.toNextPhase() // 转向下一阶段
			}
			return nil
		}
	case AllDone:
		// map和reduce任务都执行完毕，本次请求的回复通知worker准备退出
		reply.Answer = FinishAndExit
		fmt.Println("All tasks have been finished!")
	default:
		panic("The phase undefined!!!")
	}

	return nil // 整个函数的返回error
}

// Master生成Reduce任务
func (m *Master) MakeReduceTask() {
	fmt.Println("begin make reduce tasks...")
	rn := m.ReducerNum
	// Getwd返回一个对应当前工作目录的根路径
	dir, _ := os.Getwd()
	files, err := os.ReadDir(dir) // files为当前目录下所有的文件
	if err != nil {
		fmt.Println(err)
	}
	for i := 0; i < rn; i++ { // 总共生成rn个reduce任务
		id := m.GenerateTaskId()
		input := []string{} // reduce任务的输入是对应的中间文件
		// 生成的中间文件mr-X-Y存放在当前文件夹
		// 从files中找到对应的输入此reduce任务的中间文件（利用前缀后缀）
		for _, file := range files { // 注意file是文件类型，判断前后缀需要用file.Name()转换成对应的文件名字符串
			if strings.HasPrefix(file.Name(), "mr-") && strings.HasSuffix(file.Name(), strconv.Itoa(i)) {
				input = append(input, file.Name())
			}
		}

		reduceTask := Task{
			TaskId:     id,
			TaskType:   ReduceTask,
			TaskState:  Waiting,
			InputFile:  input,
			ReducerNum: m.ReducerNum,
			ReducerKth: i,
		}
		fmt.Printf("make a reduce task %d\n", id)
		m.ReduceTaskChannel <- &reduceTask
	}
}

// Master生成Map任务
func (m *Master) MakeMapTask(files []string) {
	fmt.Println("begin make map tasks...")

	// 遍历输入的文本文件，向map管道中加入map任务
	for _, file := range files {
		id := m.GenerateTaskId()
		input := []string{file}
		mapTask := Task{
			TaskId:     id,
			TaskType:   MapTask,
			TaskState:  Waiting, // 尚未分配给worker时任务状态为waiting
			InputFile:  input,   // map情况切片里面就一个分配给worker的文件
			ReducerNum: m.ReducerNum,
		}
		fmt.Printf("make a map task %d\n", id)
		m.MapTaskChannel <- &mapTask
	}
}

// 通过自增产生TaskId
func (m *Master) GenerateTaskId() int {
	res := m.TaskIdForGen
	m.TaskIdForGen++
	return res
}

func (m *Master) GetTask(args *TaskRequest, reply *TaskResponse) error {
	task := <-m.MapTaskChannel
	reply.Task = *task
	return nil
}

func (m *Master) server() {
	rpc.Register(m)
	rpc.HandleHTTP()
	l, e := net.Listen("tcp", "127.0.0.1"+":1234")
	// sockname := MasterSocket()
	// fmt.Println("unix sockname:", sockname)
	// os.Remove(sockname)
	// l, e := net.Listen("unix", sockname)
	if e != nil {
		log.Fatal("listen error:", e)
	}
	go http.Serve(l, nil)
}

// if the entire job has finished.
func (m *Master) Done() bool {
	ret := false

	// Your code here.

	return ret
}

func MakeMaster(files []string, nReduce int) *Master {
	fmt.Println("begin make a master...")
	m := Master{
		CurrentPhase:      MapPhase,                     // 开始为Map阶段
		TaskIdForGen:      0,                            // 初始为0，后面通过自增得到唯一的TaskId分配任务
		MapTaskChannel:    make(chan *Task, len(files)), // 输入文件个数决定map任务个数
		ReduceTaskChannel: make(chan *Task, nReduce),
		TaskMap:           make(map[int]*Task, len(files)+nReduce), // 最多为map任务和reduce任务的和
		MapperNum:         len(files),
		ReducerNum:        nReduce,
	}

	m.MakeMapTask(files)

	m.server()
	return &m
}