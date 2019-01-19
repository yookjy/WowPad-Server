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
 
				lRet = RegOpenKeyEx(  // ������Ʈ���� �����ϴ� �Լ�
								hKey,  // ������ Ű�� ��ƮŰ
								lpSubKey, // ������ ����Ű(���ڿ�)
								0,
								KEY_ALL_ACCESS,
								&key  // ������ Ű�� �ڵ�������
								);  // ������ ERROR_SUCCESS, ���н� 0�� �ƴѰ��� ���ϵ�
				RegCloseKey(key); // RegCreateKeyEx���� ���� �ڵ��� �ݴ� �Լ�
				return lRet == ERROR_SUCCESS;
			}

			////������Ʈ�� ��Ȯ��
			BOOL ExistsRegValue(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpValue)
			{
				LONG lRet = 0;
				LPDWORD pcbData = 0;
				lRet = RegGetValue(hKey, lpSubKey, lpValue, RRF_RT_ANY, NULL, NULL, pcbData);
	
				return lRet == ERROR_SUCCESS;
			}

			//������Ʈ�� ���
			BOOL WriteRegString(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue, LPCTSTR lpData)
			{
				HKEY key;
				if (RegCreateKeyEx(  // ������Ʈ��Ű�� ���� ������ִ� �Լ��̴�. ���� �����Ϸ��� Ű�� �����ϴ� ��� �ش� Ű�� ����.
								hKey,  // ������ Ű�� ��ƮŰ
								lpKey, // ������ ����Ű(���ڿ�)
								0, // �ݵ�� 0
								NULL, // Ű�� ������ Ŭ������(���ڿ�), (���� NULL �Է�)
								REG_OPTION_NON_VOLATILE,  // ������ ���Ͽ� ����Ѵ�. ( ���� �� �ɼ��� ��� ), REG_OPTION_VOLATILE - ������ �޸𸮿� ����մϴ�. ( �ý�������� ����� ��������. )
								KEY_WRITE,  // ����� ���õ� ��� ����, KEY_ALL_ACCESS - ��� ����, KEY_READ - �б�� ���õ� ��� ����, KEY_EXECUTE - KEY_READ�� ����
								NULL, // SECURITY_ATTRIBUTES ����ü�� ������. (���� NULL �Է�)
								&key,  // ������ Ű�� �ڵ�������
								NULL // DWORD�� ������, ������ Ű�� ����, (���� NULL �Է�)
								) != ERROR_SUCCESS)  // ������ ERROR_SUCCESS, ���н� 0�� �ƴѰ��� ���ϵ�
					return FALSE;
				
				if (RegSetValueEx( // ������Ʈ��Ű�� ���� �ϴ� �Լ�
								key,  // RegCreateKeyEx���� ���� �ڵ鰪
								lpValue, // �� �̸�
								0, // �ݵ�� 0
								REG_SZ, //���ڿ� ����Ÿ Ÿ��
								(LPBYTE)lpData, // �� ������
								lstrlen(lpData) * sizeof(TCHAR) + 1 //���� Ÿ����(REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ) �� ��� ���ڿ��� ũ��
								) != ERROR_SUCCESS)  // ������ ERROR_SUCCESS, ���н� 0�� �ƴѰ��� ���ϵ�
					return FALSE;

				RegCloseKey(key); // RegCreateKeyEx���� ���� �ڵ��� �ݴ� �Լ�
				return TRUE;
			}

			//������Ʈ�� ����
			BOOL DeleteRegValue(HKEY hKey, LPCTSTR lpKey, LPCTSTR lpValue)
			{
				HKEY key;
				LONG lRet = 0;
 
				if(RegOpenKeyEx(  // ������Ʈ���� �����ϴ� �Լ�
							hKey,  // ������ Ű�� ��ƮŰ
							lpKey, // ������ ����Ű(���ڿ�)
							0,
							KEY_ALL_ACCESS,
							&key  // ������ Ű�� �ڵ�������
							) != ERROR_SUCCESS)  // ������ ERROR_SUCCESS, ���н� 0�� �ƴѰ��� ���ϵ�
					return FALSE;
 
				lRet = RegDeleteValue( // ������Ʈ���� �����ϴ� �Լ�
											key,  // RegOpenKey���� ���� �ڵ鰪
											lpValue // �� �̸�
											);
  
				RegCloseKey(key); // RegCreateKeyEx���� ���� �ڵ��� �ݴ� �Լ�
				return ((lRet == ERROR_SUCCESS)? TRUE : FALSE);  // ������ ERROR_SUCCESS, ���н� 0�� �ƴѰ��� ���ϵ�
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
						//��Ʈ��ũ �������̽� Ÿ���� �̴���, ��������
						if (pCurrAddresses->PhysicalAddressLength == 6 
							&& (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD || pCurrAddresses->IfType == IF_TYPE_IEEE80211)) 
						{
							//DNS ī��Ʈ
							unsigned int dnsCnt = 0;
							pDnServer = pCurrAddresses->FirstDnsServerAddress;
							if (pDnServer) {
								for (int i = 0; pDnServer != NULL; i++)
								{
									pDnServer = pDnServer->Next;
									dnsCnt++;
								}
							}

							//DNS�� ����
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

				//�� �׷��� ������ dwMajorVersion = 8, dwMinorVersio = 4 �� ������ ��찡 �߻��ߴ�.
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
				//���콺 Ŀ�� �̹��� �ռ�
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

				//resizedBmp->Save(L"C:\\Users\\�ֿ�\\Pictures\\win.png", clsid, encoderParameters);

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

				//ȭ�� ĸ�� �̹�����
				HDC hFullDC = NULL;
				HBITMAP hFullBitmap = NULL;
				Bitmap *fullBmp = NULL;
				Bitmap *rszFullBmp = NULL;
				ULONG rszFullSize = 0;
				LPSTREAM iFullStream = NULL;
				
				//XOR �̹����� 
				HDC hXorDC = NULL;
				HBITMAP hXorBitmap = NULL;
				Bitmap *xorBmp = NULL;
				ULONG xorSize = 0;
				LPSTREAM iXorStream = NULL;
				
				//��ž DC
				HDC hDesktopDC = GetDC(NULL);
				
				//ȭ�� ĸ��
				hFullDC = CreateCompatibleDC(hDesktopDC);
				hFullBitmap = CreateCompatibleBitmap(hDesktopDC, width, height);

				SelectObject(hFullDC, hFullBitmap);
				BitBlt(hFullDC, 0, 0, width, height, hDesktopDC, rt.left, rt.top, SRCCOPY);
/*
				//���콺 Ŀ�� �̹��� �ռ�
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
				//ȭ�� ĸ�� ������¡
				fullBmp = Bitmap::FromHBITMAP(hFullBitmap, NULL);
				rszFullBmp = GDIPlus::ResizeClone(fullBmp, resolution.x, resolution.y, bKeepRatio);

				DeleteObject(hFullBitmap);
				DeleteDC(hFullDC);
				
				if (prevBmp != NULL)
				{
					/*TCHAR fileName1[128];
					wsprintf(fileName1, L"C:\\Users\\�ֿ�\\Pictures\\prev%ld.png", prevBmp);
					prevBmp->Save(fileName1, clsid, encoderParameters);*/

					Graphics rszFullGrp(rszFullBmp);
					Graphics prevGrp(prevBmp);

					//�̹����� DC
					hFullDC = rszFullGrp.GetHDC();
					hXorDC = prevGrp.GetHDC();
					
					rszFullBmp->GetHBITMAP(col, &hFullBitmap);
					prevBmp->GetHBITMAP(col, &hXorBitmap);

					//���� ������¡�� ĸ�� �̹����� ���� �̹����� XOR �ռ�
					SelectObject(hFullDC, hFullBitmap);
					SelectObject(hXorDC, hXorBitmap);

					//XOR �̹��� �ռ�
					BitBlt(hXorDC, 0, 0, resolution.x, resolution.y, hFullDC, 0, 0, SRCINVERT);
					
					//XOR �̹��� PNG��ȯ
					CreateStreamOnHGlobal(NULL, TRUE, &iXorStream);
					xorBmp = Bitmap::FromHBITMAP(hXorBitmap, NULL);
					xorBmp->Save(iXorStream, clsid, encoderParameters);
					
					// Find the size of the resulting buffer
					STATSTG statstg;
					iXorStream->Stat(&statstg, STATFLAG_DEFAULT);
					xorSize = (ULONG)statstg.cbSize.LowPart;

					//DC����
					rszFullGrp.ReleaseHDC(hFullDC);
					prevGrp.ReleaseHDC(hXorDC);
				}
								
				//ȭ�� ĸ�� �̹��� PNG ��ȯ
				CreateStreamOnHGlobal(NULL, TRUE, &iFullStream);
				rszFullBmp->Save(iFullStream, clsid, encoderParameters);
					
				// Find the size of the resulting buffer
				STATSTG statstgFull;
				iFullStream->Stat(&statstgFull, STATFLAG_DEFAULT);
				rszFullSize = (ULONG)statstgFull.cbSize.LowPart;

				// Copy the resulting data from the stream into our result object
				if (data != NULL) delete[] data;

				//�����̰ų� xor�̹����� ��ü ĸ�ĺ��� Ŭ���� ��ü ĸ�ĸ� ���
				if (xorSize <= 0 || xorSize >= rszFullSize || xorSize > maxXorSize)
				{
					//��ü ĸ�縦 ���
					isXor = false;
					//���� ����
					data = new char[rszFullSize];
					// Seek to the beginning of the stream
					iFullStream->Seek(li, STREAM_SEEK_SET, NULL);
					//ULONG bytesRead;
					iFullStream->Read(data, rszFullSize, bytesRead);
				}
				else
				{
					//xor�� ���
					isXor = true;
					//���ۻ���
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
				wsprintf(fileName, L"C:\\Users\\�ֿ�\\Pictures\\xor%ld.png", prevBmp);
				xorBmp->Save(fileName, clsid, encoderParameters);
				//
				//ZeroMemory(fileName, 128);
				//wsprintf(fileName, L"C:\\Users\\�ֿ�\\Pictures\\full%ld.png", prevBmp);
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

				//���� ��Ʈ�� ����
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

