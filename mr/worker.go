package mr

import (
	"encoding/json"
	"fmt"
	"hash/fnv"
	"io"
	"log"
	"net/rpc"
	"os"
	"strconv"
	"time"
)

// Map functions return a slice of KeyValue.
type KeyValue struct {
	Key   string
	Value string
}

// use ihash(key) % NReduce to choose the reduce
// task number for each KeyValue emitted by Map.
func ihash(key string) int {
	h := fnv.New32a()
	h.Write([]byte(key))
	return int(h.Sum32() & 0x7fffffff)
}

// 执行map任务
// mapf和reducef是mrworker.go创建worker时传进来的
// task是向Master请求后返回的待处理的任务
func PerformMapTask(mapf func(string, string) []KeyValue, task *Task) {
	intermediate := []KeyValue{}              // 存储map处理后的中间kv对
	for _, filename := range task.InputFile { // map阶段每个worker事实上输入只有一个文件
		file, err := os.Open(filename)
		if err != nil {
			log.Fatalf("cannot open %v", filename)
		}
		// ReadAll: 返回读取的数据和遇到的错误。成功的调用返回的err为nil而非EOF。
		content, err := io.ReadAll(file)
		if err != nil {
			log.Fatalf("cannot read %v", filename)
		}
		file.Close()
		kva := mapf(filename, string(content)) // 调用传入的map函数
		intermediate = append(intermediate, kva...)
	}

	// 对中间键值对按key排序（在这里排序似乎没有必要，因为即使在这个map任务内排序，后面将对应地多个中间文件输入reduce也需要整体再排序）
	//sort.Sort(ByKey(intermediate))

	// 将中间键值对写入中间tmp文件
	// 中间文件的合理命名方式是 `mr-X-Y` ，X是Map任务的序号，Y是ihash(key)%nreduce后的值，这样就建立了key与被分配给的reduce任务的映射关系
	// map任务。可以使用Go的`encoding/json` package去存储中间的键值对到文件以便reduce任务之后读取
	rn := task.ReducerNum
	for i := 0; i < rn; i++ {
		// 创建中间输出文件
		midFileName := "mr-tmp/" + "mr-" + strconv.Itoa(task.TaskId) + "-" + strconv.Itoa(i)
		midFile, _ := os.Create(midFileName)
		enc := json.NewEncoder(midFile)   // NewEncoder创建一个将数据写入w的*Encoder
		for _, kv := range intermediate { // 遍历intermediate寻找写入该中间文件的kv
			if ihash(kv.Key)%rn == i {
				enc.Encode(&kv) // Encode将v的json编码写入输出流，并会写入一个换行符
			}
		}
		midFile.Close()
	}
}

// 传入mapf，reducef
func Worker(mapf func(string, string) []KeyValue, reducef func(string, []string) string) {

	// workers周期性地向master请求任务，每次请求之间休眠 time.Sleep()
	loop := true

	for loop {
		re := CallGetTask()
		switch re.Answer {
		case TaskGetted: // 当worker成功被分配到任务
			task := re.Task
			switch task.TaskType {
			case MapTask: // map任务
				fmt.Printf("A worker get a map task and taskId is %d\n", task.TaskId)
				PerformMapTask(mapf, &task)
				FinishTaskAndReport(task.TaskId)
			case ReduceTask: // reduce任务
				// fmt.Printf("A worker get a reduce task and taskId is %d\n", task.TaskId)
				// PerformReduceTask(reducef, &task)
				// FinishTaskAndReport(task.TaskId)
			}
		case WaitPlz: // 本次请求并未分得任务，worker等待1s后再下次请求
			time.Sleep(time.Second)
		case FinishAndExit: // mapreduce已经处于AllDone阶段，worker准备退出
			loop = false
		default:
			fmt.Println("request task error!")
		}
	}
}

// worker完成任务后调用RPC告知master并请求修改任务状态
func FinishTaskAndReport(id int) {
	args := FinArgs{TaskId: id}
	reply := FinReply{}

	// 请求调用Master的UpdateTaskState方法
	ok := call("Master.UpdateTaskState", &args, &reply)
	if !ok { //请求失败
		fmt.Println("Call failed!")
	}
}

func CallGetTask() TaskResponse {
	// call("Master.AssignTask", &args, &reply)
	// for _, file := range reply.Task.InputFile {
	// 	fmt.Printf("got task[%d]:%s\n", reply.Task.TaskId, file)
	// }
	// task := reply.Task
	// return TaskResponse{
	// 	Answer: ,
	// 	Task: ,
	// }

	args := TaskRequest{}
	reply := TaskResponse{}

	// send the RPC request, wait for the reply.
	// 请求调用Master的AssignTask方法
	ok := call("Master.AssignTask", &args, &reply)

	if !ok { // 请求失败
		fmt.Println("Call failed!")
	}

	return reply
}

func call(rpcname string, args interface{}, reply interface{}) bool {
	c, err := rpc.DialHTTP("tcp", "127.0.0.1"+":1234")
	// sockname := MasterSocket()
	// c, err := rpc.DialHTTP("unix", sockname)
	if err != nil {
		log.Fatal("dialing:", err)
	}
	defer c.Close()

	err = c.Call(rpcname, args, reply)
	if err == nil {
		return true
	}

	fmt.Println(err)
	return false
}
