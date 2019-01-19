// WowDriver.cpp : DLL ���� ���α׷��� ���� ������ �Լ��� �����մϴ�.
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

//��ư��
BYTE button = 0;
//���� ���콺 ��
WOW_MOUSE_INFO prevMouseInfo;
//���� ��ġ��ũ�� ��
WOW_TOUCH_INFO prevTouchInfo;
//���� Ű���� ��
WOW_KEYBOARD_INFO prevKeyboardInfo;

int currentDeviceMode;

bool bThreadExit;

HANDLE hThread;

unsigned int WINAPI ThreadTouchEcho(void *pData);

// ������ 7�̸� ���� ����̹� ���, 8�̸� �ùķ��̼�(��ġ ������) ���
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
	//���������� ������ ����
	bThreadExit = FALSE;
	hThread = (HANDLE)_beginthreadex(NULL, 0, &ThreadTouchEcho, (void*)client, 0, NULL);
	return vmulti_connect(client->vmultiClient);
}

EXPORT void DisconnectDevice(PWOW_CLIENT client)
{
	//���������� ������ �Ҹ�
	//�����忡 ���� �ڵ� ����
	bThreadExit = TRUE;
	//������ ������ ���Ḧ ���� ��� ���
	WaitForSingleObject(hThread, 1000);
	//������ �ڵ� ��ȯ
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
	//���� ��� ����
	currentDeviceMode = VS_MODE_TOUCHSCREEN;
	//UAC��� ��
	POINT point;
	BOOL bUAC = !GetCursorPos(&point), bRet = FALSE;
	
	if (IsWindows7() || bUAC)
	{
		//ȭ�� �ػ� �� ����
		RECT screen;
		Windows8::GetDesktopRect(&screen);
		//ȭ�� ����
		FLOAT rx = 1, ry = 1;
		CHAR* act = NULL;
	
		if (touchInfo->resolution.x > 0 && touchInfo->resolution.y > 0)
		{
			//������ �ػ� ����
			rx = (FLOAT)(screen.right - screen.left) / (FLOAT)touchInfo->resolution.x;
			ry = (FLOAT)(screen.bottom - screen.top) / (FLOAT)touchInfo->resolution.y;
		}

		//��ġ ����̹� ��� ����ü
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
		//����̹� ���� ����
		bRet = vmulti_update_multitouch(client->vmultiClient, pTouch, touchInfo->contactCnt);
		//����̹� ��� ��ü ����
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
	//���� �ð� ����
	touchInfo->tick = GetTickCount();
	CopyMemory(&prevTouchInfo, touchInfo, sizeof(WOW_TOUCH_INFO));
	return bRet;
}

EXPORT BOOL InjectMouseDevice(PWOW_CLIENT client, PWOW_MOUSE_INFO mouseInfo)
{
	//���� ��� ����
	currentDeviceMode = VS_MODE_MOUSE;
	BOOL bRet = FALSE;

	if (mouseInfo->id[0] == 1)
	{
		//���� �հ����� ��� �ִ� ����, ���콺 Move�� �� �� �ִ� ���
		//�ι�° �հ����� ��ư ó���� ���
		if (mouseInfo->contactCnt == 2)
		{
			//�ι�° �ղٶ��� �ִٸ� ��ư�� �����Ѵ�.
			int mx = mouseInfo->position[0].x;
			int my = mouseInfo->position[0].y;
			int bx = mouseInfo->position[1].x;
			int by = mouseInfo->position[1].y;

			if (bx < mx)
			{
				if (by < my && mouseInfo->bExtendButton)
				{
					//Back ��ư
					if (mouseInfo->action[1] == VS_TOUCH_FLAG_BEGIN)
						button = VK_BROWSER_BACK;
					else if (mouseInfo->action[1] == VS_TOUCH_FLAG_END)
					{
						button = 0;
						//���� ��������
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
					//Forward ��ư
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

		//���� �հ��� 0���� ���콺 �̵� �� �ٰ� ó���� ���
		//���� ������, ������
		INT width = SCROLL_AREA_WIDTH, height = (mouseInfo->os == VS_OS_TYPE_IOS) ? width - 10 : width;
		//���� ����
		RECT vWheelArea = {mouseInfo->resolution.x - width, SCROLL_AREA_MARGIN, mouseInfo->resolution.x - SCROLL_AREA_MARGIN, mouseInfo->resolution.y - SCROLL_AREA_MARGIN};
		RECT hWheelArea = {SCROLL_AREA_MARGIN, mouseInfo->resolution.y - height, mouseInfo->resolution.x - SCROLL_AREA_MARGIN, mouseInfo->resolution.y - SCROLL_AREA_MARGIN};
		DWORD tick = GetTickCount();

		//�̵� �� ��ó��
		if (mouseInfo->action[0] == VS_TOUCH_FLAG_BEGIN)
		{
			mouseInfo->bVWheelMode = Range::Enter(vWheelArea, mouseInfo->position[0]);
			mouseInfo->bHWheelMode = Range::Enter(hWheelArea, mouseInfo->position[0]);
		}
		else if (mouseInfo->action[0] == VS_TOUCH_FLAG_MOVE)
		{
			//���� ���°� TRUE �̰�, ���� ��ǥ �� ���� ��ǥ�� ��� ������ �ִ� ���
			if (prevMouseInfo.bVWheelMode || prevMouseInfo.bHWheelMode)
			{
				INPUT input;
				memset(&input, 0, sizeof(INPUT));
				input.type = INPUT_MOUSE;
				INT distance = 0, tGaps = (INT)(tick - prevMouseInfo.tick);

				if (prevMouseInfo.bVWheelMode && Range::EnterY(vWheelArea, prevMouseInfo.position[0]) && Range::EnterY(vWheelArea, mouseInfo->position[0]))
				{
					//���� ���� ���
					distance = (prevMouseInfo.position[0].y - mouseInfo->position[0].y) * ((mouseInfo->resolution.y - height) / 120);
					input.mi.dwFlags = MOUSEEVENTF_WHEEL;
					mouseInfo->bVWheelMode = TRUE;
				}
				else if (prevMouseInfo.bHWheelMode && Range::EnterX(hWheelArea, prevMouseInfo.position[0])  && Range::EnterX(hWheelArea, mouseInfo->position[0]))
				{
					//���� ���� ���
					distance = (mouseInfo->position[0].x - prevMouseInfo.position[0].x) * ((mouseInfo->resolution.x - width) / 120);
					input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
					mouseInfo->bHWheelMode = TRUE;
				}
				else
				{
					mouseInfo->bVWheelMode = FALSE;
					mouseInfo->bHWheelMode = FALSE;
				}

				//�ٰ� ����
				if (tGaps > 0)
				{
					input.mi.mouseData = distance * 100 / tGaps;
					::SendInput(1, &input, sizeof(INPUT));
				}
			}
			else
			{
				//���콺 �̵� �̺�Ʈ �߻�
				mouseInfo->bVWheelMode = FALSE;
				mouseInfo->bHWheelMode = FALSE;
				FLOAT rx = 1, ry = 1;

				if (mouseInfo->resolution.x > 0 && mouseInfo->resolution.y > 0)
				{
					RECT screen;
					Windows8::GetDesktopRect(&screen);
					//������ �ػ� ����
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
			//�� ���� �ʱ�ȭ
			mouseInfo->bVWheelMode = FALSE;
			mouseInfo->bHWheelMode = FALSE;
		}
	}
	else
	{
		//���� �հ����� �׾� �����Ƿ�, ���� �հ����� Up�̺�Ʈ���� ó���Ѵ�. �׿��� �̺�Ʈ�� ������
		if (mouseInfo->action[0] == VS_TOUCH_FLAG_END)
		{
			button = 0;
			bRet = vmulti_update_relative_mouse(client->vmultiClient, button, 0, 0, 0);
		}
	}

	//���� �ð� ����
	mouseInfo->tick = GetTickCount();
	CopyMemory(&prevMouseInfo, mouseInfo, sizeof(WOW_MOUSE_INFO));
	return bRet;
}


EXPORT BOOL InjectKeyboardShortCut(PWOW_CLIENT client, PWOW_KEYBOARD_INFO keyboardInfo)
{
	BOOL ret = FALSE;
	//���� Ű���尪 ������
	WOW_KEYBOARD_INFO tmpInfo;
	ZeroMemory(&tmpInfo, sizeof(WOW_KEYBOARD_INFO));
	ret = vmulti_update_keyboard(client->vmultiClient, tmpInfo.shiftKeys, tmpInfo.keyCodes) ? TRUE : -1;

	//����Ű ����
	//KeyPress
	BYTE keyCodes[6];
	ZeroMemory(&keyCodes, sizeof(keyCodes));
	if (keyboardInfo->delayMilliseconds > 0)
	{
		//PTT �����̵� ��� application key�� Ű����� �������� ����� �۵����� �ʴ� ���� ������...
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
	
	//���� Ű���尪 ����
	ret = vmulti_update_keyboard(client->vmultiClient, prevKeyboardInfo.shiftKeys, prevKeyboardInfo.keyCodes) ? TRUE : -4;

	return ret;
}

EXPORT BOOL InjectKeyboardDevice(PWOW_CLIENT client, PWOW_KEYBOARD_INFO keyboardInfo)
{
	BOOL ret = FALSE;
    //���� ��� ����
	currentDeviceMode = VS_MODE_KEYBOARD;
	// See http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
	if (keyboardInfo->imeKey > 0)
	{
		//IME Ű�� ���� ó��
		int curImeKey = 0, rotateCnt = 0, curImeIndex = 0, reqImeIndex = -1;
		wchar_t szBuf[10];
		//Ű���� ����Ʈ
		UINT uLayouts = GetKeyboardLayoutList(0, NULL);
		HKL *lpList = new HKL[uLayouts * sizeof(HKL)];
		uLayouts = GetKeyboardLayoutList(uLayouts, lpList);

		GUITHREADINFO Gti;
		::ZeroMemory (&Gti,sizeof(GUITHREADINFO));
		Gti.cbSize = sizeof(GUITHREADINFO );
		::GetGUIThreadInfo(0,&Gti);
		DWORD dwThread = ::GetWindowThreadProcessId(Gti.hwndActive,0);
		HKL lang = ::GetKeyboardLayout(dwThread);
		//���� Ȱ��ȭ�� �������� Ű���� ���̾ƿ�
		LCID lcid = MAKELCID(((UINT)lang & 0xffffffff), SORT_DEFAULT);
		memset(szBuf, 0, 10);
		GetLocaleInfo(lcid, LOCALE_ILANGUAGE, szBuf, 10);
		//���� Ű���� ����� HexDecimal ��
		curImeKey = String::DecimalToHexdecimal(_wtoi(szBuf));	
		//���� Ű���� ���� ��û���� �ٸ� ��츸 ó��
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

			//��û�� IME�� ������ ���� ����
			if (reqImeIndex > -1)
			{
				WORD vks[2] = {VK_LMENU, VK_LSHIFT};
				int loopCnt = 0;
				do
				{
					Events::ShortCutKeyEvents(2, vks);
					Sleep(300);	//����Ǵµ� �ణ�� �ð��� �ɸ�

					lang = ::GetKeyboardLayout(dwThread);
					//���� Ȱ��ȭ�� �������� Ű���� ���̾ƿ�
					lcid = MAKELCID(((UINT)lang & 0xffffffff), SORT_DEFAULT);
					memset(szBuf, 0, 10);
					GetLocaleInfo(lcid, LOCALE_ILANGUAGE, szBuf, 10);
					//���� Ű���� ����� HexDecimal ��
					curImeKey = String::DecimalToHexdecimal(_wtoi(szBuf));	
					//������ġ
					loopCnt ++;
				} while (curImeKey != keyboardInfo->imeKey && loopCnt < uLayouts);
			}
		}
	}	
	//�̺�Ʈ ó��
	_D TCHAR a[256];
	_D wsprintf(a, L"shift: %d, keys : [%d, %d, %d, %d, %d, %d]\n", keyboardInfo->shiftKeys, 
	_D	keyboardInfo->keyCodes[0], keyboardInfo->keyCodes[1], keyboardInfo->keyCodes[2], 
	_D	keyboardInfo->keyCodes[3], keyboardInfo->keyCodes[4], keyboardInfo->keyCodes[5]);
	_D OutputDebugString(a);
	
	//������ ����
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
		vk[0] = VK_CAPITAL;  //���� - �Ϻ���
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 136:
		vk[0] = VK_OEM_COPY; //VK_KANA ���󰡳�,��Ÿī��, �θ��� ����
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 138:
		vk[0] = VK_CONVERT;	//��ȯ
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 139:
		vk[0] = VK_NONCONVERT;	//����ȯ
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
		vk[0] = VK_OEM_AUTO; //VK_OEM_ENLW ����,�ݰ�
		ret = Events::ShortCutKeyEvents(1, vk);
		break;
	case 176: 
		ret = Events::ShortCutKeyEvents(0, vk, 2, 0x30); //���� 00
		break;
	case 177: 
		ret = Events::ShortCutKeyEvents(0, vk, 3, 0x30); //���� 000
		break;
	case 235: 
		//����ǥ ����
		ret = WinExec("charmap.exe", SW_SHOW) > 31 ? 1 : 0;
		break;
	case 236:
		//���� ����
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
		//������ ����
		CopyMemory(&prevKeyboardInfo, keyboardInfo, sizeof(WOW_KEYBOARD_INFO));
		break;
	}
		
	return ret;
}

EXPORT void ReleaseDeviceEvent(PWOW_CLIENT client)
{
	//���������� ������ ó����
	//��ġ => BEGIN, �Ǵ� MOVE�� ��� �ִ°��
	BOOL isFound = false;
	for (int i = 0; i < sizeof(prevTouchInfo.action) / sizeof(INT); i++)
	{
		if (prevTouchInfo.action[i] == VS_TOUCH_FLAG_BEGIN || prevTouchInfo.action[i] == VS_TOUCH_FLAG_MOVE)
		{
			prevTouchInfo.action[i] = VS_TOUCH_FLAG_END;
			isFound = true;
		}
	}
	//��ġ �Է� ����
	if (isFound) 
		InjectTouchDevice(client, &prevTouchInfo);

	//���콺 => BEGIN, �Ǵ� MOVE�� ��� �ִ°��
	isFound = false;
	for (int i = 0; i < sizeof(prevMouseInfo.action) / sizeof(INT); i++)
	{
		if (prevMouseInfo.action[i] == VS_TOUCH_FLAG_BEGIN || prevMouseInfo.action[i] == VS_TOUCH_FLAG_MOVE)
		{
			prevMouseInfo.action[i] = VS_TOUCH_FLAG_END;
			isFound = true;
		}
	}
	//���콺 �Է� ����
	if (isFound) 
		InjectMouseDevice(client, &prevMouseInfo);

	//Ű���� => Ű�� ���� �ִ� ���
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
					_D 	wsprintfA(debugText, "���� �ڵ� : %ld\n", ret);
					_D 	OutputDebugStringA(debugText);
					_D }
				}
				prevTouchInfo.tick = tick;
			}
		}
	}
	return 0;
}
