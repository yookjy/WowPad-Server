#include "stdafx.h"
#include "VsUtils.h"

namespace vs 
{
	namespace utils
	{
		namespace Registry
		{
			BOOL ExistsRegKey(HKEY hKey, LPCTSTR lpSubKey)
			{
				HKEY key;
				LONG lRet = 0;
 
				lRet = RegOpenKeyEx(  // 레지스트리를 오픈하는 함수
								hKey,  // 생성할 키의 루트키
								lpSubKey, // 생성할 서브키(문자열)
								0,
								KEY_ALL_ACCESS,
								&key  // 생성된 키의 핸들포인터
								);  // 성공시 ERROR_SUCCESS, 실패시 0이 아닌값이 리턴됨
				RegCloseKey(key); // RegCreateKeyEx에서 얻은 핸들을 닫는 함수
				return lRet == ERROR_SUCCESS;
			}

			////레지스트리 값확인
			BOOL ExistsRegValue(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValue)
			{
				LONG lRet = 0;
				LPDWORD pcbData = 0;
				lRet = RegGetValue(hKey, lpSubKey, lpValue, RRF_RT_ANY, NULL, NULL, pcbData);
	
				return lRet == ERROR_SUCCESS;
			}

			//레지스트리 등록
			BOOL WriteRegString(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue, LPCTSTR lpData)
			{
				HKEY key;
				if (RegCreateKeyEx(  // 레지스트리키를 새로 만들어주는 함수이다. 만약 생성하려는 키가 존재하는 경우 해당 키를 오픈.
								hKey,  // 생성할 키의 루트키
								lpKey, // 생성할 서브키(문자열)
								0, // 반드시 0
								NULL, // 키의 지정된 클래스명(문자열), (보통 NULL 입력)
								REG_OPTION_NON_VOLATILE,  // 정보를 파일에 기록한다. ( 보통 이 옵션을 사용 ), REG_OPTION_VOLATILE - 정보를 메모리에 기록합니다. ( 시스템종료시 기록이 지워진다. )
								KEY_WRITE,  // 쓰기와 관련된 모든 권한, KEY_ALL_ACCESS - 모든 권한, KEY_READ - 읽기와 관련된 모든 권한, KEY_EXECUTE - KEY_READ와 동일
								NULL, // SECURITY_ATTRIBUTES 구조체의 포인터. (보통 NULL 입력)
								&key,  // 생성된 키의 핸들포인터
								NULL // DWORD의 포인터, 생성된 키의 상태, (보통 NULL 입력)
								) != ERROR_SUCCESS)  // 성공시 ERROR_SUCCESS, 실패시 0이 아닌값이 리턴됨
					return FALSE;
				
				if (RegSetValueEx( // 레지스트리키를 저장 하는 함수
								key,  // RegCreateKeyEx에서 얻은 핸들값
								lpValue, // 값 이름
								0, // 반드시 0
								REG_SZ, //문자열 데이타 타입
								(LPBYTE)lpData, // 값 데이터
								lstrlen(lpData) * sizeof(TCHAR) + 1 //값의 타입이(REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ) 일 경우 문자열의 크기
								) != ERROR_SUCCESS)  // 성공시 ERROR_SUCCESS, 실패시 0이 아닌값이 리턴됨
					return FALSE;

				RegCloseKey(key); // RegCreateKeyEx에서 얻은 핸들을 닫는 함수
				return TRUE;
			}

