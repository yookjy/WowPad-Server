#include "stdafx.h"
#include "VsMessageQueue.h"


VsMessageQueue::VsMessageQueue(void)
{
}

VsMessageQueue::~VsMessageQueue(void)
{
}

void VsMessageQueue::Enqueue(VsMessage* msg)
{
	VsMessageQueue* currPos = this;

	while (currPos->next != NULL)
	{
		currPos = currPos->next;
	}

	currPos->data = msg;
	currPos->next = new VsMessageQueue;
}

VsMessage* VsMessageQueue::Dequeue()
{
	VsMessage* currData = this->data;
	VsMessageQueue* nextPos = this->next;
	
	if (nextPos != NULL)
	{
		VsMessageQueue* nextNextPos = nextPos->next;
		VsMessage* nextData = nextPos->data;

		delete this->next;
		this->next = nextNextPos;
		this->data = nextData;
	}
	
	return currData;
}

VsMessage* VsMessageQueue::First()
{
	return this->data;
}

BOOL VsMessageQueue::IsEmpty()
{
	return this->Size() == 0;
}

void VsMessageQueue::Clear()
{
	while(!IsEmpty())
	{
		delete this->Dequeue();
	}
}

LONG VsMessageQueue::Size()
{
	LONG size = 0;
	VsMessageQueue* nextPos = this->next;
	while (nextPos != NULL)
	{
		nextPos = nextPos->next;
		size++;
	}
	return size;
}