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

#if defined(USE_TINYXML2)

#include <tinyxml2.h>

#include <cstdio>

// Simple wrapper for TinyXML2
namespace NppXml
{
	using NewDocument = tinyxml2::XMLDocument;
	using Document = tinyxml2::XMLDocument*;
	using Element = tinyxml2::XMLElement*;
	using Node = tinyxml2::XMLNode*;

	[[nodiscard]] inline bool loadFile(Document& doc, const wchar_t* filename) {
		FILE* file = nullptr;
		bool result = false;
		if (::_wfopen_s(&file, filename, L"rb") == 0 && file != nullptr)
		{
			result = doc->LoadFile(file) == tinyxml2::XML_SUCCESS;
			std::fclose(file);
		}
		return result;
	}

	[[nodiscard]] inline bool saveFile(Document& doc, const wchar_t* filename) {
		FILE* file = nullptr;
		bool result = false;
		if (::_wfopen_s(&file, filename, L"w") == 0 && file != nullptr)
		{
			result = doc->SaveFile(file) == tinyxml2::XML_SUCCESS;
			std::fclose(file);
		}
		return result;
	}

	[[nodiscard]] inline bool loadFileShortcut(Document doc, const wchar_t* filename) {
		return loadFile(doc, filename);
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

	[[nodiscard]] inline const char* attribute(const Element& elem, const char* name) {
		return elem->Attribute(name);
	}

	[[nodiscard]] inline int intAttribute(const Element& elem, const char* name, int defaultValue = 0) {
		return elem->IntAttribute(name, defaultValue);
	}

	inline void setAttribute(Element& elem, const char* name, const char* value) {
		elem->SetAttribute(name, value);
	}

	inline void setAttribute(Element& elem, const char* name, int value) {
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

#else

#include <pugixml.hpp>

#include <cstring>

// Simple wrapper for PugiXML
namespace NppXml
{
	using NewDocument = pugi::xml_document;
	using Document = pugi::xml_document*;
	using Element = pugi::xml_node;
	using Node = pugi::xml_node;

	[[nodiscard]] inline bool loadFile(Document doc, const wchar_t* filename) {
		return doc->load_file(filename);
	}

	[[nodiscard]] inline bool saveFile(Document doc, const wchar_t* filename) {
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_save_file_text);
	}

	[[nodiscard]] inline bool loadFileShortcut(Document doc, const wchar_t* filename) {
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes);
	}

	[[nodiscard]] inline bool saveFileShortcut(Document doc, const wchar_t* filename) {
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_save_file_text | pugi::format_control_chars_in_hexadecimal);
	}

	[[nodiscard]] inline Element firstChildElement(const Document& doc, const char* name = nullptr) {
		Node root = doc->root();
		return name ? root.find_child([name](const Element& child) {
			return std::strcmp(child.name(), name) == 0;
		}) : root.first_child();
	}

	[[nodiscard]] inline Element firstChildElement(const Node& node, const char* name = nullptr) {
		return name ? node.find_child([name](const Element& child) {
			return std::strcmp(child.name(), name) == 0;
		}) : node.first_child();
	}

	[[nodiscard]] inline Element toElement(const Node& node) {
		return node;
	}

	[[nodiscard]] inline Element nextSiblingElement(const Node& node, const char* name = nullptr) {
		return node.next_sibling(name);
	}

	[[nodiscard]] inline Node firstChild(const Node& node) {
		return node.first_child();
	}

	[[nodiscard]] inline Node nextSibling(const Node& node) {
		return node.next_sibling();
	}

	[[nodiscard]] inline const char* value(const Node& node) {
		return node.value();
	}

	[[nodiscard]] inline const char* attribute(const Element& elem, const char* name) {
		return elem.attribute(name).value();
	}

	[[nodiscard]] inline int intAttribute(Element elem, const char* name, int defaultValue = 0) {
		return elem.attribute(name).as_int(defaultValue);
	}

	inline void setAttribute(Element& elem, const char* name, const char* value) {
		elem.append_attribute(name) = value;
	}

	inline void setAttribute(Element& elem, const char* name, int value) {
		elem.append_attribute(name) = value;
	}

	inline void createNewDeclaration(Document& doc) {
		auto decl = doc->prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
	}

	inline Element createChildElement(Document& doc, const char* name) {
		return doc->append_child(name);
	}

	inline Element createChildElement(Node parent, const char* name) {
		return parent.append_child(name);
	}

	inline Node createChildText(Node parent, const char* text) {
		Node child = parent.append_child(pugi::node_pcdata);
		child.set_value(text);
		return child;
	}

	inline void deleteChild(Node& parent, Node child) {
		parent.remove_child(child);
	}
}

#endif
