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

func CallGetTask() {
	// declare an argument structure.
	args := TaskRequest{}

	// fill in the argument(s).
	args.X = 99

	// declare a reply structure.
	reply := TaskResponse{}

	// send the RPC request, wait for the reply.
	call("Master.AssignTask", &args, &reply)

	// reply.Y should be 100.
	//fmt.Printf("reply.Name %s\n", reply.Task.InputFile[0])
	for _, file := range reply.Task.InputFile {
		fmt.Printf("worker %d got task:%s\n", reply.Task.TaskId, file)
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
