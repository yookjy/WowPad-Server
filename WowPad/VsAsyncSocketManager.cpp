#include "stdafx.h"
#include "VsAsyncSocketManager.h"

VsAsyncSocketManager::VsAsyncSocketManager(VsConfig* vsConfig)
{
	//파라미터 셋팅
	this->vsConfig = vsConfig;
	//소켓 변수 초기화
	this->udpServerSocket = NULL;
	this->tcpListenSocket = NULL;
	this->tcpDataSocket = NULL;
	//상태 초기화
	this->connectionStatus = VsAsyncSocketManager::STATUS_STANDBY;
	this->bExitExtraProcThread = FALSE;
	this->lastCheckTickCount = 0;
	this->lastReceiveTickCount = 0;
	this->lSizePrevImg = 0;
	
	ZeroMemory(this->wsaUdpEvents, sizeof(this->wsaUdpEvents));
	ZeroMemory(this->wsaTcpEvents, sizeof(this->wsaTcpEvents));

	//이미지 타입 설정
	this->nMimeType = MIME_PNG;
	switch(this->nMimeType)
	{
	case MIME_JPG:
		GDIPlus::GetEncCLSID(L"image/jpeg", &clsid);
		break;
	default:
		GDIPlus::GetEncCLSID(L"image/png", &clsid);
	}
	//인코딩 옵션 설정	
	ULONG quality = 60;
	encoderParameters.Count = 1;  
	encoderParameters.Parameter[0].Guid = EncoderQuality;  
	encoderParameters.Parameter[0].Type = EncoderParameterValueType::EncoderParameterValueTypeLong ;  
	encoderParameters.Parameter[0].NumberOfValues = 1;  
	encoderParameters.Parameter[0].Value = &quality;
	
	//사용 가능한 포트 검색
	this->udpPort = Network::GetAvailableUDPPort(DEFAULT_PORT, DEFAULT_PORT + 999, 1);
	this->tcpPort = Network::GetAvailableUDPPort(this->udpPort + 1, DEFAULT_PORT + 998, 1);
	
	this->connCode = this->vsConfig->LoadConnectCode();

	if (this->connCode < 1000 || this->connCode > 9999)
	{
		//비번 생성
		srand(time(NULL));
		this->connCode = rand() % 9000 + 1000;
	}

	//프록시 생성
	this->inputEventInjector = new VsInputEventInjector(vsConfig);
}

VsAsyncSocketManager::~VsAsyncSocketManager(void)
{
	//쓰레드에 종료 코드 전송
	this->bExitExtraProcThread = TRUE;
	//안전한 스레드 종료를 위해 잠깐 대기
	WaitForSingleObject(this->hExtraThread, 1000);
	//스레드 핸들 반환
	CloseHandle(this->hExtraThread);
	
	//UDP 수신 쓰레드 종료
	WSASetEvent(this->wsaUdpEvents[0]);
	WaitForSingleObject(this->hUdpThread, 1000);
	CloseHandle(this->hUdpThread);

	//TCP 수신 쓰레드 종료
	WSASetEvent(this->wsaTcpEvents[0]);
	WaitForSingleObject(this->hTcpThread, 1000);
	CloseHandle(this->hTcpThread);

	for (int i=0; i<sizeof(this->wsaTcpEvents) / sizeof(WSAEVENT); i++)
	{
		if (i < sizeof(this->wsaUdpEvents) / sizeof(WSAEVENT))
		{
			if (this->wsaUdpEvents[i] != NULL)
				WSACloseEvent(this->wsaUdpEvents[i]);
		}

		if (this->wsaTcpEvents[i] != NULL)
			WSACloseEvent(this->wsaTcpEvents[i]);
	}

	//소켓 닫기
	shutdown(this->tcpDataSocket, SD_BOTH);
	shutdown(this->tcpListenSocket, SD_BOTH);
	shutdown(this->udpServerSocket, SD_BOTH);
	
	closesocket(this->tcpDataSocket);
	closesocket(this->tcpListenSocket);
	closesocket(this->udpServerSocket);

	//프록시 삭제
	delete inputEventInjector;
	
	//윈소켓 API 삭제
	WSACleanup();
}

BOOL VsAsyncSocketManager::Initialize()
{
	//소켓 시작
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
	{
		TCHAR errMsg[MAX_LOADSTRING];
		wsprintf(errMsg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_INITIALIZE_SOCKET), WSAGetLastError());
		SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)errMsg);
		return 0;
	}

	//인젝션 초기화
	if (this->inputEventInjector->Initialize() == 0)
		return 0;

	//소켓 이벤트 객체
	for (int i=0; i<sizeof(this->wsaTcpEvents) / sizeof(WSAEVENT); i++)
	{
		//Tcp용 이벤트 생성
		this->wsaTcpEvents[i] = WSACreateEvent();
		//Udp용 이벤트 생성
		if (sizeof(this->wsaUdpEvents) / sizeof(WSAEVENT))
			this->wsaUdpEvents[i] = WSACreateEvent();
	}
	
	BOOL nRet = this->StartReceiveUDP() + this->StartReceiveTCP();
	
	//터치 에코 스레드
	hExtraThread = (HANDLE)_beginthreadex(NULL, 0, ThreadExtraProcess, (void*)this, 0, NULL);

	//처음 이거나, 설치된 마우스가 없거나 기본포트가 사용중일때는 연결정보 창을 띄움.
	if (vsConfig->GetConfigInt(L"Welcome", 0) > 0 || GetSystemMetrics(SM_CMOUSEBUTTONS) == 0 || DEFAULT_PORT != this->GetBroadcastPort())
	{
		this->vsConfig->ShowConnectionInfoWindow(FALSE);
		//처음 사용 플래그 삭제
		this->vsConfig->WriteConfigInt(L"Welcome", 0);
	}

	return nRet;
}

