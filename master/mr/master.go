package mr

import (
	"fmt"
	"log"
	"net"
	"net/http"
	"net/rpc"
	"strconv"
	"time"

	mrpc "mit6.824/rpc"
)

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

// Master生成Map任务
func (m *Master) MakeMapTask(files []string) {
	fmt.Println("begin make map tasks...")

	// 遍历输入的文本文件，向map管道中加入map任务
	for _, file := range files {
		fmt.Println("filename:", file)
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

func (m *Master) GetTask(args *mrpc.TaskRequest, reply *mrpc.TaskResponse) error {
	reply.Name = strconv.Itoa(args.X)
	return nil
}

func (m *Master) server() {
	rpc.Register(m)
	rpc.HandleHTTP()
	l, e := net.Listen("tcp", "127.0.0.1"+":1234")
	// sockname := mrpc.MasterSocket()
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
