#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "copyright.h"
#include "synch.h"

#define NUM_FLOORS 5

class Elevator {
	public:
		Elevator();
		~Elevator();

		void Start();
		void ArrivingGoingFromTo(int atFloor, int toFloor);

	private:
		//variables
		Lock *lock;
		int curFloor;
		int curDirection;
		Condition *elevatorWaiting;

		Condition *waitingToGoUp[NUM_FLOORS];
		int numWaitingToGoUp[NUM_FLOORS];
		int totalWaitingToGoUp;

		Condition *waitingToGoDown[NUM_FLOORS];
		int numWaitingToGoDown[NUM_FLOORS];
		int totalWaitingToGoDown;

		Condition *waitingToLeave[NUM_FLOORS];
		int numWaitingToLeave[NUM_FLOORS];
		int totalWaitingToLeave;

		//methods
		int CheckInFront();
		void ChangeDirection();
		void Move(int direction);
		void EmptyElevator();
		void FillElevator();
};

#endif
