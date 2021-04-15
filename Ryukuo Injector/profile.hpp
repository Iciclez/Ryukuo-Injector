#pragma once
#include <unordered_map>

#include "widget.hpp"

class profile
{
public:
	enum widget_profile_type
	{
		pcheckbox = 1,
		ptextbox,
		pchecklistview,
		pcombobox_index,
		pcombobox_text,
	};
private:
	std::unordered_map<widget *, std::pair<std::string, widget_profile_type>> profilewidget;
	std::string profilepath;
public:
	profile();
	~profile();

	bool insert(widget * widget_pointer, const std::string &widget_name, widget_profile_type type);

	bool save();
	bool load();
};

