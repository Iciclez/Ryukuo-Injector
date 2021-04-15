#pragma once
#include <cstdint>
#include <string>

#include "window.hpp"
#include "numericupdown.hpp"
#include "button.hpp"
#include "checklistview.hpp"
#include "groupbox.hpp"

#include "inject.hpp"

class mainwindow;

typedef HINSTANCE hinstance;
typedef HANDLE handle;

class advancedinjectorwindow
{
private:
	std::string windowname();

	void set_message_handler();

	const std::vector<std::string> getdlllist() const;

public:
	enum id : int32_t
	{
		menu_librarylist_add = 1,
		menu_librarylist_remove,
		menu_librarylist_removeselected,
		menu_librarylist_opencontainingfolder,
		menu_librarylist_clear,

		menu_processlist_refresh,
		menu_processlist_suspend,
		menu_processlist_resume,
		menu_processlist_terminate,

		button_inject,
		checklistview_processlist,
		checklistview_dlllist,
		groupbox_processlist,
		groupbox_dlllist,	

	};

private:
	hinstance inst;
	window *w;

	button *m_button_inject;
	checklistview *m_checklistview_processlist;
	checklistview *m_checklistview_dlllist;
	groupbox *m_groupbox_processlist;
	groupbox *m_groupbox_dlllist;

	
private:
	mainwindow *mw;
	HIMAGELIST processlist_imagelist;

	std::function<void(inject::injection_error)> injection_error_handler;
public:
	advancedinjectorwindow(mainwindow *mw, hinstance inst);
	~advancedinjectorwindow();

	void show();
	void hide();

	void refresh_processlist();
	
	bool set_dlllist(const std::unordered_map<std::string, bool> &dlllist);
	const std::unordered_map<std::string, bool> get_dlllist();
};

