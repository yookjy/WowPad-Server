// WowDriver.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//
#include "stdafx.h"
#include "WowDriver.h"

typedef BOOL (__cdecl *InitializeTouchSimulatorProc)(UINT32, DWORD);
typedef BOOL (__cdecl *InjectTouchSimulatorProc)(PWOW_TOUCH_INFO);


typedef struct PointF {
	FLOAT x;
	FLOAT y;
} PointF;

static BOOL bWindows7 = -1;
static HINSTANCE hinstanceLib = NULL;
static InitializeTouchSimulatorProc InitializeTouchSimulator = NULL;
static InjectTouchSimulatorProc InjectTouchSimulator = NULL;

//버튼값
BYTE button = 0;
//이전 마우스 값
WOW_MOUSE_INFO prevMouseInfo;
//이전 터치스크린 값
WOW_TOUCH_INFO prevTouchInfo;
//이전 키보드 값
WOW_KEYBOARD_INFO prevKeyboardInfo;

int currentDeviceMode;

bool bThreadExit;

HANDLE hThread;

unsigned int WINAPI ThreadTouchEcho(void *pData);

// 윈도우 7이면 가상 드라이버 모드, 8이면 시뮬레이션(터치 인젝션) 모드
static bool IsWindows7()
{
	if (bWindows7 == -1)
		bWindows7 = !Windows8::IsWindows8();
	
	return bWindows7 == 1;
}

EXPORT PWOW_CLIENT AllocDevice(void)
{
	PWOW_CLIENT pWowClient = new WOW_CLIENT;
	ZeroMemory(pWowClient, sizeof(PWOW_CLIENT));
	pWowClient->vmultiClient = vmulti_alloc();
	return pWowClient;
}

EXPORT void FreeDevice(PWOW_CLIENT client)
{
	vmulti_free(client->vmultiClient);
	delete client;
}

EXPORT BOOL ConnectDevice(PWOW_CLIENT client)
{
	//롱프레스용 쓰레드 생성
	bThreadExit = FALSE;
	hThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadTouchEcho, (void*)client, 0, NULL);
	return vmulti_connect(client->vmultiClient);
}

EXPORT void DisconnectDevice(PWOW_CLIENT client)
{
	//롱프레스용 쓰레드 소멸
	//쓰레드에 종료 코드 전송
	bThreadExit = TRUE;
	//안전한 스레드 종료를 위해 잠깐 대기
	WaitForSingleObject(hThread, 1000);
	//스레드 핸들 반환
	CloseHandle(hThread);
	if (client != NULL)
		vmulti_disconnect(client->vmultiClient);
}

EXPORT BOOL InitializeTouchDevice(PWOW_CLIENT client, UINT32 maxCount, DWORD dwMode)
{
	if (IsWindows7())
		return true;
	else
	{
		if (hinstanceLib == NULL)
		{
			hinstanceLib = LoadLibrary(L"WowWrapper.dll");
			InitializeTouchSimulator = (InitializeTouchSimulatorProc) GetProcAddress(hinstanceLib, "InitializeTouchSimulator");
			InjectTouchSimulator = (InjectTouchSimulatorProc) GetProcAddress(hinstanceLib, "InjectTouchSimulator");
		}
		
		if (hinstanceLib == NULL)
		{
			SetLastError(ERROR_FILE_NOT_FOUND);
			return FALSE;
		}
		if (InitializeTouchSimulator == NULL)
		{
			SetLastError(ERROR_INVALID_FUNCTION);
			return FALSE;
		}
		
		return InitializeTouchSimulator(maxCount, dwMode);
	}
}
   
