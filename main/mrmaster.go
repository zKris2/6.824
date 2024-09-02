package main

import (
	"fmt"
	"os"
	"time"

	"mit6.824/mr"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "Usage: mrmaster inputfiles...\n")
		os.Exit(0)
	}

	m := mr.MakeMaster(os.Args[1:], 3)
	for !m.Done() {
		time.Sleep(time.Second)
	}

	fmt.Println("Master gonna exit!")
	time.Sleep(time.Second)
}
