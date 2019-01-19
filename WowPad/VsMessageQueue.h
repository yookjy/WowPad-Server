#pragma once
#include "VsMessage.h"

class VsMessageQueue
{
private:
	VsMessageQueue* next;
	VsMessage* data;
public:
	VsMessageQueue(void);
	~VsMessageQueue(void);
	void Enqueue(VsMessage*);
	VsMessage* Dequeue(void);
	VsMessage* First(void);
	BOOL IsEmpty(void);
	void Clear(void);
	LONG Size(void);
};