EXPORT BOOL InjectTouchDevice(PWOW_CLIENT client, PWOW_TOUCH_INFO touchInfo)
{
	//현재 모드 설정
	currentDeviceMode = VS_MODE_TOUCHSCREEN;
	//UAC모드 값
	POINT point;
	BOOL bUAC = !GetCursorPos(&point), bRet = FALSE;
	
	if (IsWindows7() || bUAC)
	{
		//화면 해상도 및 영역
		RECT screen;
		Windows8::GetDesktopRect(&screen);
		//화면 비율
		FLOAT rx = 1, ry = 1;
		CHAR* act = NULL;
	
		if (touchInfo->resolution.x > 0 && touchInfo->resolution.y > 0)
		{
			//폰과의 해상도 배율
			rx = (FLOAT)(screen.right - screen.left) / (FLOAT)touchInfo->resolution.x;
			ry = (FLOAT)(screen.bottom - screen.top) / (FLOAT)touchInfo->resolution.y;
		}

		//터치 드라이버 모드 구조체
		PTOUCH pTouch = new TOUCH[touchInfo->contactCnt];
		memset(pTouch, 0, sizeof(PTOUCH) * touchInfo->contactCnt);
		
		for (int i=0; i<touchInfo->contactCnt; i++)
		{
			pTouch[i].ContactID = touchInfo->id[i];
			pTouch[i].Status = MULTI_CONFIDENCE_BIT | MULTI_IN_RANGE_BIT | MULTI_TIPSWITCH_BIT;
			pTouch[i].XValue = (USHORT)((MULTI_MAX_COORDINATE / (FLOAT)screen.right) * touchInfo->position[i].x * rx);
			pTouch[i].YValue = (USHORT)((MULTI_MAX_COORDINATE / (FLOAT)screen.bottom) * touchInfo->position[i].y * ry);
			pTouch[i].Width = (USHORT)(CONTACT_WIDTH);
			pTouch[i].Height = (USHORT)(CONTACT_HEIGHT);

			if (touchInfo->action[i] == VS_TOUCH_FLAG_BEGIN)
			{
				_D act = "TOUCH DOWN";
			}
			else if (touchInfo->action[i] == VS_TOUCH_FLAG_MOVE)
			{
				_D act = "TOUCH UPDATE";
			}
			else if (touchInfo->action[i] == VS_TOUCH_FLAG_END)
			{
				_D act = "TOUCH UP";
				pTouch[i].Status = 0;
			}
			else if (touchInfo->action[i] == VS_TOUCH_FLAG_CANCELED)
			{
				_D act = "TOUCH CANCEL";
				pTouch[i].Status = 0;
			}
			else
			{
				act = "TOUCH NONE";
				pTouch[i].Status = 0;
			}
							
			_D char debugText[256] = {0};
			_D wsprintfA(debugText, "%s => x:%d, y:%d\n", act, pTouch[i].XValue, pTouch[i].YValue);
			_D OutputDebugStringA(debugText);
		}
		//드라이버 모드로 실행
		bRet = vmulti_update_multitouch(client->vmultiClient, pTouch, touchInfo->contactCnt);
		//드라이버 모드 객체 삭제
		delete[] pTouch;
	}
	else
	{
		if (InjectTouchSimulator != NULL)
			bRet = InjectTouchSimulator(touchInfo);
		else
		{
			bRet = FALSE;
			SetLastError(ERROR_INVALID_FUNCTION);
		}
	}
	//실행 시간 저장
	touchInfo->tick = GetTickCount();
	CopyMemory(&prevTouchInfo, touchInfo, sizeof(WOW_TOUCH_INFO));
	return bRet;
}

