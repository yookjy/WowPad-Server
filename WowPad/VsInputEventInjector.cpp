#include "stdafx.h"
#include "VsInputEventInjector.h"

VsInputEventInjector::VsInputEventInjector(VsConfig* vsConfig)
{
	this->vsConfig = vsConfig;
	this->battery = 0;
	this->bDoneEdge = FALSE;
	this->nextNotificationBattery = NOTIFICATION_BATTERY[0];
	this->stopWatch = new StopWatch;
	this->queue = new VsMessageQueue;
	//WowDriver 변수 초기화
	this->hinstanceLib = NULL;
	this->pWowClient = NULL;
}

VsInputEventInjector::~VsInputEventInjector(void)
{
	this->ClearBuffer();
	delete this->stopWatch;
	delete this->queue;

	this->stopWatch = NULL;
	this->queue = NULL;

	if (this->pWowClient != NULL)
	{
		//디바이스 닫기
		this->DisconnectDevice(this->pWowClient);
		//디바이스 메모리 해제
		this->FreeDevice(this->pWowClient);
	}
	//Dll 링크 제거
	if (hinstanceLib != NULL)
		FreeLibrary(hinstanceLib);
}

BOOL VsInputEventInjector::Initialize()
{
	hinstanceLib = LoadLibrary(L"WowDriver.dll");
	BOOL nRet = FALSE;

	if (hinstanceLib != NULL)
	{
		AllocDevice = (AllocDeviceProc) GetProcAddress(hinstanceLib, "AllocDevice");
		FreeDevice = (FreeDeviceProc) GetProcAddress(hinstanceLib, "FreeDevice");
		ConnectDevice = (ConnectDeviceProc) GetProcAddress(hinstanceLib, "ConnectDevice");
		DisconnectDevice = (DisconnectDeviceProc) GetProcAddress(hinstanceLib, "DisconnectDevice");

		InitializeTouchDevice = (InitializeTouchDeviceProc) GetProcAddress(hinstanceLib, "InitializeTouchDevice");
		InjectTouchDevice = (InjectTouchDeviceProc) GetProcAddress(hinstanceLib, "InjectTouchDevice");
		InjectMouseDevice = (InjectMouseDeviceProc) GetProcAddress(hinstanceLib, "InjectMouseDevice");
		InjectKeyboardDevice = (InjectKeyboardDeviceProc) GetProcAddress(hinstanceLib, "InjectKeyboardDevice");
		InjectKeyboardShortcut = (InjectKeyboardShortcutProc) GetProcAddress(hinstanceLib, "InjectKeyboardShortCut");
		ReleaseDeviceEvent = (ReleaseDeviceEventProc) GetProcAddress(hinstanceLib, "ReleaseDeviceEvent");
	}
	
	if (hinstanceLib == NULL)
	{
		TCHAR msg[MAX_LOADSTRING];
		wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), L"WowDriver.dll");
		SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)msg);
		return FALSE;
	}

	TCHAR* functionName = NULL;
	
	if (AllocDevice == NULL)
		functionName = L"AllocDevice";
	else if (FreeDevice == NULL)
		functionName = L"FreeDevice";
	else if (ConnectDevice == NULL)
		functionName = L"ConnectDevice";
	else if (DisconnectDevice == NULL)
		functionName = L"DisconnectDevice";
	else if (InitializeTouchDevice == NULL)
		functionName = L"InitializeTouchDevice";
	else if (InjectTouchDevice == NULL)
		functionName = L"InjectTouchDevice";
	else if (InjectMouseDevice == NULL)
		functionName = L"InjectMouseDevice";
	else if (InjectKeyboardDevice == NULL)
		functionName = L"InjectKeyboardDevice";
	else if (InjectKeyboardShortcut == NULL)
		functionName = L"InjectKeyboardShortcut";
	else if (ReleaseDeviceEvent == NULL)
		functionName = L"ReleaseDeviceEvent";

	if (functionName != NULL)
	{
		TCHAR msg[MAX_LOADSTRING];
		wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), functionName);
		SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)msg);
		return FALSE;
	}

	//디바이스 연결 준비
	this->pWowClient = this->AllocDevice();
	//디바이스 연결
	if (!this->ConnectDevice(this->pWowClient)) 
	{
		TCHAR msg[MAXCOMMENTSZ];
		wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DRIVER_FAILED));
		SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)msg);
		return FALSE;
	}
	//터치 인터페이스 초기화
	nRet = InitializeTouchDevice(this->pWowClient, MAX_TOUCH_COUNT, TOUCH_FEEDBACK_DEFAULT); 

	if (!nRet)
	{
		TCHAR msg[MAX_LOADSTRING];
		DWORD dwError = GetLastError();
		if (dwError == ERROR_FILE_NOT_FOUND)
			wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), L"WowWrapper.dll");
		else if (dwError == ERROR_INVALID_FUNCTION)
			wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), L"InitializeTouchSimulator");

		//기본적인 터치 오류 에러는 통과,  DLL에러만 캐치
		if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_INVALID_FUNCTION)
			SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)msg);
	}

	return nRet;
}

