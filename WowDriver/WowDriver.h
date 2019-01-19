#include <windows.h>
#include "vmulticlient.h"
#include "WowCommon.h"
#include <process.h>

#define SCROLL_AREA_WIDTH 60
#define SCROLL_AREA_MARGIN 40
#define CONTACT_WIDTH 10
#define CONTACT_HEIGHT 10
#define EXPORT extern "C" __declspec(dllexport)

typedef struct _wow_client {
	pvmulti_client vmultiClient;
} WOW_CLIENT, *PWOW_CLIENT;

EXPORT PWOW_CLIENT AllocDevice(void);

EXPORT void FreeDevice(PWOW_CLIENT client);

EXPORT BOOL ConnectDevice(PWOW_CLIENT client);

EXPORT void DisconnectDevice(PWOW_CLIENT client);

EXPORT BOOL InitializeTouchDevice(PWOW_CLIENT client, UINT32 maxCount, DWORD dwMode);

EXPORT BOOL InjectTouchDevice(PWOW_CLIENT client, PWOW_TOUCH_INFO touchInfos);

EXPORT BOOL InjectMouseDevice(PWOW_CLIENT client, PWOW_MOUSE_INFO mouseInfo);

EXPORT BOOL InjectKeyboardDevice(PWOW_CLIENT client, PWOW_KEYBOARD_INFO keyboardInfo);
