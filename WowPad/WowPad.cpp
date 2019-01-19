// WowPad.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "WowPad.h"
#include <strsafe.h>

// ���� ����:
HINSTANCE hInst;								// ���� �ν��Ͻ��Դϴ�.
TCHAR szTitle[MAX_LOADSTRING];					// ���� ǥ���� �ؽ�Ʈ�Դϴ�.
TCHAR szWindowClass[MAX_LOADSTRING];			// �⺻ â Ŭ���� �̸��Դϴ�.
TCHAR szNotificationMsg[MAX_LOADSTRING];		// �˸� â ������ �ؽ�Ʈ
INT dlgWidth = 460, dlgHeight = 140, fadeAlpha = 0;		// �˸�â ���� ���İ�
POINT offset = {50, 44};
HWND hConnDlg, hNotiDlg, hSettDlg;
LPCWSTR HSUBKEY = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
const LPCWSTR APPLICATION_NAME = L"Velostep WowPad";
const LPCWSTR HOMEPAGE_DOWNLOAD_LINK = L"http://apps.velostep.com/wowpad";
VsAsyncSocketManager* socketManager;
VsConfig* vsConfig;

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	ConnectionProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	NotificationProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	SettingsProc(HWND, UINT, WPARAM, LPARAM);
//�˸� ó�� �Լ�
void ShowNotificationWindow(TCHAR*, INT);
void ShowConnectionInfoWindow(BOOL);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	//�޸� �� �˻�
	_D _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	
	 // TODO: ���⿡ �ڵ带 �Է��մϴ�.
	MSG msg;
	HACCEL hAccelTable;
	
	// ���� ���ڿ��� �ʱ�ȭ�մϴ�.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WOWPAD, szWindowClass, MAX_LOADSTRING);
	
	if (FindWindow(szWindowClass, szTitle))
	{
		_D MessageBox(NULL, L"�̹� ���α׷��� �������Դϴ�. �ý��� Ʈ���̸� Ȯ���ϼ���.", L"�ȳ�", MB_OK);
		return 0;
	}

	MyRegisterClass(hInstance);

	//���� ������ �о����
	vsConfig = new VsConfig;
	vsConfig->Load(hInstance);

	//Gdi+ �ʱ�ȭ
	ULONG_PTR gpToken;
	GdiplusStartupInput gpsi;
	if (GdiplusStartup(&gpToken,&gpsi,NULL) != Ok) {
		MessageBox(NULL,vsConfig->GetI18nMessage(IDS_MSG_ERROR_INITIALIZE_GDI), vsConfig->GetI18nMessage(IDS_MSG_TITLE_ERROR), MB_ICONERROR);
		return FALSE;
	}


	// ���� ���α׷� �ʱ�ȭ�� �����մϴ�.
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
	
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WOWPAD));

	// �⺻ �޽��� �����Դϴ�.
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	//Gdi+ ����
	GdiplusShutdown(gpToken);

	//ȯ�漳�� ��ε�
	delete vsConfig;

	//�޸� �� �˻�
	_D _CrtDumpMemoryLeaks();
	_CrtDumpMemoryLeaks();
	return (int) msg.wParam;
}

//
//  �Լ�: MyRegisterClass()
//
//  ����: â Ŭ������ ����մϴ�.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WOWPAD));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_VSTOUCHPAD);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	
	return RegisterClassEx(&wcex);
}

//
//   �Լ�: InitInstance(HINSTANCE, int)
//
//   ����: �ν��Ͻ� �ڵ��� �����ϰ� �� â�� ����ϴ�.
//
//   ����:
//
//        �� �Լ��� ���� �ν��Ͻ� �ڵ��� ���� ������ �����ϰ�
//        �� ���α׷� â�� ���� ���� ǥ���մϴ�.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.
   
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
		
	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);
	
	return TRUE;
}