void VsInputEventInjector::ClearBuffer()
{
	//스톱워치 초기화
	this->stopWatch->Reset();
	//버퍼 비움
	this->queue->Clear();
	//엣지여부 초기화
	memset(&bEdge, 0, sizeof(bEdge));
	bDoneEdge = FALSE;
}

BOOL VsInputEventInjector::OnEdge()
{
	return bEdge.left || bEdge.top || bEdge.right || bEdge.bottom;
}

BOOL VsInputEventInjector::InnerEdge(RECT screen, RECT contact)
{
	RECT tmp;
	RECT innerEdge = {screen.left + PIXEL_BUFFER, screen.top + PIXEL_BUFFER, screen.right - PIXEL_BUFFER, screen.bottom - PIXEL_BUFFER};
	return IntersectRect(&tmp, &innerEdge, &contact);
}

BOOL VsInputEventInjector::OnEdge(RECT screen, RECT contact)
{
	BEDGE edge = this->GetEdge(screen, contact);
	return edge.left || edge.top || edge.right || edge.bottom;
}

BEDGE VsInputEventInjector::GetEdge(RECT screen, RECT contact)
{
	//픽셀버퍼의 위치
	RECT rectLeftBuff = {screen.left, screen.top, screen.left + PIXEL_BUFFER, screen.bottom};
	RECT rectTopBuff = {screen.left, screen.top, screen.right, screen.top + PIXEL_BUFFER};
	RECT rectRightBuff = {screen.right - PIXEL_BUFFER, screen.top, screen.right, screen.bottom};
	RECT rectBottomBuff = {screen.left, screen.bottom - PIXEL_BUFFER, screen.right, screen.bottom};

	//엣지 시작 여부를 저장한다.
	RECT rectDst;
	BEDGE edge = {IntersectRect(&rectDst, &rectLeftBuff, &contact), 
			IntersectRect(&rectDst, &rectTopBuff, &contact),
			IntersectRect(&rectDst, &rectRightBuff, &contact),
			IntersectRect(&rectDst, &rectBottomBuff, &contact)};
	return edge;
}

void VsInputEventInjector::SetEdge(RECT screen, RECT contact)
{
	//픽셀버퍼의 위치
	RECT rectLeftBuff = {screen.left, screen.top, screen.left + PIXEL_BUFFER, screen.bottom};
	RECT rectTopBuff = {screen.left, screen.top, screen.right, screen.top + PIXEL_BUFFER};
	RECT rectRightBuff = {screen.right - PIXEL_BUFFER, screen.top, screen.right, screen.bottom};
	RECT rectBottomBuff = {screen.left, screen.bottom - PIXEL_BUFFER, screen.right, screen.bottom};

	//엣지 시작 여부를 저장한다.
	RECT rectDst;
	bEdge.left = IntersectRect(&rectDst, &rectLeftBuff, &contact);
	bEdge.top = IntersectRect(&rectDst, &rectTopBuff, &contact);
	bEdge.right = IntersectRect(&rectDst, &rectRightBuff, &contact);
	bEdge.bottom = IntersectRect(&rectDst, &rectBottomBuff, &contact);
}

USHORT VsInputEventInjector::GetEdgeDirection(FLOAT angle)
{
	if (bEdge.left && angle > 125 && angle < 235) return LEFT_EDGE;
	else if (bEdge.top && (angle > 190 && angle < 350)) return TOP_EDGE;
	else if (bEdge.right && ((angle >= 0 && angle < 55) || (angle > 300 && angle <= 359))) return RIGHT_EDGE;
	else if (bEdge.bottom && angle > 10 && angle < 170) return BOTTOM_EDGE;
	return 0;
}

