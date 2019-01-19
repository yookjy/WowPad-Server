#pragma once
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>

using namespace Gdiplus;
using namespace std;

#pragma comment (lib,"Gdiplus.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

#define VS_OS_TYPE_IOS			1
#define VS_OS_TYPE_ANDROID		2
#define VS_OS_TYPE_WINDOWS		3

#define __SLASH(x) /##x
#define __DOUBLE_SLASH __SLASH(/)
#ifdef _DEBUG
#define _D
#define _R __DOUBLE_SLASH
#else
#define _D __DOUBLE_SLASH
#define _R
#endif

namespace vs
{
	namespace utils
	{
		namespace Registry
		{
			//레지스트리 키확인
			BOOL ExistsRegKey(HKEY hKey, LPCTSTR lpSubKey);
			//레지스트리 값확인
			BOOL ExistsRegValue(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValue);
			//레지스트리 등록
			BOOL WriteRegString(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue, LPCTSTR lpData);
			//레지스트리 삭제
			BOOL DeleteRegValue(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue);
			
		};

		namespace Network
		{
			//패킷 토큰
			char* GetPacketSeparator();
			//UDP 테이블
			PMIB_UDPTABLE GetUDPTable();
			//사용가능 UDP 포트
			int GetAvailableUDPPort(const u_short nMin, const u_short nMax, const u_short nCnt);
			//맥 어드레스
			void GetMacAddreses(byte**, int&);
		};

		namespace Windows8
		{
			BOOL IsWindows8();
			BOOL ExistsActiveApps();
			void GetDesktopRect(RECT* rect);
		};

		class StopWatch
		{
			LARGE_INTEGER frequency_;
			LARGE_INTEGER startTime_;
			LARGE_INTEGER stopTime_;
		public:
			StopWatch(void);
			~StopWatch(void);

			void Start(void);
			void Stop(void);
			void Reset(void);
			LONGLONG MilliSeconds() const;
			LONGLONG StartMilliSeconds() const;
			LONGLONG StopMilliSeconds() const;
		};

		namespace Range
		{
			BOOL Enter(RECT rt, POINT pt);
			BOOL EnterX(RECT rt, POINT pt);
			BOOL EnterY(RECT rt, POINT pt);
		};

		namespace GDIPlus
		{
			RectF MeasureString(Graphics &graphics, Font &font,LPCTSTR String,int Count);
			RectF MeasureString(Graphics &graphics, Font &font,LPCTSTR String,int Count, RectF layout);
			RectF DrawString(Graphics &graphics, LPCWSTR fontFamilyName, LPCTSTR text, Color color, REAL fontSize, REAL x, REAL y);
			RectF DrawString(Graphics &graphics, LPCWSTR fontFamilyName, FontStyle fontStyle, LPCTSTR text, Color color, REAL fontSize, REAL x, REAL y);
			void DrawString(Graphics &graphics, LPCWSTR fontFamilyName, LPCTSTR text, Color color, REAL fontSize, RectF range);
			void DrawString(Graphics &graphics, LPCWSTR fontFamilyName, FontStyle fontStyle, LPCTSTR text, Color color, REAL fontSize, RectF range);
			BOOL GetEncCLSID(WCHAR *mime, CLSID *pClsid);
			Bitmap* ResizeClone(Bitmap *bmp, INT width, INT height, BOOL bKeepRatio);
			void GetScreenshot(Bitmap*& prevBmp, CLSID *clsid, EncoderParameters *encoderParameters, 
				char*& data, ULONG* bytesRead, POINT resolution, BOOL bKeepRatio);
			BOOL GetXorScreenshot(Bitmap*& prevBmp, ULONG maxXorSize, CLSID *clsid, EncoderParameters *encoderParameters, 
				char*& data, ULONG* bytesRead, POINT resolution, BOOL bKeepRatio);
		};

		namespace String
		{
			TCHAR* GetFileName(TCHAR file_path[]);
			void GetFilePath(TCHAR filePath[], TCHAR fileFullPath[]);
			INT DecimalToHexdecimal(int value);
		};

		namespace Events
		{
			UINT ShortCutKeyEvents(const USHORT nVirtualKeyArray, const WORD *virtualKeyArray);
			UINT ShortCutKeyEvents(const USHORT nVirtualKeyArray, const WORD *virtualKeyArray, const USHORT nRepeatVirtualKey, const WORD repeatVirtualKey);
		};

		namespace Packet
		{
			INT ByteArrayToInt(BYTE btArray[], INT& offset);
			INT CharArrayToInt(CHAR btArray[], INT& offset);
		}
	}
}
