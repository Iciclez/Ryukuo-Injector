#include "mainwindow.hpp"
#include "menu.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <memory>
#include <thread>

#include "inject.hpp"
#include "resource.h"

#include <TlHelp32.h>
#include <Shlobj.h>
#include <Shlwapi.h>

std::string mainwindow::windowname()
{
	std::stringstream ss;
	ss << "Ryukuo Injector | Process Id: "
		<< std::hex << std::uppercase << std::setw(8) << std::setfill('0') << GetCurrentProcessId()
		<< " (" << std::dec << GetCurrentProcessId() << ")";

	return ss.str();
}

void mainwindow::set_message_handler()
{
	this->w->add_message_handler(std::make_pair(button_inject, [this](hwnd, wparam, lparam) -> lresult 
	{
		std::unique_ptr<inject> injector;
		switch (this->m_combobox_informationclassification->index())
		{
		case 0:
		{
			//inject into all process with that process name
			injector = std::make_unique<inject>(this->m_combobox_processenum->get_text(),
				static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1), 
				static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
				this->m_checkbox_freeze->is_checked(), 
				this->injection_error_handler);

			break;
		}
		case 1:
		{
			injector = std::make_unique<inject>(FindWindow(0, this->m_textbox_windowname->get_text().c_str()),
				static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1),
				static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
				this->m_checkbox_freeze->is_checked(), 
				this->injection_error_handler);

			break;
		}
		case 2:
		{
			injector = std::make_unique<inject>(FindWindow(this->m_textbox_classname->get_text().c_str(), 0),
				static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1),
				static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
				this->m_checkbox_freeze->is_checked(), 
				this->injection_error_handler);

			break;
		}
		case 3:
		{
			std::string process_id = this->m_combobox_processid->get_text();
			size_t process_id_begin = process_id.find_last_of("(") + 1;
			process_id = process_id.substr(process_id_begin, process_id.find_last_of(")") - process_id_begin);

			if (!process_id.empty())
			{
				injector = std::make_unique<inject>(static_cast<dword>(std::stoi(process_id)),
					static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1),
					static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
					this->m_checkbox_freeze->is_checked(), 
					this->injection_error_handler);
			}

			break;
		}
		}

		if (injector.get())
		{
			if (this->m_checkbox_mapdll->is_checked())
			{
				injector->map_dll(this->getdlllist());
			}
			else
			{
				injector->inject_dll(this->getdlllist());
			}
		}

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_librarylist_add, [this](hwnd wnd, wparam, lparam) -> lresult
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
				this->save();
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

	this->w->add_message_handler(std::make_pair(menu_advancedinjector, [this](hwnd, wparam, lparam) -> lresult
	{
		if (this->advancedinjector)
		{
			//this->advancedinjector->refresh_processlist();
			this->advancedinjector->show();
			
		}
		else
		{
			this->advancedinjector = new advancedinjectorwindow(this, this->w->get_instance());
		}

		this->advancedinjector->set_dlllist(this->get_dlllist());
		this->hide();
		
	
		//TODO add application icon

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_exit, [this](hwnd, wparam, lparam) -> lresult
	{
		this->save();

		PostQuitMessage(0);
		return 0;
	}));

	this->w->add_message_handler(std::make_pair(menu_taskmanager, [this](hwnd, wparam, lparam) -> lresult
	{
		std::unique_ptr<char[]> systemdirectory = std::make_unique<char[]>(MAX_PATH);
		std::unique_ptr<char[]> taskmanager = std::make_unique<char[]>(MAX_PATH);
		if (SHGetSpecialFolderPath(0, systemdirectory.get(), CSIDL_SYSTEM, FALSE) && PathFileExists(PathCombine(taskmanager.get(), systemdirectory.get(), "taskmgr.exe")))
		{
			ShellExecute(NULL, "OPEN", taskmanager.get(), NULL, NULL, SW_SHOWNORMAL);
		}

		return 0;
	}));
	
	this->w->add_message_handler(std::make_pair(menu_about, [this](hwnd wnd, wparam, lparam) -> lresult
	{
		MessageBox(wnd, "Ryukuo Injector			2.0.0.0\n\nCreated by Iciclez", "About Ryukuo Injector", MB_OK | MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND);
		return 0;
	}));

	this->w->add_message_handler(std::make_pair(button_refresh, [this](hwnd, wparam, lparam) -> lresult
	{
		m_combobox_processenum->clear();
		m_combobox_processid->clear();

		switch (this->m_combobox_informationclassification->index())
		{
		case 0:
		{
			handle h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 p = { 0 };

			p.dwSize = sizeof(PROCESSENTRY32);

			if (Process32First(h, &p))
			{
				while (Process32Next(h, &p))
				{
					if (!m_combobox_processenum->find_string_exact(p.szExeFile))
					{
						m_combobox_processenum->add_string(p.szExeFile);
					}
				}
			}

			if (h)
			{
				CloseHandle(h);
			}
			break;
		}
		case 3:
		{
			handle h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			PROCESSENTRY32 p = { 0 };

			p.dwSize = sizeof(PROCESSENTRY32);

			std::stringstream app_name;

			if (Process32First(h, &p))
			{
				while (Process32Next(h, &p))
				{
					app_name.str("");
					app_name 
						<< "[" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << p.th32ProcessID << "] "
						<< p.szExeFile
						<< " (" << std::dec << p.th32ProcessID << ")";

					if (!m_combobox_processid->find_string_exact(app_name.str()))
					{
						m_combobox_processid->add_string(app_name.str());
					}
				}
			}

			if (h)
			{
				CloseHandle(h);
			}
			break;
		}
		}

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(checkbox_mapdll, [this](hwnd, wparam, lparam) -> lresult
	{
		m_combobox_injectionroutine->set_enabled(!m_checkbox_mapdll->is_checked());

		return 0;
	}));

	this->w->add_message_handler(std::make_pair(checkbox_autoinjection, [this](hwnd, wparam, lparam) -> lresult
	{
		if (m_checkbox_autoinjection->is_checked())
		{
			m_button_inject->set_visible(false);
			m_textbox_autoinjection->set_enabled(false);
			m_combobox_autoinjection->set_enabled(false);
		
			std::thread([&]()
			{
				bool lock = false;
				hwnd process_window = 0;

				if (!this->m_combobox_autoinjection->index())
				{
					while (this->m_checkbox_autoinjection->is_checked())
					{
						if (!inject::get_process_id(this->m_textbox_autoinjection->get_text()).empty())
						{
							if (!lock)
							{
								inject injector(this->m_textbox_autoinjection->get_text(),
									static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1),
									static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
									this->m_checkbox_freeze->is_checked(),
									this->injection_error_handler);

								if (this->m_checkbox_mapdll->is_checked())
								{
									injector.map_dll(this->getdlllist());
								}
								else
								{
									injector.inject_dll(this->getdlllist());
								}

								lock = true;
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
							}
						}
						else
						{
							if (lock)
							{
								lock = false;
							}
						}
					}
				}
				else
				{
					while (this->m_checkbox_autoinjection->is_checked())
					{
						process_window = 0;
						switch (this->m_combobox_autoinjection->index())
						{
						case 1:
							process_window = FindWindow(0, this->m_textbox_autoinjection->get_text().c_str());
							break;

						case 2:
							process_window = FindWindow(this->m_textbox_autoinjection->get_text().c_str(), 0);
							break;
						}

						if (process_window)
						{
							if (!lock)
							{
								inject injector(process_window,
									static_cast<inject::injection_routine>(this->m_combobox_injectionroutine->index() + 1),
									static_cast<inject::injection_thread_function>(this->m_combobox_injectionthread->index() + 1),
									this->m_checkbox_freeze->is_checked(),
									this->injection_error_handler);

								if (this->m_checkbox_mapdll->is_checked())
								{
									injector.map_dll(this->getdlllist());
								}
								else
								{
									injector.inject_dll(this->getdlllist());
								}

								lock = true;
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
							}
						}
						else
						{
							if (lock)
							{
								lock = false;
							}
						}
					}
				}
				
			}).detach();
			
		}
		else
		{
			m_button_inject->set_visible();
			m_textbox_autoinjection->set_enabled();
			m_combobox_autoinjection->set_enabled();
		}
		return 0;
	}));

}

std::vector<std::string> mainwindow::getdlllist()
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

mainwindow::mainwindow(hinstance inst)
{
	this->inst = inst;

	this->w = new window(inst, this->windowname(), "Ryukuo Injector", static_cast<window::window_event_t>([this](hwnd wnd, wparam, lparam)
	{
		hfont bold_font = widget::make_font("Segoe UI", 14, 0, true);

		menu mainmenu;
		menu filemenu;
		menu toolsmenu;
		menu helpmenu;

		mainmenu.append_menu(filemenu, "File");
		mainmenu.append_menu(toolsmenu, "Tools");
		mainmenu.append_menu(helpmenu, "Help");

		filemenu.append_menu(menu_librarylist_add, "Add to Library List...");
		filemenu.append_menu(menu_advancedinjector, "Advanced Injector...");
		filemenu.append_separator();
		filemenu.append_menu(menu_exit, "Exit");
		toolsmenu.append_menu(menu_taskmanager, "Task Manager");
		helpmenu.append_menu(menu_about, "About Ryukuo Injector");

		window::get_window(wnd)->set_menu(mainmenu);

		rectangle r = window::get_window(wnd)->get_client_rectangle();

		m_groupbox_dlllist = new groupbox(groupbox_dlllist, 5, 5, r.right / 3 - 5, r.bottom - 10, const_cast<char*>("Dynamic Link Library List"), wnd);
		m_groupbox_dlllist->set_font(bold_font);

		m_checklistview_dlllist = new checklistview(checklistview_dlllist, 10, 20, r.right / 3 - 15, r.bottom - 30, wnd, std::vector<std::string>({ "", "Library Path" }), true);
		m_checklistview_dlllist->send_message(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 0, 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20 - 1);

		m_groupbox_processselection = new groupbox(groupbox_processselection, r.right / 3 + 5, 5, r.right - (r.right / 3 + 10), 75, const_cast<char*>("Process Selection"), wnd);
		m_label_informationclassification = new label(label_informationclassification, r.right / 3 + 10, 25, 135, 15, const_cast<char*>("Information Classification: "), wnd);
		m_combobox_informationclassification = new combobox(combobox_informationclassification, r.right / 3 + 150, 23, r.right - (r.right / 3 + 160), 15, wnd, true);
		m_label_information = new label(label_information, r.right / 3 + 10, 50, 135, 15, const_cast<char*>("Process Enumeration: "), wnd);
		m_combobox_processenum = new combobox(combobox_processenum, r.right / 3 + 150, 48, r.right - (r.right / 3 + 210), 15, wnd);
		m_button_refresh = new button(button_refresh, r.right - 60, 47, 50, 23, const_cast<char*>("Refresh"), wnd);

		m_combobox_processenum->set_cue_banner("Search for a target process to inject dlls into...");

		m_textbox_windowname = new textbox(textbox_windowname, r.right / 3 + 150, 48, r.right - (r.right / 3 + 160), 20, const_cast<char*>(""), wnd);
		//m_textbox_windowname->set_style(m_textbox_windowname->get_style() | ES_CENTER);
		m_textbox_windowname->set_cue_banner("Enter the name of a window to inject dlls into...");

		m_textbox_classname = new textbox(textbox_classname, r.right / 3 + 150, 48, r.right - (r.right / 3 + 160), 20, const_cast<char*>(""), wnd);
		//m_textbox_classname->set_style(m_textbox_classname->get_style() | ES_CENTER);
		m_textbox_classname->set_cue_banner("Enter a window class to inject dlls into...");

		m_combobox_processid = new combobox(combobox_processid, r.right / 3 + 150, 48, r.right - (r.right / 3 + 210), 15, wnd);
		m_combobox_processid->set_cue_banner("Search for a target process to inject dlls into...");

		m_textbox_windowname->set_visible(false);
		m_textbox_classname->set_visible(false);
		m_combobox_processid->set_visible(false);

		m_groupbox_processselection->set_font(bold_font);

		m_groupbox_processwatcher = new groupbox(groupbox_processwatcher, r.right / 3 + 5, 85, r.right - (r.right / 3 + 10), 65, const_cast<char*>("Process Watcher and Auto Injection"), wnd);
		m_groupbox_processwatcher->set_font(bold_font);

		m_combobox_autoinjection = new combobox(combobox_autoinjection, r.right / 3 + 10, 108, 135, 15, wnd, true);
		m_textbox_autoinjection = new textbox(textbox_autoinjection, r.right / 3 + 150, 108, r.right - (r.right / 3 + 160), 20, const_cast<char*>(""), wnd);
		m_checkbox_autoinjection = new checkbox(checkbox_autoinjection, r.right - 140, 128, 130, 20, const_cast<char*>("Enable Auto Injection"), wnd);
		m_textbox_autoinjection->set_style(m_textbox_autoinjection->get_style() | ES_CENTER);

		m_groupbox_additional = new groupbox(groupbox_additional, r.right / 3 + 5, 155, r.right - (r.right / 3 + 10), r.bottom - 155 - 30, const_cast<char*>("Additional Options"), wnd);
		m_groupbox_additional->set_font(bold_font);

		m_checkbox_freeze = new checkbox(checkbox_freeze, r.right / 3 + 10, 170, 180, 20, const_cast<char*>("Freeze Process During Injection"), wnd);
		m_checkbox_mapdll = new checkbox(checkbox_mapdll, r.right / 3 + 10, 190, 180, 20, const_cast<char*>("Map Dll"), wnd);

		m_button_inject = new button(button_inject, r.right - 155, r.bottom - 25, 150, 20, const_cast<char*>("Inject"), wnd);

		m_groupbox_inner_additional = new groupbox(groupbox_inner_additional, r.right - 160, 155, 155, 70, const_cast<char*>(""), wnd);
		m_combobox_injectionroutine = new combobox(combobox_injectionroutine, r.right - 155, 170, 145, 15, wnd, true);
		m_combobox_injectionthread = new combobox(combobox_injectionthread, r.right - 155, 195, 145, 15, wnd, true);

		m_combobox_informationclassification->add_string("Process Name");
		m_combobox_informationclassification->add_string("Window Name");
		m_combobox_informationclassification->add_string("Class Name");
		m_combobox_informationclassification->add_string("Process Id");
		m_combobox_informationclassification->set_index(0);

		m_combobox_autoinjection->add_string("Process Name:");
		m_combobox_autoinjection->add_string("Window Name:");
		m_combobox_autoinjection->add_string("Class Name:");
		m_combobox_autoinjection->set_index(0);

		m_combobox_injectionroutine->add_string("LoadLibraryA");
		m_combobox_injectionroutine->add_string("LoadLibraryW");
		m_combobox_injectionroutine->add_string("LoadLibraryExA");
		m_combobox_injectionroutine->add_string("LoadLibraryExW");
		m_combobox_injectionroutine->add_string("LdrLoadDll");
		m_combobox_injectionroutine->set_index(0);

		m_combobox_injectionthread->add_string("CreateRemoteThread");
		m_combobox_injectionthread->add_string("CreateRemoteThreadEx");
		m_combobox_injectionthread->add_string("NtCreateThreadEx");
		m_combobox_injectionthread->add_string("RtlCreateUserThread");
		m_combobox_injectionthread->set_index(0);

		mainwindowprofile.insert(this->m_combobox_informationclassification, "Information", profile::pcombobox_index);
		mainwindowprofile.insert(this->m_combobox_processenum, "ProcessName", profile::pcombobox_text);
		mainwindowprofile.insert(this->m_textbox_windowname, "WindowName", profile::ptextbox);
		mainwindowprofile.insert(this->m_textbox_classname, "ClassName", profile::ptextbox);
		mainwindowprofile.insert(this->m_combobox_autoinjection, "AutoInjectInformation", profile::pcombobox_index);
		mainwindowprofile.insert(this->m_textbox_autoinjection, "AutoInject", profile::ptextbox);
		mainwindowprofile.insert(this->m_checkbox_freeze, "Freeze", profile::pcheckbox);
		mainwindowprofile.insert(this->m_checkbox_mapdll, "MapDll", profile::pcheckbox);
		mainwindowprofile.insert(this->m_combobox_injectionroutine, "Routine", profile::pcombobox_index);
		mainwindowprofile.insert(this->m_combobox_injectionthread, "Thread", profile::pcombobox_index);
		mainwindowprofile.insert(this->m_checklistview_dlllist, "Dll", profile::pchecklistview);

	}), 750, 420, CW_USEDEFAULT, CW_USEDEFAULT, false, GetSysColorBrush(15), CS_HREDRAW | CS_VREDRAW, MAKEINTRESOURCE(IDI_ICON1));
	
	this->w->set_style_extra(WS_EX_ACCEPTFILES);

	this->w->add_wndproc_listener(WM_SIZE, static_cast<window::window_event_t>([this](hwnd wnd, wparam, lparam)
	{
		rectangle r = window::get_window(wnd)->get_client_rectangle();

		m_groupbox_dlllist->set_window_position(5, 5, r.right / 3 - 5, r.bottom - 10);
		m_checklistview_dlllist->set_window_position(10, 20, r.right / 3 - 15, r.bottom - 30);

		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 0, 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20);
		ListView_SetColumnWidth(m_checklistview_dlllist->get_handle(), 1, m_checklistview_dlllist->get_client_rectangle().right - 20 - 1);


		m_groupbox_processselection->set_window_position(r.right / 3 + 5, 5, r.right - (r.right / 3 + 10), 75);
		m_label_informationclassification->set_window_position(r.right / 3 + 10, 25, 135, 15);
		m_combobox_informationclassification->set_window_position(r.right / 3 + 150, 23, r.right - (r.right / 3 + 160), 15);
		m_label_information->set_window_position(r.right / 3 + 10, 50, 135, 15);
		m_combobox_processenum->set_window_position(r.right / 3 + 150, 48, r.right - (r.right / 3 + 210), 15);
		m_button_refresh->set_window_position(r.right - 60, 47, 50, 23);

		m_textbox_windowname->set_window_position(r.right / 3 + 150, 48, r.right - (r.right / 3 + 160), 20);
		m_textbox_classname->set_window_position(r.right / 3 + 150, 48, r.right - (r.right / 3 + 160), 20);
		m_combobox_processid->set_window_position(r.right / 3 + 150, 48, r.right - (r.right / 3 + 210), 15);

		m_groupbox_processwatcher->set_window_position(r.right / 3 + 5, 85, r.right - (r.right / 3 + 10), 65);
		m_combobox_autoinjection->set_window_position(r.right / 3 + 10, 108, 135, 15);
		m_textbox_autoinjection->set_window_position(r.right / 3 + 150, 108, r.right - (r.right / 3 + 160), 20);
		m_checkbox_autoinjection->set_window_position(r.right - 140, 128, 130, 20);

		m_groupbox_additional->set_window_position(r.right / 3 + 5, 155, r.right - (r.right / 3 + 10), r.bottom - 155 - 30);

		m_checkbox_freeze->set_window_position(r.right / 3 + 10, 170, 180, 20);
		m_checkbox_mapdll->set_window_position(r.right / 3 + 10, 190, 180, 20);

		m_button_inject->set_window_position(r.right - 155, r.bottom - 25, 150, 20);

		m_groupbox_inner_additional->set_window_position(r.right - 160, 155, 155, 70);
		m_combobox_injectionroutine->set_window_position(r.right - 155, 170, 145, 15);
		m_combobox_injectionthread->set_window_position(r.right - 155, 195, 145, 15);

	}));

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

	}));

	this->w->add_wndproc_listener(WM_COMMAND, [this](hwnd, wparam w, lparam) -> void
	{
		if (HIWORD(w) == CBN_SELCHANGE &&
			LOWORD(w) == combobox_informationclassification)
		{
			switch (this->m_combobox_informationclassification->index())
			{
			case 0:
				m_label_information->set_text("Process Enumeration: ");
				m_label_information->redraw();

				m_combobox_processenum->set_visible();
				m_button_refresh->set_visible();

				m_textbox_windowname->set_visible(false);
				m_textbox_classname->set_visible(false);
				m_combobox_processid->set_visible(false);

				m_button_refresh->perform_click();
				break;
			case 1:
				m_label_information->set_text("Window Name: ");
				m_label_information->redraw();

				m_combobox_processenum->set_visible(false);
				m_button_refresh->set_visible(false);


				m_textbox_windowname->set_visible();
				m_textbox_classname->set_visible(false);
				m_combobox_processid->set_visible(false);
				break;
			case 2:
				m_label_information->set_text("Class Name: ");
				m_label_information->redraw();

				m_combobox_processenum->set_visible(false);
				m_button_refresh->set_visible(false);

				m_textbox_windowname->set_visible(false);
				m_textbox_classname->set_visible(true);
				m_combobox_processid->set_visible(false);
				break;

			case 3:
				m_label_information->set_text("Process Id: ");
				m_label_information->redraw();

				m_combobox_processenum->set_visible(false);
				m_button_refresh->set_visible();

				m_textbox_windowname->set_visible(false);
				m_textbox_classname->set_visible(false);
				m_combobox_processid->set_visible();

				m_button_refresh->perform_click();
				break;
			}
		}


	});

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

		if (b) 
		{
			this->save();
		}

		DragFinish(hdrop);

	});

	this->w->add_wndproc_listener(WM_DESTROY, [this](hwnd, wparam, lparam) -> void
	{
		this->save();
		PostQuitMessage(0);
	});

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


	this->set_message_handler();

	this->m_button_refresh->perform_click();

	this->load();

}


