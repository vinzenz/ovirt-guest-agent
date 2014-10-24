#include <windows.h>
#include <named_pipe.h>
#include "resource.h"

#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")

UINT WM_TASKBARCREATED;

static TCHAR const OVIRT_TRAY_CLS[] = TEXT("OVIRT_GUEST_AGENT_TRAY");

enum {
	WM_NOTIFY_ICON_MESSAGE = WM_USER + 1,
	IDT_TIMER = 102030
};

void RegisterWindow();
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
NOTIFYICONDATA notify = {};

Service g_ioService;
NamedPipeStream g_pipe(g_ioService);

static TCHAR const * const OVIRT_PIPE_NAME = TEXT("\\\\.\\pipe\\ovirt-agent-test");

void ConnectPipe() {
    if(g_pipe.IsOpen()) {
        return;
    }
    DWORD result = g_pipe.Open(OVIRT_PIPE_NAME);
    if(result == NO_ERROR) {
        return;
    }
    else {
        g_ioService.Post(ConnectPipe);
        ::WaitNamedPipe(OVIRT_PIPE_NAME, 100);        
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{	
    g_ioService.Start();
    g_ioService.Post(ConnectPipe);
    
	WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));

	RegisterWindow();

	HWND wnd = CreateWindow(OVIRT_TRAY_CLS, TEXT("ovirt-guest-agent"), WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, 0, 0, 0, 0);
	
	notify.cbSize = sizeof(notify);
	notify.hWnd = wnd;
	notify.hIcon = LoadIcon(0, IDI_APPLICATION);	
	notify.uCallbackMessage = WM_NOTIFY_ICON_MESSAGE;
	StringCchCopy(notify.szTip, 64, TEXT("ovirt-guest-agent"));
	notify.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;	
	Shell_NotifyIcon(NIM_ADD, &notify);
	SetTimer(wnd, IDT_TIMER, 10000, 0);

	MSG message;
	while(GetMessage(&message, 0, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	Shell_NotifyIcon(NIM_DELETE, &notify);
    g_ioService.Stop();
	return 0;
}

void RegisterWindow()
{
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(wcex);
	wcex.lpfnWndProc = (WNDPROC)WindowProc;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpszClassName = OVIRT_TRAY_CLS;
	wcex.hIcon = LoadIcon(0, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(0, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(0, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	RegisterClassEx(&wcex);
}

BOOL CALLBACK AboutDlgProc(HWND h, UINT m, WPARAM w, LPARAM l) {
	if(m == WM_COMMAND) {
		if(LOWORD(w) == IDOK) {
			EndDialog(h, w);
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK WindowProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	if(m == WM_NOTIFY_ICON_MESSAGE) {
		if(l == WM_LBUTTONUP) {
			MessageBox(0, TEXT("Geez"), TEXT("Click Click"), MB_OK|MB_ICONINFORMATION);
		}
		else if(l == WM_RBUTTONUP) {
			HMENU menu = LoadMenu(0, MAKEINTRESOURCE(IDR_TRAY_MENU));
			menu = GetSubMenu(menu, 0);
			POINT pt = {};
			GetCursorPos(&pt);
			INT result = TrackPopupMenu(menu, TPM_RETURNCMD, pt.x, pt.y, 0, h, 0);
			// TrackPopupMenu()
			if(ID_TRAYMENU_ABOUT == result) {			
				DialogBox(0, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), h, (DLGPROC)(AboutDlgProc));
			}
			if(ID_TRAYMENU_QUIT == result) {
				if(MessageBox(0, TEXT("Do you really want to quit the oVirt guest agent tray application?"), TEXT("Please confirm"), MB_ICONQUESTION|MB_YESNO) == IDYES) {
					PostMessage(h, WM_CLOSE, 0, 0);
				}
			}
		}
		return 0;
	}
	if(m == WM_TIMER) {
		switch(w) {
			case IDT_TIMER:
				{
					StringCchCopy(notify.szInfo, 256, TEXT("The ovirt-guest-agent sends a notification"));
					StringCchCopy(notify.szInfoTitle, 64, TEXT("ovirt-guest-agent notifies"));
					notify.dwInfoFlags = NIIF_INFO;
					notify.uFlags |= NIF_INFO;
					Shell_NotifyIcon(NIM_MODIFY, &notify);
					KillTimer(h, IDT_TIMER);
				}
		}
	}
	if(m == WM_CLOSE) {
		PostQuitMessage(0);
	}
	return DefWindowProc(h, m, w, l);
}