INT VsAsyncSocketManager::StartReceiveUDP()
{
	char szData[MAX_UDP_DATA_SIZE] = {0};
    INT nRet = 0, nRecv = 0, nLen = sizeof(SOCKADDR_IN);
    SOCKADDR_IN addrLocal;
		
    this->udpServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->udpServerSocket == INVALID_SOCKET)
	{
		this->ShowSocketError(nRet, IDS_MSG_ERROR_SOCKET_UDP_OPEN);
		return -1;
	}
	
	addrLocal.sin_family      = AF_INET;
    addrLocal.sin_port        = htons(udpPort);
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	
	nRet = bind(this->udpServerSocket, (PSOCKADDR)&addrLocal, sizeof(SOCKADDR_IN));

	if (nRet == SOCKET_ERROR)
    {
		this->ShowSocketError(nRet, IDS_MSG_ERROR_SOCKET_UDP_BIND);
		return -1;
    }
	
	nRet = WSAEventSelect(this->udpServerSocket, this->wsaUdpEvents[1], FD_READ);
		
	if(nRet == SOCKET_ERROR)
	{
		this->ShowSocketError(nRet, IDS_MSG_ERROR_SOCKET_UDP_ASYNC);
		return -1;
	}

	hUdpThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadReceiveUDP, (void*)this, 0, NULL);
	return 0;
}