EXPORT BOOL InjectMouseDevice(PWOW_CLIENT client, PWOW_MOUSE_INFO mouseInfo)
{
	//현재 모드 설정
	currentDeviceMode = VS_MODE_MOUSE;
	BOOL bRet = FALSE;

	if (mouseInfo->id[0] == 1)
	{
		//메인 손가락이 살아 있는 경우로, 마우스 Move를 할 수 있는 경우
		//두번째 손가락은 버튼 처리에 사용
		if (mouseInfo->contactCnt == 2)
		{
			//두번째 손꾸락이 있다면 버튼을 생성한다.
			int mx = mouseInfo->position[0].x;
			int my = mouseInfo->position[0].y;
			int bx = mouseInfo->position[1].x;
			int by = mouseInfo->position[1].y;

			if (bx < mx)
			{
				if (by < my && mouseInfo->bExtendButton)
				{
					//Back 버튼
					if (mouseInfo->action[1] == VS_TOUCH_FLAG_BEGIN)
						button = VK_BROWSER_BACK;
					else if (mouseInfo->action[1] == VS_TOUCH_FLAG_END)
					{
						button = 0;
						//이전 페이지로
						WORD vks[1] = {VK_BROWSER_BACK};
						bRet = Events::ShortCutKeyEvents(1, vks);
						return bRet;
					}
				}
				else
				{
					//Left Down/Up
					if (mouseInfo->action[1] == VS_TOUCH_FLAG_BEGIN)
						button = MOUSE_BUTTON_1;
					else if (mouseInfo->action[1] == VS_TOUCH_FLAG_END)
						button = 0;
				}
			}
			else
			{
				if (by < my && mouseInfo->bExtendButton)
				{
					//Forward 버튼
					if (mouseInfo->action[1] == VS_TOUCH_FLAG_BEGIN)
						button = VK_BROWSER_FORWARD;
					else if (mouseInfo->action[1] == VS_TOUCH_FLAG_END)
					{
						button = 0;
						WORD vks[1] = {VK_BROWSER_FORWARD};
						bRet = Events::ShortCutKeyEvents(1, vks);
						return bRet;
					}
				}
				else
				{
					//Right Down/Up
					if (mouseInfo->action[1] == VS_TOUCH_FLAG_BEGIN)
						button = MOUSE_BUTTON_2;
					else if (mouseInfo->action[1] == VS_TOUCH_FLAG_END)
						button = 0;        
				}
			}
		}

		//메인 손가락 0번은 마우스 이동 및 휠값 처리에 사용
		//휠존 가로폭, 세로폭
		INT width = SCROLL_AREA_WIDTH, height = (mouseInfo->os == VS_OS_TYPE_IOS) ? width - 10 : width;
		//휠존 정의
		RECT vWheelArea = {mouseInfo->resolution.x - width, SCROLL_AREA_MARGIN, mouseInfo->resolution.x - SCROLL_AREA_MARGIN, mouseInfo->resolution.y - SCROLL_AREA_MARGIN};
		RECT hWheelArea = {SCROLL_AREA_MARGIN, mouseInfo->resolution.y - height, mouseInfo->resolution.x - SCROLL_AREA_MARGIN, mouseInfo->resolution.y - SCROLL_AREA_MARGIN};
		DWORD tick = GetTickCount();

		//이동 및 휠처리
		if (mouseInfo->action[0] == VS_TOUCH_FLAG_BEGIN)
		{
			mouseInfo->bVWheelMode = Range::Enter(vWheelArea, mouseInfo->position[0]);
			mouseInfo->bHWheelMode = Range::Enter(hWheelArea, mouseInfo->position[0]);
		}
		else if (mouseInfo->action[0] == VS_TOUCH_FLAG_MOVE)
		{
			//휠존 상태가 TRUE 이고, 이전 좌표 및 현재 좌표가 모두 휠존에 있는 경우
			if (prevMouseInfo.bVWheelMode || prevMouseInfo.bHWheelMode)
			{
				INPUT input;
				memset(&input, 0, sizeof(INPUT));
				input.type = INPUT_MOUSE;
				INT distance = 0, tGaps = (INT)(tick - prevMouseInfo.tick);

				if (prevMouseInfo.bVWheelMode && Range::EnterY(vWheelArea, prevMouseInfo.position[0]) && Range::EnterY(vWheelArea, mouseInfo->position[0]))
				{
					//세로 휠의 경우
					distance = (prevMouseInfo.position[0].y - mouseInfo->position[0].y) * ((mouseInfo->resolution.y - height) / 120);
					input.mi.dwFlags = MOUSEEVENTF_WHEEL;
					mouseInfo->bVWheelMode = TRUE;
				}
				else if (prevMouseInfo.bHWheelMode && Range::EnterX(hWheelArea, prevMouseInfo.position[0])  && Range::EnterX(hWheelArea, mouseInfo->position[0]))
				{
					//가로 휠의 경우
					distance = (mouseInfo->position[0].x - prevMouseInfo.position[0].x) * ((mouseInfo->resolution.x - width) / 120);
					input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
					mouseInfo->bHWheelMode = TRUE;
				}
				else
				{
					mouseInfo->bVWheelMode = FALSE;
					mouseInfo->bHWheelMode = FALSE;
				}

				//휠값 셋팅
				if (tGaps > 0)
				{
					input.mi.mouseData = distance * 100 / tGaps;
					::SendInput(1, &input, sizeof(INPUT));
				}
			}
			else
			{
				//마우스 이동 이벤트 발생
				mouseInfo->bVWheelMode = FALSE;
				mouseInfo->bHWheelMode = FALSE;
				FLOAT rx = 1, ry = 1;

				if (mouseInfo->resolution.x > 0 && mouseInfo->resolution.y > 0)
				{
					RECT screen;
					Windows8::GetDesktopRect(&screen);
					//폰과의 해상도 배율
					rx = (FLOAT)(screen.right - screen.left) / (FLOAT)mouseInfo->resolution.x;
					ry = (FLOAT)(screen.bottom - screen.top) / (FLOAT)mouseInfo->resolution.y;
				}
	
				BYTE bx = (BYTE)((mouseInfo->position[0].x - prevMouseInfo.position[0].x) * rx);
				BYTE by = (BYTE)((mouseInfo->position[0].y - prevMouseInfo.position[0].y) * ry);
				BYTE btn = (button == MOUSE_BUTTON_1 || button == MOUSE_BUTTON_2 ? button : 0);

				_D TCHAR szLog[100];
				_D int x = (mouseInfo->position[0].x - prevMouseInfo.position[0].x) * rx;
				_D int y = (mouseInfo->position[0].y - prevMouseInfo.position[0].y) * ry;
				_D wsprintf(szLog, L"rx : %d, ry : %d\n", x, y);
				_D OutputDebugString(szLog);
								
				bRet = vmulti_update_relative_mouse(client->vmultiClient, btn, bx, by, 0);
			}
		}
		else
		{
			//휠 설정 초기화
			mouseInfo->bVWheelMode = FALSE;
			mouseInfo->bHWheelMode = FALSE;
		}
	}
	else
	{
		//메인 손가락이 죽어 있으므로, 현재 손가락은 Up이벤트만을 처리한다. 그외의 이벤트는 무시함
		if (mouseInfo->action[0] == VS_TOUCH_FLAG_END)
		{
			button = 0;
			bRet = vmulti_update_relative_mouse(client->vmultiClient, button, 0, 0, 0);
		}
	}

	//실행 시간 저장
	mouseInfo->tick = GetTickCount();
	CopyMemory(&prevMouseInfo, mouseInfo, sizeof(WOW_MOUSE_INFO));
	return bRet;
}


