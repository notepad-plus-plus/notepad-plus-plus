/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <unordered_map>


class Strings
{
public:
	static Strings& get()
	{
		static Strings inst;
		return inst;
	};

	const std::string& currentLocale() const
	{
		return _currentLocale;
	};

	bool read(const std::string& localization);

	std::wstring operator[](const std::string& key) const;

private:
	static const char* c_localization_files_relative_path;

	Strings();

	Strings(const Strings&) = delete;
	Strings(Strings&&) = delete;

	Strings& operator=(const Strings&) = delete;

	bool readFromFile(const std::string& json_locale_file);

	std::string _currentLocale;
	std::unordered_map<std::string, std::string> _strings;
};
