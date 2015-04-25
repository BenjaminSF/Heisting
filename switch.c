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
	case MSG_ADDR_REQUEST:
		printf("Receive: MSG_ADDR_REQUEST\n");
		BufferInfo newMsg;
		encodeMessage(&newMsg, 0, srcAddr, MSG_ADDR_RESPONSE,-1, -1, -1);
		enqueue(sendQueue, &newMsg, sizeof(BufferInfo));
}