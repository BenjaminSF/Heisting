switch(myState){
	case MSG_SET_LAMP:
		printf("Receive: MSG_SET_LAMP\n");
		setButtonLamp(bufOrder.currentFloor, bufOrder.buttonType, bufOrder.active);
		break;
	case MSG_CONNECT_SEND:
		printf("Receive: MSG_CONNECT_SEND\n");
		addElevatorAddr(bufOrder.srcAddr);
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_CONNECT_RESPONSE, getMaster(), -1, -1);
		enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		break;
	case MSG_CONNECT_RESPONSE:
		if (bufOrder.masterStatus == 1){
			setMasterIP(srcAddr);
		}
		addElevatorAddr(srcAddr);
		break;
	case MSG_MASTER_REQUEST:
		printf("Receive: MSG_MASTER_REQUEST\n");
		resetAddrsList();
		int candidate = 1;
		if (srcAddr > myIP){
			candidate = 0;
		}
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, 0, MSG_MASTER_PROPOSAL, candidate, -1, -1);
		enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		break;
	case MSG_ADDR_REQUEST:
		printf("Receive: MSG_ADDR_REQUEST\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, srcAddr, MSG_ADDR_RESPONSE,-1, -1, -1);
		enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		break;
	case MSG_ADDR_RESPONSE:
		printf("Receive: MSG_ADDR_RESPONSE\n");
		addElevatorAddr(srcAddr);
		break;
	case MSG_ADD_ORDER:
		if (getMaster() == 1){
			printf("Receive: MSG_ADD_ORDER\n");
			struct order newOrder;
			newOrder.dest = bufOrder.nextFloor;
			newOrder.buttonType = bufOrder.buttonType;
			newOrder.elevator = srcAddr;
			addNewOrder(newOrder, bufOrder.currentFloor,bufOrder.nextFloor);
			if (bufOrder.buttonType == BUTTON_COMMAND){
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, bufOrder.srcAddr, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}else{
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.nextFloor, bufOrder.buttonType, 1);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
		}
		break;
	case MSG_DELETE_ORDER:
		if(getMaster() == 1){
			printf("Receive: MSG_DELETE_ORDER\n");
			deleteOrder(bufOrder.currentFloor,bufOrder.buttonType,srcAddr);
			if (bufOrder.buttonType != BUTTON_COMMAND){
				BufferInfo newMsg;
				encodeMessage(&newMsg, 0, 0, MSG_SET_LAMP, bufOrder.currentFloor, bufOrder.buttonType, 0);
				enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
			}
		}
		break;
	case MSG_ELEVSTATE:
		if(getMaster() == 1){
			printf("Receive: MSG_ELEVSTATE\n");
			int i;
			for (i = 0; i < getAddrsCount(); i++){
				if (addrsList(i) == srcAddr){
					elevStates.floor[i] = bufOrder.currentFloor;
					elevStates.nextFloor[i] = bufOrder.nextFloor;
					if (bufOrder.nextFloor == -1) elevStates.active[i] = 0;
						elevStates.button[i] = bufOrder.buttonType;
						break;
					}
				}
		}
		break;
	case MSG_CONFIRM_ORDER:
		if(getMaster() == 1){	
			printf("Receive: MSG_CONFIRM_ORDER\n");
			int i;
			for (i = 0; i < N_ORDERS; i++){
				if (orderQueue.inUse[i] && (orderQueue.Queue[i].dest == bufOrder.nextFloor) && orderQueue.enRoute[i]){
					orderQueue.enRoute[i] = 1;
				}
			}
		}
	case MSG_IM_ALIVE:
		if(getMaster() == 0){
			printf("Receive: MSG_IM_ALIVE\n");
			sem_post(&timeoutSem);
		}
	case MSG_DO_ORDER:
		if (getMaster() == 0){
			printf("Receive: DO_ORDER%d\n", bufOrder.nextFloor);
			localManQueue[bufOrder.nextFloor] = 1;
			localManButtons[bufOrder.nextFloor] = bufOrder.buttonType;
			BufferInfo newMsg;
			encodeMessage(&newMsg, 0, 0, MSG_CONFIRM_ORDER,bufOrder.nextFloor, -1, -1);
			enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
		}
	default:
		break;

}