INT VsInputEventInjector::Request(VsMessage* vsMessage)
{
	if (vsMessage->GetValidCode() == MSG_VALID)
	{
		this->battery = vsMessage->GetBattery();
		if (this->battery <= nextNotificationBattery) 
		{
			int index = 0;
			for (int i=0; i<sizeof(NOTIFICATION_BATTERY); i++)
			{
				if (NOTIFICATION_BATTERY[i] == nextNotificationBattery) 
				{
					index = i + 1;
					break;
				}
			}
			index = (index == sizeof(NOTIFICATION_BATTERY)) ? 0 : index;
			nextNotificationBattery = NOTIFICATION_BATTERY[index];

			TCHAR msg[MAX_LOADSTRING];
			wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_WARN_BATTERY), this->battery);
			this->vsConfig->ShowNotificationWindow(msg, SW_SHOW);
		}

		//상태 변경
		this->SetTouchMode(vsMessage->GetDeviceType());
		this->SetBattery(vsMessage->GetBattery());
		//큐에 담기
		this->queue->Enqueue(vsMessage);

		if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_COORDINATES)
			this->Execute((VsPointerMessage*)vsMessage);
		else if (vsMessage->GetPacketType() == VS_SOCKET_PACKET_INPUT_KEYBOARD)
			this->SendInput();
	}

	return vsMessage->GetValidCode();
}

void VsInputEventInjector::Execute(VsPointerMessage* currMsg)
{
	INT touchCnt = currMsg->GetContactCnt();
	int monitorCnt = GetSystemMetrics(SM_CMONITORS);
	INT firstIndex = currMsg->GetIndexById(1);

	//다이렉트 실행
	// 1. 싱글 모니터의 터스 스크린의 모든경우 (윈8)
	// 2. 멀티플 모니티의 터치 스크린 모드 이면서 2점 터치 이상의 경우 
	if ((monitorCnt == 1 && currMsg->GetDeviceType() == VS_MODE_TOUCHSCREEN && this->vsConfig->IsWindows8()) || touchCnt > 1)
	{
		this->SendInput();
		return;
	}

	//데스크톱 해상도
	RECT screen;
	Windows8::GetDesktopRect(&screen);
	//폰과의 해상도 배율
	FLOAT rx = (FLOAT)(screen.right - screen.left) / (FLOAT)currMsg->GetResolutionX();
	FLOAT ry = (FLOAT)(screen.bottom - screen.top) / (FLOAT)currMsg->GetResolutionY();

	//현재 포인터 좌표위치
	POINT coord = {(LONG)(currMsg->GetPosX(firstIndex) * rx) + screen.left, (LONG)(currMsg->GetPosY(firstIndex) * ry) + screen.top};
	//현재 포인터의 접촉면 위치
	RECT contact = {coord.x - (CONTACT_WIDTH / 2), coord.y - (CONTACT_HEIGHT / 2), coord.x + (CONTACT_WIDTH / 2), coord.y + (CONTACT_HEIGHT / 2)};

	if (currMsg->GetAction(firstIndex) == VS_TOUCH_FLAG_BEGIN)
	{	
		_D OutputDebugString(L"터치 : 시작\n");
		//엣지 여부를 저장한다.
		this->SetEdge(screen, contact);
		
		//엣지에서 발생하지 않았으면 그냥 이벤트를 처리한다.
		if (!this->OnEdge()) 
			this->SendInput();
		else
			stopWatch->Start();
	}
	else if (currMsg->GetAction(firstIndex) == VS_TOUCH_FLAG_MOVE)
	{
		_D OutputDebugString(L"터치 : 이동\n");
		if (queue->Size() > 1)
		{
			//현재위치가 엣지 범위를 벗어나 화면내로 들어오면 제스쳐감지
			if (this->InnerEdge(screen, contact))
			{
				stopWatch->Stop();
				LONGLONG time = stopWatch->MilliSeconds();

				if (!bDoneEdge && BEGIN_TIME <= time && time <= END_TIME)
				{
					//엣지 제스쳐 처리를 한다.
					VsPointerMessage* beginMsg = (VsPointerMessage*)queue->First();
					INT beginFirstIndex = beginMsg->GetIndexById(1);
					POINT beginCoord = {(LONG)(beginMsg->GetPosX(beginFirstIndex) * rx) + screen.left, (LONG)(beginMsg->GetPosY(beginFirstIndex) * ry) + screen.top};
					//제스쳐의 각도를 구한다.
					FLOAT angle = (FLOAT)ToDegree(atan2((FLOAT)beginCoord.y - coord.y, (FLOAT)beginCoord.x - coord.x)) ;
					if(angle<0) angle += 360;
					InjectShortcutEvent(angle);
					bDoneEdge = TRUE;
				}
				else if (time > END_TIME)
				{
					this->SendInput();
				}
			}
			else if (!this->OnEdge(screen, contact))
			{
				//현재 커서의 위치가 아얘 메인화면을 벗어난 경우
				this->SendInput();
				_D OutputDebugString(L"엣지버퍼 : 벗어남\n");
			}
			else
			{
				_D OutputDebugString(L"엣지버퍼 : 적재중\n");
			}
		}
		else
		{
			this->SendInput();
		}
	}
	else if (currMsg->GetAction(firstIndex) == VS_TOUCH_FLAG_END)
	{
		_D OutputDebugString(L"터치 : 종료\n");
		if (!this->OnEdge() && !bDoneEdge) 
			this->SendInput();
		else
			this->ClearBuffer();
	}
	else if (currMsg->GetAction(firstIndex) == VS_TOUCH_FLAG_CANCELED)
	{
		_D OutputDebugString(L"터치 : 취소\n");
		this->ClearBuffer();
	}
	else
	{
		_D OutputDebugString(L"터치 : 없음\n");
		if (!this->OnEdge() && !bDoneEdge) 
			this->SendInput();
		else
			this->ClearBuffer();
	}
}

