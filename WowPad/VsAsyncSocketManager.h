#pragma once

#include "VsMessage.h"
#include "VsGeneralMessage.h"
#include "VsInputEventInjector.h"

#define VS_UDP_IMG_PACKET_SIZE		64000
#define MAX_UDP_DATA_SIZE			512
#define DEFAULT_PORT				9000			//기본UDP포트 

#define MIME_PNG					0
#define MIME_JPG					1

#define VS_SOCKET_PACKET_SUCCESS			1
#define VS_SOCKET_PACKET_ERROR_PARSE		0
#define VS_SOCKET_PACKET_ERROR_PASSWORD		-1



class VsAsyncSocketManager
{
private:
	//멤버변수
	VsConfig* vsConfig;
	WSADATA wsa;
	SOCKET udpServerSocket, tcpDataSocket, tcpListenSocket;
	WSAEVENT wsaUdpEvents[2], wsaTcpEvents[3];
	INT udpPort, tcpPort,  connCode;
	SOCKADDR_IN lastReceivedRemote;
	HANDLE hTcpThread, hUdpThread, hExtraThread;
	CLSID clsid;
	EncoderParameters encoderParameters;  
	TCHAR connectedDeviceName[MAX_DEVICE_NAME];
	INT connectionStatus, nMimeType;
	ULONG lSizePrevImg;
	//멤버함수
	VsInputEventInjector* inputEventInjector;
	void NotificationSocketError(INT, UINT);
	void NotificationSocketInfo(INT, UINT);
	void ShowSocketError(INT nResult, UINT);
	INT CloseSocket(SOCKET&, UINT);
	INT GetSocketPacket(CHAR*, int size, VsMessage*);
	BOOL bExitExtraProcThread;
	DWORD lastCheckTickCount, lastReceiveTickCount;
	INT StartReceiveTCP();
	INT StartReceiveUDP();
	static unsigned int WINAPI ThreadReceiveTCP(void*);
	static unsigned int WINAPI ThreadReceiveUDP(void*);
	static unsigned int WINAPI ThreadExtraProcess(void*);
public:
	VsAsyncSocketManager(VsConfig*);
	~VsAsyncSocketManager(void);
	INT Initialize(void);
	INT GetConnectionCode();
	INT GetBroadcastPort();
	INT GetConnectionStatus();
	VsInputEventInjector* GetInputEventInjector();
	TCHAR* GetDeviceName();
	VOID SetDeviceName(TCHAR*);
	INT DisconnectDevice();
	INT GetDeviceMode();
	INT GetDeviceBattery();

	static const INT STATUS_STANDBY = 1;
	static const INT STATUS_TRY_CONNECTION = 2;
	static const INT STATUS_CONNECTED = 3;
	static const INT STATUS_DISCONNECTABLE = 4;
};

