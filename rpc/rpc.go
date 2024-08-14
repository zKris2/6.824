package rpc

import (
	"os"
	"strconv"
)

type TaskRequest struct {
	X int
}

type TaskResponse struct {
	Name string
}

func MasterSocket() string {
	s := "/var/tmp/6824-mr-"
	s += strconv.Itoa(os.Getuid())
	return s
}