unsigned int WINAPI VsAsyncSocketManager::ThreadReceiveUDP(void *pData)
{
	VsAsyncSocketManager* sm = (VsAsyncSocketManager*)pData;
	BOOL bContinue = TRUE;
	WSANETWORKEVENTS ntwEvents;
	DWORD dwHandleSignaled = 0;
	INT nRet = 0, cntEvt = sizeof(sm->wsaUdpEvents) / sizeof(sm->wsaUdpEvents[0]), nLen = sizeof(SOCKADDR_IN);
	SOCKADDR_IN addrRemote;
	
	const int headerSize = sizeof(int);
	Bitmap* prevBmp = NULL;
	char* imgData = NULL;
	INT sendSeq = 0, currPointer = 0;;
	BOOL isXor = FALSE;
	ULONG bytesRead = 0;
	ULONGLONG lastCaptureTicks = 0;
	SOCKET prevTcpSocket = NULL;

	// 이벤트 종류 구분하기(WSAWaitForMultipleEvents)
	while(bContinue)
	{
		dwHandleSignaled = WSAWaitForMultipleEvents(cntEvt, sm->wsaUdpEvents, FALSE, WSA_INFINITE, FALSE);
		
		switch (dwHandleSignaled)
		{
		case WSA_WAIT_EVENT_0:		//Close Event 발생
			bContinue = FALSE;
			break;
		case WSA_WAIT_EVENT_0 + 1:	//Read Event 발생
		{
			//그동안 발생된 이벤트들을 받아옴
			WSAEnumNetworkEvents(sm->udpServerSocket, sm->wsaUdpEvents[1], &ntwEvents);
			WSAResetEvent(sm->wsaUdpEvents[1]);
			if (ntwEvents.lNetworkEvents & FD_READ)
			{
				CHAR szIncoming[1024];
				VsMessage* vsMessage = NULL;
				ZeroMemory(szIncoming, sizeof(szIncoming));

				int inDataLength = recvfrom(sm->udpServerSocket,
											szIncoming,
											sizeof(szIncoming) / sizeof(szIncoming[0]),
											0,
											(PSOCKADDR) &addrRemote,
											&nLen);
				
				if (inDataLength == SOCKET_ERROR) 
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK) break;
					else sm->ShowSocketError(inDataLength, IDS_MSG_ERROR_SOCKET_NORMAL);
				}

				//메세지 형식 설정
				INT packetType = sm->GetSocketPacket(szIncoming, inDataLength, NULL);
				if (packetType == VS_SOCKET_PACKET_BROADCAST)
				{
					vsMessage = new VsMessage;
				} else if (packetType == VS_SOCKET_PACKET_COORDINATES)
				{
					vsMessage = new VsPointerMessage;
				} else if (packetType == VS_SOCKET_PACKET_INPUT_KEYBOARD)
				{
					vsMessage = new VsKeybdMessage;
				} else if (packetType == VS_SOCKET_PACKET_REQUEST_IMAGE)
				{
					vsMessage = new VsGeneralMessage;
				}
				
				//형식에 맞는 메세지 조회
				if (sm->GetSocketPacket(szIncoming, inDataLength, vsMessage))
				{
					if (sm->tcpDataSocket == 0)
					{
						//TCP 접속이 안되어 있는 경우 브로드 캐스트만 처리하고 이외는 무시
						if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_BROADCAST)
						{		
							TCHAR computerName[MAX_UDP_DATA_SIZE] = {0};
							DWORD bufCharCount = MAX_UDP_DATA_SIZE;
							GetComputerName(computerName, &bufCharCount);
			
							CHAR szComputerName[MAX_UDP_DATA_SIZE] = {0};
							WideCharToMultiByte(CP_ACP, 0, computerName, lstrlen(computerName), szComputerName, MAX_UDP_DATA_SIZE, NULL, NULL);

							int cap = sizeof(int);
							int pcNameLen = strlen(szComputerName);
							int size = (VS_SOCKET_PACKET_TEMPLATE_CNT + VS_SOCKET_PACKET_BROADCAST_CNT) * cap + pcNameLen;

							int offset = 0;
							CHAR* szConnectMsg = new CHAR[size];
							
							(int&)szConnectMsg[offset] = VS_SOCKET_PACKET_HEADER;
							(int&)szConnectMsg[offset += cap] = size;
							(int&)szConnectMsg[offset += cap] = sm->tcpPort;
							CopyMemory(szConnectMsg + (offset += cap), szComputerName, pcNameLen);
							(int&)szConnectMsg[offset += pcNameLen] = VS_SOCKET_PACKET_FOOTER;
							
							//성공이면 [접속포트|컴퓨터명]를 전송한다.
							nRet = sendto(sm->udpServerSocket,
										szConnectMsg,
										size,
										MSG_DONTROUTE,
										(PSOCKADDR)&addrRemote,
										sizeof(SOCKADDR_IN));

							//브로드 캐스트되면 접속창을 띄워 비번 표시
							TCHAR msg[MAX_LOADSTRING];
							wsprintf(msg, sm->vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_CODE_NOTICE), sm->connCode);
							sm->vsConfig->ShowNotificationWindow(msg, SW_SHOW);
						
							_D OutputDebugString(L"\n====== 브로드 캐스트 응답 ======\n");
							_D OutputDebugStringA(szConnectMsg);
							delete[] szConnectMsg;
						}
					} 
					else
					{	
						SOCKADDR_IN sock;
						int size = sizeof(sock);
						ZeroMemory(&sock, size);
						getpeername(sm->tcpDataSocket, (SOCKADDR *) &sock, &size);	
						//새로운접속이면 이미지 초기화
						if (prevTcpSocket != sm->tcpDataSocket)
						{
							prevTcpSocket = sm->tcpDataSocket;
							delete prevBmp;
							prevBmp = NULL;
							isXor = FALSE;
						}
						
						//접속 코드 및 접속된 TCP IP와 동일한지 체크 (usb로 연결된 경우 127.0.0.1이 들어올 수 있음 Windowsphone 7)
						if (vsMessage->GetConnectCode() == sm->connCode 
							&& (addrRemote.sin_addr.s_addr == sock.sin_addr.s_addr 
								|| (sock.sin_addr.S_un.S_un_b.s_b1 == 127 && sock.sin_addr.S_un.S_un_b.s_b2 == 0 && sock.sin_addr.S_un.S_un_b.s_b3 == 0 && sock.sin_addr.S_un.S_un_b.s_b4 == 1)))
						{
							//최종 수신 시간 저장
							sm->lastReceiveTickCount = GetTickCount();
							//최종 접속 리모트의 주소 저장
							CopyMemory(&sm->lastReceivedRemote, &addrRemote, sizeof(addrRemote)); 

							if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_REQUEST_IMAGE)
							{	
								sendSeq = ((VsGeneralMessage*)vsMessage)->GetSequence();
								ULONGLONG nowTick = GetTickCount64();	
								int intSize = sizeof(int);
								int headerSize = sizeof(VS_SOCKET_PACKET_HEADER) + intSize * 3 + sizeof(sendSeq);
								int frameSize = headerSize + sizeof (VS_SOCKET_PACKET_FOOTER);
								int buffSize = VS_UDP_IMG_PACKET_SIZE - frameSize;  
								int sendDataLen = buffSize;

								if (sendSeq == 0)
								{
									//마지막 캡춰 시간으로 부터  100ms가 경과 되었거나, 이전 데이터가 0인경우만 새롭게 화면 캡춰
									if ((nowTick - lastCaptureTicks > 100) || bytesRead == 0)
									{
										//이미지 초기화
										delete[] imgData;
										imgData = NULL;

										//캡춰 및 전송 시작
										POINT res = {vsMessage->GetResolutionX(), vsMessage->GetResolutionY()};
										//이미지 품질 설정
										ULONG quality = (ULONG)sm->encoderParameters.Parameter[0].Value;
										INT imgQuality = ((VsGeneralMessage*)vsMessage)->GetExtraData();
										INT flag = ((VsGeneralMessage*)vsMessage)->GetFlag();
										switch(sm->nMimeType)
										{
										case MIME_JPG:
											quality = imgQuality * 10;
											sm->encoderParameters.Parameter[0].Value = &quality;
											break;
										default:
											res.x = res.x * imgQuality / 10;
											res.y = res.y * imgQuality / 10;
										}
										
										//스크린 캡쳐
										if (flag == 1)
										{
											GDIPlus::GetScreenshot(prevBmp, &sm->clsid, &sm->encoderParameters, imgData, &bytesRead, res, FALSE);
											isXor = false;
										}
										else
											isXor = GDIPlus::GetXorScreenshot(prevBmp, (ULONG)buffSize, &sm->clsid, &sm->encoderParameters, imgData, &bytesRead, res, FALSE);
									}
								}
									 
								if (buffSize > bytesRead)
								{
									//버퍼보다 전체 용량이 작을때
									sendSeq = 0;
									currPointer = 0;
									sendDataLen = bytesRead;
								}
								else
								{
									if ((sendSeq + 1) * buffSize > bytesRead)
									{
										//마지막 데이터
										sendDataLen = bytesRead % buffSize;
										//잘못된 데이터가 들어온 경우 서버가 죽는 것을 방지하기 위해 들어온 파라미터 수정
										sendSeq = bytesRead / buffSize;
									}
									else 
									{
										sendDataLen = buffSize;
									}
									//수정됐을수도 있는 sendSeq를 반영
									currPointer = sendSeq * buffSize;
								}

								CHAR buffer[VS_UDP_IMG_PACKET_SIZE];
								(int&)buffer[0] = VS_SOCKET_PACKET_HEADER;
								int totalSize = bytesRead; 

								//사이즈에 문제가 있거나, 이전 데이터와 용량이 같고 0번 인덱스이면 0으로 보냄.
								if ((currPointer + sendDataLen > bytesRead) || (sm->lSizePrevImg == bytesRead && sendSeq == 0))
								{
									totalSize = 0;
									sendDataLen = 0;
									sendSeq = 0;
									isXor = 0;
								}
								
								(int&)buffer[4] = totalSize;	//totalSize
								(int&)buffer[8] = sendDataLen;	//crrent 패킷 사이즈
								(int&)buffer[12] = sendSeq;	//seq
								(int&)buffer[16] = isXor;	//seq
								CopyMemory(buffer + headerSize, imgData + currPointer, sendDataLen);
								(int&)buffer[headerSize + sendDataLen] = VS_SOCKET_PACKET_FOOTER;

								//헤더 전송
								int ret = sendto(sm->udpServerSocket, 
									buffer, 
									sendDataLen + frameSize,
									MSG_DONTROUTE,
									(PSOCKADDR)&addrRemote,
									sizeof(SOCKADDR_IN));

								//보낸 데이터가 존재하는 경우
								if (sendDataLen > 0)
								{
									//마지막으로 보낸값 저장
									sm->lSizePrevImg = bytesRead;
								}

								//디버그용 메세지
								_D if (sendDataLen > 0)
								_D {
								_D	TCHAR str[100] = {0};
								_D	wsprintf(str, L"xor: %d, seq: %ld, iamge size: %ld of %ld Bytes\n", isXor, sendSeq, sendDataLen, totalSize);
								_D	OutputDebugString(str);
								_D }

							}
							else
							{
								//처리 요청
								sm->GetInputEventInjector()->Request(vsMessage);
								//메세지 삭제는 큐에서 사용후 내부적으로 처리
							}
						}
					}
				}

				if ((packetType != VS_SOCKET_PACKET_COORDINATES && packetType != VS_SOCKET_PACKET_INPUT_KEYBOARD) || sm->tcpDataSocket == 0)
					delete vsMessage;
			}
			break;
		}
		}		
	}
	//이미지 해제
	delete[] imgData;
	delete prevBmp;

	return 0;
}

