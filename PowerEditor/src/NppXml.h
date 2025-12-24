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

#include <cstdio>
#include <cstdint>

// Simple wrapper for TinyXML2
namespace NppXml
{
	using NewDocument = tinyxml2::XMLDocument;
	using Document = tinyxml2::XMLDocument*;
	using Element = tinyxml2::XMLElement*;
	using Node = tinyxml2::XMLNode*;

	[[nodiscard]] inline bool loadFile(Document& doc, const wchar_t* filename) {
		FILE* file = nullptr;
		::_wfopen_s(&file, filename, L"rb");
		bool result = false;
		if (file != nullptr)
		{
			result = doc->LoadFile(file) == tinyxml2::XML_SUCCESS;
			std::fclose(file);
		}
		return result;
	}

	[[nodiscard]] inline bool saveFile(Document& doc, const wchar_t* filename) {
		FILE* file = nullptr;
		::_wfopen_s(&file, filename, L"w");
		bool result = false;
		if (file != nullptr)
		{
			result = doc->SaveFile(file) == tinyxml2::XML_SUCCESS;
			std::fclose(file);
		}
		return result;
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

	[[nodiscard]] inline Node firstChild(const Node& node) {
		return node->FirstChild();
	}

	[[nodiscard]] inline Node nextSibling(const Node& node) {
		return node->NextSibling();
	}

	[[nodiscard]] inline const char* value(const Node& node) {
		return node->Value();
	}

	[[nodiscard]] inline const char* attribute(const Node& node, const char* name) {
		return node->ToElement()->Attribute(name);
	}

	[[nodiscard]] inline const char* attribute(const Element& elem, const char* name) {
		return elem->Attribute(name);
	}

	[[nodiscard]] inline int intAttribute(const Node& node, const char* name, int defaultValue = 0) {
		return node->ToElement()->IntAttribute(name, defaultValue);
	}

	[[nodiscard]] inline int intAttribute(const Element& elem, const char* name, int defaultValue = 0) {
		return elem->IntAttribute(name, defaultValue);
	}

	[[nodiscard]] inline int64_t int64Attribute(const Node& node, const char* name, int64_t defaultValue = 0) {
		return node->ToElement()->Int64Attribute(name, defaultValue);
	}

	[[nodiscard]] inline int64_t int64Attribute(const Element& elem, const char* name, int64_t defaultValue = 0) {
		return elem->Int64Attribute(name, defaultValue);
	}

	[[nodiscard]] inline uint64_t uint64Attribute(const Node& node, const char* name, uint64_t defaultValue = 0) {
		return node->ToElement()->Unsigned64Attribute(name, defaultValue);
	}

	[[nodiscard]] inline uint64_t uint64Attribute(const Element& elem, const char* name, uint64_t defaultValue = 0) {
		return elem->Unsigned64Attribute(name, defaultValue);
	}

	inline void setAttribute(Element& elem, const char* name, const char* value) {
		elem->SetAttribute(name, value);
	}

	inline void setAttribute(Element& elem, const char* name, int value) {
		elem->SetAttribute(name, value);
	}

	inline void setInt64Attribute(Element& elem, const char* name, int64_t value) {
		elem->SetAttribute(name, value);
	}

	inline void setUInt64Attribute(Element& elem, const char* name, uint64_t value) {
		elem->SetAttribute(name, value);
	}

	inline Node createNewDeclaration(Document& doc) {
		return doc->LinkEndChild(doc->NewDeclaration(nullptr));
	}

	inline Element createChildElement(Document& doc, const char* name) {
		Element elem = doc->GetDocument()->NewElement(name);
		doc->InsertEndChild(elem);
		return elem;
	}

	inline Element createChildElement(Node parent, const char* name) {
		Element elem = parent->GetDocument()->NewElement(name);
		parent->InsertEndChild(elem);
		return elem;
	}

	inline Node createChildText(Node parent, const char* text) {
		Node node = parent->GetDocument()->NewText(text);
		parent->InsertEndChild(node);
		return node;
	}

	inline void deleteChild(Node& parent, Node child) {
		parent->DeleteChild(child);
	}
}