//
//  �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ����: �� â�� �޽����� ó���մϴ�.
//
//  WM_COMMAND	- ���� ���α׷� �޴��� ó���մϴ�.
//  WM_PAINT	- �� â�� �׸��ϴ�.
//  WM_DESTROY	- ���� �޽����� �Խ��ϰ� ��ȯ�մϴ�.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	NOTIFYICONDATA nid;

	switch (message)
	{
	case WM_CREATE:
		//�ý��� Ʈ���� ������ ���
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hWnd;
		nid.uID = 100;
		nid.uVersion = NOTIFYICON_VERSION;
		nid.uCallbackMessage = WM_TRAYCOMMAND;
		nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAYICON));
		//TODO : ��⿡ ���� ���¿� ��Ʈ�� ǥ��������... [��û�� ��ġ�е�� ���� �غ����Դϴ�.... (��Ʈ: xxxx)]
		TCHAR toolTip[128];
		wsprintf(toolTip, L"%s Version %s", vsConfig->GetI18nMessage(IDS_APP_TITLE), vsConfig->GetProductVersion());
		wcscpy_s(nid.szTip, toolTip);
		
		//���� ���� ���̾�α� ����
		hConnDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DLGCONNECTION), hWnd, ConnectionProc);
		//�˸� ���̾�α� ����
		hNotiDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DLGNOTIFICATION), hWnd, NotificationProc);
		//���� ���̾�α� ����
		hSettDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DLGSETTINGS), hWnd, SettingsProc);
		
		//�ý��� Ʈ���� ������ ���
		nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		Shell_NotifyIcon(NIM_ADD, &nid);
		
		////����Ű ���
		RegisterHotKey(hWnd, 0, MOD_WIN | MOD_ALT, 0x4D /*M*/);	//�������� ����
		RegisterHotKey(hWnd, 0, MOD_WIN | MOD_ALT, 0x47 /*G*/);	//���� ������ ����
		RegisterHotKey(hWnd, 0, MOD_WIN | MOD_ALT, 0x53 /*S*/);	//���� �ڵ� ����

		//���� Ŭ���� ����
		vsConfig->SetHWnd(hWnd);
		vsConfig->SetCbConnectionMsgBox(ShowConnectionInfoWindow);
		vsConfig->SetCbNotificationMsgBox(ShowNotificationWindow);
		socketManager = new VsAsyncSocketManager(vsConfig);
		socketManager->Initialize();
		
		break;
	case WM_HOTKEY:
	{
		WORD hw = HIWORD(lParam);
		WORD lw = LOWORD(lParam);
		lw &= ~(MOD_ALT|MOD_WIN);

		if (lw == 0)
		{
			switch(hw )
			{
			case 0x4D:
				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_CONNECTION, 0), 0);
				break;
			case 0x47:
				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_SETTINGS, 0), 0);
				break;
			case 0x53:
				SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_SAVE_CONNECTCODE, 0), 0);
				break;
			}
		}
		break;
	}
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �޴� ������ ���� �м��մϴ�.
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case IDM_AUTOSTART:
		{
			TCHAR szPath[MAX_PATH];
			if (Registry::ExistsRegValue(HKEY_CURRENT_USER, HSUBKEY, APPLICATION_NAME))
			{
				Registry::DeleteRegValue(HKEY_CURRENT_USER, HSUBKEY, APPLICATION_NAME);
			}
			else
			{
				GetModuleFileName(NULL, szPath, MAX_PATH);
				Registry::WriteRegString(HKEY_CURRENT_USER, HSUBKEY, APPLICATION_NAME, szPath);
			}
			break;
		}
		case IDM_CLOSE_TIMEOUT_LEVEL0:
			vsConfig->WriteCloseTimeout(0);
			break;
		case IDM_CLOSE_TIMEOUT_LEVEL1:
			vsConfig->WriteCloseTimeout(1800);
			break;
		case IDM_CLOSE_TIMEOUT_LEVEL2:
			vsConfig->WriteCloseTimeout(3600);
			break;
		case IDM_CLOSE_TIMEOUT_LEVEL3:
			vsConfig->WriteCloseTimeout(7200);
			break;
		case IDM_CLOSE_TIMEOUT_LEVEL4:
			vsConfig->WriteCloseTimeout(14400);
			break;
		case IDM_SAVE_CONNECTCODE:
			if (vsConfig->GetConnectCode() < 1000 || vsConfig->GetConnectCode() > 9999)
				vsConfig->WriteConnectCode(socketManager->GetConnectionCode());
			else
				vsConfig->WriteConnectCode(0);
			break;
		case IDM_CONNECTION:
			if (IsWindowVisible(hConnDlg))
				ShowWindow(hConnDlg, SW_HIDE);
			else
			{
				RECT rect;
				Windows8::GetDesktopRect(&rect);
				SetWindowPos(hConnDlg, HWND_TOPMOST, rect.left + offset.x, rect.bottom + (-1 * offset.y - dlgHeight), dlgWidth, dlgHeight, SWP_NOSIZE | SWP_SHOWWINDOW);
			}
			break;
		case IDM_SETTINGS:
			if (IsWindowVisible(hSettDlg))
				ShowWindow(hSettDlg, SW_HIDE);
			else
			{
				RECT rect;
				Windows8::GetDesktopRect(&rect);
				SetWindowPos(hSettDlg, HWND_TOPMOST, rect.right - dlgWidth - 200 - offset.x , rect.bottom + (-1 * offset.y - dlgHeight * 2), dlgWidth + 200, dlgHeight * 2, SWP_NOSIZE);
				ShowWindow(hSettDlg, SW_SHOW);
			}
		break;
		case IDM_DISCONNECT:
			socketManager->DisconnectDevice();
			break;
		case IDM_UPDATE:
			ShellExecute(NULL, L"open", HOMEPAGE_DOWNLOAD_LINK, NULL, NULL, SW_SHOWNORMAL);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_INITMENU:
	{
		//������������
		if (IsWindowVisible(hConnDlg))
			CheckMenuItem((HMENU)wParam, IDM_CONNECTION, MF_CHECKED);
		else
			CheckMenuItem((HMENU)wParam, IDM_CONNECTION, MF_UNCHECKED);
		//�ڵ�����
		if (Registry::ExistsRegValue(HKEY_CURRENT_USER, HSUBKEY, APPLICATION_NAME))
			CheckMenuItem((HMENU)wParam, IDM_AUTOSTART, MF_CHECKED);
		else
			CheckMenuItem((HMENU)wParam, IDM_AUTOSTART, MF_UNCHECKED);
		//�������� ����
		INT value = vsConfig->LoadCloseTimeout();
		for (int i=IDM_CLOSE_TIMEOUT_LEVEL0; i<IDM_CLOSE_TIMEOUT_LEVEL4; i++)
		{
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL0 + i, MF_UNCHECKED);
		}
		switch(value)
		{
		case 1800:
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL1, MF_CHECKED);
			break;
		case 3600:
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL2, MF_CHECKED);
			break;
		case 7200:
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL3, MF_CHECKED);
			break;
		case 14400:
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL4, MF_CHECKED);
			break;
		default:
			CheckMenuItem((HMENU)wParam, IDM_CLOSE_TIMEOUT_LEVEL0, MF_CHECKED);
		}
		//���� ���� ����
		if (socketManager->GetConnectionStatus() == VsAsyncSocketManager::STATUS_CONNECTED)
			EnableMenuItem((HMENU)wParam, IDM_DISCONNECT, MF_ENABLED);
		else
			EnableMenuItem((HMENU)wParam, IDM_DISCONNECT, MF_DISABLED);
		//�����ڵ� ���� ����
		value = vsConfig->LoadConnectCode();
		if (value < 1000 || value > 9999)
			CheckMenuItem((HMENU)wParam, IDM_SAVE_CONNECTCODE, MF_UNCHECKED);
		else
			CheckMenuItem((HMENU)wParam, IDM_SAVE_CONNECTCODE, MF_CHECKED);
		//������������
		if (IsWindowVisible(hSettDlg))
			CheckMenuItem((HMENU)wParam, IDM_SETTINGS, MF_CHECKED);
		else
			CheckMenuItem((HMENU)wParam, IDM_SETTINGS, MF_UNCHECKED);
		break;
	}
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: ���⿡ �׸��� �ڵ带 �߰��մϴ�.
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
	{
		UINT titleId = HIWORD(wParam);
		if (titleId == IDS_MSG_TITLE_ERROR)
		{
			TCHAR* msg = (TCHAR*)lParam;
			MessageBox(hWnd, msg, vsConfig->GetI18nMessage(titleId), MB_ICONERROR);
		}
		
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hWnd;
		nid.uID = 100;
		Shell_NotifyIcon(NIM_DELETE, &nid);

		//���� Ŭ���� ����
		delete socketManager;

		PostQuitMessage(0);
		break;
	}
	case WM_TRAYCOMMAND:
		switch(lParam)
		{
		case WM_LBUTTONDOWN:
		{
			TCHAR verInfo[128];
			wsprintf(verInfo, vsConfig->GetI18nMessage(IDS_VERSION_INFOMATION), vsConfig->GetProductVersion());

			HMENU hMenu, hPopup, hTimeoutPopup, hAbout;
			//�˾��޴�
			hMenu = CreateMenu();
			hPopup = CreateMenu(); 
			hTimeoutPopup = CreateMenu(); 
			hAbout = CreateMenu();
			AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hPopup, L"");
			AppendMenu(hPopup, MF_STRING, IDM_AUTOSTART, vsConfig->GetI18nMessage(IDS_MENU_LAUNCH_ON_STARTUP));
			AppendMenu(hPopup, MF_STRING|MF_POPUP, (UINT)hTimeoutPopup, vsConfig->GetI18nMessage(IDS_MENU_AUTOCLOSE)); 
			AppendMenu(hTimeoutPopup, MF_STRING, IDM_CLOSE_TIMEOUT_LEVEL0, vsConfig->GetI18nMessage(IDS_MENU_CLOSE_TIMEOUT_LEVEL0));
			AppendMenu(hTimeoutPopup, MF_STRING, IDM_CLOSE_TIMEOUT_LEVEL1, vsConfig->GetI18nMessage(IDS_MENU_CLOSE_TIMEOUT_LEVEL1)); 
			AppendMenu(hTimeoutPopup, MF_STRING, IDM_CLOSE_TIMEOUT_LEVEL2, vsConfig->GetI18nMessage(IDS_MENU_CLOSE_TIMEOUT_LEVEL2)); 
			AppendMenu(hTimeoutPopup, MF_STRING, IDM_CLOSE_TIMEOUT_LEVEL3, vsConfig->GetI18nMessage(IDS_MENU_CLOSE_TIMEOUT_LEVEL3)); 
			AppendMenu(hTimeoutPopup, MF_STRING, IDM_CLOSE_TIMEOUT_LEVEL4, vsConfig->GetI18nMessage(IDS_MENU_CLOSE_TIMEOUT_LEVEL4)); 
			AppendMenu(hPopup, MF_STRING, IDM_SAVE_CONNECTCODE, vsConfig->GetI18nMessage(IDS_MENU_SAVE_CONNECTCODE));
			AppendMenu(hPopup, MF_STRING, IDM_SETTINGS, vsConfig->GetI18nMessage(IDS_MENU_SETTINGS)); 
			AppendMenu(hPopup, MF_SEPARATOR, NULL, NULL);
			AppendMenu(hPopup, MF_STRING, IDM_CONNECTION, vsConfig->GetI18nMessage(IDS_MENU_CONNECTION_INFO));
			AppendMenu(hPopup, MF_STRING|MF_POPUP, (UINT)hAbout, verInfo);
			AppendMenu(hAbout, MF_STRING, IDM_UPDATE, vsConfig->GetI18nMessage(IDS_MENU_UPDATE));
			AppendMenu(hPopup, MF_SEPARATOR, NULL, NULL);
			AppendMenu(hPopup, MF_STRING, IDM_DISCONNECT, vsConfig->GetI18nMessage(IDS_MENU_DISCONNECT)); 
			AppendMenu(hPopup, MF_STRING, IDM_EXIT, vsConfig->GetI18nMessage(IDS_MENU_EXIT)); 

			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			TrackPopupMenuEx(hPopup, TPM_RIGHTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, hWnd, NULL);

			DestroyMenu(hTimeoutPopup);
			DestroyMenu(hPopup);
			DestroyMenu(hMenu);
			break;
		}
		/*case WM_RBUTTONDBLCLK:
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hWnd;
			nid.uID = 100;
			wcscpy_s(nid.szTip, L"�ٲ������");
			nid.uFlags = NIF_TIP;
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			
			break;*/
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*
 * ���� ���������� �޼��� ó����
 */
