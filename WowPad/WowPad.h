#pragma once

#include "resource.h"
#include <commctrl.h >

#include "VsConfig.h"
#include "VsAsyncSocketManager.h"

#define WM_TRAYCOMMAND	(WM_USER + 100)
#define MAX_LOADSTRING 100
#define DLG_NOTIFICATION_DELAY 3000		//5초 딜레이
#define DLG_NOTIFICATION_ALPHA 60		//투명도 60%
#define	DLG_CONNECTION_ALPHA 90		//투명도 90%
#define INFO_BUFFER_SIZE 32767
//Enabling Visual Styles
#define ISOLATION_AWARE_ENABLED 1

#pragma comment(lib, "comctl32.lib") 
#pragma comment(lib, "version.lib")
//Enabling Visual Styles
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