mainwindow::~mainwindow()
{
	/*
	
		generated by the following python code:

		stuff = '''checklistview *m_checklistview_dlllist;

		groupbox *m_groupbox_processselection;
		label *m_label_informationclassification;
		combobox *m_combobox_informationclassification;
		label *m_label_information;
		combobox *m_combobox_processenum;
		button *m_button_refresh;

		textbox *m_textbox_windowname;
		textbox *m_textbox_classname;
		textbox *m_textbox_processid;
		checkbox *m_checkbox_hexadecimal;

		groupbox *m_groupbox_processwatcher;
		combobox *m_combobox_autoinjection;
		textbox *m_textbox_autoinjection;
		checkbox *m_checkbox_autoinjection;

		groupbox *m_groupbox_additional;
		checkbox *m_checkbox_freeze;
		checkbox *m_checkbox_mapdll;

		button *m_button_inject;

		groupbox *m_groupbox_inner_additional;
		combobox *m_combobox_injectionroutine;
		combobox *m_combobox_injectionthread;'''.split('\n')


		def get_string_in_between(start, end, string):
			return string[string.find(start) + len(start):string.rfind(end)]

		for line in stuff:
			if line != "":
				temp = get_string_in_between("*", ";", line)
				print("if (" + temp + ") delete " + temp + ";" )

	
	*/
	if (m_groupbox_dlllist) delete m_groupbox_dlllist;
	if (m_checklistview_dlllist) delete m_checklistview_dlllist;
	if (m_groupbox_processselection) delete m_groupbox_processselection;
	if (m_label_informationclassification) delete m_label_informationclassification;
	if (m_combobox_informationclassification) delete m_combobox_informationclassification;
	if (m_label_information) delete m_label_information;
	if (m_combobox_processenum) delete m_combobox_processenum;
	if (m_button_refresh) delete m_button_refresh;
	if (m_textbox_windowname) delete m_textbox_windowname;
	if (m_textbox_classname) delete m_textbox_classname;
	if (m_combobox_processid) delete m_combobox_processid;
	if (m_groupbox_processwatcher) delete m_groupbox_processwatcher;
	if (m_combobox_autoinjection) delete m_combobox_autoinjection;
	if (m_textbox_autoinjection) delete m_textbox_autoinjection;
	if (m_checkbox_autoinjection) delete m_checkbox_autoinjection;
	if (m_groupbox_additional) delete m_groupbox_additional;
	if (m_checkbox_freeze) delete m_checkbox_freeze;
	if (m_checkbox_mapdll) delete m_checkbox_mapdll;
	if (m_button_inject) delete m_button_inject;
	if (m_groupbox_inner_additional) delete m_groupbox_inner_additional;
	if (m_combobox_injectionroutine) delete m_combobox_injectionroutine;
	if (m_combobox_injectionthread) delete m_combobox_injectionthread;
}

