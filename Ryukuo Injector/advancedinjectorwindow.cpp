#include "advancedinjectorwindow.hpp"
#include "mainwindow.hpp"

#include <sstream>
#include <iomanip>

#include <VersionHelpers.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <Psapi.h>

#include "inject.hpp"
#include "resource.h"

#pragma comment (lib, "Comctl32.lib")


std::string advancedinjectorwindow::windowname()
{
	std::stringstream ss;
	ss << "Ryukuo Injector: Advanced Injector | Process Id: ";
	ss << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << GetCurrentProcessId();
	ss << " (";
	ss << std::dec << GetCurrentProcessId();
	ss << ")";

	return ss.str();
}

void advancedinjectorwindow::set_message_handler()
{
	this->w->add_message_handler(std::make_pair(menu_librarylist_add, [this](hwnd wnd, wparam, lparam)->lresult
	{
		OPENFILENAME ofn = { 0 };
		char szFile[MAX_PATH] = { 0 };

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = wnd;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "Dynamic-link Library (*.dll)\0*.dll\0\0"; // "Dynamic-link Library (*.dll)", "*.dll";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = 0;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = "Ryukuo Injector: Add to Library List...";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

		if (GetOpenFileName(&ofn))
		{

			if (!m_checklistview_dlllist->contains(1, std::string(szFile)))
			{
				m_checklistview_dlllist->insert_item(std::vector<std::string>({ szFile }));
			}
			else
			{
				MessageBox(wnd, "The dynamic-link library file you have just selected is already on the dynamic-link library list. A duplicate of the same dynamic-link library will not be accepted.", "Error: Ryukuo Injector", MB_OK | MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
			}
		}

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_librarylist_remove, [this](hwnd, wparam, lparam) -> lresult
	{
		m_checklistview_dlllist->remove_selected_item();

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_librarylist_removeselected, [this](hwnd, wparam, lparam) -> lresult
	{
		m_checklistview_dlllist->remove_selected_items();

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_librarylist_opencontainingfolder, [this](hwnd, wparam, lparam) -> lresult
	{
		std::string dir = m_checklistview_dlllist->text(m_checklistview_dlllist->get_next_selected_item(), 1);
		dir = dir.substr(0, dir.find_last_of('\\'));
		ShellExecute(0, TEXT("OPEN"), TEXT("EXPLORER.EXE"), dir.c_str(), 0, SW_SHOWNORMAL);

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_librarylist_clear, [this](hwnd, wparam, lparam) -> lresult
	{
		m_checklistview_dlllist->clear();

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_processlist_refresh, [this](hwnd, wparam, lparam) -> lresult
	{
		this->m_checklistview_processlist->clear();

		HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 p = { 0 };

		p.dwSize = sizeof(PROCESSENTRY32);

		SHFILEINFO sfi = { 0 };

		if (Process32First(h, &p))
		{
			if (!SUCCEEDED(SHGetFileInfo(".exe", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)))
			{
				CloseHandle(h);
				return 0;
			}
		}

		int32_t default_icon = ImageList_AddIcon(processlist_imagelist, sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		LVITEM lvI;
		char executable_path[MAX_PATH];

		do
		{
			std::memset(&lvI, 0, sizeof(LVITEM));

			lvI.mask = LVIF_TEXT | LVIF_IMAGE;
			lvI.iItem = this->m_checklistview_processlist->size();
			lvI.iImage = -1;

			lvI.iSubItem = 0;
			ListView_InsertItem(this->m_checklistview_processlist->get_handle(), &lvI);

			for (lvI.iSubItem = lvI.iSubItem + 1; lvI.iSubItem != 6; ++lvI.iSubItem)
			{
				if (lvI.iSubItem == 1)
				{
					lvI.iImage = default_icon;
					lvI.pszText = p.szExeFile;
	
					HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, p.th32ProcessID);
					if (process != INVALID_HANDLE_VALUE)
					{
						//GetModuleFileNameEx
						DWORD max_path = MAX_PATH;
						if (QueryFullProcessImageName(process, 0, executable_path, &max_path))
						{
							if (SUCCEEDED(SHGetFileInfo(executable_path, static_cast<DWORD>(-1), &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)))
							{
								int32_t app_icon = ImageList_AddIcon(processlist_imagelist, sfi.hIcon);
								if (app_icon != -1)
								{
									lvI.iImage = app_icon;
									DestroyIcon(sfi.hIcon);
								}
							}
						}
						CloseHandle(process);
					}
				}
				else if (lvI.iSubItem > 1)
				{
					lvI.iImage = -1;

					switch (lvI.iSubItem)
					{
					case 2:
						lvI.pszText = const_cast<char*>(std::to_string(p.th32ProcessID).c_str());
						break;
					case 3:
						lvI.pszText = const_cast<char*>(std::to_string(p.th32ParentProcessID).c_str());
						break;
					case 4:
						lvI.pszText = const_cast<char*>(std::to_string(p.cntThreads).c_str());
						break;
					case 5:
						lvI.pszText = const_cast<char*>(std::to_string(p.pcPriClassBase).c_str());
						break;
					}

				}

				ListView_SetItem(this->m_checklistview_processlist->get_handle(), &lvI);
			}

			ListView_RedrawItems(this->m_checklistview_processlist->get_handle(), this->m_checklistview_processlist->size(), this->m_checklistview_processlist->size());

		} while (Process32Next(h, &p));

		ListView_EnsureVisible(this->m_checklistview_processlist->get_handle(), this->m_checklistview_processlist->size(), TRUE);
		

		if (h)
		{
			CloseHandle(h);
		}

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_processlist_suspend, [this](hwnd, wparam, lparam) -> lresult
	{		
		handle process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, std::stoi(this->m_checklistview_processlist->text(this->m_checklistview_processlist->get_next_selected_item(), 2)));
		inject::suspend(process);
		CloseHandle(process);
		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_processlist_resume, [this](hwnd, wparam, lparam) -> lresult
	{
		handle process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, std::stoi(this->m_checklistview_processlist->text(this->m_checklistview_processlist->get_next_selected_item(), 2)));
		inject::resume(process);
		CloseHandle(process);
		return 0;

	}));

	this->w->add_message_handler(std::make_pair(menu_processlist_terminate, [this](hwnd, wparam, lparam) -> lresult
	{
		handle process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, std::stoi(this->m_checklistview_processlist->text(this->m_checklistview_processlist->get_next_selected_item(), 2)));
		TerminateProcess(process, 0);
		CloseHandle(process);
		this->refresh_processlist();
		return 0;
	}));

	this->w->add_message_handler(std::make_pair(button_inject, [this](hwnd, wparam, lparam) -> lresult
	{
		std::vector<uint32_t> checkedlist = this->m_checklistview_processlist->get_checked_list();
		for (const uint32_t n : checkedlist)
		{
			inject(static_cast<dword>(std::stoi(this->m_checklistview_processlist->text(n, 2).c_str())), inject::injection_routine::LOADLIBRARYA, inject::injection_thread_function::CREATEREMOTETHREAD, false, this->injection_error_handler).inject_dll(this->getdlllist());
		}
		
		return 0;
	}));

}

const std::vector<std::string> advancedinjectorwindow::getdlllist() const
{
	std::vector<std::string> selecteddlllist;

	std::vector<std::string> dll = m_checklistview_dlllist->column_text(1);
	if (dll.empty())
	{
		return selecteddlllist;
	}

	std::vector<uint32_t> checked = m_checklistview_dlllist->get_checked_list();
	if (checked.empty())
	{
		return selecteddlllist;
	}

	selecteddlllist.reserve(checked.size());

	for (const uint32_t n : checked)
	{
		selecteddlllist.push_back(dll.at(n));
	}

	return selecteddlllist;
}


advancedinjectorwindow::advancedinjectorwindow(mainwindow *mw, hinstance inst)
{
	this->mw = mw;
	this->inst = inst;


	this->injection_error_handler = [this](inject::injection_error error)
	{
		switch (error)
		{
		case inject::injection_error::ERROR_INVALID_PROCESS_ID:
			MessageBox(0, "An error has occured where the target process id is invalid.", "Error: Ryukuo Injector", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
			break;

		case inject::injection_error::ERROR_INVALID_PROCESS_HANDLE:
			MessageBox(0, "An error has occured where the target process handle is invalid.", "Error: Ryukuo Injector", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
			break;

		case inject::injection_error::ERROR_DLL_MAPPING_UNSUPPORTED:
			MessageBox(0, "DLL mapping is not supported by this DLL. Use the normal method for injection.", "Error: Ryukuo Injector", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
			break;

		default:
			MessageBox(0, "An unknown error has been encountered during the injection process.", "Error: Ryukuo Injector", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
			break;
		}
	};


	this->w = new window(inst, this->windowname(), "Ryukuo Injector: Advanced Injector", [&](hwnd wnd, wparam, lparam) -> void
	{
		rectangle r = window::get_window(wnd)->get_client_rectangle();
		hfont bold_font = widget::make_font("Segoe UI", 14, 0, true);


		this->m_groupbox_processlist = new groupbox(groupbox_processlist, r.left + 5, r.top + 5, r.right - 10, r.bottom / 3 * 2 - 5, "Process List", wnd);
		this->m_groupbox_dlllist = new groupbox(groupbox_dlllist, r.left + 5, r.bottom / 3 * 2 + 5, r.right - 10, r.bottom - (r.bottom / 3 * 2 + 35), "Dynamic Link Library List", wnd);

		this->m_groupbox_processlist->set_font(bold_font);
		this->m_groupbox_dlllist->set_font(bold_font);

		this->m_checklistview_processlist = new checklistview(checklistview_processlist, r.left + 10, r.top + 20, r.right - 20, r.bottom / 3 * 2 - 30, wnd, std::vector<std::string>({ "" , "Process Name", "Process Id", "Parent Process Id", "Thread Counts", "Priority" }), true, true);
		
		this->processlist_imagelist = ImageList_Create(16, 16, ILC_COLOR32, 0, 256);
		ListView_SetImageList(m_checklistview_processlist->get_handle(), this->processlist_imagelist, LVSIL_SMALL);
		this->m_checklistview_processlist->send_message(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_OWNERDATA | LVS_EX_SUBITEMIMAGES);

		this->m_checklistview_dlllist = new checklistview(checklistview_dlllist, r.left + 10, r.bottom / 3 * 2 + 20, r.right - 20, r.bottom - (r.bottom / 3 * 2 + 55), wnd, std::vector<std::string>({ "" , "Library Path" }), true);
		this->m_checklistview_dlllist->send_message(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
		
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 0, 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20 - 1);


		this->m_button_inject = new button(button_inject, r.right - 155, r.bottom - 25, 150, 20, "Inject", wnd);

	
	}, 900, 500, CW_USEDEFAULT, CW_USEDEFAULT, false, GetSysColorBrush(15), CS_HREDRAW | CS_VREDRAW, MAKEINTRESOURCE(IDI_ICON1));

	this->w->set_style_extra(WS_EX_ACCEPTFILES);

	[](HWND wnd)
	{
		if (IsWindowsVistaOrGreater())
		{
			#define WM_COPYGLOBALDATA 0x0049

			ChangeWindowMessageFilterEx(wnd, WM_DROPFILES, MSGFLT_ALLOW, NULL);
			ChangeWindowMessageFilterEx(wnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
			ChangeWindowMessageFilterEx(wnd, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
		}
	}(w->get_handle());

	this->w->add_wndproc_listener(WM_CLOSE, static_cast<window::lresult_window_event_t>([&](hwnd, wparam, lparam) -> lresult
	{
		this->hide();
		this->mw->set_dlllist(this->get_dlllist());
		this->mw->show();
		
		return 1;
	}));

	this->w->add_wndproc_listener(WM_SIZE, [&](hwnd wnd, wparam, lparam) -> void
	{
		rectangle r = window::get_window(wnd)->get_client_rectangle();

		this->m_groupbox_processlist->set_window_position(r.left + 5, r.top + 5, r.right - 10, r.bottom / 3 * 2 - 5);
		this->m_groupbox_dlllist->set_window_position(r.left + 5, r.bottom / 3 * 2 + 5, r.right - 10, r.bottom - (r.bottom / 3 * 2 + 35));

		this->m_checklistview_processlist->set_window_position(r.left + 10, r.top + 20, r.right - 20, r.bottom / 3 * 2 - 30);
		this->m_checklistview_dlllist->set_window_position(r.left + 10, r.bottom / 3 * 2 + 20, r.right - 20, r.bottom - (r.bottom / 3 * 2 + 55));

		this->m_button_inject->set_window_position(r.right - 155, r.bottom - 25, 150, 20);

	});

	this->w->add_wndproc_listener(WM_NOTIFY, static_cast<window::window_event_t>([this](hwnd wnd, wparam, lparam l)
	{
		if ((reinterpret_cast<LPNMHDR>(l))->code == NM_RCLICK &&
			(reinterpret_cast<LPNMHDR>(l))->hwndFrom == m_checklistview_dlllist->get_handle())
		{
			popupmenu pm;
			pm.append_menu(menu_librarylist_add, "Add Library");
			pm.append_separator();
			pm.append_menu(menu_librarylist_remove, "Remove Library");
			pm.append_menu(menu_librarylist_removeselected, "Remove Selected Libraries");
			pm.append_separator();
			pm.append_menu(menu_librarylist_opencontainingfolder, "Open Containing Folder");
			pm.append_separator();
			pm.append_menu(menu_librarylist_clear, "Clear Libraries");

			point p;

			GetCursorPos(&p);
			TrackPopupMenu(pm.get_handle(), TPM_LEFTALIGN, p.x, p.y, 0, wnd, NULL);
		}
		else if ((reinterpret_cast<LPNMHDR>(l))->code == NM_RCLICK &&
			(reinterpret_cast<LPNMHDR>(l))->hwndFrom == m_checklistview_processlist->get_handle())
		{
			popupmenu pm;
			pm.append_menu(menu_processlist_refresh, "Refresh");
			pm.append_separator();
			pm.append_menu(menu_processlist_suspend, "Suspend");
			pm.append_menu(menu_processlist_resume, "Resume");
			pm.append_separator();
			pm.append_menu(menu_processlist_terminate, "Terminate");

			point p;

			GetCursorPos(&p);
			TrackPopupMenu(pm.get_handle(), TPM_LEFTALIGN, p.x, p.y, 0, wnd, NULL);
		}
	}));

	this->w->add_wndproc_listener(WM_DROPFILES, [this](hwnd, wparam w, lparam) -> void
	{
		HDROP hdrop = reinterpret_cast<HDROP>(w);

		bool b = false;

		std::unique_ptr<char[]> path = std::make_unique<char[]>(MAX_PATH);
		for (int32_t i = 0; DragQueryFile(hdrop, i, path.get(), MAX_PATH); ++i)
		{
			if (!(GetFileAttributes(path.get()) & FILE_ATTRIBUTE_DIRECTORY) &&
				!lstrcmpi(PathFindExtension(path.get()), ".dll"))
			{
				if (!m_checklistview_dlllist->contains(1, std::string(path.get())))
				{
					m_checklistview_dlllist->insert_item(std::vector<std::string>({ path.get() }));
					b = true;
				}
			}
		}

		DragFinish(hdrop);

	});

	this->set_message_handler();
	this->refresh_processlist();
}

advancedinjectorwindow::~advancedinjectorwindow()
{
	if (m_button_inject) delete m_button_inject;
	if (m_checklistview_processlist) delete m_checklistview_processlist;
	if (m_checklistview_dlllist) delete m_checklistview_dlllist;
	if (m_groupbox_processlist) delete m_groupbox_processlist;
	if (m_groupbox_dlllist) delete m_groupbox_dlllist;

	if (processlist_imagelist) ImageList_Destroy(processlist_imagelist);
}

void advancedinjectorwindow::show()
{
	this->w->set_visible(true);
}

void advancedinjectorwindow::hide()
{
	this->w->set_visible(false);
}

void advancedinjectorwindow::refresh_processlist()
{
	this->w->send_message(WM_COMMAND, menu_processlist_refresh);
}

bool advancedinjectorwindow::set_dlllist(const std::unordered_map<std::string, bool>& dlllist)
{
	this->m_checklistview_dlllist->clear();
	for (const std::pair<std::string, bool> &dll : dlllist)
	{
		this->m_checklistview_dlllist->insert_item(std::vector<std::string>({ std::string(dll.first.begin(), dll.first.end()) }));
		this->m_checklistview_dlllist->set_check_state(this->m_checklistview_dlllist->size() - 1, dll.second);
	}

	return true;
}

const std::unordered_map<std::string, bool> advancedinjectorwindow::get_dlllist()
{
	std::unordered_map<std::string, bool> dll_list;
	std::vector<std::string> dll_name = this->m_checklistview_dlllist->column_text(1);
	
	/*for (size_t n = 0; n < dll_name.size(); ++n)
	{
		dll_list[std::string(dll_name.at(n).begin(), dll_name.at(n).end())] = this->m_checklistview_dlllist->get_check_state(n);
	}
	*/

	for (std::vector<std::string>::iterator it = dll_name.begin(); it != dll_name.end(); ++it)
	{
		dll_list[std::string((*it).begin(), (*it).end())] = this->m_checklistview_dlllist->get_check_state(it - dll_name.begin());
	}

	return dll_list;
}
