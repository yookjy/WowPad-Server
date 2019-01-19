// WowWrapper.cpp : DLL 응용 프로그램을 위해 내보낸 함수를 정의합니다.
//

#include "stdafx.h"
#include "../WowDriver/WowDriver.h"

#define EXPORT extern "C" __declspec(dllexport)

EXPORT BOOL InitializeTouchSimulator(UINT32 maxCount, DWORD dwMode)
{
	return InitializeTouchInjection(maxCount, dwMode);
}

EXPORT BOOL InjectTouchSimulator(PWOW_TOUCH_INFO touchInfo)
{
	RECT screen;
	Windows8::GetDesktopRect(&screen);
	//화면 비율
	FLOAT rx = 1, ry = 1;
	CHAR* act = NULL;
	BOOL bRet = FALSE, fakeUpEvent = FALSE;
	
	if (touchInfo->resolution.x > 0 && touchInfo->resolution.y > 0)
	{
		//폰과의 해상도 배율
		rx = (FLOAT)(screen.right - screen.left) / (FLOAT)touchInfo->resolution.x;
		ry = (FLOAT)(screen.bottom - screen.top) / (FLOAT)touchInfo->resolution.y;
	}
	
	//터치 시뮬레이터 구조체 초기화
	POINTER_TOUCH_INFO* contact = new POINTER_TOUCH_INFO[touchInfo->contactCnt];
	memset(contact, 0, sizeof(POINTER_TOUCH_INFO) * touchInfo->contactCnt);

	for (int i=0; i<touchInfo->contactCnt; i++)
	{
		LONG dx = screen.left + (LONG)(touchInfo->position[i].x * rx);
		LONG dy = screen.top + (LONG)(touchInfo->position[i].y * ry);

		if (dx < screen.left) dx = screen.left;
		if (dy < screen.top) dy = screen.top;
		if (dx > screen.right) dx = screen.right;
		if (dy > screen.bottom) dy = screen.bottom;

		contact[i].pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
		contact[i].pointerInfo.pointerId = touchInfo->id[i];          //contact id
		contact[i].pointerInfo.ptPixelLocation.x = dx;
		contact[i].pointerInfo.ptPixelLocation.y = dy;
						
		contact[i].touchFlags = TOUCH_FLAG_NONE;
		contact[i].touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
		contact[i].orientation = 90;
		contact[i].pressure = 32000;
			
		// 접촉 면적 설정
		contact[i].rcContact.top = dy - (CONTACT_HEIGHT / 2);
		contact[i].rcContact.right = dx + (CONTACT_WIDTH / 2);
		contact[i].rcContact.bottom = dy + (CONTACT_HEIGHT / 2);
		contact[i].rcContact.left = dx - (CONTACT_WIDTH / 2);
		
		if (touchInfo->action[i] == VS_TOUCH_FLAG_BEGIN)
		{
			_D act = "TOUCH DOWN";
			contact[i].pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
		}
		else if (touchInfo->action[i] == VS_TOUCH_FLAG_MOVE)
		{
			_D act = "TOUCH UPDATE";
			contact[i].pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
		}
		else if (touchInfo->action[i] == VS_TOUCH_FLAG_END)
		{
			_D act = "TOUCH UP";
			contact[i].pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
			fakeUpEvent = TRUE;
		}
		else if (touchInfo->action[i] == VS_TOUCH_FLAG_CANCELED)
		{
			_D act = "TOUCH CANCEL";
			contact[i].pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_CANCELED;
		}
		else
		{
			act = "TOUCH NONE";
			contact[i].pointerInfo.pointerFlags = POINTER_FLAG_NONE;
		}
							
		_D char debugText[256] = {0};
		_D wsprintfA(debugText, "%s 시뮬레이션 => x:%d, y:%d\n", act, dx, dy);
		_D OutputDebugStringA(debugText);
	}

	if (fakeUpEvent)
	{
		InjectTouchInput(touchInfo->contactCnt, contact);
		for (int i=0; i<touchInfo->contactCnt; i++)
		{
			if (touchInfo->action[i] == VS_TOUCH_FLAG_END)
			{
				contact[i].pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_INRANGE;
			}
		} 
	}
		
	bRet = InjectTouchInput(touchInfo->contactCnt, contact);
	if (bRet == 0)
	{
		_D DWORD ret = GetLastError();
		_D char debugText[256] = {0};
		_D wsprintfA(debugText, "%ld\n", ret);
		_D OutputDebugStringA(debugText);
	}
	//메모리 해제
	delete[] contact;
	return bRet;
}