INT VsAsyncSocketManager::StartReceiveTCP()
{
	char szData[MAX_UDP_DATA_SIZE] = {0};
	char szResult[MAX_UDP_DATA_SIZE] = {0};
    INT nRet = 0, nRecv = 0, nLen = sizeof(SOCKADDR_IN);
    SOCKADDR_IN addrLocal;
		
	//소켓 열기
	this->tcpListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (this->tcpListenSocket == INVALID_SOCKET)
	{
		this->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_TCP_OPEN);
		return -1;
	}
	
	addrLocal.sin_family      = AF_INET;
    addrLocal.sin_port        = htons(tcpPort);
    addrLocal.sin_addr.s_addr = htonl(INADDR_ANY);

	nRet = bind(this->tcpListenSocket, (PSOCKADDR)&addrLocal, sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR)
	{
		this->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_TCP_BIND);
		return -1;
	}
	
	nRet = WSAEventSelect(this->tcpListenSocket, this->wsaTcpEvents[1], FD_ACCEPT);
	
	if(nRet == SOCKET_ERROR)
	{
		this->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_TCP_ASYNC);
		return -1;
	}
	
	if(listen(this->tcpListenSocket, 5) == SOCKET_ERROR)
	{
		this->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_TCP_LISTEN);
		return -1;
	}

	hTcpThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadReceiveTCP, (void*)this, 0, NULL);
	
	return 0;
}