INT_PTR CALLBACK ConnectionProc(HWND hConnDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HBRUSH hBrush = NULL;
	HDC hdc, hdcMem;
	PAINTSTRUCT ps;
	HBITMAP hImage;
	TCHAR szConnectionMsg[MAX_LOADSTRING];
				
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set WS_EX_LAYERED on this window 
		SetWindowLong(hConnDlg, GWL_EXSTYLE, GetWindowLong(hConnDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
		// Make this window 90% hConnDlg
		SetLayeredWindowAttributes(hConnDlg, 0, (255 * DLG_CONNECTION_ALPHA) / 100, LWA_ALPHA);
		RECT rect;
		Windows8::GetDesktopRect(&rect);
		SetWindowPos(hConnDlg, HWND_TOPMOST, rect.left + offset.x, rect.bottom + (-1 * offset.y - dlgHeight), dlgWidth, dlgHeight, SWP_NOACTIVATE);
		return (INT_PTR)TRUE;
	}
	case WM_LBUTTONDBLCLK:
		ShowWindow(hConnDlg, SW_HIDE);
		break;
	case WM_NCLBUTTONDBLCLK:
		ShowWindow(hConnDlg, SW_HIDE);
		break;
	case WM_CTLCOLORDLG:
		hBrush = CreateSolidBrush(RGB(0,0,0));
		return (INT_PTR)hBrush;
	case WM_PAINT:
	{
		Color fontColor = Color(255, 255, 255);
		hdc = BeginPaint(hConnDlg, &ps);
		// TODO: ���⿡ �׸��� �ڵ带 �߰��մϴ�.
		SetBkMode(hdc, TRANSPARENT);
		Graphics graphics(hdc);

		if (socketManager->GetConnectionStatus() == VsAsyncSocketManager::STATUS_CONNECTED)
		{
			//����� ���� ������
			//���̸�, ���۸��, ���͸�
			BOOL deviceMode = socketManager->GetDeviceMode() ;
			hImage = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(deviceMode == 1 ? IDB_CONNECTION_TOUCH : IDB_CONNECTION_MOUSE), IMAGE_BITMAP, 0, 0, 0);
			hdcMem = CreateCompatibleDC(hdc);
			SelectObject(hdcMem, hImage);
			StretchBlt(hdc, 35, 35, 32, 32, hdcMem, 0, 0, 32, 32, SRCCOPY);
			DeleteObject(hdcMem);
			
			wsprintf(szConnectionMsg, socketManager->GetDeviceName());
			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionTopFontSize(), vsConfig->GetConnectionPaddingLeft() < 80 ? 80 : vsConfig->GetConnectionPaddingLeft(), 20);

			//���� ���
			TCHAR* strMode = deviceMode == 1 ? vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_MODE_TOUCH) : vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_MODE_MOUSE);
			wsprintf(szConnectionMsg, vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_MODE), strMode);
			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionBottomFontSize(), vsConfig->GetConnectionPaddingLeft(), 90);
			
			//���͸�
			INT battery = socketManager->GetDeviceBattery();
			wsprintf(szConnectionMsg, vsConfig->GetI18nMessage(battery > 100 ? IDS_ERROR_CONNECTION_BATTERY : IDS_MSG_CONNECTION_BATTERY), battery);
			
			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionBottomFontSize(), ((dlgWidth - vsConfig->GetConnectionPaddingLeft()) / 2) + vsConfig->GetConnectionPaddingLeft(), 90);
		}
		else
		{
			//���� ��� ������ ����
			hImage = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_CONNECTION_NONE), IMAGE_BITMAP, 0, 0, 0);
			hdcMem = CreateCompatibleDC(hdc);
			SelectObject(hdcMem, hImage);
			StretchBlt(hdc, 35, 35, 32, 32, hdcMem, 0, 0, 32, 32, SRCCOPY);
			DeleteObject(hdcMem);
			
			wsprintf(szConnectionMsg, vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_NOT_CONNECTED));
			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionTopFontSize(), vsConfig->GetConnectionPaddingLeft() < 80 ? 80 : vsConfig->GetConnectionPaddingLeft(), 20);

			//���� ��Ʈ
			wsprintf(szConnectionMsg, vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_PORT), socketManager->GetBroadcastPort());
			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionBottomFontSize(), vsConfig->GetConnectionPaddingLeft(), 90);
			
			//���� �ڵ�	
			wsprintf(szConnectionMsg, vsConfig->GetI18nMessage(IDS_MSG_CONNECTION_CODE), socketManager->GetConnectionCode());

			GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), szConnectionMsg, fontColor, 
				vsConfig->GetConnectionBottomFontSize(), ((dlgWidth - vsConfig->GetConnectionPaddingLeft()) / 2) + vsConfig->GetConnectionPaddingLeft() + 5, 90);
		}
		
		EndPaint(hConnDlg, &ps);
		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hConnDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

