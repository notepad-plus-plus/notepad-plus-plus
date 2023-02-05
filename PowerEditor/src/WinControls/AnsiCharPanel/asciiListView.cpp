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


#include "asciiListView.h"
#include "Parameters.h"

void AsciiListView::resetValues(int codepage)
{
	if (codepage == -1)
		codepage = 0;

	if (_codepage == codepage)
		return;

	ListView_DeleteAllItems(_hSelf);
	setValues(codepage);
}

generic_string AsciiListView::getAscii(unsigned char value)
{
	switch (value)
	{
		case 0:
			return TEXT("NULL");
		case 1:
			return TEXT("SOH");
		case 2:
			return TEXT("STX");
		case 3:
			return TEXT("ETX");
		case 4:
			return TEXT("EOT");
		case 5:
			return TEXT("ENQ");
		case 6:
			return TEXT("ACK");
		case 7:
			return TEXT("BEL");
		case 8:
			return TEXT("BS");
		case 9:
			return TEXT("TAB");
		case 10:
			return TEXT("LF");
		case 11:
			return TEXT("VT");
		case 12:
			return TEXT("FF");
		case 13:
			return TEXT("CR");
		case 14:
			return TEXT("SO");
		case 15:
			return TEXT("SI");
		case 16:
			return TEXT("DLE");
		case 17:
			return TEXT("DC1");
		case 18:
			return TEXT("DC2");
		case 19:
			return TEXT("DC3");
		case 20:
			return TEXT("DC4");
		case 21:
			return TEXT("NAK");
		case 22:
			return TEXT("SYN");
		case 23:
			return TEXT("ETB");
		case 24:
			return TEXT("CAN");
		case 25:
			return TEXT("EM");
		case 26:
			return TEXT("SUB");
		case 27:
			return TEXT("ESC");
		case 28:
			return TEXT("FS");
		case 29:
			return TEXT("GS");
		case 30:
			return TEXT("RS");
		case 31:
			return TEXT("US");
		case 32:
			return TEXT("Space");
		case 127:
			return TEXT("DEL");
		default:
		{
			wchar_t charStr[10]{};
			char ascii[2]{};
			ascii[0] = value;
			ascii[1] = '\0';
			MultiByteToWideChar(_codepage, 0, ascii, -1, charStr, _countof(charStr));
			return charStr;
		}

	}
}

