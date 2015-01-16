
package main

import (
 _ "fmt"
"runtime"
"time"
)


//var test int = 0
var j int = 0
var test int = 0
func main(){
	/*i := make(chan int)
	go func() {	
	i <- 0
	}()*/
	inUse := make(chan bool)
	go func(){
	inUse <- true
	}()
	runtime.GOMAXPROCS(runtime.NumCPU())

	go countUpwards(inUse)

	runtime.GOMAXPROCS(runtime.NumCPU())
	
	go countDownwards(inUse)

	time.Sleep(1000*time.Millisecond)
	var res int = j
	println(res, test)
}

func countUpwards(inUse chan bool){
	//var tmp int
	for k := 0; k< 1000000; k++{
		//tmp = <- i
		//tmp++
		//i <- tmp
		test++
		if (<-inUse){	
			j++
			inUse <- true
		}else{
			k--
		}
	}
}

func countDownwards(inUse chan bool){
//var tmp int
for k:= 0; k < 1000; k++ {
	//tmp = <- i
	//tmp--
	//i <- tmp
	if (<-inUse){	
		j--
		inUse <- true
	}else{
		k--
	}
}

}

