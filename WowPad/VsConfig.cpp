#include "stdafx.h"
#include "VsConfig.h"

const TCHAR* sectionKey = L"Translation";
const TCHAR* VsConfig::CLOSE_TIMEOUT = L"Timeout.Close";
const TCHAR* VsConfig::CONNECT_CODE = L"Connect.Code";
const TCHAR* direction[] = {L"Top", L"Left", L"Right", L"Bottom"};

VsConfig::VsConfig(void)
{
	TCHAR fileName[MAX_PATH];
	GetModuleFileName(NULL, fileName, MAX_PATH);
	
	char* buffer = NULL;
	DWORD infoSize = 0;

    // 파일로부터 버전정보데이터의 크기가 얼마인지를 구합니다.
    infoSize = GetFileVersionInfoSize(fileName, 0);
    if(infoSize > 0)
	{
		// 버퍼할당
		buffer = new char[infoSize];
		if(buffer)
		{
			// 버전정보데이터를 가져옵니다.
			if(GetFileVersionInfo(fileName, 0, infoSize, buffer) != 0)
			{
				VS_FIXEDFILEINFO* pFineInfo = NULL;
				UINT bufLen = 0;
				// buffer로 부터 VS_FIXEDFILEINFO 정보를 가져옵니다.
				if(VerQueryValue(buffer,L"\\",(LPVOID*)&pFineInfo, &bufLen) !=0)
				{    
					WORD majorVer, minorVer, buildNum, revisionNum;
					majorVer = HIWORD(pFineInfo->dwFileVersionMS);
					minorVer = LOWORD(pFineInfo->dwFileVersionMS);
					buildNum = HIWORD(pFineInfo->dwFileVersionLS);
					revisionNum = LOWORD(pFineInfo->dwFileVersionLS);

					// 파일버전 출력
					wsprintf(szProductVersion, L"%d.%d.%d", majorVer, minorVer, buildNum);
				}
			}
			delete[] buffer;
		}
	}

	TCHAR szPath[MAX_PATH];
	if ( SUCCEEDED( SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath ) ) )
	{
		ZeroMemory(&szConfigFileName, sizeof(szConfigFileName));
		lstrcpynW(szConfigFileName, szPath, lstrlenW(szPath) + 1);
	}
	
	vs::utils::String::GetFilePath(szLanguageFileName, fileName);
	PathAppend(szConfigFileName, L"Velostep");
	PathAppend(szConfigFileName, L"WowPad");
	PathAppend(szConfigFileName, L"Config.ini");

	//디버그용
	_D ZeroMemory(&szConfigFileName, sizeof(szConfigFileName));
	_D vs::utils::String::GetFilePath(szConfigFileName, fileName);
	_D PathAppend(szConfigFileName, L"Config.ini");

	//언어파일 경로
	PathAppend(szLanguageFileName, L"Language.ini");

	msgMap = new hash_map<UINT, TCHAR*>;
	bWindows8 = Windows8::IsWindows8();
	
	//엣지 제스쳐 기본값 설정
	defaultGesture[0] = IDS_SETTINGS_EDGE_GESTURE_MINIMIZE;//top
	
	if (bWindows8)
	{
		defaultGesture[1] = IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH_WIN8;//left 
		defaultGesture[2] = IDS_SETTINGS_EDGE_GESTURE_CHARM_BAR;//right
		defaultGesture[3] = IDS_SETTINGS_EDGE_GESTURE_APP_BAR;//bottom
	}
	else
	{
		defaultGesture[1] = IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH;//left 
		defaultGesture[2] = IDS_SETTINGS_EDGE_GESTURE_EXPLORE;//right
		defaultGesture[3] = IDS_SETTINGS_EDGE_GESTURE_THUMBNAIL;//bottom
	}
}

TCHAR* VsConfig::GetProductVersion()
{
	return szProductVersion;
}

VsConfig::~VsConfig(void)
{
	this->ClearMsgMap();
	delete[] this->edgeShortcutInfo;
	delete msgMap;
}

VOID VsConfig::ClearMsgMap()
{
	hash_map<UINT, TCHAR*>::iterator eraseIter = msgMap->begin();
	while (eraseIter != msgMap->end())
	{
		delete[] eraseIter->second;
		eraseIter = msgMap->erase(eraseIter);
	} 
}

TCHAR* VsConfig::GetI18nMessage(UINT key)
{
	hash_map<UINT, TCHAR*>::iterator findIter = msgMap->find(key);

	if(findIter != msgMap->end())
		return findIter->second;
	else
		return NULL;
}

