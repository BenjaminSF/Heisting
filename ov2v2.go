package main

import (
 _ "fmt"
"runtime"
"time"
)

func main(){
	runtime.GOMAXPROCS(runtime.NumCPU())
	done := make(chan int)
	i := make(chan int)
	go func(){
		i <- 0
	}()
	go countUpwards(done, i)
	go countDownwards(done, i)
	if (<-done) == 1{
		var tmp int
		for (<-done) != 1{
			i<-i
		}
	}
	time.Sleep(1000*time.Millisecond)
	var res int
	res <- i
	println(res)
}

func countUpwards(done chan int, i chan int){
	var tmp int
	for k := 0; k< 1000000; k++{
		tmp <- i
		tmp++
		i <- tmp
		if (k == 999999){
			done <- 1
		}
	}
}

func countDownwards(done chan int, i chan int){
	var tmp int
	for k:= 0; k < 1000; k++ {
		tmp <- i
		tmp--
		i <- tmp
		if (k == 999){
			done <- 1
		}
	}
}