unsigned int WINAPI VsAsyncSocketManager::ThreadReceiveTCP(void *pData)
{
	VsAsyncSocketManager* sm = (VsAsyncSocketManager*)pData;
	BOOL bContinue = TRUE;
	INT nRet = 0, cntEvt = sizeof(sm->wsaTcpEvents) / sizeof(sm->wsaTcpEvents[0]);
	WSANETWORKEVENTS ntwEvents;
	CHAR szIncoming[1024];
	VsGeneralMessage* vsMessage = NULL;

	// 이벤트 종류 구분하기(WSAWaitForMultipleEvents)
	while(bContinue)
	{
		DWORD dwHandleSignaled = WSAWaitForMultipleEvents(cntEvt, sm->wsaTcpEvents, FALSE, WSA_INFINITE, FALSE);
			
		switch (dwHandleSignaled)
		{
		case WSA_WAIT_EVENT_0:
			bContinue = FALSE;
			break;
		case WSA_WAIT_EVENT_0 + 1:
		{
			//그동안 발생된 이벤트들을 받아옴
			WSAEnumNetworkEvents(sm->tcpListenSocket, sm->wsaTcpEvents[1], &ntwEvents);
			WSAResetEvent(sm->wsaTcpEvents[1]);
			if (ntwEvents.lNetworkEvents & FD_ACCEPT)
			{
				//끊기 대기 상태이면 이전 접속 끊기
				if (sm->GetConnectionStatus() == VsAsyncSocketManager::STATUS_DISCONNECTABLE)
				{
					sm->vsConfig->WriteConfig(L"Connection.Closed.cause", L"timeout - tcp");
					sm->DisconnectDevice();
				}
				
				sockaddr sockAddrClient;
				int size = sizeof(sockaddr);
				SOCKET currConnSocket = accept(sm->tcpListenSocket, &sockAddrClient, &size); 
				if (currConnSocket == INVALID_SOCKET)
				{
					sm->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_NORMAL);
					bContinue = FALSE;
				} 
				else
				{
					if (sm->tcpDataSocket != 0)
					{
						sm->vsConfig->WriteConfig(L"Connection.Closed.cause", L"accept new client");
						//접속되어 있는 경우 접속 요청 제거
						closesocket(currConnSocket);
					}
					else
					{
						//소켓접속이 없는 경우 신규 접속 허용
						sm->tcpDataSocket = currConnSocket;
						sm->connectionStatus = VsAsyncSocketManager::STATUS_TRY_CONNECTION;
						nRet = WSAEventSelect(sm->tcpDataSocket, sm->wsaTcpEvents[2], FD_READ | FD_CLOSE);
						if(nRet == SOCKET_ERROR)
						{
							sm->ShowSocketError(SOCKET_ERROR, IDS_MSG_ERROR_SOCKET_TCP_ASYNC);
							bContinue = FALSE;
						}
						//최종 수신 시간 저장
						sm->lastReceiveTickCount = GetTickCount();
						//Nangle 알고리즘 사용 안함. (즉시전송 사용)
						char noDelay[1] = {'1'};
						setsockopt(sm->tcpDataSocket, IPPROTO_TCP, TCP_NODELAY, noDelay, sizeof(noDelay));
					}
				}
			}
			WSAResetEvent(sm->wsaTcpEvents[1]);
			break;
		}
		case WSA_WAIT_EVENT_0 + 2:
		{
			//그동안 발생된 이벤트들을 받아옴
			WSAEnumNetworkEvents(sm->tcpDataSocket, sm->wsaTcpEvents[2], &ntwEvents);
			WSAResetEvent(sm->wsaTcpEvents[2]);
			//접속이 끊어진 상태에서 들어오는 경우 통과. (Close 할때 한번씩 더 들어옴... 원인불명)
			if (sm->tcpDataSocket == 0) break;

			if (ntwEvents.lNetworkEvents & FD_READ)
			{
				ZeroMemory(szIncoming, sizeof(szIncoming));
				nRet = recv(sm->tcpDataSocket,
							szIncoming,
							sizeof(szIncoming) / sizeof(szIncoming[0]),
							0);

				if (nRet != SOCKET_ERROR)
				{
					BOOL bSuccess = FALSE;

					//벨로스텝 헤더 판단
					int headerSize = sizeof(VS_SOCKET_PACKET_HEADER) + sizeof(int);
					int footerSize = sizeof(VS_SOCKET_PACKET_FOOTER);

					int totLen = nRet;
					CHAR* totBuff = new CHAR[totLen];
					
					CHAR* tmpBuff = NULL;
					CHAR rpBuff[1024];
					bool rpRecieve = false; 

					//받은 문자열 복사
					CopyMemory(totBuff, szIncoming, totLen);
					
					while (true)
					{
						//시작점
						if (headerSize + footerSize > totLen || rpRecieve)
						{
							//더 받기 루프
							ZeroMemory(rpBuff, sizeof(rpBuff));
							int ret = recv(sm->tcpDataSocket,
										rpBuff,
										sizeof(rpBuff) / sizeof(rpBuff[0]),
										0);

							if (ret != SOCKET_ERROR)
							{
								//이전 데이터와 받은 데이터 결합
								tmpBuff = totBuff;
								totBuff = new CHAR[totLen + ret];
								CopyMemory(totBuff, tmpBuff, totLen);
								CopyMemory(totBuff + totLen, rpBuff, ret);
								totLen += ret; 
								delete[] tmpBuff;
								rpRecieve = false;
							}
							else
							{
								//잘못된 데이터
								break;
							}
						}
						else 
						{
							//사이즈 만큼 잘라서 처리
							int offset = 0;
							int vsHeader = Packet::CharArrayToInt(totBuff, offset);
							if (vsHeader != VS_SOCKET_PACKET_HEADER) 
							{
								break; //패킷 에러
							}

							//전체 패킷의 길이
							int dataSize = Packet::CharArrayToInt(totBuff, offset);

							if (totLen < dataSize)
							{
								rpRecieve = true;
								continue;
							}

							tmpBuff = new CHAR[dataSize];
							CopyMemory(tmpBuff, totBuff, dataSize);
							totLen -= dataSize;

							//패킷 처리
							///////////////////////////////////////////////////////////////////////////////////////////////
							vsMessage = new VsGeneralMessage;
							INT parseResult = sm->GetSocketPacket(tmpBuff, dataSize, vsMessage);
							if (parseResult == VS_SOCKET_PACKET_SUCCESS)
							{
								int cap = sizeof(int), offset = 0, size = 0;
								CHAR *ret = NULL;
																
								//인증용 패킷
								if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_AUTEHNTICATION)
								{
									UINT uLayouts = 0;
									wchar_t szBuf[10];
									HKL *lpList = NULL;
									//맥어드레스 리스트
									int macAddressCount = 0;
									byte** macAddresses = new byte*[6];

									bool bConnect = vsMessage->GetValidCode() && vsMessage->GetConnectCode() == sm->connCode;
									if (bConnect)
									{
										//모드
										sm->GetInputEventInjector()->SetTouchMode(vsMessage->GetDeviceType());
										//배터리
										sm->GetInputEventInjector()->SetBattery(vsMessage->GetBattery());
										//접속 성공된 스마트폰의 이름을 전역 변수에 저장
										sm->SetDeviceName(vsMessage->GetDeviceName());
										//최종 수신 시간 저장
										sm->lastReceiveTickCount = GetTickCount();
										//접속 성공 메세지
										sm->NotificationSocketInfo(nRet, IDS_MSG_INFO_CONNECTION_OPEN);
										//접속 정보 갱신
										sm->connectionStatus = VsAsyncSocketManager::STATUS_CONNECTED;
										//키보드 리스트
										uLayouts = GetKeyboardLayoutList(0, NULL);
										lpList = new HKL[uLayouts * sizeof(HKL)];
										uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

										//맥어드레스 리스트
										Network::GetMacAddreses(macAddresses, macAddressCount);
										
										_D OutputDebugString(L" ====== 인증 성공 ======\n");
									}
																		
									// 6 => 헤더, 총길이, 성공여부, 키보드 랭귀지 갯수, 맥주소 갯수, 풋터
									size = (6 + uLayouts) * cap + (macAddressCount * 6); 
									ret = new CHAR[size];

									(int&)ret[offset] = VS_SOCKET_PACKET_HEADER;
									(int&)ret[offset += cap] = size;
									(int&)ret[offset += cap] = bConnect;
									(int&)ret[offset += cap] = uLayouts;
									(int&)ret[offset += cap] = macAddressCount;

									//키보드 랭귀지
									for(int i = 0; i < uLayouts; ++i)
									{
										memset(szBuf, 0, 10);
										GetLocaleInfo(MAKELCID(((UINT)lpList[i] & 0xffffffff), SORT_DEFAULT), LOCALE_ILANGUAGE, szBuf, 10);
										//GetLocaleInfo(MAKELCID(((UINT)GetKeyboardLayout(0) & 0xffffffff), SORT_HUNGARIAN_DEFAULT), LOCALE_SNAME, szBuf, 10);
										(int&)ret[offset += cap] = String::DecimalToHexdecimal(_wtoi(szBuf));
									}

									offset += cap;
									offset -= sizeof(byte);
									//맥 어드레스
									for(int i = 0; i < macAddressCount; ++i)
									{
										for (int j = 0; j < 6; j++)
										{
											ret[offset += sizeof(byte)] = macAddresses[i][j];
										}
									}

									(int&)ret[offset += sizeof(byte)] = VS_SOCKET_PACKET_FOOTER;
																											
									nRet = send(sm->tcpDataSocket, ret, size, 0);

									delete[] ret;
									delete[] lpList;
									for (int i = 0; i < macAddressCount; i++)
									{
										delete[] macAddresses[i];
									}
									delete[] macAddresses;

									//인증 실패시 소켓 종료
									if (!bConnect)
									{
										_D OutputDebugString(L" ====== 인증 실패 ======\n");
										//소켓 닫기
										nRet = sm->CloseSocket(sm->tcpDataSocket, IDS_MSG_ERROR_SOCKET_TCP_CLOSE);
										//커넥트 이벤트 리셋
										WSAResetEvent(sm->wsaTcpEvents[1]);
									}
									else 
										sm->connectionStatus = VsAsyncSocketManager::STATUS_CONNECTED;

									bSuccess = TRUE;
								}
								else
								{
									if (vsMessage->GetValidCode())
									{
										if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_CHANGE_MODE
											|| vsMessage->GetPacketType() == VS_SOCKET_PACKET_VIRTUAL_BUTTON
											|| vsMessage->GetPacketType() == VS_SOCKET_PACKET_AUTO_CONNECT
											|| vsMessage->GetPacketType() == VS_SOCKET_PACKET_CHECK_CONNECTION)
										{
											//모드
											sm->GetInputEventInjector()->SetTouchMode(vsMessage->GetDeviceType());
											//배터리
											sm->GetInputEventInjector()->SetBattery(vsMessage->GetBattery());
											//접속 성공된 스마트폰의 이름을 전역 변수에 저장
											sm->SetDeviceName(((VsGeneralMessage*)vsMessage)->GetDeviceName());
											
											if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_CHECK_CONNECTION)
											{
												//접속유지 확인용 패킷
												//성공여부 리턴
												int cap = sizeof(int);
												int nInfo = 4;
												int size = (4 + nInfo) * cap;
												int offset = 0;
												CHAR* ret = new CHAR[size];
									
												//PPT SlideShow 윈도우
												int screenMode = VS_SCREEN_MODE_NONE;
												HWND currWnd = GetForegroundWindow();
												HWND pptWnd = FindWindow(L"PP12FrameClass", NULL);
												if (pptWnd && pptWnd == currWnd)
												{
													screenMode = VS_SCREEN_MODE_POWERPOINT;
												}

												if (pptWnd && FindWindow(L"screenClass", NULL) == currWnd)
												{
													screenMode = VS_SCREEN_MODE_POWERPOINT_SLIDESHOW;
												}
												
												(int&)ret[offset] = VS_SOCKET_PACKET_HEADER;
												(int&)ret[offset += cap] = size;
												(int&)ret[offset += cap] = 1;
												(int&)ret[offset += cap] = screenMode;
												(int&)ret[offset += cap] = 0;
												(int&)ret[offset += cap] = 0;
												(int&)ret[offset += cap] = 0;
												(int&)ret[offset += cap] = VS_SOCKET_PACKET_FOOTER;
																											
												nRet = send(sm->tcpDataSocket, ret, size, 0);
												bSuccess = TRUE;

												delete[] ret;	
												_D OutputDebugString(L" ====== 접속체크======\n");
											}
											else
											{
												//최종 수신 시간 저장
												sm->lastReceiveTickCount = GetTickCount();
												sm->SetDeviceName(((VsGeneralMessage*)vsMessage)->GetDeviceName());
												bSuccess = TRUE;
												//이전 이미지 사이즈 초기화
												sm->lSizePrevImg = 0;

												if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_CHANGE_MODE)
												{
													//모드 전환 패킷
													sm->vsConfig->ShowConnectionInfoWindow(TRUE);
													//접속 직후 연속 패킷으로 날라오는 경우가 있어서 .... 별로넹..
													//TCHAR msgMode[MAX_LOADSTRING];
													//int paramKey = (sm->GetDeviceMode() == VS_MODE_TOUCHSCREEN) ? 
													//				IDS_MSG_CONNECTION_MODE_TOUCH : IDS_MSG_CONNECTION_MODE_MOUSE;
													//
													//wsprintf(msgMode, sm->vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_MODE), sm->vsConfig->GetI18nMessage(paramKey));
													//sm->vsConfig->ShowNotificationWindow(msgMode, SW_SHOW);
													_D OutputDebugString(L" ====== 모드 전환 ======\n");
												}
												else if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_VIRTUAL_BUTTON)
												{
													int buttonType = ((VsGeneralMessage*)vsMessage)->GetExtraData();
													sm->GetInputEventInjector()->PressVirtualButton(buttonType);
													_D OutputDebugString(L" ====== 버튼키 눌림======\n");
												}
												else if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_AUTO_CONNECT)
												{
													sm->vsConfig->WriteConnectCode(sm->GetConnectionCode());
													_D OutputDebugString(L" ====== 자동 접속설정======\n");
												}
											}
											sm->connectionStatus = VsAsyncSocketManager::STATUS_CONNECTED;
										}
										_D else OutputDebugString(L"이도 저도 아닌 요청\n");
									}
								}
							} 
							else 
							{
								_D OutputDebugString(L"패킷오류: ");
								_D OutputDebugStringA(szIncoming);
								_D OutputDebugString(L"\n");
							}
							//메세지 삭제
							delete vsMessage;
							///////////////////////////////////////////////////////////////////////////////////////////////

							//사용한 패킷 제거
							delete[] tmpBuff;
							
							//나머지가 남았다면 루프
							if (totLen > 0)
							{
								tmpBuff = totBuff;
								totBuff = new CHAR[totLen];
								CopyMemory(totBuff, tmpBuff + dataSize, totLen);
								delete[] tmpBuff;
							}
							else
							{
								//정상 종료
								bSuccess = TRUE;
								break;
							}
						}
					}
					
					//소켓 데이터 카피본 삭제
					delete[] totBuff;

					//성공하지 못한경우 종료 시킴
					if (!bSuccess)
					{
						sm->vsConfig->WriteConfig(L"Connection.Closed.cause", L"invalid tcp packet");
						sm->CloseSocket(sm->tcpDataSocket,IDS_MSG_ERROR_SOCKET_TCP_CLOSE);
						WSAResetEvent(sm->wsaTcpEvents[2]);
					}						
				}
				else
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK) break;
					else sm->ShowSocketError(nRet, IDS_MSG_ERROR_SOCKET_NORMAL);
				}
			}
			else if ((ntwEvents.lNetworkEvents & FD_CLOSE))
			{
				sm->vsConfig->WriteConfig(L"Connection.Closed.cause", L"tcp remote closed");
				sm->DisconnectDevice();
			}
			break;
		}
		}
	}
	return 0;
}