/*
 * �޼��� ���������� �޼��� ó����
 */
INT_PTR CALLBACK NotificationProc(HWND hNotiDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HBRUSH hBrush = NULL;
	HDC hdc;
	PAINTSTRUCT ps;
	Color fontColor = Color(255, 255, 255);
				
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Set WS_EX_LAYERED on this window 
		SetWindowLong(hNotiDlg, GWL_EXSTYLE, GetWindowLong(hNotiDlg, GWL_EXSTYLE) | WS_EX_LAYERED);
		// Make this window 90% hConnDlg
		SetLayeredWindowAttributes(hNotiDlg, 0, (255 * DLG_NOTIFICATION_ALPHA) / 100, LWA_ALPHA);
		// ��ġ �� ũ�� ����
		RECT rect;
		Windows8::GetDesktopRect(&rect);
		SetWindowPos(hNotiDlg, HWND_TOPMOST, (rect.right - rect.left) / 2 - dlgWidth / 2 + rect.left, rect.bottom + (-1 * offset.y - dlgHeight), dlgWidth, dlgHeight, SWP_NOACTIVATE);

		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		hBrush = CreateSolidBrush(RGB(0,0,0));
		return (INT_PTR)hBrush;
	case WM_PAINT:
		hdc = BeginPaint(hNotiDlg, &ps);
		// TODO: ���⿡ �׸��� �ڵ带 �߰��մϴ�.
		SetBkMode(hdc, TRANSPARENT);
		GDIPlus::DrawString(Graphics(hdc), vsConfig->GetFontFamily(), szNotificationMsg, fontColor, vsConfig->GetNotificationFontSize(), RectF(0, 0, (REAL)dlgWidth, (REAL)dlgHeight));
		EndPaint(hNotiDlg, &ps);
		break;
	case WM_SHOWWINDOW:
		if (wParam == SW_HIDE)
		{
			//������ ������ ����
			SetLayeredWindowAttributes(hNotiDlg, 0, (255 * DLG_NOTIFICATION_ALPHA) / 100, LWA_ALPHA);
		}
		else
		{
			//���� �ʱ�ȭ �� Ÿ�̸� ����
			fadeAlpha = DLG_NOTIFICATION_ALPHA;
			SetTimer(hNotiDlg, IDD_DLGNOTIFICATION, DLG_NOTIFICATION_DELAY, NULL);
		}
		break;
	case WM_TIMER:
		if (DLG_NOTIFICATION_ALPHA == fadeAlpha)
		{
			KillTimer(hNotiDlg, IDD_DLGNOTIFICATION);
		}

		//���̾�α� fade out
		while(fadeAlpha > 0)
		{
			fadeAlpha -= 3;
			SetLayeredWindowAttributes(hNotiDlg, 0, (255 * fadeAlpha) / 100, LWA_ALPHA);
			Sleep(50);
		}
		//���̾�α� ����		
		ShowWindow(hNotiDlg, SW_HIDE);

		break;
	}
	return (INT_PTR)FALSE;
}