EXPORT BOOL InjectKeyboardShortCut(PWOW_CLIENT client, PWOW_KEYBOARD_INFO keyboardInfo)
{
	BOOL ret = FALSE;
	//현재 키보드값 릴리즈
	WOW_KEYBOARD_INFO tmpInfo;
	ZeroMemory(&tmpInfo, sizeof(WOW_KEYBOARD_INFO));
	ret = vmulti_update_keyboard(client->vmultiClient, tmpInfo.shiftKeys, tmpInfo.keyCodes) ? TRUE : -1;

	//단축키 실행
	//KeyPress
	BYTE keyCodes[6];
	ZeroMemory(&keyCodes, sizeof(keyCodes));
	if (keyboardInfo->delayMilliseconds > 0)
	{
		//PTT 슬라이드 쇼에서 application key가 키보드로 눌렀을때 제대로 작동하지 않는 버그 때문에...
		vmulti_update_relative_mouse(client->vmultiClient, MOUSE_BUTTON_2, 0, 0, 0);
		vmulti_update_relative_mouse(client->vmultiClient, 0, 0, 0, 0);
		
		for(int i=0; i<sizeof(keyboardInfo->keyCodes) / sizeof(keyboardInfo->keyCodes[0]); i++)
		{
			keyCodes[0] = keyboardInfo->keyCodes[i];
			ret = vmulti_update_keyboard(client->vmultiClient, keyboardInfo->shiftKeys, keyCodes) ? TRUE : -2;
			Sleep(keyboardInfo->delayMilliseconds);
			
		}
	}
	else
	{
		ret = vmulti_update_keyboard(client->vmultiClient, keyboardInfo->shiftKeys, keyboardInfo->keyCodes) ? TRUE : -2;
	}
	//KeyRelease
	ret = vmulti_update_keyboard(client->vmultiClient, tmpInfo.shiftKeys, tmpInfo.keyCodes) ? TRUE : -3;
	
	//이전 키보드값 복원
	ret = vmulti_update_keyboard(client->vmultiClient, prevKeyboardInfo.shiftKeys, prevKeyboardInfo.keyCodes) ? TRUE : -4;

	return ret;
}