void VsAsyncSocketManager::NotificationSocketError(INT nResult, UINT key)
{
	if (nResult == SOCKET_ERROR)
	{
		TCHAR errMsg[MAX_LOADSTRING];
		wsprintf(errMsg, vsConfig->GetI18nMessage(key), WSAGetLastError());
		this->vsConfig->ShowNotificationWindow(errMsg, SW_SHOW);
	}
}

void VsAsyncSocketManager::NotificationSocketInfo(INT nResult, UINT key)
{
	if (nResult != SOCKET_ERROR && nResult != INVALID_SOCKET)
	{
		TCHAR errMsg[MAX_LOADSTRING];
		wsprintf(errMsg, this->vsConfig->GetI18nMessage(key));
		this->vsConfig->ShowNotificationWindow(errMsg, SW_SHOW);
	}
}

void VsAsyncSocketManager::ShowSocketError(INT nResult, UINT key)
{
	if (nResult == SOCKET_ERROR)
	{
		TCHAR errMsg[MAX_LOADSTRING];
		wsprintf(errMsg, this->vsConfig->GetI18nMessage(key), WSAGetLastError());
		SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)errMsg);
	}
}

INT VsAsyncSocketManager::GetConnectionCode()
{
	return this->connCode;
}

INT VsAsyncSocketManager::GetBroadcastPort()
{
	return this->udpPort;
}
VsInputEventInjector *VsAsyncSocketManager::GetInputEventInjector()
{
	return this->inputEventInjector;
}

