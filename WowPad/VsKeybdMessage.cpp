#include "stdafx.h"
#include "VsKeybdMessage.h"

VsKeybdMessage::VsKeybdMessage(void) : VsMessage()
{
	this->shiftFlags = 0;
	this->imeKey = 0;
	for (int i=0; i<6; i++)
	{
		this->keyCodes[i] = 0;
	}
}

VsKeybdMessage::~VsKeybdMessage(void)
{
}

BYTE VsKeybdMessage::GetShiftFlags(void)
{
	return shiftFlags;
}

BYTE *VsKeybdMessage::GetKeyCodes(void)
{
	return keyCodes;
}

INT VsKeybdMessage::GetImeKey(void)
{
	return imeKey;
}

INT VsKeybdMessage::Parse(CHAR* msg)
{
	int nBit = 0;
	int nIntSize = sizeof(int);
	int offset = 0;
	VsMessage::Parse(msg, offset);

	if (this->deviceType == VS_MODE_KEYBOARD) 
	{
		//����Ű
		nBit = Packet::CharArrayToInt(msg, offset);
		this->imeKey = nBit;

		//����Ʈ Ű �÷���
		this->shiftFlags = msg[offset++];

		//Ű�ڵ�
		int nSize = sizeof(this->keyCodes) / sizeof(BYTE);
		CopyMemory(this->keyCodes, msg + offset, nSize);
		offset += nSize;
	}
	
	return this->validCode;
}