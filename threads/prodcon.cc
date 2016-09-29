#include <stdio.h> 
#include "prodcon.h"
#include <new>

const char *PROD_STRING = "Hello World\n";

ProdCon::ProdCon()
{
	lock = new(std::nothrow) Lock("prod_con lock");
	isFull = new(std::nothrow) Condition("prod_con buff full condition");
	isEmpty = new(std::nothrow) Condition("prod_con buff empty condition");
	prodIndex = 0;
	prodStringIndex = 0;
	conIndex = 0;
	numInBuff = 0;
}

ProdCon::~ProdCon()
{
	delete lock;
	delete isFull;
	delete isEmpty;
}

void
ProdCon::Produce()
{
	lock->Acquire();
	while(numInBuff == BUFF_SIZE) {
		isFull->Wait(lock);
	}
	buff[prodIndex] = PROD_STRING[prodStringIndex];
	prodIndex  = (prodIndex + 1) % BUFF_SIZE;
	prodStringIndex = (prodStringIndex + 1) % strlen(PROD_STRING);
	numInBuff++;
	ASSERT(numInBuff <= BUFF_SIZE);
	isEmpty->Signal(lock);
	lock->Release();
}

void
ProdCon::Consume()
{
	lock->Acquire();
	while(!numInBuff) {
		isEmpty->Wait(lock);
	}
	char consumed = buff[conIndex];
	printf("%c", consumed);
	conIndex = (conIndex + 1) % BUFF_SIZE;
	numInBuff--;	
	ASSERT(numInBuff >= 0);
	isFull->Signal(lock);
	lock->Release();
}

