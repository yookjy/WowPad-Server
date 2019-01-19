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
		//브라우저 버튼 사용 여부
		nBit = Packet::CharArrayToInt(msg, offset);
		this->useExtendButton = nBit;

		for (int i = 0 ;  i < 5 ; i++)
		{
			//아이디
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->id[i] = nBit;

			//액션
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

			//x 좌표
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->posX[i] = nBit;

			//y 좌표
			nBit = Packet::CharArrayToInt(msg, offset);
			if (nBit < 0) 
			{
				this->validCode = offset / nIntSize;
				return this->validCode;
			}
			this->posY[i] = nBit;
		}

		//아이디 순서대로 정렬한다.
		bool hasFirst = false;
		for (int i = 0; i < this->contactCnt; i++) 
		{
			for (int j = i + 1; j < this->contactCnt; j++)
			{
				if (this->id[i] > this->id[j])
				{
					//id 스왑
					this->id[i] ^= this->id[j];
					this->id[j] ^= this->id[i];
					this->id[i] ^= this->id[j];
					//action 스왑
					this->action[i] ^= this->action[j];
					this->action[j] ^= this->action[i];
					this->action[i] ^= this->action[j];
					//x 스왑
					this->posX[i] ^= this->posX[j];
					this->posX[j] ^= this->posX[i];
					this->posX[i] ^= this->posX[j];
					//y 스왑
					this->posY[i] ^= this->posY[j];
					this->posY[j] ^= this->posY[i];
					this->posY[i] ^= this->posY[j];
				}
			}
		}

		//또한 마우스 모드라면 가장 작은 두개의 값만을 유지 한다. 아이디 1번이 없다면 가장 작은 한개만 유지 하고 나머지 데이터는 드롭시킨다.
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