// file_operations.h — File I/O, encoding detection, open/save
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#import <Foundation/Foundation.h>
#include <string>

int detectEncoding(NSData* data);
int detectEOLMode(const char* text, size_t len);
std::string decodeFileData(NSData* data, int encoding);
NSData* encodeForSave(const char* utf8Text, size_t len, int encoding);
bool openFileAtPath(NSString* path);
void openFile();
void saveCurrentFile();