generic_string AsciiListView::getHtmlName(unsigned char value)
{
	switch (value)
	{
		case 33:
			return TEXT("&excl;");
		case 34:
			return TEXT("&quot;");
		case 35:
			return TEXT("&num;");
		case 36:
			return TEXT("&dollar;");
		case 37:
			return TEXT("&percnt;");
		case 38:
			return TEXT("&amp;");
		case 39:
			return TEXT("&apos;");
		case 40:
			return TEXT("&lpar;");
		case 41:
			return TEXT("&rpar;");
		case 42:
			return TEXT("&ast;");
		case 43:
			return TEXT("&plus;");
		case 44:
			return TEXT("&comma;");
		case 45:
			return TEXT("&minus;");
		case 46:
			return TEXT("&period;");
		case 47:
			return TEXT("&sol;");
		case 58:
			return TEXT("&colon;");
		case 59:
			return TEXT("&semi;");
		case 60:
			return TEXT("&lt;");
		case 61:
			return TEXT("&equals;");
		case 62:
			return TEXT("&gt;");
		case 63:
			return TEXT("&quest;");
		case 64:
			return TEXT("&commat;");
		case 91:
			return TEXT("&lbrack;");
		case 92:
			return TEXT("&bsol;");
		case 93:
			return TEXT("&rbrack;");
		case 94:
			return TEXT("&Hat;");
		case 95:
			return TEXT("&lowbar;");
		case 96:
			return TEXT("&grave;");
		case 123:
			return TEXT("&lbrace;");
		case 124:
			return TEXT("&vert;");
		case 125:
			return TEXT("&rbrace;");
		case 126:
			return TEXT(""); // ascii tilde
		case 128:
			return TEXT("&euro;");
		case 130:
			return TEXT("&sbquo;");
		case 131:
			return TEXT("&fnof;");
		case 132:
			return TEXT("&bdquo;");
		case 133:
			return TEXT("&hellip;");
		case 134:
			return TEXT("&dagger;");
		case 135:
			return TEXT("&Dagger;");
		case 136:
			return TEXT("&circ;");
		case 137:
			return TEXT("&permil;");
		case 138:
			return TEXT("&Scaron;");
		case 139:
			return TEXT("&lsaquo;");
		case 140:
			return TEXT("&OElig;");
		case 142:
			return TEXT("&Zcaron;");
		case 145:
			return TEXT("&lsquo;");
		case 146:
			return TEXT("&rsquo;");
		case 147:
			return TEXT("&ldquo;");
		case 148:
			return TEXT("&rdquo;");
		case 149:
			return TEXT("&bull;");
		case 150:
			return TEXT("&ndash;");
		case 151:
			return TEXT("&mdash;");
		case 152:
			return TEXT("&tilde;");
		case 153:
			return TEXT("&trade;");
		case 154:
			return TEXT("&scaron;");
		case 155:
			return TEXT("&rsaquo;");
		case 156:
			return TEXT("&oelig;");
		case 158:
			return TEXT("&zcaron;");
		case 159:
			return TEXT("&Yuml;");
		case 160:
			return TEXT("&nbsp;");
		case 161:
			return TEXT("&iexcl;");
		case 162:
			return TEXT("&cent;");
		case 163:
			return TEXT("&pound;");
		case 164:
			return TEXT("&curren;");
		case 165:
			return TEXT("&yen;");
		case 166:
			return TEXT("&brvbar;");
		case 167:
			return TEXT("&sect;");
		case 168:
			return TEXT("&uml;");
		case 169:
			return TEXT("&copy;");
		case 170:
			return TEXT("&ordf;");
		case 171:
			return TEXT("&laquo;");
		case 172:
			return TEXT("&not;");
		case 173:
			return TEXT("&shy;");
		case 174:
			return TEXT("&reg;");
		case 175:
			return TEXT("&macr;");
		case 176:
			return TEXT("&deg;");
		case 177:
			return TEXT("&plusmn;");
		case 178:
			return TEXT("&sup2;");
		case 179:
			return TEXT("&sup3;");
		case 180:
			return TEXT("&acute;");
		case 181:
			return TEXT("&micro;");
		case 182:
			return TEXT("&para;");
		case 183:
			return TEXT("&middot;");
		case 184:
			return TEXT("&cedil;");
		case 185:
			return TEXT("&sup1;");
		case 186:
			return TEXT("&ordm;");
		case 187:
			return TEXT("&raquo;");
		case 188:
			return TEXT("&frac14;");
		case 189:
			return TEXT("&frac12;");
		case 190:
			return TEXT("&frac34;");
		case 191:
			return TEXT("&iquest;");
		case 192:
			return TEXT("&Agrave;");
		case 193:
			return TEXT("&Aacute;");
		case 194:
			return TEXT("&Acirc;");
		case 195:
			return TEXT("&Atilde;");
		case 196:
			return TEXT("&Auml;");
		case 197:
			return TEXT("&Aring;");
		case 198:
			return TEXT("&AElig;");
		case 199:
			return TEXT("&Ccedil;");
		case 200:
			return TEXT("&Egrave;");
		case 201:
			return TEXT("&Eacute;");
		case 202:
			return TEXT("&Ecirc;");
		case 203:
			return TEXT("&Euml;");
		case 204:
			return TEXT("&Igrave;");
		case 205:
			return TEXT("&Iacute;");
		case 206:
			return TEXT("&Icirc;");
		case 207:
			return TEXT("&Iuml;");
		case 208:
			return TEXT("&ETH;");
		case 209:
			return TEXT("&Ntilde;");
		case 210:
			return TEXT("&Ograve;");
		case 211:
			return TEXT("&Oacute;");
		case 212:
			return TEXT("&Ocirc;");
		case 213:
			return TEXT("&Otilde;");
		case 214:
			return TEXT("&Ouml;");
		case 215:
			return TEXT("&times;");
		case 216:
			return TEXT("&Oslash;");
		case 217:
			return TEXT("&Ugrave;");
		case 218:
			return TEXT("&Uacute;");
		case 219:
			return TEXT("&Ucirc;");
		case 220:
			return TEXT("&Uuml;");
		case 221:
			return TEXT("&Yacute;");
		case 222:
			return TEXT("&THORN;");
		case 223:
			return TEXT("&szlig;");
		case 224:
			return TEXT("&agrave;");
		case 225:
			return TEXT("&aacute;");
		case 226:
			return TEXT("&acirc;");
		case 227:
			return TEXT("&atilde;");
		case 228:
			return TEXT("&auml;");
		case 229:
			return TEXT("&aring;");
		case 230:
			return TEXT("&aelig;");
		case 231:
			return TEXT("&ccedil;");
		case 232:
			return TEXT("&egrave;");
		case 233:
			return TEXT("&eacute;");
		case 234:
			return TEXT("&ecirc;");
		case 235:
			return TEXT("&euml;");
		case 236:
			return TEXT("&igrave;");
		case 237:
			return TEXT("&iacute;");
		case 238:
			return TEXT("&icirc;");
		case 239:
			return TEXT("&iuml;");
		case 240:
			return TEXT("&eth;");
		case 241:
			return TEXT("&ntilde;");
		case 242:
			return TEXT("&ograve;");
		case 243:
			return TEXT("&oacute;");
		case 244:
			return TEXT("&ocirc;");
		case 245:
			return TEXT("&otilde;");
		case 246:
			return TEXT("&ouml;");
		case 247:
			return TEXT("&divide;");
		case 248:
			return TEXT("&oslash;");
		case 249:
			return TEXT("&ugrave;");
		case 250:
			return TEXT("&uacute;");
		case 251:
			return TEXT("&ucirc;");
		case 252:
			return TEXT("&uuml;");
		case 253:
			return TEXT("&yacute;");
		case 254:
			return TEXT("&thorn;");
		case 255:
			return TEXT("&yuml;");
		default:
		{
			return TEXT("");
		}
	}
}

