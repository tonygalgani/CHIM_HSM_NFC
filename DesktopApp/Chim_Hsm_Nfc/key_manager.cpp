#include "key_manager.h"

std::string OpenFile(HWND owner) {
  CHAR fileBuffer[MAX_PATH] = {0};
  OPENFILENAME ofn; // Struct to hold info about the file name

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = owner;
  ofn.lpstrFile = fileBuffer;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrFilter = "Text Files\0*.TXT\0All Files\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileName(&ofn)) {
    return std::string(ofn.lpstrFile);
  }
  return "";
}

std::string generateKey(size_t length) {
  // Shuffled ascii characters
  const char charset[] = ":_eD;PFv@XHUld#*SQE<K}B?i2^w>k.s=hNoCj3!10rq6ZY-"
                         "gLbzT$t%mx[a8]WMIyAcf&n+5R/(V97{JuOG,)p4";
  const size_t max_index = (sizeof(charset) - 1);

  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<> distribution(0, max_index - 1);

  std::string key;

  for (size_t i = 0; i < length; ++i) {
    key += charset[distribution(generator)];
  }

  return key;
}

std::string importKeys(const std::string &filePath) {
  std::ifstream file(filePath);
  std::string line;
  if (file.is_open()) {
    std::getline(file, line);
    file.close();
  } else {
    MessageBox(NULL, "Failed to open file.", "Error", MB_OK);
  }
  return line;
}

void exportKeys(std::string keys, std::string BackupPath) {
  // Export the two first segments of the key
  // std::cout << "File path verif : " << BackupPath + "\\ChimBackup.txt" << std::endl;
  std::ofstream keysBackup(BackupPath + "\\ChimBackup.txt");
  // Write to the file
  keysBackup << keys;
  // Close the file
  keysBackup.close();
  std::cout << "File has been written." << std::endl;
}

bool ShowFolderPicker(HWND owner, std::string &outFolderPath) {
  BROWSEINFO bi = {0};
  bi.lpszTitle = "Select a folder";
  bi.hwndOwner = owner;
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

  if (pidl != 0) {
    // Get the name of the folder and put it in path
    char path[MAX_PATH];
    if (SHGetPathFromIDList(pidl, path)) {
      outFolderPath = path;
      IMalloc *imalloc = 0;
      if (SUCCEEDED(SHGetMalloc(&imalloc))) {
        imalloc->Free(pidl);
        imalloc->Release();
      }
      return true;
    }
  }
  return false;
}
