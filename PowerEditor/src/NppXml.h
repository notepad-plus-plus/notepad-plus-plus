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

#include <pugixml.hpp>

#include <cstdint>
#include <cstring>
#include <concepts>

// Simple wrapper for PugiXML
namespace NppXml
{
	using NewDocument = pugi::xml_document;
	using Document = pugi::xml_document*;
	using Element = pugi::xml_node;
	using Node = pugi::xml_node;
	using Attribute = pugi::xml_attribute;

	[[nodiscard]] inline pugi::string_t normalizeEOL(const pugi::string_t& text);

	[[nodiscard]] inline bool loadFile(Document doc, const wchar_t* filename, bool bReportIfError = false) {
		//return doc->load_file(filename, pugi::parse_default | pugi::parse_comments | pugi::parse_declaration);
		const pugi::xml_parse_result result = doc->load_file(filename, pugi::parse_default | pugi::parse_comments | pugi::parse_declaration);
		if (result.status == pugi::xml_parse_status::status_ok) {
			return true;
		}
		else {
			if (bReportIfError) {
				const wchar_t* szEncoding;
				switch (result.encoding) {
					case pugi::xml_encoding::encoding_auto:
						szEncoding = L"auto (via BOM or < / <? detection, UTF8 if BOM not found)";
						break;
					case pugi::xml_encoding::encoding_utf8:
						szEncoding = L"utf8";
						break;
					case pugi::xml_encoding::encoding_utf16_le:
						szEncoding = L"utf16_le";
						break;
					case pugi::xml_encoding::encoding_utf16_be:
						szEncoding = L"utf16_be";
						break;
					case pugi::xml_encoding::encoding_utf16:
						szEncoding = L"utf16 (native endianness)";
						break;
					case pugi::xml_encoding::encoding_utf32_le:
						szEncoding = L"utf32_le";
						break;
					case pugi::xml_encoding::encoding_utf32_be:
						szEncoding = L"utf32_be";
						break;
					case pugi::xml_encoding::encoding_utf32:
						szEncoding = L"utf32 (native endianness)";
						break;
					case pugi::xml_encoding::encoding_wchar:
						szEncoding = L"wchar (either UTF16 or UTF32)";
						break;
					case pugi::xml_encoding::encoding_latin1:
						szEncoding = L"latin1";
						break;
					default:
						szEncoding = L"unknown enum: ";
				}

				std::wstring wstrDescription;
				const int descLen = ::MultiByteToWideChar(CP_ACP, 0, result.description(), -1, nullptr, 0);
				if (descLen > 0)
				{
					wstrDescription.resize(descLen - 1, L'\0');
					::MultiByteToWideChar(CP_ACP, 0, result.description(), -1, wstrDescription.data(), descLen);
				}

				std::wstring msg = filename;
				msg += L"\n\n- status: " + std::to_wstring(result.status) + L" (" + wstrDescription + L")";
				msg += L"\n- last parsed offset: " + std::to_wstring(result.offset) + L" (char_t units)";
				msg += L"\n- source document encoding: ";
				msg += szEncoding;
				msg += (lstrcmpW(szEncoding, L"unknown enum: ") == 0) ? std::to_wstring(result.encoding) : L"";

				::MessageBoxW(nullptr, msg.c_str(), L"pugixml::load_file", MB_OK | MB_APPLMODAL | MB_ICONWARNING);
			}
			return false;
		}
	}

