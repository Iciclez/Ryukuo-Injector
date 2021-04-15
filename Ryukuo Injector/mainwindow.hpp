#pragma once
#include <windows.h>
#include <memory>
#include <thread>

#include "window.hpp"
#include "groupbox.hpp"
#include "checklistview.hpp"
#include "label.hpp"
#include "combobox.hpp"
#include "textbox.hpp"
#include "checkbox.hpp"

#include "profile.hpp"

#include "advancedinjectorwindow.hpp"

typedef HINSTANCE hinstance;
typedef HANDLE handle;

class mainwindow
{
private:
	std::string windowname();

	void set_message_handler();
public:
	enum id : int32_t
	{
		menu_librarylist_add = 1,
		menu_librarylist_remove,
		menu_librarylist_removeselected,
		menu_librarylist_opencontainingfolder,
		menu_librarylist_clear,
		menu_advancedinjector,
		menu_exit,
		menu_taskmanager,
		menu_about,

		groupbox_dlllist,
		checklistview_dlllist,

		groupbox_processselection,
		label_informationclassification,
		combobox_informationclassification,
		label_information,
		combobox_processenum,
		button_refresh,

		textbox_windowname,
		textbox_classname,
		combobox_processid,

		groupbox_processwatcher,
		combobox_autoinjection,
		textbox_autoinjection,
		checkbox_autoinjection,

		groupbox_additional,
		checkbox_freeze,
		checkbox_cloakdll,

		button_inject,
		
		groupbox_inner_additional,
		combobox_injectionroutine,
		combobox_injectionthread

	};
private:
	hinstance inst;
	window *w;

	groupbox *m_groupbox_dlllist;
	checklistview *m_checklistview_dlllist;

	groupbox *m_groupbox_processselection;
	label *m_label_informationclassification;
	combobox *m_combobox_informationclassification;
	label *m_label_information;
	combobox *m_combobox_processenum;
	button *m_button_refresh;

	textbox *m_textbox_windowname;
	textbox *m_textbox_classname;
	
	combobox *m_combobox_processid;

	groupbox *m_groupbox_processwatcher;
	combobox *m_combobox_autoinjection;
	textbox *m_textbox_autoinjection;
	checkbox *m_checkbox_autoinjection;

	groupbox *m_groupbox_additional;
	checkbox *m_checkbox_freeze;
	checkbox *m_checkbox_cloakdll;

	button *m_button_inject;

	groupbox *m_groupbox_inner_additional;
	combobox *m_combobox_injectionroutine;
	combobox *m_combobox_injectionthread;

private:
	std::vector<std::string> getdlllist();
	
private:
	profile mainwindowprofile;
	advancedinjectorwindow *advancedinjector;
	std::function<void(inject::injection_error)> injection_error_handler;

public:
	mainwindow(hinstance inst);
	~mainwindow();

	int32_t message_loop();

	window *get_window() const;

	void update();

	void save();
	void load();

	void show();
	void hide();

	bool set_dlllist(const std::unordered_map<std::string, bool> &dlllist);
	const std::unordered_map<std::string, bool> get_dlllist();
};

