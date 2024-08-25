package mr

import (
	"fmt"
	"log"
	"net/rpc"
)

// Map functions return a slice of KeyValue.
type KeyValue struct {
	Key   string
	Value string
}

func Worker() {
	for {
		args := TaskRequest{}
		reply := TaskResponse{}
		CallGetTask(&args, &reply)
		if reply.Answer == FinishAndExit {
			return
		}
		//TODO 完成任务通知master UpdateTaskState
		FinishTaskAndReport(reply.Task.TaskId)
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

func CallGetTask(args *TaskRequest, reply *TaskResponse) {
	call("Master.AssignTask", &args, &reply)
	for _, file := range reply.Task.InputFile {
		fmt.Printf("got task[%d]:%s\n", reply.Task.TaskId, file)
	}
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
