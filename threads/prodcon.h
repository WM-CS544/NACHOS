#ifndef PROD_CON_H
#define PROD_CON_H

#include "copyright.h"
#include "synch.h"

#define BUFF_SIZE 20

class ProdCon {
	public:
		ProdCon();
		~ProdCon();

		void Produce();

		void Consume();

	private:
		Lock *lock;
		int prodIndex;
		int prodStringIndex;
		Condition *isFull;
		int conIndex;
		Condition *isEmpty;
		int numInBuff;
		char buff[BUFF_SIZE];
};

#endif