/*
 * ���� ���������� �޼��� ó����
 */
INT_PTR CALLBACK SettingsProc(HWND hSettDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HBRUSH hBrush = NULL;
	HDC hdc;
	PAINTSTRUCT ps;
	Color fontColor = Color(0, 0, 0);
	UINT cbId = 10, hkId = cbId + 10 , chId = hkId + 10, btId = chId + 10;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		RECT rect;
		INT left = 240, top = 80, height = 22, margin = 5;
		USHORT nLabels = 0, nItems = 0;
		UINT *edgeLblIds = NULL, *itemIds = NULL;

		// ��ġ �� ũ�� ����
		Windows8::GetDesktopRect(&rect);
		SetWindowPos(hSettDlg, HWND_TOPMOST, rect.right - dlgWidth - 200 - offset.x , rect.bottom + (-1 * offset.y - dlgHeight * 2), dlgWidth + 200, dlgHeight * 2, SWP_NOACTIVATE);

		// Ensure that the common control DLL is loaded. 
		INITCOMMONCONTROLSEX icex;  //declare an INITCOMMONCONTROLSEX Structure
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_HOTKEY_CLASS;   //set dwICC member to ICC_HOTKEY_CLASS    
										// this loads the Hot Key control class.
		InitCommonControlsEx(&icex);  

		//��� ���ҽ� ID�ε�
		vsConfig->GetEdgeLabels(FALSE, nLabels, edgeLblIds);
		vsConfig->GetEdgeLabels(TRUE, nItems, itemIds);

		//��Ʈ �ʱ�ȭ �� ����
		LOGFONT lf;
		ZeroMemory(&lf, sizeof(LOGFONT));
		lf.lfHeight = 16;
		CopyMemory(lf.lfFaceName, vsConfig->GetFontFamily(), lstrlen(vsConfig->GetFontFamily()) + 1);
		HFONT font = CreateFontIndirect(&lf);
		
		for (int i=0; i<nLabels; i++)
		{
			int relatedTop = top + (i * (height + 20)), relatedLeft = left, width = 80;
			HWND cHWnd = CreateWindow(L"STATIC", vsConfig->GetI18nMessage(edgeLblIds[i]), WS_CHILD | WS_VISIBLE,
						 relatedLeft, relatedTop + 2, width, height, hSettDlg, NULL, hInst, NULL);
			SendMessage(cHWnd, WM_SETFONT, (WPARAM)font, 0);

			relatedLeft += width + margin;
			width = 100;
			cHWnd = CreateWindow(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                     relatedLeft, relatedTop, width, height + 200, hSettDlg, (HMENU)(cbId + i), hInst, NULL);
			SendMessage(cHWnd, WM_SETFONT, (WPARAM)font, 0);

			int nWidth = 0;
			HDC hDC = NULL;
			SIZE sz = {0};

			hDC = GetDC(cHWnd);
			SelectObject(hDC, (HGDIOBJ)font);
			
			for (int j=0; j<nItems; j++)
			{
				SendMessage(cHWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCWSTR)vsConfig->GetI18nMessage(itemIds[j])));
				GetTextExtentPoint32(hDC, vsConfig->GetI18nMessage(itemIds[j]), lstrlen(vsConfig->GetI18nMessage(itemIds[j])), &sz);
				nWidth = max(sz.cx, nWidth);
			}
						
			//�޺� ��Ӵٿ� ������ ����
			SendMessage(cHWnd, CB_SETDROPPEDWIDTH, nWidth + 10, 0);
			ReleaseDC(cHWnd, hDC);
			
			relatedLeft += width + margin;
			width = 120;
			cHWnd = CreateWindowEx(0, HOTKEY_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_DISABLED,
						 relatedLeft, relatedTop, width, height, hSettDlg, (HMENU)(hkId + i), hInst, NULL);
			SendMessage(cHWnd, WM_SETFONT, (WPARAM)font, 0);
			
			relatedLeft += width + margin;
			width = 20;
			cHWnd = CreateWindow(L"BUTTON", NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX  | WS_DISABLED,
						 relatedLeft, relatedTop, width, height, hSettDlg, (HMENU)(chId + i), hInst, NULL);
			SendMessage(cHWnd, WM_SETFONT, (WPARAM)font, 0);

			relatedLeft += width;
			width = 70;
			cHWnd = CreateWindow(L"STATIC", L"+ Windows", WS_CHILD | WS_VISIBLE,
						 relatedLeft, relatedTop + 2, width, height, hSettDlg, NULL, hInst, NULL);
			SendMessage(cHWnd, WM_SETFONT, (WPARAM)font, 0);
		}

		delete[] edgeLblIds;
		delete[] itemIds;

		CreateWindow(L"BUTTON", vsConfig->GetI18nMessage(IDS_DEFAULT),  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					410,245,75,23, hSettDlg, (HMENU)btId, hInst, 0);
		SendMessage(GetDlgItem(hSettDlg, btId++), WM_SETFONT, (WPARAM)font, 0);

		CreateWindow(L"BUTTON", vsConfig->GetI18nMessage(IDS_OK),  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					490,245,75,23, hSettDlg, (HMENU)btId, hInst, 0);
		SendMessage(GetDlgItem(hSettDlg, btId++), WM_SETFONT, (WPARAM)font, 0);

		CreateWindow(L"BUTTON", vsConfig->GetI18nMessage(IDS_CANCEL),  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					570,245,75,23, hSettDlg, (HMENU)btId, hInst, 0);
		SendMessage(GetDlgItem(hSettDlg, btId++), WM_SETFONT, (WPARAM)font, 0);
		
		return (INT_PTR)TRUE;
	}
	case WM_CTLCOLORDLG:
		hBrush = CreateSolidBrush(RGB(255, 255, 255));
		return (INT_PTR)hBrush;
	case WM_CTLCOLORSTATIC:
	{
		hBrush = CreateSolidBrush(RGB(255, 255, 255));
		HDC hdcStatic = (HDC) wParam;
		//SetTextColor(hdcStatic, RGB(0,0,0));
		SetBkColor(hdcStatic, RGB(255, 255, 255));
		return (INT_PTR)hBrush;
	}
	case WM_PAINT:
	{
		hdc = BeginPaint(hSettDlg, &ps);
		// TODO: ���⿡ �׸��� �ڵ带 �߰��մϴ�.
		Graphics graphics(hdc);
		//SetBkMode(hdc, TRANSPARENT);
		Pen pen(Color(230, 230, 230));
		//�ܰ��� �׸���
		graphics.DrawRectangle(&pen, 0, 0, dlgWidth + 200 - 1, dlgHeight * 2 - 1);
		//Ÿ��Ʋ ���ڿ� 
		GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), FontStyle::FontStyleBold, vsConfig->GetI18nMessage(IDS_SETTINGS_EDGE_GESTURE_SETTING_TITLE), fontColor, 25, 10, 15);
		GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), FontStyle::FontStyleBold, vsConfig->GetI18nMessage(IDS_SETTINGS_EDGE_GESTURE_SETTING_SUBTITLE), Color(8, 73, 176), 11, 20, 80);
		RectF rectF(20, 80, 200, 110);
		GDIPlus::DrawString(graphics, vsConfig->GetFontFamily(), FontStyle::FontStyleBold, vsConfig->GetI18nMessage(IDS_SETTINGS_EDGE_GESTURE_SETTING_MESSAGE), fontColor, 10, rectF);
		//�̹��� �׸���
		HBITMAP hImage;
		hImage = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_EDGE_SETTING), IMAGE_BITMAP, 0, 0, 0);
		HDC hdcMem = CreateCompatibleDC(hdc);
		SelectObject(hdcMem, hImage);
		StretchBlt(hdc, 60, 170, 120, 65, hdcMem, 0, 0, 200, 115, SRCCOPY);
		DeleteObject(hdcMem);
		EndPaint(hSettDlg, &ps);
		break;
	}
	case WM_SHOWWINDOW:
	{
		if (wParam != SW_HIDE)
		{
			//ȯ�� ���� �ε�...
			USHORT nCnt = 0;
			EDGE_GESTURE_INFO* edgeInfo = NULL;
			USHORT nItems = 0;
			UINT *itemsIds = NULL;
			
			vsConfig->GetEdgeLabels(TRUE, nItems, itemsIds);
			vsConfig->GetEdgeGesture(nCnt, edgeInfo);

			for (int i=0; i<nCnt; i++) 
			{
				//�޺��ڽ� �� ����
				SendMessage(GetDlgItem(hSettDlg, cbId + i), CB_SETCURSEL, edgeInfo[i].index, NULL);
				//Ŀ���� Ű �� ����
				SendMessage(GetDlgItem(hSettDlg, hkId + i), HKM_SETHOTKEY, edgeInfo[i].hotKey, NULL);
				CheckDlgButton(hSettDlg, chId + i, edgeInfo[i].winKey);

				BOOL bCheck = (itemsIds[edgeInfo[i].index] == IDS_SETTINGS_EDGE_GESTURE_CUSTOM);

				//��Ű �Է»��� Ȱ��/��Ȱ��ȭ
				EnableWindow(GetDlgItem(hSettDlg, hkId + i), bCheck);
				//������Ű �Է»��� Ȱ��/��Ȱ��ȭ
				EnableWindow(GetDlgItem(hSettDlg, chId + i), bCheck);
			}

			delete[] itemsIds;
		}
		break;
	}
	case WM_COMMAND:
	{
		UINT nDlgId = LOWORD(wParam);

		if (nDlgId >= cbId && nDlgId < hkId)
		{
			INT lastIndex = 0;
			//����� ����Ű ���ý� ��Ʈ�� Ȱ��/��Ȱ��
			INT rowId = SendMessage(GetDlgItem(hSettDlg, nDlgId), CB_GETCURSEL, 0, 0);
			USHORT nItems = 0;
			UINT *itemsIds = NULL;
			vsConfig->GetEdgeLabels(TRUE, nItems, itemsIds);
			BOOL bCheck = itemsIds[rowId] == IDS_SETTINGS_EDGE_GESTURE_CUSTOM;

			//��Ű �Է»��� Ȱ��/��Ȱ��ȭ
			EnableWindow(GetDlgItem(hSettDlg, nDlgId + 10), bCheck);
			//������Ű �Է»��� Ȱ��/��Ȱ��ȭ
			EnableWindow(GetDlgItem(hSettDlg, nDlgId + 20), bCheck);
			if (!bCheck)
			{
				//����� ���� �޺� �������� ���õ� ����� ��Ű �� ������ Ű �ʱ�ȭ
				SendMessage(GetDlgItem(hSettDlg, nDlgId + 10), HKM_SETHOTKEY, NULL, NULL);
				CheckDlgButton(hSettDlg, nDlgId + 20, bCheck);
			}

			delete[] itemsIds;
		}
		else if (nDlgId >= hkId && nDlgId < chId)
		{
			//�����Ŵ.. 
		}
		else if (nDlgId >= chId && nDlgId <btId)
		{
			BOOL bCheck = IsDlgButtonChecked(hSettDlg, nDlgId);
			CheckDlgButton(hSettDlg, nDlgId, !bCheck);
		}
		else 
		{
			if (nDlgId == btId)
			{
				//�⺻������ �ǵ�����
				USHORT nDefaultCnt = 0;
				UINT* defaultGestures = NULL;
				vsConfig->GetDefaultGesture(nDefaultCnt, defaultGestures);

				for (int i=0; i<nDefaultCnt; i++)
				{
					USHORT nItems = 0, defaultIndex = 0;
					UINT *edgeLblIds = NULL;
					vsConfig->GetEdgeLabels(i + 1, nItems, edgeLblIds);

					for (int j=0; j<nItems; j++)
					{
						if (edgeLblIds[j] == defaultGestures[i]) 
						{
							defaultIndex = j;
							break;
						}
					}
					delete[] edgeLblIds;

					//�⺻������ �ѹ�
					SendMessage(GetDlgItem(hSettDlg, cbId + i), CB_SETCURSEL, defaultIndex, NULL);
					SendMessage(GetDlgItem(hSettDlg, hkId + i), HKM_SETHOTKEY, NULL, NULL);
					CheckDlgButton(hSettDlg, chId + i, FALSE);
					//��Ű �Է»��� ��Ȱ��ȭ
					EnableWindow(GetDlgItem(hSettDlg, hkId + i), FALSE);
					//������Ű �Է»��� ��Ȱ��ȭ
					EnableWindow(GetDlgItem(hSettDlg, chId + i), FALSE);
				}
			}
			else if (nDlgId == btId + 1)
			{
				//���õ� ���� ������ ����
				EDGE_GESTURE_INFO edgeInfo[4];
				ZeroMemory(edgeInfo, sizeof(EDGE_GESTURE_INFO) * 4);

				for (int i=0; i<4; i++)
				{
					WORD rt = SendMessage(GetDlgItem(hSettDlg, hkId + i), HKM_GETHOTKEY, NULL, NULL);
					BOOL bCheck = IsDlgButtonChecked(hSettDlg, chId + i);

					edgeInfo[i].index = SendMessage(GetDlgItem(hSettDlg, cbId + i), CB_GETCURSEL, 0, 0);
					edgeInfo[i].hotKey = rt;
					edgeInfo[i].winKey = bCheck;
				}

				vsConfig->SaveEdgeGesture(4, edgeInfo);
				ShowWindow(hSettDlg, SW_HIDE);
			}
			else
			{
				ShowWindow(hSettDlg, SW_HIDE);
			}
		}

		break;
	}
	}
	return (INT_PTR)FALSE;
}