			//레지스트리 삭제
			BOOL DeleteRegValue(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue)
			{
				HKEY key;
				LONG lRet = 0;
 
				if(RegOpenKeyEx(  // 레지스트리를 오픈하는 함수
							hKey,  // 생성할 키의 루트키
							lpKey, // 생성할 서브키(문자열)
							0,
							KEY_ALL_ACCESS,
							&key  // 생성된 키의 핸들포인터
							) != ERROR_SUCCESS)  // 성공시 ERROR_SUCCESS, 실패시 0이 아닌값이 리턴됨
					return FALSE;
 
				lRet = RegDeleteValue( // 레지스트리를 삭제하는 함수
											key,  // RegOpenKey에서 얻은 핸들값
											lpValue // 값 이름
											);
  
				RegCloseKey(key); // RegCreateKeyEx에서 얻은 핸들을 닫는 함수
				return ((lRet == ERROR_SUCCESS)? TRUE : FALSE);  // 성공시 ERROR_SUCCESS, 실패시 0이 아닌값이 리턴됨
			}
		};

		namespace Network
		{
			char* GetPacketSeparator()
			{
				return "|";
			}

			PMIB_UDPTABLE GetUDPTable() {
 
				PMIB_UDPTABLE pTable = NULL;
				DWORD         dwSize =    0;

				if(::GetUdpTable(NULL, &dwSize, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
 
					if(dwSize > 0) {
 
						pTable = new MIB_UDPTABLE[dwSize];

						if(::GetUdpTable(pTable, &dwSize, TRUE) == NO_ERROR) {
 
							return pTable;
						}
					}
				}
 
				return NULL;
			}
			
			int GetAvailableUDPPort(const u_short nMin, const u_short nMax, const u_short nCnt) {
 
				PMIB_UDPTABLE pTable = GetUDPTable();
 
				if(pTable) {
 
					int nAvaPort = -1;

					for(u_short nport=nMin;nport<=nMax;nport++) {
 
						BOOL bfind = TRUE;

						for(DWORD nidx=0;nidx<pTable->dwNumEntries;nidx++) {
 
							for(u_short noff=0;noff<nCnt;noff++) {
 
								if(::htons(nport+noff) == (u_short)pTable->table[nidx].dwLocalPort) {
 
									nport += noff;
									bfind = FALSE;
 
									break;
								}
							}
 
							if(bfind == FALSE) {
 
								break;
							}
						}
 
						if(bfind == TRUE) {
 
							nAvaPort = (int)nport;
 
							break;
						}
					}
 
					delete[] pTable;
 
					return nAvaPort;
				}
 
				return -1;
			}

			void GetMacAddreses(byte** macAddresses, int& macAddressCount)
			{
				DWORD dwSize = 0;
				DWORD dwRetVal = 0;		

				// Set the flags to pass to GetAdaptersAddresses
				ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

				// default to unspecified address family (both)
				ULONG family = AF_INET;

				LPVOID lpMsgBuf = NULL;


				PIP_ADAPTER_ADDRESSES pAddresses = NULL;
				PIP_ADAPTER_DNS_SERVER_ADDRESS pDnServer = NULL;

				ULONG outBufLen = 0;
				ULONG Iterations = 0;

				outBufLen = 15000;

				do {
					pAddresses = new IP_ADAPTER_ADDRESSES[outBufLen];
					if (pAddresses == NULL) {
						return;
					}

					dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

					if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
						delete[] pAddresses;
						pAddresses = NULL;
					} else {
						break;
					}

					Iterations++;

				} while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < 3));

				if (dwRetVal == NO_ERROR) {
					PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;
				
					while (pCurrAddresses)
					{
						//네트워크 인터페이스 타입이 이더넷, 와이파이
						if (pCurrAddresses->PhysicalAddressLength == 6 
							&& (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD || pCurrAddresses->IfType == IF_TYPE_IEEE80211)) 
						{
							//DNS 카운트
							unsigned int dnsCnt = 0;
							pDnServer = pCurrAddresses->FirstDnsServerAddress;
							if (pDnServer) {
								for (int i = 0; pDnServer != NULL; i++)
								{
									pDnServer = pDnServer->Next;
									dnsCnt++;
								}
							}

							//DNS가 존재
							if (dnsCnt > 0)
							{
								macAddresses[macAddressCount] = new byte[6];

								for (int i = 0; i < 6; i++) 
									macAddresses[macAddressCount][i] = (byte) pCurrAddresses->PhysicalAddress[i];

								macAddressCount++;
							}
						}
						pCurrAddresses = pCurrAddresses->Next;
					}
				}

