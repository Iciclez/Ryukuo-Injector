#include <windows.h>
#include <VersionHelpers.h>
#include <cstdint>
#include <functional>
#include "mainwindow.hpp"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment (lib, "Icy")
#pragma comment (lib, "Shlwapi")

typedef HINSTANCE hinstance;
typedef LPSTR lpstr;

int32_t _stdcall WinMain(hinstance inst, hinstance, lpstr, int32_t) 
{
	//allow process to never be denied OpenProcess
	[]()
	{
		handle process = GetCurrentProcess();
		handle token = 0;
		if (OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES, &token))
		{
			CloseHandle(token);
			CloseHandle(process);
			return false;
		}

		LUID luid = { 0 };
		if (!LookupPrivilegeValueA(0, "SeDebugPrivilege", &luid))
		{
			CloseHandle(token);
			CloseHandle(process);
			return false;
		}

		TOKEN_PRIVILEGES privileges = { 0 };
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Luid = luid;
		privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &privileges, 0, 0, 0);

		CloseHandle(token);
		CloseHandle(process);

		return true;
	}();

	mainwindow w(inst);

	[](HWND wnd)
	{
		if (IsWindowsVistaOrGreater())
		{
			uint32_t WM_COPYGLOBALDATA = 0x0049;

			ChangeWindowMessageFilterEx(wnd, WM_DROPFILES, MSGFLT_ALLOW, NULL);
			ChangeWindowMessageFilterEx(wnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
			ChangeWindowMessageFilterEx(wnd, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
		}
	}(w.get_window()->get_handle());

	return w.message_loop();
}