void ShowNotificationWindow(TCHAR* msg, INT nCmdShow)
{
	if (nCmdShow == SW_SHOW)
	{
		//�޼��� ����
		lstrcpyW(szNotificationMsg, msg);
		//��ž�������� â�̵�
		RECT rect;
		Windows8::GetDesktopRect(&rect);
		SetWindowPos(hNotiDlg, HWND_TOPMOST, (rect.right - rect.left) / 2 - dlgWidth / 2 + rect.left, rect.bottom + (-1 * offset.y - dlgHeight), dlgWidth, dlgHeight, SWP_NOSIZE);
		
		if (IsWindowVisible(hNotiDlg))
		{
			const RECT rt = {0, 0, dlgWidth, dlgHeight};
			InvalidateRect(hNotiDlg, &rt, TRUE);
		}
		else
			ShowWindow(hNotiDlg, nCmdShow);
	}
	else
		ShowWindow(hNotiDlg, nCmdShow);
	//���� ���� ����
	ShowConnectionInfoWindow(TRUE);
}

void ShowConnectionInfoWindow(BOOL bUpdate)
{
	RECT rect;
	Windows8::GetDesktopRect(&rect);
	SetWindowPos(hConnDlg, HWND_TOPMOST, rect.left + offset.x, rect.bottom + (-1 * offset.y - dlgHeight), dlgWidth, dlgHeight, SWP_NOSIZE);

	if (bUpdate)
	{
		if (IsWindowVisible(hConnDlg))
		{
			const RECT rt = {0, 0, dlgWidth, dlgHeight};
			InvalidateRect(hConnDlg, &rt, TRUE);
		}
	}
	else
		ShowWindow(hConnDlg, SW_SHOW);
}