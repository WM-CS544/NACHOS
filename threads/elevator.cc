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

		while (!totalWaitingToGoUp && !totalWaitingToGoDown && !totalWaitingToLeave) {
			printf("no one using the elevator, going to sleep\n");
			elevatorWaiting->Wait(lock);
			printf("elevator requested, waking up\n");
		}

		if (!CheckInFront()) {
			ChangeDirection();
		}

		FillElevator();
	
		if (curFloor == (NUM_FLOORS-1) && curDirection == 1) {
			printf("should change directions\n");
			curDirection = -1;
		}
		if	(curFloor == 0 && curDirection == -1) {
			curDirection = 1;
		}

		Move(curDirection);

		printf("=================================================\n");
	}

	lock->Release();
}

int
Elevator::CheckInFront()
{
	if (totalWaitingToLeave) {
		return 1;
	}
	int tempFloor = curFloor;
	while (tempFloor >= 0 && tempFloor <= (NUM_FLOORS-1)) {
		//don't check opposite direction on current floor
		if (tempFloor == curFloor) {
			if (curDirection == 1) {
				if (numWaitingToGoUp[tempFloor]) {
					return 1;
				}
			} else {
				if (numWaitingToGoDown[tempFloor]) {
					return 1;
				}
			}
		//someone waiting in current direction
		} else {
			if (numWaitingToGoUp[tempFloor] || numWaitingToGoDown[tempFloor]) {
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
		printf("elevator waiting for %d to exit\n", numWaitingToLeave[curFloor]);
		while (numWaitingToLeave[curFloor] > 0) {
			elevatorWaiting->Wait(lock);
			printf("everyone exited current floor\n");
		}
	}
}

void
Elevator::FillElevator()
{
	if (curDirection == 1) { //going up
		if (numWaitingToGoUp[curFloor] > 0) {
			waitingToGoUp[curFloor]->Broadcast(lock);
			printf("waiting for %d to board on current floor\n", numWaitingToGoUp[curFloor]);
			while (numWaitingToGoUp[curFloor] > 0) {
				elevatorWaiting->Wait(lock);
				printf("everyone on current floor is on the elevator\n");
			}
		}
	} else {	//going down
		if (numWaitingToGoDown[curFloor] > 0) {
			waitingToGoDown[curFloor]->Broadcast(lock);
			printf("waiting for %d to board on current floor\n", numWaitingToGoDown[curFloor]);
			while (numWaitingToGoDown[curFloor] > 0) {
				elevatorWaiting->Wait(lock);
				printf("everyone on current floor is on the elevator\n");
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
			waitingToGoUp[atFloor]->Wait(lock);
		} while (curFloor != atFloor);
		printf("getting on elevator\n");
		totalWaitingToGoUp--;
		numWaitingToGoUp[atFloor]--;
	} else {
		do {
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
			elevatorWaiting->Signal(lock);
		}
	} else {
		if (!numWaitingToGoDown[atFloor]) {
			elevatorWaiting->Signal(lock);
		}
	}

	while (curFloor != toFloor) {
		waitingToLeave[toFloor]->Wait(lock);
		printf("getting off elevator\n");
	}

	totalWaitingToLeave--;
	numWaitingToLeave[toFloor]--;

	if (!numWaitingToLeave[toFloor]) {
		elevatorWaiting->Signal(lock);
	}

	lock->Release();
}
