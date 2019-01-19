#include "stdafx.h"
#include "VsPointerMessage.h"


VsPointerMessage::VsPointerMessage(const VsPointerMessage& udpMessage) : VsMessage(udpMessage)
{
	this->contactCnt = udpMessage.contactCnt;
	for (int i=0; i<5; i++)
	{
		this->id[i] = udpMessage.id[i];
		this->action[i] = udpMessage.action[i];
		this->posX[i] = udpMessage.posX[i];
		this->posY[i] = udpMessage.posY[i];
	}
}

VsPointerMessage::VsPointerMessage(void) : VsMessage()
{
	this->contactCnt = 0;
	for (int i=0; i<5; i++)
	{
		this->id[i] = 0;
		this->action[i] = 0;
		this->posX[i] = 0;
		this->posY[i] = 0;
	}
}


VsPointerMessage::~VsPointerMessage(void)
{
}


BOOL VsPointerMessage::GetUseExtendButton()
{
	return useExtendButton;
}

INT VsPointerMessage::GetId(INT index)
{
	return id[index];
}

INT VsPointerMessage::GetAction(INT index)
{
	return action[index];
}

INT VsPointerMessage::GetPosX(INT index)
{
	return posX[index];
}

INT VsPointerMessage::GetPosY(INT index)
{
	return posY[index];
}

INT VsPointerMessage::GetIndexById(INT id)
{
	for (int i=0; i<sizeof(this->id); i++)
	{
		if (this->id[i] == id) 
		{
			return i;
		}
	}
	return 0;
}

INT VsPointerMessage::GetContactCnt()
{
	return contactCnt;
}

INT VsPointerMessage::Parse(CHAR* msg)
{
	int nBit = 0;
	int nIntSize = sizeof(int);
	int offset = 0;
	VsMessage::Parse(msg, offset);

	if (this->deviceType == VS_MODE_MOUSE 
		|| this->deviceType == VS_MODE_TOUCHSCREEN) 
	{
		//������ ��ư ��� ����
		nBit = Packet::CharArrayToInt(msg, offset);
		this->useExtendButton = nBit;

		for (int i = 0 ;  i < 5 ; i++)
		{
			//���̵�
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->id[i] = nBit;

			//�׼�
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit != VS_TOUCH_FLAG_BEGIN
				&& nBit != VS_TOUCH_FLAG_MOVE
				&& nBit != VS_TOUCH_FLAG_END
				&& nBit != VS_TOUCH_FLAG_NONE
				&& nBit != VS_TOUCH_FLAG_CANCELED) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->action[i] = nBit;
			if (this->action[i] != VS_TOUCH_FLAG_NONE)
				this->contactCnt++;

			//x ��ǥ
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->posX[i] = nBit;

			//y ��ǥ
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->posY[i] = nBit;
		}

		//���̵� ������� �����Ѵ�.
		bool hasFirst = false;
		for (int i = 0; i < this->contactCnt; i++) 
		{
			for (int j = i + 1; j < this->contactCnt; j++)
			{
				if (this->id[i] > this->id[j])
				{
					//id ����
					this->id[i] ^= this->id[j];
					this->id[j] ^= this->id[i];
					this->id[i] ^= this->id[j];
					//action ����
					this->action[i] ^= this->action[j];
					this->action[j] ^= this->action[i];
					this->action[i] ^= this->action[j];
					//x ����
					this->posX[i] ^= this->posX[j];
					this->posX[j] ^= this->posX[i];
					this->posX[i] ^= this->posX[j];
					//y ����
					this->posY[i] ^= this->posY[j];
					this->posY[j] ^= this->posY[i];
					this->posY[i] ^= this->posY[j];
				}
			}
		}

		//���� ���콺 ����� ���� ���� �ΰ��� ������ ���� �Ѵ�. ���̵� 1���� ���ٸ� ���� ���� �Ѱ��� ���� �ϰ� ������ �����ʹ� ��ӽ�Ų��.
		if (this->deviceType == VS_MODE_MOUSE)
		{
			int pContactCnt = contactCnt;
			if (this->id[0] != 1)
				contactCnt = 1;
			
			if (this->id[0] == 1 && pContactCnt > 1)
				contactCnt = 2;

			for (int i = 0; i < pContactCnt; i++)
			{
				if (i < contactCnt) 
					continue;
				else
				{
					this->id[i] = 0;
					this->action[i] = VS_TOUCH_FLAG_NONE;
					this->posX[i] = 0;
					this->posY[i] = 0;
				}
			}
		}
	}
		
	this->validCode = MSG_VALID;
		
	return this->validCode;
}