				delete[] pAddresses;
			}
		};

		namespace Windows8
		{
			BOOL IsWindows8()
			{
				OSVERSIONINFO osvi;
				ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
				osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
				BOOL bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

				//왜 그런지 모르지만 dwMajorVersion = 8, dwMinorVersio = 4 이 나오는 경우가 발생했다.
				return ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 2) 
					|| (osvi.dwMajorVersion == 8) && (osvi.dwMinorVersion == 4) 
					|| FindWindow(L"ImmersiveLauncher", NULL) != NULL);
			}

			BOOL ExistsActiveApps()
			{
				HWND hRootApp = FindWindow(L"Windows.UI.Core.CoreWindow", NULL);
				return hRootApp ? true : false;
			}

			void GetDesktopRect(RECT* rect)
			{
				HWND hRootApp = FindWindow(L"ImmersiveLauncher", NULL);
				BOOL ret = GetWindowRect(hRootApp, rect);
				if (!ret)
				{
					rect->left = 0;
					rect->top = 0;
					rect->right = GetSystemMetrics(SM_CXSCREEN);
					rect->bottom = GetSystemMetrics(SM_CYSCREEN);
				}
			}
		};

		namespace Range
		{
			BOOL Enter(RECT rt, POINT pt)
			{
				return rt.left <= pt.x && pt.x <= rt.right && rt.top <= pt.y && pt.y <= rt.bottom;
			}
			BOOL EnterX(RECT rt, POINT pt)
			{
				return rt.left <= pt.x && pt.x <= rt.right;
			}
			BOOL EnterY(RECT rt, POINT pt)
			{
				return rt.top <= pt.y && pt.y <= rt.bottom;
			}
		};

		namespace GDIPlus
		{
			RectF MeasureString(Graphics &graphics, Font &font,LPCTSTR String,int Count)
			{
				 RectF layout(0,0,65536,65536);
				 CharacterRange cr(0,Count == -1 ? lstrlen(String):Count);
				 StringFormat sf;
				 sf.SetMeasurableCharacterRanges(1,&cr);
				 Region rgn;
				 graphics.MeasureCharacterRanges(String,Count,&font,layout,&sf,1,&rgn);
				 RectF rt;
				 rgn.GetBounds(&rt,&graphics);
				 return rt;
			}

			RectF MeasureString(Graphics &graphics, Font &font,LPCTSTR String,int Count, RectF layout)
			{
				 CharacterRange cr(0,Count == -1 ? lstrlen(String):Count);
				 StringFormat sf;
				 sf.SetMeasurableCharacterRanges(1,&cr);
				 Region rgn;
				 graphics.MeasureCharacterRanges(String,Count,&font,layout,&sf,1,&rgn);
				 RectF rt;
				 rgn.GetBounds(&rt,&graphics);
				 return rt;
			}

			RectF DrawString(Graphics &graphics, LPCWSTR fontFamilyName, FontStyle fontStyle, LPCTSTR text, Color color, REAL fontSize, REAL x, REAL y)
			{
				Font font(fontFamilyName, fontSize, fontStyle, UnitPoint);
				graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
				graphics.DrawString(text, -1, &font, PointF(x,y), &SolidBrush(color));
				return GDIPlus::MeasureString(graphics, font, text, -1);
			}

			RectF DrawString(Graphics &graphics, LPCWSTR fontFamilyName, LPCTSTR text, Color color, REAL fontSize, REAL x, REAL y)
			{
				return GDIPlus::DrawString(graphics, fontFamilyName, FontStyleRegular, text, color, fontSize, x, y);
			}

			void DrawString(Graphics &graphics, LPCWSTR fontFamilyName, FontStyle fontStyle, LPCTSTR text, Color color, REAL fontSize, RectF range)
			{
				Font font(fontFamilyName, fontSize, FontStyleRegular, UnitPoint);
				
				RectF rf = GDIPlus::MeasureString(graphics, font, text, -1, range);

				range.X += (range.Width - rf.Width) / 2;
				range.Y += (range.Height - rf.Height) / 2;
				 
				CharacterRange cr(0, lstrlen(text));
				StringFormat sf;
				sf.SetMeasurableCharacterRanges(1,&cr);

				graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
				graphics.DrawString(text,-1, &font, range, &sf,&SolidBrush(color));
			}

			void DrawString(Graphics &graphics, LPCWSTR fontFamilyName, LPCTSTR text, Color color, REAL fontSize, RectF range)
			{
				DrawString(graphics, fontFamilyName, FontStyleRegular, text, color, fontSize, range);
			}

			BOOL GetEncCLSID(WCHAR *mime, CLSID *pClsid)
			{
				UINT num,size,i;
				ImageCodecInfo *arCod;
				BOOL bFound=FALSE;

				GetImageEncodersSize(&num,&size);
				arCod = (ImageCodecInfo *)malloc(size);
				if (arCod != NULL)
				{
					GetImageEncoders(num,size,arCod);

					for (i=0;i<num;i++) {
						if(wcscmp(arCod[i].MimeType,mime)==0) {
							*pClsid=arCod[i].Clsid;
							bFound=TRUE;
							break;
						}    
					}
					free(arCod);
				}
				
				return bFound;
			}

			Bitmap* ResizeClone(Bitmap *bmp, INT width, INT height, BOOL bKeepRatio)
			{
				UINT o_height = bmp->GetHeight();
				UINT o_width = bmp->GetWidth();
				INT n_width = width;
				INT n_height = height;

				if (bKeepRatio)
				{
					double ratio = ((double)o_width) / ((double)o_height);
					if (o_width > o_height) {
						// Resize down by width
						n_height = static_cast<UINT>(((double)n_width) / ratio);
					} else {
						n_width = static_cast<UINT>(n_height * ratio);
					}
				}
				
				Gdiplus::Bitmap* newBitmap = new Gdiplus::Bitmap(n_width, n_height, bmp->GetPixelFormat());
				Gdiplus::Graphics graphics(newBitmap);
				graphics.DrawImage(bmp, 0, 0, n_width, n_height);
				return newBitmap;
			}
			
			void GetScreenshot(Bitmap*& resizedBmp, CLSID *clsid, EncoderParameters *encoderParameters, 
				char*& data, ULONG* bytesRead, POINT resolution, BOOL bKeepRatio)
			{
				RECT rt;
				Windows8::GetDesktopRect(&rt);
				int width = rt.right - rt.left;
				int height = rt.bottom - rt.top;

				HDC hDesktopDC = GetDC(NULL);
				HDC hCaptureDC = CreateCompatibleDC(hDesktopDC);
				HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hDesktopDC, width, height);

				SelectObject(hCaptureDC, hCaptureBitmap);
				BitBlt(hCaptureDC, 0, 0, width, height, hDesktopDC, rt.left, rt.top, SRCCOPY);
				
				/*
				//마우스 커서 이미지 합성
				CURSORINFO cursorInfo = { 0 };
				cursorInfo.cbSize = sizeof(cursorInfo);

				if (::GetCursorInfo(&cursorInfo))
				{
					ICONINFO ii = {0};
					GetIconInfo(cursorInfo.hCursor, &ii);
					DeleteObject(ii.hbmColor);
					DeleteObject(ii.hbmMask);
					::DrawIcon(hCaptureDC, cursorInfo.ptScreenPos.x - ii.xHotspot, cursorInfo.ptScreenPos.y - ii.yHotspot, cursorInfo.hCursor);
				}
				*/
				delete resizedBmp;
				Bitmap *pBitmap = Bitmap::FromHBITMAP(hCaptureBitmap, NULL);
				resizedBmp = GDIPlus::ResizeClone(pBitmap, resolution.x, resolution.y, bKeepRatio);
				delete pBitmap;

				//resizedBmp->Save(L"C:\\Users\\주용\\Pictures\\win.png", clsid, encoderParameters);

				//IStream *iStream = NULL;
				LPSTREAM iStream = NULL;
				CreateStreamOnHGlobal(NULL, TRUE, &iStream);
				resizedBmp->Save(iStream, clsid, encoderParameters);
					
				// Find the size of the resulting buffer
				STATSTG statstg;
				iStream->Stat(&statstg, STATFLAG_DEFAULT);
				ULONG bmpBufferSize = (ULONG)statstg.cbSize.LowPart;
				int length = bmpBufferSize;
 
				// Seek to the beginning of the stream
				LARGE_INTEGER li = {0};
				iStream->Seek(li, STREAM_SEEK_SET, NULL);
 
				// Copy the resulting data from the stream into our result object
				if (data != NULL)
					delete[] data;
				
				data = new char[bmpBufferSize];

				//ULONG bytesRead;
				iStream->Read(data, bmpBufferSize, bytesRead);
				
				iStream->Release();
				ReleaseDC(0, hDesktopDC);
				DeleteDC(hCaptureDC);
				DeleteObject(hCaptureBitmap);
			}

			BOOL GetXorScreenshot(Bitmap*& prevBmp, ULONG maxXorSize, CLSID *clsid, EncoderParameters *encoderParameters, 
				char*& data, ULONG* bytesRead, POINT resolution, BOOL bKeepRatio)
			{
				RECT rt;
				Windows8::GetDesktopRect(&rt);
				int width = rt.right - rt.left;
				int height = rt.bottom - rt.top;
				BOOL isXor = true;
				Color col(255, 255, 255);
				LARGE_INTEGER li = {0};

				//화면 캡쳐 이미지용
				HDC hFullDC = NULL;
				HBITMAP hFullBitmap = NULL;
				Bitmap *fullBmp = NULL;
				Bitmap *rszFullBmp = NULL;
				ULONG rszFullSize = 0;
				LPSTREAM iFullStream = NULL;
				
				//XOR 이미지용 
				HDC hXorDC = NULL;
				HBITMAP hXorBitmap = NULL;
				Bitmap *xorBmp = NULL;
				ULONG xorSize = 0;
				LPSTREAM iXorStream = NULL;
				
				//데탑 DC
				HDC hDesktopDC = GetDC(NULL);
				
				//화면 캡쳐
				hFullDC = CreateCompatibleDC(hDesktopDC);
				hFullBitmap = CreateCompatibleBitmap(hDesktopDC, width, height);

				SelectObject(hFullDC, hFullBitmap);
				BitBlt(hFullDC, 0, 0, width, height, hDesktopDC, rt.left, rt.top, SRCCOPY);
/*
				//마우스 커서 이미지 합성
				CURSORINFO cursorInfo = { 0 };
				cursorInfo.cbSize = sizeof(cursorInfo);

				if (::GetCursorInfo(&cursorInfo))
				{
					ICONINFO ii = {0};
					GetIconInfo(cursorInfo.hCursor, &ii);
					DeleteObject(ii.hbmColor);
					DeleteObject(ii.hbmMask);
					::DrawIcon(hFullDC, cursorInfo.ptScreenPos.x - ii.xHotspot, cursorInfo.ptScreenPos.y - ii.yHotspot, cursorInfo.hCursor);
				}
*/
				//화면 캡춰 리사이징
				fullBmp = Bitmap::FromHBITMAP(hFullBitmap, NULL);
				rszFullBmp = GDIPlus::ResizeClone(fullBmp, resolution.x, resolution.y, bKeepRatio);

				DeleteObject(hFullBitmap);
				DeleteDC(hFullDC);
				
				if (prevBmp != NULL)
				{
					/*TCHAR fileName1[128];
					wsprintf(fileName1, L"C:\\Users\\주용\\Pictures\\prev%ld.png", prevBmp);
					prevBmp->Save(fileName1, clsid, encoderParameters);*/

					Graphics rszFullGrp(rszFullBmp);
					Graphics prevGrp(prevBmp);

					//이미지의 DC
					hFullDC = rszFullGrp.GetHDC();
					hXorDC = prevGrp.GetHDC();
					
					rszFullBmp->GetHBITMAP(col, &hFullBitmap);
					prevBmp->GetHBITMAP(col, &hXorBitmap);

					//현재 리사이징된 캡쳐 이미지와 이전 이미지의 XOR 합성
					SelectObject(hFullDC, hFullBitmap);
					SelectObject(hXorDC, hXorBitmap);

					//XOR 이미지 합성
					BitBlt(hXorDC, 0, 0, resolution.x, resolution.y, hFullDC, 0, 0, SRCINVERT);
					
					//XOR 이미지 PNG변환
					CreateStreamOnHGlobal(NULL, TRUE, &iXorStream);
					xorBmp = Bitmap::FromHBITMAP(hXorBitmap, NULL);
					xorBmp->Save(iXorStream, clsid, encoderParameters);
					
					// Find the size of the resulting buffer
					STATSTG statstg;
					iXorStream->Stat(&statstg, STATFLAG_DEFAULT);
					xorSize = (ULONG)statstg.cbSize.LowPart;

					//DC해제
					rszFullGrp.ReleaseHDC(hFullDC);
					prevGrp.ReleaseHDC(hXorDC);
				}
								
				//화면 캡춰 이미지 PNG 변환
				CreateStreamOnHGlobal(NULL, TRUE, &iFullStream);
				rszFullBmp->Save(iFullStream, clsid, encoderParameters);
					
				// Find the size of the resulting buffer
				STATSTG statstgFull;
				iFullStream->Stat(&statstgFull, STATFLAG_DEFAULT);
				rszFullSize = (ULONG)statstgFull.cbSize.LowPart;

				// Copy the resulting data from the stream into our result object
				if (data != NULL) delete[] data;

				//최초이거나 xor이미지가 전체 캡쳐보다 클때는 전체 캡쳐를 사용
				if (xorSize <= 0 || xorSize >= rszFullSize || xorSize > maxXorSize)
				{
					//전체 캡춰를 사용
					isXor = false;
					//버퍼 생성
					data = new char[rszFullSize];
					// Seek to the beginning of the stream
					iFullStream->Seek(li, STREAM_SEEK_SET, NULL);
					//ULONG bytesRead;
					iFullStream->Read(data, rszFullSize, bytesRead);
				}
				else
				{
					//xor를 사용
					isXor = true;
					//버퍼생성
					data = new char[xorSize];
					// Seek to the beginning of the stream					
					iXorStream->Seek(li, STREAM_SEEK_SET, NULL);
					//ULONG bytesRead;
					iXorStream->Read(data, xorSize, bytesRead);
				}

				/*
				TCHAR fileName[128];
				if (xorSize > 0)
				{
				wsprintf(fileName, L"C:\\Users\\주용\\Pictures\\xor%ld.png", prevBmp);
				xorBmp->Save(fileName, clsid, encoderParameters);
				//
				//ZeroMemory(fileName, 128);
				//wsprintf(fileName, L"C:\\Users\\주용\\Pictures\\full%ld.png", prevBmp);
				//rszFullBmp->Save(fileName, clsid, encoderParameters);
				}
				*/
				if (iXorStream != NULL) iXorStream->Release();
				if (iFullStream != NULL) iFullStream->Release();
				
				DeleteObject(hFullBitmap);
				DeleteObject(hXorBitmap);

				ReleaseDC(0, hDesktopDC);
				/*DeleteDC(hFullDC);
				DeleteDC(hXorDC);*/

				delete fullBmp;
				delete xorBmp;
				delete prevBmp;

				//현재 비트맵 설정
				prevBmp = rszFullBmp;

				return isXor;
			}
		};

		namespace String
		{
			TCHAR* GetFileName(TCHAR file_path[])
			{
				TCHAR *file_name = NULL;
 
				while(*file_path)
				{
					if(*file_path == '\\' && (file_path +1) != NULL )
					{
						file_name = file_path+1;
					}
            
					file_path++; //mv pointer
               
				} 
				return file_name;
			}

			void GetFilePath(TCHAR filePath[], TCHAR fileFullPath[])
			{
				for (int i=lstrlen(fileFullPath); i>0 ;i--)
				{
					if (fileFullPath[i] == '\\')
					{
						lstrcpynW(filePath, fileFullPath, i + 2);
						break;
					}
				}
			}

			INT DecimalToHexdecimal(INT value)
			{
				int ch;
				char sh[5];
				int j=0,k=0;
 
				_itoa_s(value, sh, 10);
 
				int number = 0;
				j =strlen(sh);
				k=0;
				while(j!=0)
				{
					ch = sh[k];
 
					if(('0' <= ch && ch <= '9'))
					{
						number = number * 16;
						number = number + (ch - '0');
					}
 
					if(('A' <= ch && ch && ch <= 'Z'))
					{
						number = number * 16;
						number = number + (ch - '7');
					}
 
					j--;
					k++;
				}
				return number;
			}
		};

		namespace Events
		{
			UINT ShortCutKeyEvents(const USHORT nVirtualKeyArray, const WORD *virtualKeyArray)
			{
				return ShortCutKeyEvents(nVirtualKeyArray, virtualKeyArray, 0, 0);
			}

			UINT ShortCutKeyEvents(const USHORT nVirtualKeyArray, const WORD *virtualKeyArray, const USHORT nRepeatVirtualKey, const WORD repeatVirtualKey)
			{
				UINT ret = 0;
				INT index = 0, nCnt = nVirtualKeyArray + nRepeatVirtualKey;
				INPUT* inputs = new INPUT[nCnt * 2];
				memset(inputs, 0, sizeof(INPUT) * (nCnt * 2));

				for (; index<nCnt - nRepeatVirtualKey; index++)
				{
					inputs[index].type = INPUT_KEYBOARD;
					inputs[index].ki.wVk = virtualKeyArray[index];
					inputs[index].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
				}

				for (int i=1; i<=nRepeatVirtualKey * 2; i++)
				{
					if (i % 2 == 0)
					{
						inputs[index].type = INPUT_KEYBOARD;
						inputs[index].ki.wVk = repeatVirtualKey;
						inputs[index++].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
					}
					else
					{
						inputs[index].type = INPUT_KEYBOARD;
						inputs[index].ki.wVk = repeatVirtualKey;
						inputs[index++].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
					}
				}

				for (int i=nCnt - nRepeatVirtualKey; i>0; i--)
				{
					inputs[index].type = INPUT_KEYBOARD;
					inputs[index].ki.wVk = virtualKeyArray[i - 1];
					inputs[index++].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
				}

				ret = ::SendInput(nCnt * 2, inputs, sizeof(INPUT));
				delete[] inputs;
				return ret;
			}
		};

		namespace Packet
		{
			INT ByteArrayToInt(BYTE btArray[], INT &offset)
			{
				BYTE bits[sizeof(int)];
				CopyMemory(bits, btArray + offset, sizeof(int));			
				offset += sizeof(int);
				return *(int*)bits;
			}

			INT CharArrayToInt(CHAR chArray[], INT &offset)
			{
				return ByteArrayToInt((byte*)chArray, offset);
			}
		}
	}
}

