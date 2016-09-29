#include "elevator.h"
#include "system.h"
#include <new>
#include <stdio.h>

#define NUM_LOOPS 100

Elevator::Elevator()
{
	lock = new(std::nothrow) Lock("elevator lock");
	curFloor = 0;
	curDirection = 1;
	totalWaitingToGoUp = 0;
	totalWaitingToGoDown = 0;
	totalWaitingToLeave = 0;
	elevatorWaiting = new(std::nothrow) Condition("elevator waiting condition");

	for (unsigned int i = 0; i < sizeof(waitingToGoUp)/sizeof(waitingToGoUp[0]); i++) {
		waitingToGoUp[i] = new(std::nothrow) Condition("floor waiting condition");
	}

	for (unsigned int i = 0; i < sizeof(numWaitingToGoUp)/sizeof(numWaitingToGoUp[0]); i++) {
		numWaitingToGoUp[i] = 0;
	}

	for (unsigned int i = 0; i < sizeof(waitingToGoDown)/sizeof(waitingToGoDown[0]); i++) {
		waitingToGoDown[i] = new(std::nothrow) Condition("floor waiting condition");
	}

	for (unsigned int i = 0; i < sizeof(numWaitingToGoDown)/sizeof(numWaitingToGoDown[0]); i++) {
		numWaitingToGoDown[i] = 0;
	}

	for (unsigned int i = 0; i < sizeof(waitingToLeave)/sizeof(waitingToLeave[0]); i++) {
		waitingToLeave[i] = new(std::nothrow) Condition("floor waiting condition");
	}

	for (unsigned int i = 0; i < sizeof(numWaitingToLeave)/sizeof(numWaitingToLeave[0]); i++) {
		numWaitingToLeave[i] = 0;
	}
}

Elevator::~Elevator()
{
	delete lock;
	delete elevatorWaiting;

	for (unsigned int i = 0; i < sizeof(waitingToGoUp)/sizeof(waitingToGoUp[0]); i++) {
		delete waitingToGoUp[i];
	}

	for (unsigned int i = 0; i < sizeof(waitingToGoDown)/sizeof(waitingToGoDown[0]); i++) {
		delete waitingToGoDown[i];
	}

	for (unsigned int i = 0; i < sizeof(waitingToLeave)/sizeof(waitingToLeave[0]); i++) {
		delete waitingToLeave[i];
	}
}

void
Elevator::Start()
{
	lock->Acquire();

	while (1) {

		printf("curFloor %d\n", curFloor);
		printf("curDirection %d\n", curDirection);

		EmptyElevator();

		for (unsigned int i = 0; i < sizeof(numWaitingToGoDown)/sizeof(numWaitingToGoDown[0]); i++) {
			//printf("numWaitingToGoDown: %d\n", numWaitingToGoUp[i]);
		}

		while (!totalWaitingToGoUp && !totalWaitingToGoDown && !totalWaitingToLeave) {
			elevatorWaiting->Wait(lock);
			printf("elevator waking up\n");
			//printf("%d %d %d\n", totalWaitingToGoUp, totalWaitingToGoDown, totalWaitingToLeave);
		}

		if (!CheckInFront()) {
			ChangeDirection();
		}

		printf("after check in front\n");
		
		FillElevator();
	
		printf("after fillElevator\n");

		if (curFloor == (NUM_FLOORS-1) && curDirection == 1) {
			printf("should change directions\n");
			curDirection = -1;
		}
		if	(curFloor == 0 && curDirection == -1) {
			curDirection = 1;
		}

		Move(curDirection);

		printf("------------------------------------\n");
	}

	lock->Release();
}

int
Elevator::CheckInFront()
{
	if (totalWaitingToLeave) {
		printf("someone waiting to leave%d\n", totalWaitingToLeave);
		return 1;
	}
	int tempFloor = curFloor;
	while (tempFloor >= 0 && tempFloor <= (NUM_FLOORS-1)) {
		//don't check opposite direction on current floor
		if (tempFloor == curFloor) {
			if (curDirection == 1) {
				if (numWaitingToGoUp[tempFloor]) {
					printf("someone waiting on cur floor\n");
					return 1;
				}
			} else {
				if (numWaitingToGoDown[tempFloor]) {
					printf("someone waiting on cur floor\n");
					return 1;
				}
			}
		//someone waiting in current direction
		} else {
			if (numWaitingToGoUp[tempFloor] || numWaitingToGoDown[tempFloor]) {
				printf("someone waiting in cur direction\n");
				return 1;
			}
		}
		tempFloor += curDirection;
	}
	return 0;
}

