#pragma once
#include "vsmessage.h"
class VsGeneralMessage :
	public VsMessage
{
private: 
	TCHAR deviceName[MAX_DEVICE_NAME];
	INT extraData;
	INT sequence;
	INT flag;
public:
	VsGeneralMessage(void);
	~VsGeneralMessage(void);
	TCHAR* GetDeviceName(void);
	INT GetExtraData();
	INT GetSequence();
	INT GetFlag();
	INT Parse(CHAR*) override;
};

