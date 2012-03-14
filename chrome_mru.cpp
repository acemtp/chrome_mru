#include <windows.h>
#include <assert.h>

#include "resource.h"

using namespace std;

#define WM_TRAYMENU (WM_APP)

#define NAME_APP "Chrome MRU v1.0"

#define SETKEY_WIDTH 200
#define SETKEY_HEIGHT 50
#define SETKEY_TEXT_HEIGHT 20

LRESULT CALLBACK winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK winprocSetKey( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RegisterHook ();
void UnregisterHook ();

WNDCLASS wclass=
{     
	CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
	winproc,
    0,
	0,
	0,
	0,
	0,
	(HBRUSH)COLOR_WINDOW,
	NULL,
	TEXT("Cmw")
};

WNDCLASS wclassSetKey=
{     
	CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
	winprocSetKey,
    0,
	0,
	0,
	0,
	0,
	(HBRUSH)COLOR_WINDOW,
	NULL,
	TEXT("CmwSetKey")
};

// Default key to remap
unsigned int nHotKeyChar=0xde; // (tilde)

int mutex=0;
HFONT hFont;
HWND hWnd=NULL;
HWND hWndSetKey=NULL;
HWND hWndSetKeyText;
HKEY hKey;
HICON hIcon;
HMENU hMainMenu=NULL;
HMENU hPopup=NULL;
HINSTANCE g_hInstance;
HHOOK  hHook = NULL;
//char *lpKey="software\\acemtp\\ChromeMRU";

// stuffs to detect if the exe is already running
#pragma comment(linker, "/SECTION:.shr,RWS")
#pragma data_seg(".shr")
DWORD gProcessId = 0;
#pragma data_seg()


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	// stuffs to detect if the exe is already running
	bool AlreadyRunning;
	HANDLE hMutexOneInstance = ::CreateMutex (NULL, TRUE, "chromemru-189AA840-C10D-11D3-BC30-006066409622");
	AlreadyRunning = (GetLastError() == ERROR_ALREADY_EXISTS);
	if (hMutexOneInstance != NULL) ::ReleaseMutex(hMutexOneInstance);

	if (AlreadyRunning) {
		DWORD id = gProcessId;
		if (id != 0) {
			HANDLE hOther = OpenProcess (PROCESS_ALL_ACCESS,FALSE,id);
			TerminateProcess (hOther, 0);
			CloseHandle (hOther);
		}
	}

	gProcessId = GetCurrentProcessId ();

	g_hInstance = hInstance;

	// Create the font
	hFont = CreateFont (
		13,						// logical height of font
		0,						// logical average character width
		0,						// angle of escapement
		0,						// base-line orientation angle
		FW_NORMAL,				// font weight
		FALSE,					// italic attribute flag
		FALSE,					// underline attribute flag
		FALSE,					// strikeout attribute flag
		DEFAULT_CHARSET,		// character set identifier
		OUT_DEVICE_PRECIS,		// output precision
		CLIP_DEFAULT_PRECIS,	// clipping precision
		DEFAULT_QUALITY,        // output quality
		DEFAULT_PITCH|FF_DONTCARE,  // pitch and family
		"Arial"					// pointer to typeface name string
	);

	// Create the icon
	hIcon = (HICON)LoadImage (hInstance, MAKEINTRESOURCE(IDI_MAIN_ICO), IMAGE_ICON, 16, 16, 0);

	// Register a class
	wclass.hInstance = hInstance;
	wclass.hIcon = hIcon;
	RegisterClass(&wclass);
	wclassSetKey.hInstance = hInstance;
	wclassSetKey.hIcon = hIcon;
	RegisterClass( &wclassSetKey);

	// Create the window
	hWnd = CreateWindowEx (
		WS_EX_TOOLWINDOW,
		TEXT("Cmw"),
		"ChromeMru",
		WS_POPUP|WS_DLGFRAME|WS_CAPTION|WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		100,
		100,
		NULL,
		NULL,
		hInstance,
		0
	);
	assert (hWnd);

	// Read reg
/*
	RegCreateKey ( HKEY_CURRENT_USER, lpKey, &hKey);
	DWORD nSize = sizeof (unsigned int);
	DWORD nType;
	RegQueryValueEx (hKey, "1", NULL, &nType, (LPBYTE)&nHotKeyChar, &nSize);
*/

	// Register hotkey
	RegisterHook ();

	// Tray menu
	NOTIFYICONDATA pnid;
	pnid.cbSize = sizeof(NOTIFYICONDATA);
	pnid.hIcon = hIcon;
	pnid.hWnd = hWnd;
	pnid.uID = 0;
	strcpy_s (pnid.szTip,NAME_APP);
	pnid.uCallbackMessage = WM_TRAYMENU;
	pnid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
	Shell_NotifyIcon (NIM_ADD, &pnid);

	// Go to message loop..
	MSG msg;
	while (GetMessage (&msg, NULL, 0, 0)) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	if (IsWindow (hWnd)) DestroyWindow (hWnd);

	// Write to register
//	RegSetValueEx ( hKey, "1", NULL, REG_DWORD, (LPBYTE)&nHotKeyChar, sizeof(unsigned int));
//	RegCloseKey(hKey);

	// Unregister hotkeys
	UnregisterHook ();

	// Free resources
	DestroyIcon (hIcon);
	DeleteObject (hFont);

	return 0;
}

