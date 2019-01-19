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

	//�ٿ뵵 Ű (�̹��� ��忡�� �̹��� ǰ��, �����ư���� ��ư Ÿ��)
	nBit = Packet::CharArrayToInt(msg, offset);
	this->extraData = nBit;

	//������
	nBit = Packet::CharArrayToInt(msg, offset);
	this->sequence= nBit;
		
	//��� �÷��� (�̹��� ��ü ���ιޱ� ��)
	nBit = Packet::CharArrayToInt(msg, offset);
	this->flag= nBit;

	const char* mbs = msg + offset;
	//��ġ�� ����
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