// This file is part of Notepad++ project
// Copyright (c) 2025 ozone10 and Notepad++ team

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

#include <tinyxml2.h>


// Simple wrapper for TinyXML2
namespace NppXml
{
	using NewDocument = tinyxml2::XMLDocument;
	using Document = tinyxml2::XMLDocument*;
	using Element = tinyxml2::XMLElement*;
	using Node = tinyxml2::XMLNode*;

	[[nodiscard]] inline bool loadFile(Document doc, const char* filename) {
		return doc->LoadFile(filename) == tinyxml2::XML_SUCCESS;
	}

	[[nodiscard]] inline bool saveFile(Document doc, const char* filename) {
		return doc->SaveFile(filename) == tinyxml2::XML_SUCCESS;
	}

	[[nodiscard]] inline Element firstChildElement(const Document& doc, const char* name = nullptr) {
		return doc->FirstChildElement(name);
	}

	[[nodiscard]] inline Element firstChildElement(const Node& node, const char* name = nullptr) {
		return node->FirstChildElement(name);
	}

	[[nodiscard]] inline Element toElement(const Node& node) {
		return node->ToElement();
	}

	[[nodiscard]] inline Element nextSiblingElement(const Node& node, const char* name = nullptr) {
		return node->NextSiblingElement(name);
	}

	[[nodiscard]] inline Node nextSibling(const Node& node) {
		return node->NextSibling();
	}

	[[nodiscard]] inline const char* attribute(const Element& elem, const char* name) {
		return elem->Attribute(name);
	}

	[[nodiscard]] inline int intAttribute(Element elem, const char* name, int defaultValue = 0) {
		return elem->IntAttribute(name, defaultValue);
	}
}
