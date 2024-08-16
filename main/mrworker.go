package main

import (
	"mit6.824/mr"
)

func main() {
	// if len(os.Args) != 2 {
	// 	fmt.Fprintf(os.Stderr, "Usage: mrworker xxx.so\n")
	// 	os.Exit(1)
	// }

	//rpc call
	mr.CallGetTask()
}
