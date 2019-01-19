#pragma once
#include "../WowDriver/WowCommon.h"

class VsMessage
{
protected:
	INT packetType;
	INT connectCode;
	INT os;
	INT version;
	INT subVersion;
	INT resolutionX;
	INT resolutionY;
	INT battery;
	INT deviceType;
	//에러 코드
	INT validCode;
public:
	VsMessage(VOID);
	VsMessage(const VsMessage&);
	~VsMessage(VOID);
	VOID SetPacketType(INT);
	INT GetPacketType(VOID);
	VOID SetConnectCode(INT);
	INT GetConnectCode(VOID);
	INT GetOs(VOID);
	INT GetVersion(VOID);
	INT GetSubVersion(VOID);
	INT GetResolutionX(VOID);
	INT GetResolutionY(VOID);
	INT GetBattery(VOID);
	INT GetDeviceType(VOID);
	//에러코드
	INT GetValidCode();
	virtual INT Parse(CHAR*);
	virtual INT Parse(CHAR*, INT&);
};

