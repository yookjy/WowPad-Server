#include "stdafx.h"
#include "VsMessage.h"

VsMessage::VsMessage(const VsMessage& udpMessage)
{
	this->resolutionX = udpMessage.resolutionX;
	this->resolutionY = udpMessage.resolutionY;
	this->battery = udpMessage.battery;
	this->validCode = udpMessage.validCode;
	this->os = udpMessage.os;
	this->version = udpMessage.version;
	this->subVersion = udpMessage.subVersion;
	this->deviceType = udpMessage.deviceType;

	
}

VsMessage::VsMessage(void)
{
	this->os = 0;
	this->version = 0;
	this->subVersion = 0;
	this->deviceType = 0;

	this->resolutionX = 0;
	this->resolutionY = 0;
	this->battery = 0;
	this->validCode = MSG_INVALID_PARAMETER_CNT;
}

VsMessage::~VsMessage(void)
{
}

INT VsMessage::GetValidCode(VOID)
{
	return this->validCode;
}

VOID VsMessage::SetPacketType(INT packetType)
{
	this->packetType = packetType;
}

INT VsMessage::GetPacketType()
{
	return this->packetType;
}

VOID VsMessage::SetConnectCode(INT connectCode)
{
	this->connectCode = connectCode;
}

INT VsMessage::GetConnectCode()
{
	return this->connectCode;
}

INT VsMessage::GetOs()
{
	return os;
}

INT VsMessage::GetVersion()
{
	return version;
}

INT VsMessage::GetSubVersion()
{
	return subVersion;
}

INT VsMessage::GetResolutionX()
{
	return resolutionX;
}

INT VsMessage::GetResolutionY()
{
	return resolutionY;
}

INT VsMessage::GetBattery()
{
	return battery;
}

INT VsMessage::GetDeviceType()
{
	return deviceType;
}


INT VsMessage::Parse(CHAR* msg)
{
	int offset = 0;
	return this->Parse(msg, offset);
}

INT VsMessage::Parse(CHAR* msg, INT& offset)
{
	int nIntSize = sizeof(int);
	int len = lstrlenA(msg);
	//int nPacketCnt = len / nIntSize;

	INT nBit = Packet::CharArrayToInt(msg, offset);
	//os 체크
	if (nBit < 0) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->os = nBit;
	
	//os 메이져 버전
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit < 0) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->version = nBit;
	
	//os 마이너 버전
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit < 0) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->subVersion = nBit;
	
	//화면 가로 사이즈
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit < 200) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->resolutionX = nBit;

	//화면 세로 사이즈
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit < 200) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->resolutionY = nBit;

	//배터리 상태
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit > 101) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->battery = nBit < 0 ? 0 : nBit;
	
	//장치 타입
	nBit = Packet::CharArrayToInt(msg, offset);
	if (nBit != VS_MODE_MOUSE 
		&& nBit != VS_MODE_TOUCHSCREEN
		&& nBit != VS_MODE_KEYBOARD) 
	{
		this->validCode = offset / nIntSize;
		return this->validCode;
	}
	this->deviceType = nBit;
			
	this->validCode = MSG_VALID;
		
	return this->validCode;
}