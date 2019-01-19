#pragma once
#include "VsMessage.h"

class VsPointerMessage :public VsMessage
{
private:
	//마우스/터치 스크린 속성
	INT useExtendButton;
	INT contactCnt;
	INT id[5];
	INT action[5];
	INT posX[5];
	INT posY[5];

public:
	VsPointerMessage(void);
	VsPointerMessage(const VsPointerMessage& udpMessage);
	~VsPointerMessage(void);
	//마우스/터치 스크린 메소드
	INT GetUseExtendButton(VOID);
	INT GetId(INT);
	INT GetAction(INT);
	INT GetPosX(INT);
	INT GetPosY(INT);
	INT GetIndexById(INT);
	INT GetContactCnt(VOID);
	INT Parse(CHAR*) override;
};