INT VsInputEventInjector::SendInput()
{
	VsMessage* currMsg = NULL;
	while((currMsg = queue->Dequeue()) != NULL)
	{
		switch(currMsg->GetDeviceType())
		{
		case VS_MODE_TOUCHSCREEN:
			this->InjectTouchEvent((VsPointerMessage*)currMsg);
			break;
		case VS_MODE_MOUSE:
			this->InjectMouseEvent((VsPointerMessage*)currMsg);
			break;
		case VS_MODE_KEYBOARD:
			this->InjectKeyboardEvent((VsKeybdMessage*)currMsg);
			break;
		case VS_MODE_JOYSTIC:
			break;
		case VS_MODE_STYLUS_PEN:
			break;
		}
		delete currMsg;
	}
	
	this->ClearBuffer();
	return 0;
}

void VsInputEventInjector::InjectMouseEvent(VsPointerMessage* udpMsg)
{
	WOW_MOUSE_INFO mouseInfo;
	ZeroMemory(&mouseInfo, sizeof(WOW_MOUSE_INFO));
	mouseInfo.os = udpMsg->GetOs();
	mouseInfo.resolution.x = udpMsg->GetResolutionX() - 1;
	mouseInfo.resolution.y = udpMsg->GetResolutionY() - (udpMsg->GetOs() == VS_OS_TYPE_IOS ? 9 : 1);
	mouseInfo.contactCnt = udpMsg->GetContactCnt();
	mouseInfo.bExtendButton = udpMsg->GetUseExtendButton();

	for (int i=0; i<udpMsg->GetContactCnt(); i++)
	{
		mouseInfo.id[i] = udpMsg->GetId(i);
		mouseInfo.action[i] = udpMsg->GetAction(i);
		mouseInfo.position[i].x = udpMsg->GetPosX(i);
		mouseInfo.position[i].y = udpMsg->GetPosY(i);
	}

	//마우스 이벤트 발생
	this->InjectMouseDevice(this->pWowClient, &mouseInfo);
}