LONG GetRegKey (HKEY key, LPCTSTR subkey, LPTSTR retdata) {
    HKEY hkey;
    LONG retval = RegOpenKeyEx (key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS)  {
        long datasize = MAX_PATH;
        char data[MAX_PATH];
        RegQueryValue (hkey, NULL, data, &datasize);
        lstrcpy (retdata,data);
        RegCloseKey (hkey);
    }
    return retval;
}

void GotoURL (LPCTSTR url) {
    char key[MAX_PATH + MAX_PATH];

    // First try ShellExecute()
    HINSTANCE result = ShellExecute (NULL, "open", url, NULL,NULL, SW_SHOW);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) {
        if (GetRegKey(HKEY_CLASSES_ROOT, ".htm", key) == ERROR_SUCCESS) {
            lstrcat(key, "\\shell\\open\\command");

            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) {
                char *pos;
                pos = strstr(key, "\"%1\"");
                if (pos == NULL) {                     // No quotes found
                    pos = strstr(key, "%1");       // Check for %1, without quotes 
                    if (pos == NULL)                   // No parameter at all...
                        pos = key+lstrlen(key)-1;
                    else
                        *pos = '\0';                   // Remove the parameter
                } else
                    *pos = '\0';                       // Remove the parameter

                lstrcat (pos, " ");
                lstrcat (pos, url);
                result = (HINSTANCE) WinExec (key, SW_SHOW);
            }
        }
    }
}

char *GetHotKey (int mod=MOD_CONTROL, int key=nHotKeyChar) {
	static char sKey[512];
	static char sKey2[512];
	sKey[0]=0;
	if (mod&MOD_CONTROL)
		strcat_s (sKey, "Ctrl+");
	if (mod&MOD_WIN)
		strcat_s (sKey, "Win+");
	if (mod&MOD_ALT)
		strcat_s (sKey, "Alt+");
	int size=strlen (sKey);
	sKey[size]=MapVirtualKey (key, 2)&127;
	sKey[size+1]=0;
	return sKey;
}