	[[nodiscard]] inline bool saveFile(const Document doc, const wchar_t* filename) {
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_save_file_text);
	}

	[[nodiscard]] inline bool loadFileNativeLang(Document doc, const wchar_t* filename) {
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes | pugi::parse_eol | pugi::parse_comments | pugi::parse_declaration);
	}

	[[nodiscard]] inline bool loadFileContextMenu(Document doc, const wchar_t* filename) {
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes | pugi::parse_eol);
	}

	[[nodiscard]] inline bool loadFileShortcut(Document doc, const wchar_t* filename) {
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes | pugi::parse_comments | pugi::parse_declaration);
	}

	[[nodiscard]] inline bool saveFileShortcut(const Document doc, const wchar_t* filename) {
		// Without pugi::parse_eol comments are not eol normalized when loaded.
		// To avoid issue with CRLF converting to CRCRLF on save, comments are normalized on save
		// to have LF eol.
		struct eol_norm_walker : pugi::xml_tree_walker
		{
			bool for_each(pugi::xml_node& node) override
			{
				if (node.type() == pugi::node_comment)
				{
					const pugi::string_t normalizedText = normalizeEOL(node.value());
					node.set_value(normalizedText.c_str());
				}
				return true;
			}
		};

		eol_norm_walker walker;
		doc->traverse(walker);
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_save_file_text | pugi::format_control_chars_in_hexadecimal);
	}

	[[nodiscard]] inline bool loadFileFunctionParser(Document doc, const wchar_t* filename) {
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes | pugi::parse_eol);
	}

	[[nodiscard]] inline bool saveFileProject(const Document doc, const wchar_t* filename) {
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_no_declaration | pugi::format_save_file_text);
	}

	[[nodiscard]] inline bool loadFileUDL(Document doc, const wchar_t* filename)
	{
		// UDL lists can contain EOL separator, so UDL must be loaded without pugi::parse_eol.
		return doc->load_file(filename, pugi::parse_cdata | pugi::parse_escapes | pugi::parse_comments | pugi::parse_declaration);
	}

	[[nodiscard]] inline bool saveFileUDL(const Document doc, const wchar_t* filename)
	{
		// Without pugi::parse_eol EOL are not normalized when loaded.
		// To avoid issue with CRLF converting to CRCRLF on save, EOL are normalized on save
		// to have LF eol.
		struct eol_norm_walker : pugi::xml_tree_walker
		{
			bool for_each(pugi::xml_node& node) override
			{
				const pugi::string_t normalizedText = normalizeEOL(node.value());
				node.set_value(normalizedText.c_str());
				return true;
			}
		};

		eol_norm_walker walker;
		doc->traverse(walker);
		return doc->save_file(filename, "    ", pugi::format_indent | pugi::format_no_declaration | pugi::format_save_file_text);
	}

	[[nodiscard]] inline Element firstChildElement(const Document& doc, const char* name = nullptr) {
		Node root = doc->root();
		return name ? root.find_child([&name](const Element& child) {
			return (child.type() == pugi::node_element) && (std::strcmp(child.name(), name) == 0);
		}) : root.first_child();
	}

	[[nodiscard]] inline Element firstChildElement(const Node& node, const char* name = nullptr) {
		return name ? node.find_child([&name](const Element& child) {
			return (child.type() == pugi::node_element) && (std::strcmp(child.name(), name) == 0);
		}) : node.first_child();
	}

	[[nodiscard]] inline Element nextSiblingElement(const Node& node, const char* name = nullptr) {
		return name ? node.next_sibling(name) : node.next_sibling();
	}

	[[nodiscard]] inline Node firstChild(const Node& node) {
		return node.first_child();
	}

	[[nodiscard]] inline Node lastChild(const Node& node) {
		return node.last_child();
	}

	[[nodiscard]] inline Node nextSibling(const Node& node) {
		return node.next_sibling();
	}

	[[nodiscard]] inline const char* name(const Node& node) {
		return node.name();
	}

	[[nodiscard]] inline const char* value(const Node& node) {
		return node.value();
	}

	inline bool setValue(Node& node, const char* value) {
		return node.set_value(value);
	}

	inline bool setValue(Node& node, const pugi::string_t& value) {
		return node.set_value(value);
	}

	[[nodiscard]] inline const char* attribute(const Element& elem, const char* name, const char* defaultValue = nullptr) {
		return elem.attribute(name).as_string(defaultValue);
	}

	[[nodiscard]] inline int intAttribute(const Element& elem, const char* name, int defaultValue = 0) {
		return elem.attribute(name).as_int(defaultValue);
	}

	[[nodiscard]] inline unsigned int uintAttribute(const Element& elem, const char* name, unsigned int defaultValue = 0) {
		return elem.attribute(name).as_uint(defaultValue);
	}

	[[nodiscard]] inline int64_t int64Attribute(const Element& elem, const char* name, int64_t defaultValue = 0) {
		return elem.attribute(name).as_llong(defaultValue);
	}

	[[nodiscard]] inline uint64_t uint64Attribute(const Element& elem, const char* name, uint64_t defaultValue = 0) {
		return elem.attribute(name).as_ullong(defaultValue);
	}

	template <typename T>
		requires (!std::same_as<T, pugi::string_t>)
	inline bool setAttribute(Element& elem, const char* name, T value)
	{
		auto attr = elem.attribute(name);
		if (!attr)
		{
			attr = elem.append_attribute(name);
		}
		return attr.set_value(value);
	}

	template <typename T>
		requires (std::same_as<T, pugi::string_t>)
	inline bool setAttribute(Element& elem, const char* name, const T& value)
	{
		auto attr = elem.attribute(name);
		if (!attr)
		{
			attr = elem.append_attribute(name);
		}
		return attr.set_value(value);
	}

	inline void createNewDeclaration(Document& doc) {
		auto decl = doc->prepend_child(pugi::node_declaration);
		decl.append_attribute("version") = "1.0";
		decl.append_attribute("encoding") = "UTF-8";
	}

	inline Element createChildElement(Document& doc, const char* name) {
		return doc->append_child(name);
	}

	inline Element createChildElement(Node& parent, const char* name) {
		return parent.append_child(name);
	}

	inline Node insertEndChild(Node& parent, const Node& child) {
		return parent.append_copy(child);
	}

	inline Node createChildText(Node& parent, const char* text) {
		Node child = parent.append_child(pugi::node_pcdata);
		child.set_value(text);
		return child;
	}

	inline Node createChildText(Node& parent, const pugi::string_t& text) {
		Node child = parent.append_child(pugi::node_pcdata);
		child.set_value(text);
		return child;
	}

	inline bool deleteChild(Document& doc, const Node& child) {
		return doc->remove_child(child);
	}

	inline bool deleteChild(Node& parent, const Node& child) {
		return parent.remove_child(child);
	}

	inline bool clear(Node& parent) {
		return parent.remove_children();
	}

	[[nodiscard]] inline Attribute firstAttribute(const Element& elem) {
		return elem.first_attribute();
	}

	[[nodiscard]] inline Attribute next(const Attribute& attr) {
		return attr.next_attribute();
	}

	[[nodiscard]] inline const char* name(const Attribute& attr) {
		return attr.name();
	}

	[[nodiscard]] inline const char* value(const Attribute& attr) {
		return attr.value();
	}

	pugi::string_t normalizeEOL(const pugi::string_t& text)
	{
		pugi::string_t normalized;
		const size_t len = text.length();

		for (size_t i = 0; i < len; ++i)
		{
			if (text[i] == PUGIXML_TEXT('\r'))
			{
				if (i + 1 < len && text[i + 1] == PUGIXML_TEXT('\n'))
				{
					normalized += PUGIXML_TEXT('\n');
					++i;
				}
				else
				{
					normalized += PUGIXML_TEXT('\n');
				}
			}
			else
			{
				normalized += text[i];
			}
		}
		return normalized;
	}
}