void VsInputEventInjector::InjectTouchEvent(VsPointerMessage* udpMsg)
{
	WOW_TOUCH_INFO touchInfo;
	ZeroMemory(&touchInfo, sizeof(WOW_TOUCH_INFO));
	touchInfo.os = udpMsg->GetOs();
	touchInfo.resolution.x = udpMsg->GetResolutionX() - 1;
	touchInfo.resolution.y = udpMsg->GetResolutionY() - (udpMsg->GetOs() == VS_OS_TYPE_IOS ? 9 : 1);
	touchInfo.contactCnt = udpMsg->GetContactCnt();

	for (int i=0; i<udpMsg->GetContactCnt(); i++)
	{
		touchInfo.id[i] = udpMsg->GetId(i);
		touchInfo.position[i].x = udpMsg->GetPosX(i);
		touchInfo.position[i].y = udpMsg->GetPosY(i);
		touchInfo.action[i] = udpMsg->GetAction(i);
	}
	//터치 이벤트 발생
	BOOL nRet = this->InjectTouchDevice(this->pWowClient, &touchInfo);

	if (!nRet)
	{
		TCHAR msg[MAX_LOADSTRING];
		DWORD dwError = GetLastError();
		if (dwError == ERROR_FILE_NOT_FOUND)
			wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), L"WowWrapper.dll");
		else if (dwError == ERROR_INVALID_FUNCTION)
			wsprintf(msg, vsConfig->GetI18nMessage(IDS_MSG_ERROR_DLL_MISSING), L"InjectTouchSimulator");
		
		//기본적인 터치 오류 에러는 통과,  DLL에러만 캐치
		if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_INVALID_FUNCTION)
			SendMessage(this->vsConfig->GetHWnd(), WM_DESTROY, MAKEWPARAM(0, IDS_MSG_TITLE_ERROR), (LPARAM)msg);
	}
}

