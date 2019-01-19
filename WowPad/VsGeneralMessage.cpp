#include "stdafx.h"
#include "VsGeneralMessage.h"


VsGeneralMessage::VsGeneralMessage(void)
{
	this->extraData = 0;
	this->sequence = 0;
	ZeroMemory(&deviceName, sizeof(deviceName));
}


VsGeneralMessage::~VsGeneralMessage(void)
{
}

TCHAR* VsGeneralMessage::GetDeviceName()
{
	return this->deviceName;
}

INT VsGeneralMessage::GetExtraData()
{
	return this->extraData;
}

INT VsGeneralMessage::GetSequence()
{
	return this->sequence;
}

INT VsGeneralMessage::GetFlag()
{
	return this->flag;
}

INT VsGeneralMessage::Parse(CHAR* msg)
{
	int nBit = 0;
	int nIntSize = sizeof(int);
	int offset = 0;
	VsMessage::Parse(msg, offset);

	//다용도 키 (이미지 모드에선 이미지 품질, 가상버튼에선 버튼 타입)
	nBit = Packet::CharArrayToInt(msg, offset);
	this->extraData = nBit;

	//시퀀스
	nBit = Packet::CharArrayToInt(msg, offset);
	this->sequence= nBit;
		
	//기능 플래그 (이미지 전체 새로받기 등)
	nBit = Packet::CharArrayToInt(msg, offset);
	this->flag= nBit;

	const char* mbs = msg + offset;
	//장치명 저장
	if (strlen(mbs) > 0)
	{
		size_t requiredSize = 0;
		::mbstowcs_s(&requiredSize, NULL, 0, mbs, 0);
		wchar_t* wcs = new wchar_t[requiredSize + 1];
		::mbstowcs_s(&requiredSize, wcs, requiredSize + 1, mbs, requiredSize);
		//wchar_t* wcs = new wchar_t[requiredSize];
		//::mbstowcs_s(&requiredSize, wcs, requiredSize, mbs, requiredSize);
		if(requiredSize != 0)
		{
			lstrcpyn(this->deviceName, wcs, MAX_DEVICE_NAME);
		}
		delete[] wcs;
	}
	
	return this->validCode;
}