void VsConfig::Load(HINSTANCE hInstance)
{
	if (msgMap->size() > 0) this->ClearMsgMap();

	const UINT menuIds[] = {
		IDS_APP_TITLE,
		IDS_MENU_EXIT, 
		IDS_MENU_CONNECTION_INFO,
		IDS_MENU_LAUNCH_ON_STARTUP,
		IDS_MSG_TITLE_ERROR,
		IDS_MSG_ERROR_INITIALIZE_GDI,
		IDS_MSG_ERROR_INITIALIZE_SOCKET,
		IDS_MSG_WARN_BATTERY,
		IDS_MSG_INFO_NO_TASK,
		IDS_MSG_CONNECTION_CODE_NOTICE,
		IDS_MSG_CONNECTION_CODE,
		IDS_MSG_CONNECTION_PORT,
		IDS_MSG_ERROR_SOCKET_NORMAL,
		IDS_MSG_ERROR_SOCKET_UDP_OPEN,
		IDS_MSG_ERROR_SOCKET_TCP_OPEN,
		IDS_MSG_ERROR_SOCKET_UDP_BIND,
		IDS_MSG_ERROR_SOCKET_TCP_BIND,
		IDS_MSG_ERROR_SOCKET_UDP_ASYNC,
		IDS_MSG_ERROR_SOCKET_TCP_ASYNC,
		IDS_MSG_ERROR_SOCKET_TCP_LISTEN,
		IDS_MSG_ERROR_SOCKET_TCP_CLOSE,
		IDS_MSG_ERROR_DLL_MISSING,
		IDS_MSG_ERROR_DRIVER_FAILED,
		IDS_MSG_INFO_CONNECTION_CLOSE,
		IDS_MSG_INFO_CONNECTION_OPEN,
		IDS_MSG_ERROR_CONNECTCODE,
		IDS_MSG_CONNECTION_NOT_CONNECTED,
		IDS_MSG_CONNECTION_MODE,
		IDS_MSG_CONNECTION_BATTERY,
		IDS_MSG_CONNECTION_MODE_TOUCH,
		IDS_MSG_CONNECTION_MODE_MOUSE,
		IDS_MENU_AUTOCLOSE,
		IDS_MENU_CLOSE_TIMEOUT_LEVEL0,
		IDS_MENU_CLOSE_TIMEOUT_LEVEL1,
		IDS_MENU_CLOSE_TIMEOUT_LEVEL2,
		IDS_MENU_CLOSE_TIMEOUT_LEVEL3,
		IDS_MENU_CLOSE_TIMEOUT_LEVEL4,
		IDS_MENU_DISCONNECT,
		IDS_MENU_UPDATE,
		IDS_MENU_SAVE_CONNECTCODE,
		IDS_MENU_SETTINGS,
		IDS_SETTINGS_EDGE_DIRECTION_TOP,
		IDS_SETTINGS_EDE_DIRECTION_LEFT,
		IDS_SETTINGS_EDGE_DIRECTION_RIGHT,
		IDS_SETTINGS_EDGE_DIRECTION_BOTTOM,
		IDS_DEFAULT,
		IDS_SETTINGS_EDGE_GESTURE_MINIMIZE,
		IDS_SETTINGS_EDGE_GESTURE_LOCK,
		IDS_SETTINGS_EDGE_GESTURE_THUMBNAIL,
		IDS_SETTINGS_EDGE_GESTURE_VOL_UP,
		IDS_SETTINGS_EDGE_GESTURE_VOL_DOWN,
		IDS_SETTINGS_EDGE_GESTURE_VOL_MUTE,
		IDS_SETTINGS_EDGE_GESTURE_EXPLORE,
		IDS_SETTINGS_EDGE_GESTURE_SEARCH,
		IDS_SETTINGS_EDGE_GESTURE_RUN,
		IDS_SETTINGS_EDGE_GESTURE_CUSTOM,
		IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH,
		IDS_SETTINGS_EDGE_GESTURE_BROWSER_BACK,
		IDS_SETTINGS_EDGE_GESTURE_BROWSER_FORWARD,
		IDS_OK,
		IDS_CANCEL,
		IDS_SETTINGS_EDGE_GESTURE_SETTING_TITLE,
		IDS_SETTINGS_EDGE_GESTURE_SETTING_SUBTITLE,
		IDS_SETTINGS_EDGE_GESTURE_SETTING_MESSAGE,
		IDS_SETTINGS_EDGE_GESTURE_PROGRAM_EXIT,
		IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH_WIN8,
		IDS_SETTINGS_EDGE_GESTURE_APP_BAR,
		IDS_SETTINGS_EDGE_GESTURE_CHARM_BAR,
		IDS_VERSION_INFOMATION,
		IDS_ERROR_CONNECTION_BATTERY
	};

	TCHAR szKey[KEY_LENGTH];
	TCHAR szDefault[MAX_LOADSTRING];
	TCHAR* szValue = NULL;
	
	for (int i=0; i<sizeof(menuIds) / sizeof(UINT); i++)
	{
		szValue = new TCHAR[MAXCOMMENTSZ];
		_itow_s(menuIds[i], szKey, 10);
		LoadString(hInstance, menuIds[i], szDefault, MAXCOMMENTSZ);
		GetPrivateProfileString(sectionKey, szKey, szDefault, szValue, MAXCOMMENTSZ, szLanguageFileName);
		msgMap->insert(hash_map<UINT, TCHAR*>::value_type(menuIds[i], szValue));
	}

	//공통 폰트
	GetPrivateProfileString(L"Font", L"Common.Family", DEFAULT_FONT, szFontFamily, MAX_LOADSTRING, szConfigFileName);
	//연결정보창 설정
	nConnectionInfoPaddingLeft = (REAL)GetPrivateProfileInt(L"Layout", L"Connection.Padding.Left", 90, szConfigFileName);
	nConnectionInfoTopFontSize = (REAL)GetPrivateProfileInt(L"Font", L"Connection.Title.Size", 35, szConfigFileName);
	nConnectionInfoBottomFontSize = (REAL)GetPrivateProfileInt(L"Layout", L"Connection.SubTitle.Size", 16, szConfigFileName);
	//알림창 설정	
	nNotificationFontSize = (REAL)GetPrivateProfileInt(L"Font", L"Notification.Size", 24, szConfigFileName);
	//제스쳐 설정 로드
	this->LoadEdgeGesture();
}