VOID VsInputEventInjector::InjectShortcutEvent(FLOAT angle)
{
	USHORT nCnt = 0, nItems = 0, nDirection = 0, nShortcutIndex = 0, nExec =1;
	WORD* shortcutKeys = NULL;
	UINT *edgeLblIds = NULL, *defaultGestures = NULL;
	EDGE_GEUSTURE_INFO* edgeInfo = NULL;
	nDirection = this->GetEdgeDirection(angle);

	if (nDirection == 0) 
	{
		_D OutputDebugString(L"엣지버퍼 : 감지할 수 없음\n");
		return;
	}

	_D TCHAR szDirection[50];
	_D if (nDirection == LEFT_EDGE)
	_D 	wsprintf(szDirection, L"엣지버퍼 : %s\n", L"좌측");
	_D else if(nDirection == RIGHT_EDGE)
	_D 	wsprintf(szDirection, L"엣지버퍼 : %s\n", L"우측");
	_D else if(nDirection == BOTTOM_EDGE)
	_D 	wsprintf(szDirection, L"엣지버퍼 : %s\n", L"하단");
	_D else if(nDirection == TOP_EDGE)
	_D 	wsprintf(szDirection, L"엣지버퍼 : %s\n", L"상단");
	_D OutputDebugString(szDirection);	 
	
	WOW_KEYBOARD_INFO wowKeybdInfo;
	ZeroMemory(&wowKeybdInfo, sizeof(WOW_KEYBOARD_INFO));

	this->vsConfig->GetEdgeGesture(nCnt, edgeInfo);
	this->vsConfig->GetEdgeLabels(nDirection, nItems, edgeLblIds);
	//nDirection은 1부터 시작하므로 1을 빼주면 해당 제스쳐에 대한 액션이 나온다.
	switch(edgeLblIds[edgeInfo[nDirection - 1].index])
	{
	case IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH_WIN8:
	{
		if (!Windows8::ExistsActiveApps())
		{
			TCHAR msg[MAX_LOADSTRING];
			wsprintf(msg, this->vsConfig->GetI18nMessage(IDS_MSG_INFO_NO_TASK));
			this->vsConfig->ShowNotificationWindow(msg, SW_SHOW);
		}
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT | KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 42;	//Backspace
		break;
	}
	case IDS_SETTINGS_EDGE_GESTURE_CHARM_BAR:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 6;	//C
		break;
	case IDS_SETTINGS_EDGE_GESTURE_APP_BAR:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 29;	//Z
		break;
	case IDS_SETTINGS_EDGE_GESTURE_MINIMIZE:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 81;	//↓
		break;
	case IDS_SETTINGS_EDGE_GESTURE_LOCK:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 15;	//L
		break;
	case IDS_SETTINGS_EDGE_GESTURE_THUMBNAIL:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 23;	//T
		break;
	case IDS_SETTINGS_EDGE_GESTURE_EXPLORE:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 8; //E
		break;
	case IDS_SETTINGS_EDGE_GESTURE_SEARCH:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 9;	//F
		break;
	case IDS_SETTINGS_EDGE_GESTURE_RUN:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;
		wowKeybdInfo.keyCodes[0] = 21;	//R
		break;
	case IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT | KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 43;	//TAB
		break;
	case IDS_SETTINGS_EDGE_GESTURE_PROGRAM_EXIT:
		wowKeybdInfo.shiftKeys = KBD_LALT_BIT;
		wowKeybdInfo.keyCodes[0] = 61;	//F4
		break;
	case IDS_SETTINGS_EDGE_GESTURE_VOL_UP:
		nShortcutIndex = 1;
		shortcutKeys = new WORD[nShortcutIndex];
		shortcutKeys[0] = VK_VOLUME_UP;
		break;
	case IDS_SETTINGS_EDGE_GESTURE_VOL_DOWN:
		nShortcutIndex = 1;
		shortcutKeys = new WORD[nShortcutIndex];
		shortcutKeys[0] = VK_VOLUME_DOWN;
		break;
	case IDS_SETTINGS_EDGE_GESTURE_VOL_MUTE:
		nShortcutIndex = 1;
		shortcutKeys = new WORD[nShortcutIndex];
		shortcutKeys[0] = VK_VOLUME_MUTE;
		break;
	case IDS_SETTINGS_EDGE_GESTURE_BROWSER_BACK:
		nShortcutIndex = 1;
		shortcutKeys = new WORD[nShortcutIndex];
		shortcutKeys[0] = VK_BROWSER_BACK;
		break;
	case IDS_SETTINGS_EDGE_GESTURE_BROWSER_FORWARD:
		nShortcutIndex = 1;
		shortcutKeys = new WORD[nShortcutIndex];
		shortcutKeys[0] = VK_BROWSER_FORWARD;
		break;
	default:
	{
		//사용자 정의 키
		BYTE combKey = HIBYTE(edgeInfo[nDirection - 1].hotKey);
		BYTE virtkey = LOBYTE(edgeInfo[nDirection - 1].hotKey);
		BYTE dfKey[3] = {HOTKEYF_CONTROL, HOTKEYF_SHIFT, HOTKEYF_ALT};
		BYTE dfVKey[3] = {VK_LCONTROL, VK_LSHIFT, VK_LMENU};
		shortcutKeys = new WORD[5];
		//Ext가 뭔지 모르니... 제거
		combKey &= ~HOTKEYF_EXT;
		BYTE tmpKey = combKey;
				
		for (int i=0; i<sizeof(dfKey) / sizeof(dfKey[0]); i++)
		{
			combKey &= ~dfKey[i];
			if (combKey != tmpKey)
			{
				tmpKey = combKey;
				shortcutKeys[i] = dfVKey[i];
				nShortcutIndex++;
			}
		}
				
		if (edgeInfo[nDirection - 1].winKey)
		{
			shortcutKeys[nShortcutIndex] = VK_LWIN;
			nShortcutIndex++;
		}
				
		if (virtkey > 0)
		{
			shortcutKeys[nShortcutIndex] = virtkey;
			nShortcutIndex++;
		}
	}
	}

	if (wowKeybdInfo.keyCodes[0] != 0 || wowKeybdInfo.shiftKeys != 0)
	{
		this->InjectKeyboardShortcut(this->pWowClient, &wowKeybdInfo);
	}
	else
	{
		//엣지 이벤트 실행
		for (int i=0; i<nExec; i++)
			Events::ShortCutKeyEvents(nShortcutIndex, shortcutKeys);
	}

	delete[] edgeLblIds;
}

void VsInputEventInjector::InjectKeyboardEvent(VsKeybdMessage* udpMsg)
{
	WOW_KEYBOARD_INFO wowKeybdInfo;
	wowKeybdInfo.shiftKeys = udpMsg->GetShiftFlags();
	CopyMemory(&wowKeybdInfo.keyCodes, udpMsg->GetKeyCodes(), sizeof(wowKeybdInfo.keyCodes) / sizeof(BYTE));
	wowKeybdInfo.imeKey = udpMsg->GetImeKey();
	this->InjectKeyboardDevice(this->pWowClient, &wowKeybdInfo);
}