LRESULT CALLBACK LowLevelKeyboardProc (INT iCode, WPARAM wParam, LPARAM lParam) {
	if (iCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT *)lParam;
		BOOL bCtrl = GetAsyncKeyState (VK_CONTROL) >> (8 * sizeof(SHORT) - 1);
		if (bCtrl && pkbhs->vkCode == VK_TAB) {
			TCHAR tcInput [256];
			HWND h = GetForegroundWindow();
			GetWindowText (h, tcInput, 256);
			if(strstr(tcInput, "Google Chrome") != NULL) {
				INPUT input[1];
				ZeroMemory(input, sizeof(input));
				input[0].type = INPUT_KEYBOARD;
				input[0].ki.wVk = nHotKeyChar;
				if(wParam == WM_KEYUP) input[0].ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, input, sizeof(INPUT));
				return 1;
			}
		}
	}
	return CallNextHookEx(hHook, iCode, wParam, lParam);
}

void UnregisterHook () {
	if (hHook) {
		UnhookWindowsHookEx (hHook);
		hHook = NULL;
	}
}

void RegisterHook () {
	UnregisterHook();
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, g_hInstance, 0);
}

LRESULT CALLBACK winproc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (!hWndSetKey) {
		switch (uMsg) {
		case WM_COMMAND: {
				int wNotifyCode = HIWORD (wParam); // notification code 
				int wID = LOWORD (wParam);         // item, control, or accelerator identifier 
				HWND hwndCtl = (HWND) lParam;      // handle of control  
				switch (wID) {
				case ID_TRAY_HELP:
					GotoURL ("http://ploki.info/index.php?n=Main.ChromeMRU");
					break;
				case ID_TRAY_OPEN:
					ShowWindow (hWnd, SW_SHOW);
					break;
				case ID_TRAY_ABOUT:
					MessageBox (hWnd, "Chrome MRU Tab by acemtp", NAME_APP, MB_OK|MB_ICONINFORMATION);
					break;
				case ID_TRAY_SETHOTKEY: {
						HWND hDesktop=GetDesktopWindow();
						RECT rect;
						GetWindowRect (hDesktop, &rect);
						hWndSetKey=CreateWindowEx (
							WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
							TEXT("CmwSetKey"),
							"Choose your CTRL-TAB remapping (CTRL+Q, CTRL+~, ...)",
							WS_POPUP|WS_DLGFRAME|WS_CAPTION|WS_SYSMENU|WS_VISIBLE,
							(rect.right-rect.left-SETKEY_WIDTH)/2,
							(rect.bottom-rect.top-SETKEY_HEIGHT)/2,
							SETKEY_WIDTH,
							SETKEY_HEIGHT,
							NULL,
							NULL,
							g_hInstance,
							0
						);
						assert (hWndSetKey);
						GetClientRect (hWndSetKey, &rect);
						hWndSetKeyText=CreateWindowEx (
							0,
							TEXT("STATIC"),
							GetHotKey (),
							WS_CHILD|WS_VISIBLE|SS_CENTER,
							0,
							(rect.bottom-rect.top-SETKEY_TEXT_HEIGHT)/2,
							rect.right-rect.left,
							SETKEY_TEXT_HEIGHT,
							hWndSetKey,
							NULL,
							g_hInstance,
							0
						);
						assert (hWndSetKeyText);
					}
					break;
				case ID_TRAY_QUIT:
					PostMessage (hWnd, WM_QUIT, 0, 0);
					break;
				}
			}
			break;
		case WM_TRAYMENU: {
				switch(lParam) {
				case WM_LBUTTONDBLCLK:
					break;
				case WM_RBUTTONDOWN: {
						POINT pPos;

						// Create menu
						if (hMainMenu==NULL) hMainMenu = LoadMenu (g_hInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));

						GetCursorPos(&pPos);
						hPopup = GetSubMenu (hMainMenu, 0);

						assert (hPopup!=NULL);
						SetForegroundWindow (hWnd);
						TrackPopupMenuEx (hPopup, 0,	pPos.x, pPos.y, hWnd, NULL);
						PostMessage (hWnd, WM_NULL, 0, 0);
						break;
					}
				}
			}
			break;
		}
	}
	return DefWindowProc (hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK winprocSetKey (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static int mod;
	switch (uMsg) {
	case WM_CREATE:
		mod=0;
		break;
	case WM_DESTROY:
		hWndSetKey=NULL;
		break;
	case WM_SYSCHAR: {
			wParam=VkKeyScan (wParam);
			uMsg=WM_KEYDOWN;
		}
	case WM_KEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
			if ((int) wParam==VK_ESCAPE)
				DestroyWindow (hwnd);

			if (GetAsyncKeyState(VK_MENU)&0x8000)
				mod|=MOD_ALT;
			else
				mod&=~MOD_ALT;
			if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				mod|=MOD_CONTROL;
			else
				mod&=~MOD_CONTROL;
			if ((GetAsyncKeyState(VK_LWIN)&0x8000)||(GetAsyncKeyState(VK_RWIN)&0x1000))
				mod|=MOD_WIN;
			else
				mod&=~MOD_WIN;

			char *msg=GetHotKey (mod, '?');
			SetWindowText (hWndSetKeyText, msg);

			if ((uMsg==WM_KEYDOWN)) {
				if (((int)wParam!=VK_ESCAPE)&&
					((int)wParam!=WM_KEYDOWN)&&
					((int)wParam!=VK_LBUTTON)&&
					((int)wParam!=VK_RBUTTON)&&
					((int)wParam!=VK_CANCEL)&&
					((int)wParam!=VK_MBUTTON)&&
					((int)wParam!=VK_CLEAR)&&
					((int)wParam!=VK_RETURN)&&
					((int)wParam!=VK_SHIFT)&&
					((int)wParam!=VK_CONTROL)&&
					((int)wParam!=VK_MENU)&&
					((int)wParam!=VK_PAUSE)&&
					((int)wParam!=VK_CAPITAL)&&
					((int)wParam!=VK_KANA)&&
					((int)wParam!=VK_HANGEUL)&&
					((int)wParam!=VK_HANGUL)&&
					((int)wParam!=VK_JUNJA)&&
					((int)wParam!=VK_FINAL)&&
					((int)wParam!=VK_HANJA)&&
					((int)wParam!=VK_KANJI)&&
					((int)wParam!=VK_ESCAPE)&&
					((int)wParam!=VK_CONVERT)&&
					((int)wParam!=VK_NONCONVERT)&&
					((int)wParam!=VK_ACCEPT)&&
					((int)wParam!=VK_MODECHANGE)&&
					((int)wParam!=VK_PRIOR)&&
					((int)wParam!=VK_SELECT)&&
					((int)wParam!=VK_PRINT)&&
					((int)wParam!=VK_EXECUTE)&&
					((int)wParam!=VK_SNAPSHOT)&&
					((int)wParam!=VK_HELP)&&
					((int)wParam!=VK_LWIN)&&
					((int)wParam!=VK_RWIN)&&
					((int)wParam!=VK_APPS)&&
					((int)wParam!=VK_NUMLOCK)&&
					((int)wParam!=VK_SCROLL)&&
					((int)wParam!=VK_LSHIFT)&&
					((int)wParam!=VK_RSHIFT)&&
					((int)wParam!=VK_LCONTROL)&&
					((int)wParam!=VK_RCONTROL)&&
					((int)wParam!=VK_LMENU)&&
					((int)wParam!=VK_RMENU)&&
					((int)wParam!=VK_ATTN)&&
					((int)wParam!=VK_CRSEL)&&
					((int)wParam!=VK_EXSEL)&&
					((int)wParam!=VK_EREOF)&&
					((int)wParam!=VK_PLAY)&&
					((int)wParam!=VK_ZOOM)&&
					((int)wParam!=VK_NONAME)&&
					((int)wParam!=VK_PA1)&&
					((int)wParam!=VK_OEM_CLEAR))
				{
					nHotKeyChar=(TCHAR) wParam;
					DestroyWindow (hWndSetKey);
					RegisterHook ();
				}
			}
		}
		return 0;
	}
	return DefWindowProc( hwnd, uMsg, wParam, lParam);
}
