#include "profile.hpp"
#include "json.hpp"

using namespace nlohmann;

#include "checkbox.hpp"
#include "textbox.hpp"
#include "checklistview.hpp"
#include "combobox.hpp"

#include <Shlwapi.h>
#include <Shlobj.h>

#include <fstream>
#include <sstream>

profile::profile()
{
	std::unique_ptr<char[]> path = std::make_unique<char[]>(MAX_PATH);
	
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path.get()) == S_OK)
	{
		if (PathAppend(path.get(), "Ryukuo Injector") != FALSE)
		{
			auto is_invalid_directory = [](char * w) { uint32_t attributes = GetFileAttributes(w); return (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)); };
			
			if (is_invalid_directory(path.get()))
			{
				CreateDirectory(path.get(), 0);
			}

			if (PathAppend(path.get(), "Ryukuo Injector.json") != FALSE)
			{
				this->profilepath = path.get();
			}	
		}		
	}
}


profile::~profile()
{
}

bool profile::insert(widget * widget_pointer, const std::string &widget_name, widget_profile_type type)
{
	profilewidget[widget_pointer] = std::make_pair(widget_name, type);
	return true;
}

bool profile::save()
{
	json j;
	for (const std::pair<widget*, std::pair<std::string, widget_profile_type>> &p : profilewidget)
	{
		switch (p.second.second)
		{
		case pcheckbox:
			j[p.second.first] = reinterpret_cast<checkbox*>(p.first)->is_checked();
			break;

		case ptextbox:
			j[p.second.first] = reinterpret_cast<textbox*>(p.first)->get_text();
			break;

		case pchecklistview:
			j[p.second.first] = [](checklistview *dlllistview) -> std::unordered_map<std::string, bool>
			{
				std::unordered_map<std::string, bool> dll_list;
				std::vector<std::string> dll_name = dlllistview->column_text(1);

				for (size_t n = 0; n < dll_name.size(); ++n)
				{
					dll_list[std::string(dll_name.at(n).begin(), dll_name.at(n).end())] = dlllistview->get_check_state(n);
				}
	
				return dll_list;

			}(reinterpret_cast<checklistview*>(p.first));
			break;

		case pcombobox_index:
			j[p.second.first] = reinterpret_cast<combobox*>(p.first)->index();
			break;

		case pcombobox_text:
			j[p.second.first] = reinterpret_cast<combobox*>(p.first)->get_text();
			break;
		}
		
	}

	std::ofstream fs(this->profilepath);
	if (fs.is_open())
	{
		fs << j.dump();
		fs.close();
	}

	return true;
}

bool profile::load()
{
	std::ifstream fs(this->profilepath);
	std::stringstream ss;
	ss << fs.rdbuf();

	if (ss.str().empty())
	{
		return true;
	}

	json j = json::parse(ss);
	for (const std::pair<widget*, std::pair<std::string, widget_profile_type>> &p : profilewidget)
	{
		switch (p.second.second)
		{
		case pcheckbox:
			reinterpret_cast<checkbox*>(p.first)->set_checked(j[p.second.first]);
			break;

		case ptextbox:
			reinterpret_cast<textbox*>(p.first)->set_text(j[p.second.first]);
			break;

		case pchecklistview:
			[](checklistview *dlllistview, const std::unordered_map<std::string, bool> &dll_list) -> void
			{
				
				for (const std::pair<std::string, bool> &dll : dll_list)
				{
					dlllistview->insert_item(std::vector<std::string>({std::string(dll.first.begin(), dll.first.end())}));
					dlllistview->set_check_state(dlllistview->size() - 1, dll.second);
				}


			}(reinterpret_cast<checklistview*>(p.first), j[p.second.first]);
			break;

		case pcombobox_index:
			reinterpret_cast<combobox*>(p.first)->set_index(j[p.second.first]);
			break;

		case pcombobox_text:
			reinterpret_cast<combobox*>(p.first)->set_text(j[p.second.first]);
			break;
		}
	}



	return true;
}