void
Elevator::ChangeDirection()
{
	if (curDirection == 1) {
		curDirection = -1;
	} else {
		curDirection = 1;
	}
}

void
Elevator::Move(int direction)
{
	for (unsigned int i = 0; i < NUM_LOOPS; i++) {
		IntStatus oldLevel = interrupt->SetLevel(IntOff);
		(void) interrupt->SetLevel(oldLevel);
	}

	if (direction == 1) {
		curFloor++;
	} else {
		curFloor--;
	}

	printf("moving to floor:%d\n", curFloor);

	ASSERT(curFloor >= 0);
	ASSERT(curFloor < NUM_FLOORS);
}

void
Elevator::EmptyElevator()
{
	if (numWaitingToLeave[curFloor] > 0) {
		waitingToLeave[curFloor]->Broadcast(lock);
		while (numWaitingToLeave[curFloor] > 0) {
			elevatorWaiting->Wait(lock);
			printf("elevator woke up after empty\n");
		}
	}
}

void
Elevator::FillElevator()
{
	if (curDirection == 1) { //going up
		if (numWaitingToGoUp[curFloor] > 0) {
			waitingToGoUp[curFloor]->Signal(lock);
			waitingToGoUp[curFloor]->Broadcast(lock);
			printf("number waiting at current floor %d\n", numWaitingToGoUp[curFloor]);
			while (numWaitingToGoUp[curFloor] > 0) {
				elevatorWaiting->Wait(lock);
				printf("elevator woke up after fill\n");
			}
		}
	} else {	//going down
		if (numWaitingToGoDown[curFloor] > 0) {
			waitingToGoDown[curFloor]->Signal(lock);
			waitingToGoDown[curFloor]->Broadcast(lock);
			printf("number waiting at current floor %d\n", numWaitingToGoDown[curFloor]);
			while (numWaitingToGoDown[curFloor] > 0) {
				elevatorWaiting->Wait(lock);
				printf("elevator woke up after fill\n");
			}
		}
	}
}

void
Elevator::ArrivingGoingFromTo(int atFloor, int toFloor)
{
	lock->Acquire();

	if (toFloor > atFloor) {	//what if same floor
		totalWaitingToGoUp++;
		numWaitingToGoUp[atFloor]++;
	} else {
		totalWaitingToGoDown++;
		numWaitingToGoDown[atFloor]++;
	}
	
	elevatorWaiting->Signal(lock);

	if (toFloor > atFloor) {	//what if same floor
		do {
			printf("waiting at floor\n");
			waitingToGoUp[atFloor]->Wait(lock);
		} while (curFloor != atFloor);
		printf("getting on elevator\n");
		totalWaitingToGoUp--;
		numWaitingToGoUp[atFloor]--;
	} else {
		do {
			printf("waiting at floor\n");
			waitingToGoDown[atFloor]->Wait(lock);
		} while (curFloor != atFloor);
		printf("getting on elevator\n");
		totalWaitingToGoDown--;
		numWaitingToGoDown[atFloor]--;
	}
	
	numWaitingToLeave[toFloor]++;
	totalWaitingToLeave++;
	if (curDirection == 1) {
		if (!numWaitingToGoUp[atFloor]) {
			printf("everyone on elevator\n");
			elevatorWaiting->Signal(lock);
		}
	} else {
		if (!numWaitingToGoDown[atFloor]) {
			printf("everyone on elevator\n");
			elevatorWaiting->Signal(lock);
		}
	}

	fprintf(stderr, "numWaitingToLeave %d\n", numWaitingToLeave[toFloor]);

	while (curFloor != toFloor) {
		waitingToLeave[toFloor]->Wait(lock);
	}

	totalWaitingToLeave--;
	numWaitingToLeave[toFloor]--;

	if (!numWaitingToLeave[toFloor]) {
		printf("everyone off elevator\n");
		elevatorWaiting->Signal(lock);
	}

	lock->Release();
}