INT VsAsyncSocketManager::CloseSocket(SOCKET& socket, UINT key)
{
	_D OutputDebugString(L"접속 끊음\n");
	//프록시 초기화
	this->inputEventInjector->Reset();

	INT nRet = 0;
	if (socket != 0)
		nRet = closesocket(socket);

	if (nRet != SOCKET_ERROR)
	{
		memset(&lastReceivedRemote, 0, sizeof(SOCKADDR_IN));
		this->ShowSocketError(nRet, key);
		this->connectionStatus = VsAsyncSocketManager::STATUS_STANDBY;
		socket = 0;
	}
	return nRet;
}

TCHAR* VsAsyncSocketManager::GetDeviceName()
{
	return this->connectedDeviceName;
}

VOID VsAsyncSocketManager::SetDeviceName(TCHAR* deviceName)
{
	lstrcpyn(this->connectedDeviceName, deviceName, MAX_DEVICE_NAME);
}

INT VsAsyncSocketManager::DisconnectDevice()
{
	INT nRet = 0;
	if (this->tcpDataSocket != 0 || this->connectionStatus != VsAsyncSocketManager::STATUS_STANDBY)
	{
		nRet = this->CloseSocket(this->tcpDataSocket, IDS_MSG_ERROR_SOCKET_TCP_CLOSE);
		WSAResetEvent(this->wsaTcpEvents[2]);	
		this->NotificationSocketInfo(nRet, IDS_MSG_INFO_CONNECTION_CLOSE);
	}
	//최종 수신 시간 저장
	this->lastReceiveTickCount = 0;
	return nRet;
}

INT VsAsyncSocketManager::GetConnectionStatus()
{
	//상태
	//접속 대기중 : 1
	//접속 시도중 : 2
	//접속 되었음 : 3
	//접속차단 할 수 있음: 4  => 마지막 수신시간이 30분 이상 되었음. 또는 접속시도 30초가 지났음.
	if (this->tcpDataSocket != 0 && (this->connectionStatus == VsAsyncSocketManager::STATUS_CONNECTED
		|| this->connectionStatus == VsAsyncSocketManager::STATUS_TRY_CONNECTION)) 
	{
		LONG secDiff = GetTickCount() - this->lastReceiveTickCount;
		// => 마지막 수신시간이 30분 이상 되었음. 또는 접속시도 30초가 지났음.
		int closeTimeout = this->connectionStatus == VsAsyncSocketManager::STATUS_CONNECTED ? this->vsConfig->GetCloseTimeout() : 30;
		if (this->lastReceiveTickCount > 0 && closeTimeout > 0 && secDiff / 1000 > closeTimeout)
			this->connectionStatus = VsAsyncSocketManager::STATUS_DISCONNECTABLE;
	}

	return this->connectionStatus;
}