TCHAR* VsConfig::GetFontFamily()
{
	return this->szFontFamily;
}

REAL VsConfig::GetConnectionTopFontSize()
{
	return this->nConnectionInfoTopFontSize;
}

REAL VsConfig::GetConnectionBottomFontSize()
{
	return this->nConnectionInfoBottomFontSize;
}
REAL VsConfig::GetNotificationFontSize()
{
	return this->nNotificationFontSize;
}

REAL VsConfig::GetConnectionPaddingLeft()
{
	return this->nConnectionInfoPaddingLeft;
}

BOOL VsConfig::WriteConfig(LPCTSTR key, LPCTSTR value)
{
	return WritePrivateProfileString(L"Config", key, value, szConfigFileName);
}

BOOL VsConfig::WriteConfigInt(LPCTSTR key, INT value)
{
	CHAR strVal[MAX_LOADSTRING];
	TCHAR wStrVal[MAX_LOADSTRING];
	_itoa_s(value, strVal, 10);
	MultiByteToWideChar(CP_ACP, 0, strVal, -1, wStrVal, MAX_LOADSTRING);
	return this->WriteConfig(key, wStrVal);
}
DWORD VsConfig::GetConfigString(LPCTSTR key, LPCTSTR lpDefault, LPTSTR value)
{
	return GetPrivateProfileString(L"Config", key, lpDefault, value, MAX_LOADSTRING, szConfigFileName);
}

UINT VsConfig::GetConfigInt(LPCTSTR key, INT nDefault)
{
	return GetPrivateProfileInt(L"Config", key, nDefault, szConfigFileName);
}

INT VsConfig::GetCloseTimeout()
{
	return this->closeTimeout;
}

INT VsConfig::LoadCloseTimeout()
{
	this->closeTimeout =  this->GetConfigInt(VsConfig::CLOSE_TIMEOUT, 0);
	return this->closeTimeout;
}

BOOL VsConfig::WriteCloseTimeout(INT value)
{
	this->closeTimeout = value;
	return this->WriteConfigInt(VsConfig::CLOSE_TIMEOUT, this->closeTimeout);
}

INT VsConfig::GetConnectCode()
{
	return this->connectCode;
}

INT VsConfig::LoadConnectCode()
{
	this->connectCode =  this->GetConfigInt(VsConfig::CONNECT_CODE, 0);
	return this->connectCode;
}

BOOL VsConfig::WriteConnectCode(INT value)
{
	this->connectCode = value;
	return this->WriteConfigInt(VsConfig::CONNECT_CODE, this->connectCode);
}

VOID VsConfig::SetHWnd(HWND hWnd)
{
	this->hWnd = hWnd;
}

HWND VsConfig::GetHWnd()
{
	return this->hWnd;
}

VOID VsConfig::SetCbConnectionMsgBox(void (*ShowConnectionInfoWindowProc)(BOOL))
{
	this->ShowConnectionInfoWindowProc = ShowConnectionInfoWindowProc;
}

VOID VsConfig::SetCbNotificationMsgBox(void (*ShowNotificationWindowProc)(TCHAR*, INT))
{
	this->ShowNotificationWindowProc = ShowNotificationWindowProc;
}

VOID VsConfig::ShowNotificationWindow(TCHAR* msg, INT nCmdShow)
{
	this->ShowNotificationWindowProc(msg, nCmdShow);
}

