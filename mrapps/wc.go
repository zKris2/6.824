package main

import (
	"crypto/rand"
	"math/big"
	"os"
	"strconv"
	"strings"
	"time"
	"unicode"

	"mit6.824/mr"
)

func maybeCrash() {
	max := big.NewInt(1000)
	rr, _ := rand.Int(rand.Reader, max)
	if rr.Int64() < 330 {
		// crash!
		os.Exit(1)
	} else if rr.Int64() < 660 {
		// delay for a while.
		maxms := big.NewInt(10 * 1000)
		ms, _ := rand.Int(rand.Reader, maxms)
		time.Sleep(time.Duration(ms.Int64()) * time.Millisecond)
	}
}

func Map(filename string, contents string) []mr.KeyValue {
	//maybeCrash()
	ff := func(r rune) bool { return !unicode.IsLetter(r) }

	// split contents into an array of words.
	words := strings.FieldsFunc(contents, ff)

	kva := []mr.KeyValue{}
	for _, w := range words {
		kv := mr.KeyValue{w, "1"}
		kva = append(kva, kv)
	}
	return kva
}

func Reduce(key string, values []string) string {
	//maybeCrash()
	return strconv.Itoa(len(values))
}
