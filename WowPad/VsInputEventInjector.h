#pragma once
#include <vector>
#include <math.h>
#include <commctrl.h >
#include "VsConfig.h"
#include "VsMessage.h"
#include "VsKeybdMessage.h"
#include "VsPointerMessage.h"
#include "VsMessageQueue.h"
#include "../WowDriver/WowDriver.h"

#pragma comment( lib, "comctl32.lib") 

using namespace std;

#define PI 3.1415926536
#define ToDegree(radian) ((radian)*(180.0f/PI))

const INT PIXEL_BUFFER = 20;
const INT BEGIN_TIME = 20;
const INT END_TIME = 425;
const INT NOTIFICATION_BATTERY[7] = {40, 30, 20, 10, 5, 3, 1};

typedef struct BEDGE { 
    BOOL left; 
    BOOL top;
	BOOL right;
	BOOL bottom;
} BEDGE; 

typedef PWOW_CLIENT (__cdecl *AllocDeviceProc)(void);
typedef void (__cdecl *FreeDeviceProc)(PWOW_CLIENT);
typedef BOOL (__cdecl *ConnectDeviceProc)(PWOW_CLIENT);
typedef void (__cdecl *DisconnectDeviceProc)(PWOW_CLIENT);
typedef BOOL (__cdecl *InitializeTouchDeviceProc)(PWOW_CLIENT, UINT32, DWORD);
typedef BOOL (__cdecl *InjectTouchDeviceProc)(PWOW_CLIENT, PWOW_TOUCH_INFO);
typedef BOOL (__cdecl *InjectMouseDeviceProc)(PWOW_CLIENT, PWOW_MOUSE_INFO);
typedef BOOL (__cdecl *InjectKeyboardDeviceProc)(PWOW_CLIENT, PWOW_KEYBOARD_INFO);
typedef BOOL (__cdecl *InjectKeyboardShortcutProc)(PWOW_CLIENT, PWOW_KEYBOARD_INFO);
typedef void (__cdecl *ReleaseDeviceEventProc)(PWOW_CLIENT);

class VsInputEventInjector
{
private:
	//Dll인스턴스
	HINSTANCE hinstanceLib;
	//WowDriver 함수 포인터
	AllocDeviceProc AllocDevice;
	FreeDeviceProc FreeDevice;
	ConnectDeviceProc ConnectDevice;
	DisconnectDeviceProc DisconnectDevice;
	InitializeTouchDeviceProc InitializeTouchDevice;
	InjectTouchDeviceProc InjectTouchDevice;
	InjectMouseDeviceProc InjectMouseDevice;
	InjectKeyboardDeviceProc InjectKeyboardDevice;
	InjectKeyboardShortcutProc InjectKeyboardShortcut;
	ReleaseDeviceEventProc ReleaseDeviceEvent;
	//WowDriver용 변수
	PWOW_CLIENT pWowClient;

	//멈버변수
	VsConfig* vsConfig;
	BEDGE bEdge;
	BOOL bDoneEdge, bTouchMode;
	VsMessageQueue* queue;
	INT nextNotificationBattery, battery;
	StopWatch* stopWatch;
	//멤버함수
	VOID Execute(VsPointerMessage*);
	VOID ClearBuffer();
	INT SendInput();
	BOOL OnEdge();
	BOOL OnEdge(RECT, RECT);
	BOOL InnerEdge(RECT, RECT);
	BEDGE GetEdge(RECT, RECT);
	VOID SetEdge(RECT, RECT);
	USHORT GetEdgeDirection(FLOAT);
	VOID InjectMouseEvent(VsPointerMessage*);
	VOID InjectTouchEvent(VsPointerMessage*);
	VOID InjectShortcutEvent(FLOAT angle);
	VOID InjectKeyboardEvent(VsKeybdMessage*);
public:
	VsInputEventInjector(VsConfig*);
	~VsInputEventInjector(VOID);
	BOOL Initialize();
	INT Request(VsMessage*);
	VOID SetTouchMode(INT);
	BOOL IsTouchMode();
	INT GetBattery();
	VOID SetBattery(INT);
	VOID Reset();
	VOID PressVirtualButton(INT);
};