VOID VsConfig::ShowConnectionInfoWindow(BOOL bUpdate)
{
	this->ShowConnectionInfoWindowProc(bUpdate);
}

VOID VsConfig::GetEdgeGesture(USHORT& nCnt, EDGE_GESTURE_INFO*& info)
{
	if (this->edgeShortcutInfo == NULL)
			this->LoadEdgeGesture();
	
	nCnt = 4;
	info = this->edgeShortcutInfo;
}

VOID VsConfig::LoadEdgeGesture()
{
	if (this->edgeShortcutInfo == NULL)
		this->edgeShortcutInfo = new EDGE_GESTURE_INFO[4];

	for (int i=0; i<4; i++)
	{
		TCHAR szlabel[MAX_LOADSTRING]; 
		USHORT nItems = 0, defaultIndex = 0;
		UINT *edgeLblIds = NULL;
		this->GetEdgeLabels(i + 1, nItems, edgeLblIds);

		for (int j=0; j<nItems; j++)
		{
			if (edgeLblIds[j] == this->defaultGesture[i]) 
			{
				defaultIndex = j;
				break;
			}
		}
		delete[] edgeLblIds;

		wsprintf(szlabel, L"Edge.%s.Item", direction[i]);
		this->edgeShortcutInfo[i].index = this->GetConfigInt(szlabel, defaultIndex);
		wsprintf(szlabel, L"Edge.%s.CustomKey", direction[i]);
		this->edgeShortcutInfo[i].hotKey = this->GetConfigInt(szlabel, 0);
		wsprintf(szlabel, L"Edge.%s.WinKey", direction[i]);
		this->edgeShortcutInfo[i].winKey = this->GetConfigInt(szlabel, 0);

		if (this->edgeShortcutInfo[i].index < 0) this->edgeShortcutInfo[i].index = 0;
		if (this->edgeShortcutInfo[i].hotKey < 0) this->edgeShortcutInfo[i].hotKey = 0;
		if (this->edgeShortcutInfo[i].winKey < 0) this->edgeShortcutInfo[i].winKey = 0;
	}
}

VOID VsConfig::SaveEdgeGesture(USHORT nCnt, EDGE_GESTURE_INFO* info)
{
	for (int i=0; i<nCnt; i++)
	{
		TCHAR szlabel[MAX_LOADSTRING]; 
		wsprintf(szlabel, L"Edge.%s.Item", direction[i]);
		this->WriteConfigInt(szlabel, info[i].index);
		wsprintf(szlabel, L"Edge.%s.CustomKey", direction[i]);
		this->WriteConfigInt(szlabel, info[i].hotKey);
		wsprintf(szlabel, L"Edge.%s.WinKey", direction[i]);
		this->WriteConfigInt(szlabel, info[i].winKey);
	}
	//저장후 다리 로드
	this->LoadEdgeGesture();
}

VOID VsConfig::GetEdgeLabels(USHORT flag, USHORT& nCnt, UINT*& labels)
{
	UINT* directionLabeIds = NULL, idx = 0;
	switch(flag)
	{
	case 0:
		//엣지 문자열 ID
		nCnt = 4;
		directionLabeIds = new UINT[nCnt];
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_DIRECTION_TOP;
		directionLabeIds[idx++] = IDS_SETTINGS_EDE_DIRECTION_LEFT;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_DIRECTION_RIGHT;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_DIRECTION_BOTTOM;
		labels = directionLabeIds;
		break;
	case TOP_EDGE:
	case LEFT_EDGE:
	case RIGHT_EDGE:
	case BOTTOM_EDGE:
		//윗쪽, 아랫쪽 엣지 져스쳐 콤보박스에 들어갈 문자열 ID
		nCnt = bWindows8 ? 17 : 14;	
		directionLabeIds = new UINT[nCnt];
		if (bWindows8)
		{
			directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH_WIN8;
			directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_CHARM_BAR;
			directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_APP_BAR;
		}
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_TASK_SWITCH;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_MINIMIZE;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_LOCK;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_THUMBNAIL;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_PROGRAM_EXIT;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_EXPLORE;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_SEARCH;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_RUN;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_BROWSER_BACK;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_BROWSER_FORWARD;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_VOL_UP;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_VOL_DOWN;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_VOL_MUTE;
		directionLabeIds[idx++] = IDS_SETTINGS_EDGE_GESTURE_CUSTOM;
		labels = directionLabeIds;
		break;
	}
}

BOOL VsConfig::IsWindows8()
{
	return bWindows8;
}

VOID VsConfig::GetDefaultGesture (USHORT& nCnt, UINT*& defaultGestures)
{
	nCnt = 4;
	defaultGestures = this->defaultGesture;
}