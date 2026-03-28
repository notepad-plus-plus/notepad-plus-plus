#pragma once

#include <string>
#include <vector>

struct FunctionListNode
{
	std::string title;
	int line = 0;     // 1-based
	int position = 0; // byte offset from document start
	bool isContainer = false;
	std::vector<FunctionListNode> children;
};

std::vector<FunctionListNode> parseFunctionListNodes(int languageIndex, const std::string& utf8Text);
