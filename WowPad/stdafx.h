// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����:
#include <windows.h>
#define _CRTDBG_MAP_ALLOC
// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <crtdbg.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.
#include <objidl.h>
#include <gdiplus.h>
#include "resource.h"
#include <shellapi.h>
#include <WinSock2.h>
#include <process.h>
#include <string.h>
#include <math.h>
#include <string>
#include <iostream>
#include "../WowCommon/VsUtils.h"



using namespace Gdiplus;
using namespace std;
using namespace vs::utils;

#pragma comment (lib,"Gdiplus.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "WowCommon.lib")
#pragma comment(lib, "Imm32.lib")