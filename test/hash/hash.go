package main

import (
	"fmt"
	"hash/fnv"
)

func main() {
	h := fnv.New32a()
	str := "it"
	//str := "Being"
	h.Write([]byte(str))
	result := h.Sum32()
	fmt.Println(result)
}