void VsInputEventInjector::PressVirtualButton(INT buttonType)
{
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

	bool bExecute = (screenMode == VS_SCREEN_MODE_POWERPOINT_SLIDESHOW);
	WOW_KEYBOARD_INFO wowKeybdInfo;
	ZeroMemory(&wowKeybdInfo, sizeof(WOW_KEYBOARD_INFO));

	switch(buttonType)
	{
	case VS_BUTTON_WINDOWS:
		wowKeybdInfo.shiftKeys = KBD_LGUI_BIT;	//Windows Key
		bExecute = (screenMode == VS_SCREEN_MODE_NONE);
		break;
	case VS_BUTTON_OPEN_SLIDESHOW_FIRST_PAGE:
		wowKeybdInfo.keyCodes[0] = 62;	//F5
		bExecute = (screenMode == VS_SCREEN_MODE_POWERPOINT);
		break;
	case VS_BUTTON_OPEN_SLIDESHOW_CURRENT_PAGE:
		wowKeybdInfo.shiftKeys = KBD_LSHIFT_BIT;
		wowKeybdInfo.keyCodes[0] = 62;	//F5
		bExecute = (screenMode == VS_SCREEN_MODE_POWERPOINT);
		break;
	case VS_BUTTON_CLOSE_SLIDESHOW:
		//wowKeybdInfo.keyCodes[0] = 41;	//ESC
		wowKeybdInfo.keyCodes[0] = 101;		//application menu
		wowKeybdInfo.keyCodes[1] = 8;		//E
		break;
	case VS_BUTTON_PREVIOUS_SLIDE:
		wowKeybdInfo.keyCodes[0] = 80;	//<-
		break;
	case VS_BUTTON_NEXT_SLIDE:
		wowKeybdInfo.keyCodes[0] = 79;	//->
		break;
	case VS_BUTTON_SHOW_POINTER:
		wowKeybdInfo.shiftKeys = KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 4;	//A
		break;
	case VS_BUTTON_BALLPOINT_PEN:
		wowKeybdInfo.keyCodes[0] = 101;		//application menu
		wowKeybdInfo.keyCodes[1] = 18;		//O
		wowKeybdInfo.keyCodes[2] = 5;		//B
		wowKeybdInfo.delayMilliseconds = 50;
		break;
	case VS_BUTTON_INK_PEN:
		wowKeybdInfo.shiftKeys = KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 19;	//P
		break;
	case VS_BUTTON_INK_TOGGLE:
		wowKeybdInfo.shiftKeys = KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 16;	//M
		break;
	case VS_BUTTON_HIGHLIGHTER:
		wowKeybdInfo.keyCodes[0] = 101;		//application menu
		wowKeybdInfo.keyCodes[1] = 18;		//O
		wowKeybdInfo.keyCodes[2] = 11;		//H
		wowKeybdInfo.delayMilliseconds = 50;
		break;
	case VS_BUTTON_ERASER:
		wowKeybdInfo.shiftKeys = KBD_LCONTROL_BIT;
		wowKeybdInfo.keyCodes[0] = 8;	//E
		break;
	case VS_BUTTON_PAGE_ERAGER:
		wowKeybdInfo.keyCodes[0] = 101;		//application menu
		wowKeybdInfo.keyCodes[1] = 18;		//O
		wowKeybdInfo.keyCodes[2] = 8;		//E
		wowKeybdInfo.delayMilliseconds = 50;
		break;
	default:
		buttonType = VS_BUTTON_NONE;
		bExecute = false;
		break;
	}

	if (bExecute)
	{
		this->InjectKeyboardShortcut(this->pWowClient, &wowKeybdInfo);
		//if (buttonType == VS_BUTTON_CLOSE_SLIDESHOW)
		//{
		//	Sleep(1000);
		//	wowKeybdInfo.keyCodes[0] = 7;	//D 
		//	this->InjectKeyboardShortcut(this->pWowClient, &wowKeybdInfo);
		//}
	}
}

void VsInputEventInjector::SetTouchMode(INT mode)
{
	BOOL bMode = mode == VS_MODE_TOUCHSCREEN;
	this->bTouchMode = bMode;
}

BOOL VsInputEventInjector::IsTouchMode()
{
	return bTouchMode;
}

INT VsInputEventInjector::GetBattery()
{
	return this->battery;
}

void VsInputEventInjector::SetBattery(INT battery)
{
	this->battery = battery;
}

void VsInputEventInjector::Reset()
{
	this->battery = 0;
	this->nextNotificationBattery = NOTIFICATION_BATTERY[0];
	this->ClearBuffer();
	this->ReleaseDeviceEvent(this->pWowClient);
}