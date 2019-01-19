#pragma once

#include <hash_map>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"


using namespace stdext;

#define MAX_LOADSTRING	100
#define KEY_LENGTH		4
//#define DEFAULT_FONT L"Segoe UI Light"	//기본 폰트
#define DEFAULT_FONT L"Segoe UI"	//기본 폰트

#define TOP_EDGE	 1
#define LEFT_EDGE	 2
#define RIGHT_EDGE	 3
#define BOTTOM_EDGE	 4

typedef struct EDGE_GESTURE_INFO {
	INT index;
	WORD hotKey;
	BOOL winKey;
} EDGE_GEUSTURE_INFO;

class VsConfig
{
private:
	HWND hWnd;
	INT closeTimeout, connectCode;
	UINT defaultGesture[4];
	BOOL bWindows8;
	TCHAR szConfigFileName[MAX_PATH], szLanguageFileName[MAX_PATH], szFontFamily[MAX_LOADSTRING], szProductVersion[MAX_LOADSTRING];
	hash_map<UINT, TCHAR*>* msgMap;
	EDGE_GESTURE_INFO* edgeShortcutInfo;
	REAL nConnectionInfoTopFontSize, nConnectionInfoBottomFontSize, nNotificationFontSize, nConnectionInfoPaddingLeft;
	void (*ShowNotificationWindowProc)(TCHAR*, INT);
	void (*ShowConnectionInfoWindowProc)(BOOL);
public:
	static const TCHAR *CLOSE_TIMEOUT, *CONNECT_CODE;
	VsConfig(void);
	~VsConfig(void);
	TCHAR* GetI18nMessage(UINT);
	VOID Load(HINSTANCE);
	TCHAR* GetProductVersion();
	TCHAR* GetPrivateProfileFileName();
	TCHAR* GetDefaultFontFamily();
	TCHAR* GetFontFamily();
	REAL GetConnectionTopFontSize();
	REAL GetConnectionBottomFontSize();
	REAL GetNotificationFontSize();
	REAL GetConnectionPaddingLeft();
	BOOL WriteConfig(LPCTSTR, LPCTSTR);
	BOOL WriteConfigInt(LPCTSTR, INT);
	DWORD GetConfigString(LPCTSTR, LPCTSTR, LPTSTR);
	UINT GetConfigInt(LPCTSTR, INT);
	INT LoadCloseTimeout();
	INT GetCloseTimeout();
	BOOL WriteCloseTimeout(INT);
	INT LoadConnectCode();
	INT GetConnectCode();
	BOOL WriteConnectCode(INT);
	VOID ClearMsgMap();
	VOID SetHWnd(HWND);
	HWND GetHWnd();
	VOID SetCbNotificationMsgBox(void (*ShowNotificationWindowProc)(TCHAR*, INT));
	VOID SetCbConnectionMsgBox(void (*ShowConnectionInfoWindowProc)(BOOL));
	VOID ShowNotificationWindow(TCHAR*, INT);
	VOID ShowConnectionInfoWindow(BOOL);
	VOID GetEdgeGesture(USHORT&, EDGE_GESTURE_INFO*&);
	VOID LoadEdgeGesture();
	VOID SaveEdgeGesture(USHORT, EDGE_GESTURE_INFO*);
	VOID GetEdgeLabels(USHORT, USHORT&, UINT*&);
	BOOL IsWindows8();
	VOID GetDefaultGesture(USHORT&, UINT*&);
};

