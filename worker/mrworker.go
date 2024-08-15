package main

import (
	"fmt"
	"os"

	"mit6.824/worker/mr"
)

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage: mrworker xxx.so\n")
		os.Exit(1)
	}

	//rpc call
	mr.CallGetTask()
}