EXPORT BOOL InjectKeyboardDevice(PWOW_CLIENT client, PWOW_KEYBOARD_INFO keyboardInfo)
{
	BOOL ret = FALSE;
    //현재 모드 설정
	currentDeviceMode = VS_MODE_KEYBOARD;
	// See http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
	if (keyboardInfo->imeKey > 0)
	{
		//IME 키에 대한 처리
		int curImeKey = 0, rotateCnt = 0, curImeIndex = 0, reqImeIndex = -1;
		wchar_t szBuf[10];
		//키보드 리스트
		UINT uLayouts = GetKeyboardLayoutList(0, NULL);
		HKL *lpList = new HKL[uLayouts * sizeof(HKL)];
		uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

		GUITHREADINFO Gti;
		::ZeroMemory (&Gti,sizeof(GUITHREADINFO));
		Gti.cbSize = sizeof(GUITHREADINFO );
		::GetGUIThreadInfo(0,&Gti);
		DWORD dwThread = ::GetWindowThreadProcessId(Gti.hwndActive,0);
		HKL lang = ::GetKeyboardLayout(dwThread);
		//현재 활성화된 윈도우의 키보드 레이아웃
		LCID lcid = MAKELCID(((UINT)lang & 0xffffffff), SORT_DEFAULT);
		memset(szBuf, 0, 10);
		GetLocaleInfo(lcid, LOCALE_ILANGUAGE, szBuf, 10);
		//현재 키보드 언어의 HexDecimal 값
		curImeKey = String::DecimalToHexdecimal(_wtoi(szBuf));	
		//현재 키보드 언어와 요청값이 다른 경우만 처리
		if (curImeKey != keyboardInfo->imeKey)
		{
			for (int i = 0; i < uLayouts; ++i)
			{
				memset(szBuf, 0, 10);
				GetLocaleInfo(MAKELCID(((UINT)lpList[i] & 0xffffffff), SORT_DEFAULT), LOCALE_ILANGUAGE, szBuf, 10);
				//GetLocaleInfo(MAKELCID(((UINT)GetKeyboardLayout(0) & 0xffffffff), SORT_HUNGARIAN_DEFAULT), LOCALE_SNAME, szBuf, 10);
				int kbdLayout = String::DecimalToHexdecimal(_wtoi(szBuf));

				if (keyboardInfo->imeKey == kbdLayout)
				{
					reqImeIndex = i;
					break;
				}
			}

			//요청한 IME가 존재할 때만 수행
			if (reqImeIndex > -1)
			{
				WORD vks[2] = {VK_LMENU, VK_LSHIFT};
				int loopCnt = 0;
				do
				{
					Events::ShortCutKeyEvents(2, vks);
					Sleep(300);	//적용되는데 약간의 시간이 걸림

					lang = ::GetKeyboardLayout(dwThread);
					//현재 활성화된 윈도우의 키보드 레이아웃
					lcid = MAKELCID(((UINT)lang & 0xffffffff), SORT_DEFAULT);
					memset(szBuf, 0, 10);
					GetLocaleInfo(lcid, LOCALE_ILANGUAGE, szBuf, 10);
					//현재 키보드 언어의 HexDecimal 값
					curImeKey = String::DecimalToHexdecimal(_wtoi(szBuf));	
					//안전장치
					loopCnt ++;
				} while (curImeKey != keyboardInfo->imeKey && loopCnt < uLayouts);
			}
		}
	}	
	//이벤트 처리
	_D TCHAR a[256];
	_D wsprintf(a, L"shift: %d, keys : [%d, %d, %d, %d, %d, %d]\n", keyboardInfo->shiftKeys, 
	_D	keyboardInfo->keyCodes[0], keyboardInfo->keyCodes[1], keyboardInfo->keyCodes[2], 
	_D	keyboardInfo->keyCodes[3], keyboardInfo->keyCodes[4], keyboardInfo->keyCodes[5]);
	_D OutputDebugString(a);
	
	//이전값 삭제
	ZeroMemory(&prevKeyboardInfo, sizeof(WOW_KEYBOARD_INFO));

	WORD vk[1];
	switch (keyboardInfo->keyCodes[0])
	{
	case 127: 
		vk[0] = VK_VOLUME_MUTE;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 129: 
		vk[0] = VK_VOLUME_DOWN;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 128: 
		vk[0] = VK_VOLUME_UP;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 130:
		vk[0] = VK_CAPITAL;  //영수 - 일본어
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 136:
		vk[0] = VK_OEM_COPY; //VK_KANA 히라가나,가타카나, 로마자 변경
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 138:
		vk[0] = VK_CONVERT;	//변환
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 139:
		vk[0] = VK_NONCONVERT;	//무변환
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 144: 
		vk[0] = VK_HANGUL;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 145: 
		vk[0] = VK_HANJA;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 148: 
		//vk[0] = VK_JUNJA;	
		vk[0] = VK_OEM_AUTO; //VK_OEM_ENLW 전각,반각
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 176: 
		ret = Events::ShortCutKeyEvents(0, vk, 2, 0x30); //숫자 00
		break;
	case 177: 
		ret = Events::ShortCutKeyEvents(0, vk, 3, 0x30); //숫자 000
		break;
	case 235: 
		//문자표 실행
		ret = WinExec("charmap.exe", SW_SHOW) > 31 ? 1 : 0;
		break;
	case 236:
		//계산기 실행
		ret = WinExec("calc.exe", SW_SHOW) > 31 ? 1 : 0;
		break;
	case 237: 
		vk[0] = VK_MEDIA_STOP;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 238: 
		vk[0] = VK_MEDIA_PREV_TRACK;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 239: 
		vk[0] = VK_MEDIA_PLAY_PAUSE;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 240: 
		vk[0] = VK_MEDIA_NEXT_TRACK;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 241: 
		vk[0] = VK_BROWSER_BACK;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 242: 
		vk[0] = VK_BROWSER_FORWARD;
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	default:
		ret = vmulti_update_keyboard(client->vmultiClient, keyboardInfo->shiftKeys, keyboardInfo->keyCodes);
		//이전값 저장
		CopyMemory(&prevKeyboardInfo, keyboardInfo, sizeof(WOW_KEYBOARD_INFO));
		break;
	}
		
	return ret;
}

