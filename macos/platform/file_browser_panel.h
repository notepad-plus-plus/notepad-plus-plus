#pragma once
#include <string>

void initializeFileBrowserPanel();
void destroyFileBrowserPanel();
void setFileBrowserEnabled(bool enabled);
void openFolderInFileBrowser(const std::string& path);
void openFolderWithDialog();
void* fileBrowserContainerView();
