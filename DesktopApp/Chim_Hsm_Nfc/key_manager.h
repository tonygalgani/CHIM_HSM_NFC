#pragma once


#include <fstream>
#include <iostream>
#include <string>
#include <random>
#include <shlobj.h> // Include for SHBrowseForFolder
#include <windows.h>
#include <commdlg.h>

std::string generateKey(size_t length);
std::string importKeys(const std::string &filePath);
void exportKeys(std::string keys, std::string BackupPath);
bool keyRecovery(std::string *key1, std::string *key2);
bool writeKeys(char *key1, char *key2, char dualCards);
std::string OpenFile(HWND owner);
bool ShowFolderPicker(HWND owner, std::string &outFolderPath);
std::string GetDocumentsFolderPath();