EXPORT void ReleaseDeviceEvent(PWOW_CLIENT client)
{
	//이전값들을 릴리즈 처리함
	//터치 => BEGIN, 또는 MOVE가 들어 있는경우
	BOOL isFound = false;
	for (int i = 0; i < sizeof(prevTouchInfo.action) / sizeof(INT); i++)
	{
		if (prevTouchInfo.action[i] == VS_TOUCH_FLAG_BEGIN || prevTouchInfo.action[i] == VS_TOUCH_FLAG_MOVE)
		{
			prevTouchInfo.action[i] = VS_TOUCH_FLAG_END;
			isFound = true;
		}
	}
	//터치 입력 해제
	if (isFound) 
		InjectTouchDevice(client, &prevTouchInfo);

	//마우스 => BEGIN, 또는 MOVE가 들어 있는경우
	isFound = false;
	for (int i = 0; i < sizeof(prevMouseInfo.action) / sizeof(INT); i++)
	{
		if (prevMouseInfo.action[i] == VS_TOUCH_FLAG_BEGIN || prevMouseInfo.action[i] == VS_TOUCH_FLAG_MOVE)
		{
			prevMouseInfo.action[i] = VS_TOUCH_FLAG_END;
			isFound = true;
		}
	}
	//마우스 입력 해제
	if (isFound) 
		InjectMouseDevice(client, &prevMouseInfo);

	//키보드 => 키가 눌려 있는 경우
	BYTE keyCodes[KBD_KEY_CODES] = {0, 0, 0, 0, 0, 0};
	vmulti_update_keyboard(client->vmultiClient, 0, keyCodes);
}


unsigned int WINAPI ThreadTouchEcho(void *pData)
{
	PWOW_CLIENT client = (PWOW_CLIENT)pData;
	while(!bThreadExit)
	{
		Sleep(100);
		if (currentDeviceMode == VS_MODE_TOUCHSCREEN)
		{
			DWORD tick = GetTickCount();
			
			if (prevTouchInfo.contactCnt == 1
				&& (prevTouchInfo.action[0] == VS_TOUCH_FLAG_BEGIN
					|| prevTouchInfo.action[0] == VS_TOUCH_FLAG_MOVE))
			{
				{
					prevTouchInfo.action[0] = VS_TOUCH_FLAG_MOVE;

					int nRet = InjectTouchDevice(client, &prevTouchInfo);

					_D if (nRet == 0)
					_D {
					_D 	DWORD ret = GetLastError();
					_D 	char debugText[256] = {0};
					_D 	wsprintfA(debugText, "에러 코드 : %ld\n", ret);
					_D 	OutputDebugStringA(debugText);
					_D }
				}
				prevTouchInfo.tick = tick;
			}
		}
	}
	return 0;
}