int32_t mainwindow::message_loop()
{
	return w->handle_message();
}

window * mainwindow::get_window() const
{
	return this->w;
}

void mainwindow::update()
{
	this->w->send_message(WM_COMMAND, MAKEWPARAM(combobox_informationclassification, CBN_SELCHANGE));
	this->w->send_message(WM_COMMAND, checkbox_mapdll, 0);
}

void mainwindow::save()
{
	this->mainwindowprofile.save();
}

void mainwindow::load()
{
	this->mainwindowprofile.load();
	this->update();
}

void mainwindow::show()
{
	this->w->set_visible();
}

void mainwindow::hide()
{
	this->w->set_visible(false);
}

bool mainwindow::set_dlllist(const std::unordered_map<std::string, bool>& dlllist)
{
	this->m_checklistview_dlllist->clear();
	for (const std::pair<std::string, bool> &dll : dlllist)
	{
		this->m_checklistview_dlllist->insert_item(std::vector<std::string>({ std::string(dll.first.begin(), dll.first.end()) }));
		this->m_checklistview_dlllist->set_check_state(this->m_checklistview_dlllist->size() - 1, dll.second);
	}

	return true;
}

const std::unordered_map<std::string, bool> mainwindow::get_dlllist()
{
	std::unordered_map<std::string, bool> dll_list;
	std::vector<std::string> dll_name = this->m_checklistview_dlllist->column_text(1);

	for (size_t n = 0; n < dll_name.size(); ++n)
	{
		dll_list[std::string(dll_name.at(n).begin(), dll_name.at(n).end())] = this->m_checklistview_dlllist->get_check_state(n);
	}

	return dll_list;
}
