package main

import (
 _ "fmt"
"runtime"
)

func main(){
	runtime.GOMAXPROCS(runtime.NumCPU())
	done := make(chan int)
	i := make(chan int)
	go func(){
		i <- 0
	}()
    go syncingThread(done, i)
	go countUpwards(done, i)
	go countDownwards(done, i)
    <-done
    <-done 
	res := <- i
	println(res)
}

func countUpwards(done chan int, i chan int){
	for k := 0; k< 1000000; k++{
		tmp := <- i
		tmp++
		i <- tmp
		if (k == 999999){
			done <- 1
		}
	}
}

func countDownwards(done chan int, i chan int){
	for k:= 0; k < 1000; k++ {
		tmp := <- i
		tmp--
		i <- tmp
		if (k == 999){
			done <- 1
		}
	}
}

func syncingThread(done chan int, i chan int){
    //var threadsDone int = 0
    for {
        select {
            case tmp := <- i:
                i <- tmp
            /* case <-done:
                if threadsDone == 1{
                    return
                }else{
                    threadsDone++
                }*/
        }
    }
}