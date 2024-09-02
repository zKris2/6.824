package main

//
// a MapReduce pseudo-application that sometimes crashes,
// and sometimes takes a long time,
// to test MapReduce's ability to recover.
//
// go build -buildmode=plugin crash.go
//

import (
	"crypto/rand"
	"fmt"
	"math/big"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"

	"mit6.824/mr"
)

func maybeCrash() {
	max := big.NewInt(1000)
	rr, _ := rand.Int(rand.Reader, max)
	if rr.Int64() < 330 {
		// crash!
		fmt.Println("OS crash and exit!")
		os.Exit(1)
	} else if rr.Int64() < 660 {
		// delay for a while.
		maxms := big.NewInt(10 * 1000)
		ms, _ := rand.Int(rand.Reader, maxms)
		fmt.Println("OS sleep:", time.Duration(ms.Int64())*time.Millisecond)
		time.Sleep(time.Duration(ms.Int64()) * time.Millisecond)
	}
}

func Map(filename string, contents string) []mr.KeyValue {
	maybeCrash()

	kva := []mr.KeyValue{}
	kva = append(kva, mr.KeyValue{"a", filename})
	kva = append(kva, mr.KeyValue{"b", strconv.Itoa(len(filename))})
	kva = append(kva, mr.KeyValue{"c", strconv.Itoa(len(contents))})
	kva = append(kva, mr.KeyValue{"d", "xyzzy"})
	return kva
}

func Reduce(key string, values []string) string {
	maybeCrash()

	// sort values to ensure deterministic output.
	vv := make([]string, len(values))
	copy(vv, values)
	sort.Strings(vv)

	val := strings.Join(vv, " ")
	return val
}