int AsciiListView::getHtmlNumber(unsigned char value)
{
	switch (value)
	{
		case 45:
			return 8722;
		case 128:
			return 8364;
		case 130:
			return 8218;
		case 131:
			return 402;
		case 132:
			return 8222;
		case 133:
			return 8230;
		case 134:
			return 8224;
		case 135:
			return 8225;
		case 136:
			return 710;
		case 137:
			return 8240;
		case 138:
			return 352;
		case 139:
			return 8249;
		case 140:
			return 338;
		case 142:
			return 381;
		case 145:
			return 8216;
		case 146:
			return 8217;
		case 147:
			return 8220;
		case 148:
			return 8221;
		case 149:
			return 8226;
		case 150:
			return 8211;
		case 151:
			return 8212;
		case 152:
			return 732;
		case 153:
			return 8482;
		case 154:
			return 353;
		case 155:
			return 8250;
		case 156:
			return 339;
		case 158:
			return 382;
		case 159:
			return 376;
		default:
		{
			return -1;
		}
	}
}

void AsciiListView::setValues(int codepage)
{
	_codepage = codepage;

	for (int i = 0 ; i < 256 ; ++i)
	{
		constexpr size_t bufSize = 8;
		constexpr size_t bufSizeHex = 9;
		wchar_t dec[bufSize]{};
		wchar_t hex[bufSize]{};
		wchar_t htmlNumber[bufSize]{};
		wchar_t htmlHexNumber[bufSizeHex]{};
		generic_string htmlName;
		swprintf(dec, bufSize, TEXT("%d"), i);
		swprintf(hex, bufSize, TEXT("%02X"), i);
		generic_string s = getAscii(static_cast<unsigned char>(i));

		if (codepage == 0 || codepage == 1252)
		{
			if ((i >= 32 && i <= 126 && i != 45) || (i >= 160 && i <= 255))
			{
				swprintf(htmlNumber, bufSize, TEXT("&#%d;"), i);
				swprintf(htmlHexNumber, bufSize, TEXT("&#x%x;"), i);
			}
			else
			{
				int n = getHtmlNumber(static_cast<unsigned char>(i));
				if (n > -1)
				{
					swprintf(htmlNumber, bufSize, TEXT("&#%d;"), n);
					swprintf(htmlHexNumber, bufSizeHex, TEXT("&#x%x;"), n);
				}
				else
				{
					swprintf(htmlNumber, bufSize, TEXT(""));
					swprintf(htmlHexNumber, bufSizeHex, TEXT(""));
				}
			}

			htmlName = getHtmlName(static_cast<unsigned char>(i));
		}
		else
		{
			swprintf(htmlNumber, bufSize, TEXT(""));
			swprintf(htmlHexNumber, bufSizeHex, TEXT(""));
			htmlName = TEXT("");
		}

		std::vector<generic_string> values2Add;

		values2Add.push_back(dec);
		values2Add.push_back(hex);
		values2Add.push_back(s);
		values2Add.push_back(htmlNumber);
		values2Add.push_back(htmlName);
		values2Add.push_back(htmlHexNumber);

		addLine(values2Add);
	}
}
