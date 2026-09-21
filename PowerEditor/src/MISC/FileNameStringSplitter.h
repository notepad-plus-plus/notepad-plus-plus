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
		std::wstring str;
        bool fini = false;

		for (pStr = (wchar_t *)fileNameStr ; !fini ; )
		{
			switch (*pStr)
			{
				case '"':
				{
					if (isInsideQuotes)
					{
						if (!str.empty())
							_fileNames.push_back(str);
						str.clear();
					}
					isInsideQuotes = !isInsideQuotes;
					pStr++;
					break;
				}

				case ' ':
				{
					if (isInsideQuotes)
					{
						str += *pStr;
					}
					else
					{
						if (!str.empty())
							_fileNames.push_back(str);
						str.clear();
					}
                    pStr++;
					break;
				}

                case '\0':
				{
					if (!str.empty())
						_fileNames.push_back(str);
                    fini = true;
					break;
				}

				default :
				{
					str += *pStr;
					pStr++;
					break;
				}
			}
		}
	}

	const stringVector& getFileNames() const {
		return _fileNames;
	}

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