INT VsAsyncSocketManager::GetSocketPacket(CHAR* data, int packetSize, VsMessage* vsMessage)
{
	int templateSize = sizeof(VS_SOCKET_PACKET_HEADER) + sizeof(int) + sizeof(VS_SOCKET_PACKET_FOOTER);
	if (packetSize < templateSize)
	{
		return FALSE;
	}
	
	int offset = 0;
	int nBit = 0;
	BOOL bRet = FALSE;

	//벨로스텝 헤더 판단
	int vsHeader = Packet::CharArrayToInt(data, offset);
	if (vsHeader != VS_SOCKET_PACKET_HEADER) return FALSE;

	//전체 패킷의 길이
	int dataSize = Packet::CharArrayToInt(data, offset);
	//전송받은 길이와 보낸 길이가 다르면 에러
	/*
	_D	if (vsMessage != NULL)
	_D	{
	_D		TCHAR log[100];
	_D		wsprintf(log, L"Data Size: %d, Packet Length : %d\n", dataSize, packetSize); 
	_D		OutputDebugString(log);
	_D	}
	*/
	if (dataSize != packetSize) return FALSE;

	offset = dataSize - sizeof(VS_SOCKET_PACKET_FOOTER);
	dataSize -= templateSize;
	
	//벨로스텝 풋터 판단
	int vsFooter = Packet::CharArrayToInt(data, offset);
	if (vsFooter != VS_SOCKET_PACKET_FOOTER) return FALSE;
		
	CHAR* packetData = new CHAR[dataSize];
	CopyMemory(packetData, data + sizeof(VS_SOCKET_PACKET_HEADER) + sizeof(int), dataSize);

	offset = 0;
	//타입 판단
	int type = Packet::CharArrayToInt(packetData, offset);

	if (vsMessage == NULL)
	{
		delete[] packetData;
		return type;
	}

	if (type == VS_SOCKET_PACKET_BROADCAST)
	{
		//브로드 캐스트 패킷이 더이상 패킷 길이가 존재하면 오류
		if (dataSize / sizeof(int) ==  VS_SOCKET_PACKET_BROADCAST_CNT) 
		{
			vsMessage->SetPacketType(type);
			bRet = TRUE;
		}
	}
	else if (type == VS_SOCKET_PACKET_AUTEHNTICATION)
	{
		nBit = Packet::CharArrayToInt(packetData, offset);
		int validCode = MSG_INVALID_PARAMETER_CNT;
		validCode = ((VsGeneralMessage*)vsMessage)->Parse(packetData + offset);

		vsMessage->SetPacketType(type);
		vsMessage->SetConnectCode(nBit);

		if (validCode == MSG_VALID)
			bRet = TRUE;
	}
	else
	{
		nBit = Packet::CharArrayToInt(packetData, offset);
		int validCode = MSG_INVALID_PARAMETER_CNT;
		if (nBit == this->connCode)
		{
			if (type == VS_SOCKET_PACKET_COORDINATES)
			{
				if (VS_SOCKET_PACKET_COORDINATES_CNT * sizeof(int) == dataSize)
				{
					validCode = ((VsPointerMessage*)vsMessage)->Parse(packetData + offset);
				}
			}
			else if (type == VS_SOCKET_PACKET_INPUT_KEYBOARD)
			{
				if (VS_SOCKET_PACKET_KEYINPUTS_INT_CNT * sizeof(int)
					+ VS_SOCKET_PACKET_KEYINPUTS_BYTE_CNT * sizeof(byte) == dataSize)
				{
					validCode = ((VsKeybdMessage*)vsMessage)->Parse(packetData + offset);
				}
			}
			else if (type == VS_SOCKET_PACKET_REQUEST_IMAGE
				|| type == VS_SOCKET_PACKET_CHANGE_MODE
				|| type == VS_SOCKET_PACKET_VIRTUAL_BUTTON
				|| type == VS_SOCKET_PACKET_AUTO_CONNECT
				|| type == VS_SOCKET_PACKET_CHECK_CONNECTION)
			{
				validCode = ((VsGeneralMessage*)vsMessage)->Parse(packetData + offset);
			}

			vsMessage->SetPacketType(type);
			vsMessage->SetConnectCode(nBit);
			
			if (validCode == MSG_VALID)
				bRet = TRUE;
		}
		else
		{
			//패스워드 틀림 => 종료
			this->NotificationSocketInfo(0, IDS_MSG_ERROR_CONNECTCODE);
			_D OutputDebugString(L"패스워드 오류\n");
		}
	}

	delete[] packetData;
	return bRet;
}

unsigned int WINAPI VsAsyncSocketManager::ThreadExtraProcess(void *pData)
{
	VsAsyncSocketManager* sm = (VsAsyncSocketManager*)pData;
	
	while(!sm->bExitExtraProcThread)
	{
		Sleep(3000);
		DWORD currTickCount = GetTickCount();
		//3초마다 접속 상태를 체크
		if (sm->lastCheckTickCount == 0 || currTickCount - sm->lastCheckTickCount > 3000)
		{	
			//접속상태 관리
			INT status = sm->GetConnectionStatus();
			if (status == VsAsyncSocketManager::STATUS_DISCONNECTABLE)
			{
				sm->vsConfig->WriteConfig(L"Connection.Closed.cause", L"timeout - thread");
				sm->DisconnectDevice();
				sm->vsConfig->ShowConnectionInfoWindow(TRUE);
			}
			//시간 초기화
			sm->lastCheckTickCount = currTickCount;
		}
	}
	return 0;
}

INT VsAsyncSocketManager::GetDeviceMode()
{
	return this->inputEventInjector->IsTouchMode();
}

INT VsAsyncSocketManager::GetDeviceBattery()
{
	return this->inputEventInjector->GetBattery();
}