// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

typedef std::vector<std::wstring> stringVector;

class FileNameStringSplitter
{
public:
	FileNameStringSplitter(const wchar_t *fileNameStr)
	{
		wchar_t *pStr = NULL;
		bool isInsideQuotes = false;
		const int filePathLength = MAX_PATH;

		wchar_t str[filePathLength];
		int i = 0;
        bool fini = false;

		for (pStr = (wchar_t *)fileNameStr ; !fini ; )
		{
			if (i >= filePathLength)
				break;

			switch (*pStr)
			{
				case '"':
				{
					if (isInsideQuotes)
					{
						str[i] = '\0';
                        if (str[0])
							_fileNames.push_back(std::wstring(str));
						i = 0;
					}
					isInsideQuotes = !isInsideQuotes;
					pStr++;
					break;
				}

				case ' ':
				{
					if (isInsideQuotes)
					{
						str[i] = *pStr;
						i++;
					}
					else
					{
						str[i] = '\0';
                        if (str[0])
							_fileNames.push_back(std::wstring(str));
						i = 0;
					}
                    pStr++;
					break;
				}

                case '\0':
				{
                    str[i] = *pStr;
                    if (str[0])
						_fileNames.push_back(std::wstring(str));
                    fini = true;
					break;
				}

				default :
				{
					str[i] = *pStr;
					i++; pStr++;
					break;
				}
			}
		}
	}

	const stringVector& getFileNames() const {
		return _fileNames;
	};

	const wchar_t * getFileName(size_t index) const {
		if (index >= _fileNames.size())
			return NULL;
		return _fileNames[index].c_str();
	}

	int size() const {
		return int(_fileNames.size());
	}

private :
	stringVector _fileNames;
};
