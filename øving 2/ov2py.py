
from threading import Thread,Lock
import time
i = 0
lock = Lock()

def countUpwards():
	global i
	for k in range(1000000):
		lock.acquire()
		try:
			i = i+1
		finally:
			lock.release()
	
def countDownwards():
	global i
	for k in range(1000000):
		lock.acquire()
		try:
			i = i-1
		finally:
			lock.release()
		
def main():
	thread1 = Thread(target = countUpwards, args = (),)
	thread2 = Thread(target = countDownwards, args = (),)

	thread1.start()
	time.sleep(2)
	thread2.start()
	time.sleep(2)


main()
print i
