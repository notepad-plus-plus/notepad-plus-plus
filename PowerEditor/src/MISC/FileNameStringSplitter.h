//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef FILENAME_STRING_SPLITTER_H
#define FILENAME_STRING_SPLITTER_H

typedef std::vector<std::generic_string> stringVector;

class FileNameStringSplitter
{
public :
	FileNameStringSplitter(const TCHAR *fileNameStr)  {
		//if (!fileNameStr) return;
		TCHAR *pStr = NULL;
		bool isInsideQuotes = false;
		TCHAR str[MAX_PATH];
		int i = 0;
        bool fini = false;
		for (pStr = (TCHAR *)fileNameStr ; !fini ; )
		{
			switch (*pStr)
			{
				case '"' :
					if (isInsideQuotes)
					{
						str[i] = '\0';
                        if (str[0])
							_fileNames.push_back(std::generic_string(str));
						i = 0;
					}
					isInsideQuotes = !isInsideQuotes;
					pStr++;
					break;
				
				case ' ' :
					if (isInsideQuotes)
					{
						str[i] = *pStr;
						i++; 
					}
					else
					{
						str[i] = '\0';
                        if (str[0])
							_fileNames.push_back(std::generic_string(str));
						i = 0;
					}
                    pStr++;
					break;
					
                case '\0' :
                    str[i] = *pStr;
                    if (str[0])
						_fileNames.push_back(std::generic_string(str));
                    fini = true;
					break;

				default :
					str[i] = *pStr;
					i++; pStr++;
					break;
			}
		}
	};
	
	const TCHAR * getFileName(int index) const {
		return _fileNames[index].c_str();
	};
	
	int size() const {
		return int(_fileNames.size());
	};
	
private :
	stringVector _fileNames;
};

#endif //FILENAME_STRING_SPLITTER_H
