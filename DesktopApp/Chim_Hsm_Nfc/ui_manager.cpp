#include "ui_manager.h"

// Flags to check is there is already an admin password
bool passwordProtected;
// Flags to check if the password entered is the correct admin password
bool isPasswordCorrect;

// String to stored all key segments.
std::string finalKey;



std::string GetUserDocumentsDirectory() {
    PWSTR path = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
    if (SUCCEEDED(hr)) {
        // Convert the wchar_t* path to a std::string using wcstombs_s
        char charPath[MAX_PATH];
        size_t convertedChars = 0;
        wcstombs_s(&convertedChars, charPath, path, MAX_PATH);
        std::string userDocumentsPath(charPath);
        CoTaskMemFree(path);  // Free the memory allocated for the path
        return userDocumentsPath;
    }
    else {
        // Handle the error, for example, by returning an empty string or throwing an exception
        return "";
    }
}

// Default location for keys backup
std::string backupLocation = GetUserDocumentsDirectory();

// Main window procedure function that handles messages for the window
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  static HBRUSH hbrBackground = NULL;

  switch (uMsg) {
  case WM_CREATE: {
    // Create the components for admin password verification/creation
    createAdminInterface(hwnd, hbrBackground);

    // Match the window background
    hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    return 0;
  }

  case WM_DESTROY: {             // Case hanlder for destroying the window
    DeleteObject(hbrBackground); // Message for closing the window
    // Posts a quit message which results in the message loop ending
    PostQuitMessage(0);
    return 0;
  }
  case WM_COMMAND: {
    if (LOWORD(wParam) == IDC_SUBMIT_BUTTON) { // Submit button clicked
      char password[32] = {0};
      char confirm_password[32] = {0};
      char password_length =
          GetDlgItemText(hwnd, IDC_ADMINPASSWORD_EDIT, password, 64);
      GetDlgItemText(hwnd, IDC_ADMINPASSWORDCONFIRM_EDIT, confirm_password, 64);
      if (password_length > 32) { // Check if password is not too long
        // Inform the user password is too long
        MessageBox(NULL, "Password longer than 32 characters", "Error", MB_OK);
        ;
        return 0;
      }
      if (!passwordProtected) {
        if (strcmp(password, confirm_password)) {
          SetWindowText(GetDlgItem(hwnd, IDC_ADMINPASSWORD_STATIC),
                        "The passwords do not match");
          return 0;
        }
        if (!create_admin_password(password)) {
          // std::cout << "password creation successful" << std::endl;
        } else {
          // std::cout << "password creation failed" << std::endl;
        }

      } else {
        isPasswordCorrect = admin_password_verification(password);
        if (isPasswordCorrect) { // Incorrect password
          SetWindowText(GetDlgItem(hwnd, IDC_ADMINPASSWORD_STATIC),
                        "Wrong password, Try again");
        }

      } // if(passwordProtected)

      if (!isPasswordCorrect) { // Correct password
        // Destroy every components for admin password verification/creation.
        DestroyWindow(GetDlgItem(hwnd, IDC_ADMINPASSWORD_STATIC));
        DestroyWindow(GetDlgItem(hwnd, IDC_ADMINPASSWORD_EDIT));
        DestroyWindow(GetDlgItem(hwnd, IDC_ADMINPASSWORDCONFIRM_EDIT));
        DestroyWindow(GetDlgItem(hwnd, IDC_SUBMIT_BUTTON));

        // Create every components for key management
        keyManagementInterface(hwnd, hbrBackground);
      } // if(!isPasswordCorrect)
    }
    if (LOWORD(wParam) == IDC_GENERATEKEYS_BUTTON) { 
      // Generates two 32 characters key
      std::string key1 = generateKey(32);
      std::string key2 = generateKey(32);
      // Send both key to the textboxess
      SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), NULL);
      SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), NULL);
      SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), key1.c_str());
      SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), key2.c_str());
    }

    if (LOWORD(wParam) == IDC_IMPORTKEYS_BUTTON) { // Import button clicke
        destroykeyManagementInterface(hwnd);
        decryptionPasswordInterface(hwnd, hbrBackground);
        
    }
    if (LOWORD(wParam) == IDC_IMPORTKEYSCANCEL_BUTTON) { 
        destroyDecryptionPasswordInterface(hwnd);
        keyManagementInterface(hwnd, hbrBackground);
    }
    if (LOWORD(wParam) == IDC_EXPORTKEYSCANCEL_BUTTON) {
        destroyEncryptionPasswordInterface(hwnd);
        keyManagementInterface(hwnd, hbrBackground);
    }
    if (LOWORD(wParam) == IDC_DECRYPTIONSUBMIT_BUTTON){
        char backupPassword[256] = { 0 };
        char confirm_backupPassword[256] = { 0 };
        size_t password_length =
            GetDlgItemText(hwnd, IDC_DECRYPTIONPASSWORD_EDIT, backupPassword, 256);
        GetDlgItemText(hwnd, IDC_DECRYPTIONPASSWORDCONFIRM_EDIT, confirm_backupPassword, 256);
        if (password_length > 256) { // Check if password is not too long
            // Inform the user password is too long
            MessageBox(NULL, "Password longer than 256 characters", "Error", MB_OK);
            ;
            return 0;
        }
        if (strcmp(backupPassword, confirm_backupPassword)) {
            SetWindowText(GetDlgItem(hwnd, IDC_DECRYPTIONPASSWORD_STATIC),
                "The passwords do not match");
            return 0;
        }
        
        std::string filePath = OpenFile(hwnd);
        if (!filePath.empty()) {
            // Recover the keys from save file
            std::string fileContent = importKeys(filePath);
            std::string encodedSalt = fileContent.substr(0, 64);  // Retreive salt file
            std::string encodedKeys = fileContent.substr(64, 160); // Retreive keys from file
        
            /*std::cout << "fileContent: " << fileContent << std::endl;
            std::cout << "encodedSalt : " << encodedSalt << std::endl;
            std::cout << "encodedKeys : " << encodedKeys << std::endl;*/

        std::string decryptedKeys; // buffer for decrypted keys
        decryption(backupPassword, encodedSalt, encodedKeys, decryptedKeys);
        
        //std::cout << "decryptedKeys: " << decryptedKeys << std::endl;

        destroyDecryptionPasswordInterface(hwnd);
        keyManagementInterface(hwnd, hbrBackground);

        SendMessage(GetDlgItem(hwnd, IDC_BACKUP_CHECKBOX), BM_SETCHECK, BST_UNCHECKED, 0);

        // Clear the textboxes before writting the keys in them
        SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), NULL);
        SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), NULL);

        // Isolate each keys
        std::string key1 = decryptedKeys.substr(0, 32);
        std::string key2 = decryptedKeys.substr(32, 32);

        // Write the keys in the textboxes
        SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), key1.c_str());
        SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), key2.c_str());
        } else {
        MessageBox(hwnd, "No file selected or failed to open file.", "Error",
                    MB_ICONERROR);
        }
    }
    if (LOWORD(wParam) == IDC_GETNFCKEYS_BUTTON) { // Import button clicke

      std::string key1;
      std::string key2;

      keyRecovery(&key1, &key2);

      // std::cout << "key1:" << key1 << std::endl;
      // std::cout << "key2:" << key2 << std::endl;

      // Clear the textboxes before writting the keys in them
      SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), NULL);
      SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), NULL);
      // Write the keys in the textboxes
      SetWindowText(GetDlgItem(hwnd, IDC_KEY1_EDIT), key1.c_str());
      SetWindowText(GetDlgItem(hwnd, IDC_KEY2_EDIT), key2.c_str());
    }
    if (LOWORD(wParam) == IDC_ENCRYPTIONSUBMIT_BUTTON) {
      // buffer to hold the encryption password
      char encryptPassword[256] = {0};
      // buffer to hold the encryption password confirmation
      char confirm_encryptPassword[256] = {0};
      uint16_t encryptPassword_length = // Get length of the encryption password
          GetDlgItemText(hwnd, IDC_ENCRYPTIONPASSWORD_EDIT, encryptPassword,
                         260);
      GetDlgItemText(hwnd, IDC_ENCRYPTIONPASSWORDCONFIRM_EDIT,
                     confirm_encryptPassword, 260);
      if (encryptPassword_length > 256) { // Check if password is not too long
        // Inform the user password is too long
        MessageBox(NULL, "Password longer than 256 characters", "Error", MB_OK);
        return false;
      }
      if (strcmp(
              encryptPassword,
              confirm_encryptPassword)) { // Check if both password are the same
        // Inform the user passwords are different
        MessageBox(NULL, "Passwords are different", "Error", MB_OK);
        return false;
      }
      //std::cout << " encryptPassword" << encryptPassword << std::endl;
      std::string encryptedKeys;
      std::string salt;
      encryption(encryptPassword, finalKey.substr(0, 64), salt, encryptedKeys);
      
      std::string exportData;
      exportData.append(salt);
      exportData.append(encryptedKeys);
      //std::cout << "Exportdata: " << exportData << std::endl;
      exportKeys(exportData,
                 backupLocation); // Export the two first segments into a txt
                                  // file Display the finalKey composed with the
                                  // two (or three) segments
      std::cout << "ID =" << finalKey << std::endl;

      SendMessage(hwnd, WM_CLOSE, 0, 0); // Send WM_CLOSE message
    }
    if (LOWORD(wParam) == IDC_CONFIRM_BUTTON) { // Confirm button clicked
      char password[32] = {0};                  // buffer to hold the password
      char confirm_password[32] = {
          0}; // buffer to hold the password confirmation
      char password_length =
          GetDlgItemText(hwnd, IDC_PASSWORD_EDIT, password, 64);
      GetDlgItemText(hwnd, IDC_PASSWORDCONFIRM_EDIT, confirm_password, 64);
      if (password_length > 32) { // Check if password is not too long
        // Inform the user password is too long
        MessageBox(NULL, "Password longer than 32 characters", "Error", MB_OK);
        ;
        return 0;
      }
      if (strcmp(password,
                 confirm_password)) { // Check if both password are the same
        // Inform the user passwords are different
        MessageBox(NULL, "Passwords are different", "Error", MB_OK);
        ;
        return 0;
      }

      char buffer[32]; // Buffer to hold the keys
      GetDlgItemText(hwnd, IDC_KEY1_EDIT, buffer,
                     sizeof(buffer) +
                         1); // + 1 ohterwise last character missing
      // std::cout << "buff1:" << buffer << std::endl;
      finalKey.append(buffer); // Append the first segment to the final key
      // std::cout << "finak1:" << finalKey << std::endl;
      GetDlgItemText(hwnd, IDC_KEY2_EDIT, buffer, sizeof(buffer) + 1);
      // std::cout << "buff2:" << buffer << std::endl;
      finalKey.append(buffer); // Append the second segment to the final key
      // std::cout << "finak2:" << finalKey << std::endl;
      finalKey.append(password); // Append the password to the final key
      DWORD checkboxState =
          SendMessage(GetDlgItem(hwnd, IDC_BACKUP_CHECKBOX), BM_GETCHECK, 0, 0);

      if (checkboxState == BST_CHECKED) { // if backup option checked
        // std::cout << "Got Here 1" << std::endl;
        destroykeyManagementInterface(hwnd);
        // std::cout << "Got Here 2" << std::endl;
        encryptionPasswordInterface(hwnd, hbrBackground);
        // std::cout << "Got Here 3" << std::endl;
      } else {

        // Display the finalKey composed with the two (or three) segments
        std::cout << "ID =" << finalKey << std::endl;
        Sleep(500);
        SendMessage(hwnd, WM_CLOSE, 0, 0); // Send WM_CLOSE message
      }
    }
    if (LOWORD(wParam) == IDC_WRITEKEYS_BUTTON) { // Confirm button clicked

      char dualCards; // dual cards flag
      char key1[32];  // Buffer to hold the keys
      char key2[32];
      // std::cout << "sizeof(key)" << sizeof(key1) << std::endl;
      GetDlgItemText(hwnd, IDC_KEY1_EDIT, key1, sizeof(key1) + 1);
      GetDlgItemText(hwnd, IDC_KEY2_EDIT, key2, sizeof(key2) + 1);

      // std::cout << "key1:" << key1 << std::endl;
      // std::cout << "key2:" << key2 << std::endl;

      // Verification of the checkbox's state
      DWORD checkboxState = SendMessage(
          GetDlgItem(hwnd, IDC_DUALCARDS_CHECKBOX), BM_GETCHECK, 0, 0);
      // If checked flag is set to 1, otherwise flag is set to 0.
      (checkboxState == BST_CHECKED) ? dualCards = '1' : dualCards = '0';
      // std::cout << "dualCards: " << dualCards << std::endl;

      // Use serial to send flag + keys
      writeKeys(key1, key2, dualCards);
    }
    if (LOWORD(wParam) == IDC_BACKUP_BUTTON) {
      ShowFolderPicker(hwnd, backupLocation);
      std::string label = "Backup Location: " + backupLocation;
      SetWindowText(GetDlgItem(hwnd, IDC_BACKUP_STATIC), label.c_str());
    }
    if (LOWORD(wParam) == IDC_BACKUP_CHECKBOX) { // Backup checkbox
      // Toggle the checkbox state upon click
      HWND hCheckbox = GetDlgItem(hwnd, IDC_BACKUP_CHECKBOX);
      DWORD dwState = SendMessage(hCheckbox, BM_GETCHECK, 0, 0);
      SendMessage(hCheckbox, BM_SETCHECK,
                  (dwState == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED, 0);
    }
    if (LOWORD(wParam) == IDC_DUALCARDS_CHECKBOX) { // Backup checkbox
      // Toggle the checkbox state upon click
      HWND hCheckbox = GetDlgItem(hwnd, IDC_DUALCARDS_CHECKBOX);
      DWORD dwState = SendMessage(hCheckbox, BM_GETCHECK, 0, 0);
      SendMessage(hCheckbox, BM_SETCHECK,
                  (dwState == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED, 0);
    }
    return 0;
  }
  case WM_GETMINMAXINFO: {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = WINDOWWIDTH;  // Minimum width of the window
    lpMMI->ptMinTrackSize.y = WINDOWHEIGHT; // Minimum height of the window
    lpMMI->ptMaxTrackSize.x = WINDOWWIDTH;  // Maximum width of the window
    lpMMI->ptMaxTrackSize.y = WINDOWHEIGHT; // Maximum height of the window
  }
    return 0;
  case WM_CTLCOLORSTATIC: {
    HDC hdcStatic = (HDC)wParam;
    SetTextColor(hdcStatic, RGB(0, 0, 0)); // Set text color
    SetBkMode(hdcStatic, TRANSPARENT);     // Set background mode to transparent
    return (INT_PTR)hbrBackground;
  }
  case WM_PAINT: // Message for painting the window's client area
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps); // Prepare the window for painting

    // Fill the window's client area with a color
    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    EndPaint(hwnd, &ps); // Marks the end of painting in the window
  }
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam,
                       lParam); // Default handling for any unhandled messages
}

void createAdminInterface(HWND hwnd, HBRUSH hdrBackground) {
  if (passwordProtected) {
    CreateWindowEx(0, "STATIC", "Enter admin password",
                   WS_CHILD | WS_VISIBLE | SS_CENTER, (WINDOWWIDTH - 400) / 2,
                   ((WINDOWHEIGHT - 20) / 2) - 60, 400, IDC_ANYHEIGHT, hwnd,
                   (HMENU)IDC_ADMINPASSWORD_STATIC, GetModuleHandle(NULL),
                   NULL);

    // Create Text box for password input
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                 (WINDOWWIDTH - IDC_TEXTBOXWIDTH) / 2,
                 (WINDOWHEIGHT - 20) / 2 - 20, IDC_TEXTBOXWIDTH, IDC_ANYHEIGHT,
                 hwnd, (HMENU)IDC_ADMINPASSWORD_EDIT, GetModuleHandle(NULL),
                 NULL);

    // Create button to validate input
    CreateWindow("BUTTON", "Submit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 (WINDOWWIDTH - 80) / 2, ((WINDOWHEIGHT - 20) / 2 + 20), 80,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_SUBMIT_BUTTON,
                 GetModuleHandle(NULL), NULL);
  } else {
    CreateWindowEx(0, "STATIC", "Create admin password (32 characters max)",
                   WS_CHILD | WS_VISIBLE | SS_CENTER, (WINDOWWIDTH - 400) / 2,
                   ((WINDOWHEIGHT - 20) / 2) - 60, 400, IDC_ANYHEIGHT, hwnd,
                   (HMENU)IDC_ADMINPASSWORD_STATIC, GetModuleHandle(NULL),
                   NULL);

    // Create Text box for password input
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                 (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 - 20, 200,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_ADMINPASSWORD_EDIT,
                 GetModuleHandle(NULL), NULL);

    // Create Text box for password confirmation
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
                 (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 + 20, 200,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_ADMINPASSWORDCONFIRM_EDIT,
                 GetModuleHandle(NULL), NULL);

    CreateWindow("BUTTON", "Submit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 (WINDOWWIDTH - 80) / 2, ((WINDOWHEIGHT - 20) / 2 + 60), 80,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_SUBMIT_BUTTON,
                 GetModuleHandle(NULL), NULL);
  }
}

void keyManagementInterface(HWND hwnd, HBRUSH hbrBackground) {
  // Screen dimensions
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  // Calculate centered position
  int xPos = (screenWidth - WINDOWWIDTH) / 2;
  int yPos = (screenHeight - WINDOWHEIGHT) / 2;

  // Create a textbox for key 1
  CreateWindowEx(0, "EDIT", "",
                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL |
                     ES_PASSWORD | ES_READONLY,
                 (WINDOWWIDTH - IDC_TEXTBOXWIDTH) / 2 - IDC_TEXTBOXDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) - 100, IDC_TEXTBOXWIDTH,
                 IDC_ANYHEIGHT, // Position and dimensions
                 hwnd, (HMENU)IDC_KEY1_EDIT, GetModuleHandle(NULL), NULL);

  // Create a textbox for key 2
  CreateWindowEx(0, "EDIT", "",
                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL |
                     ES_PASSWORD | ES_READONLY,
                 (WINDOWWIDTH - IDC_TEXTBOXWIDTH) / 2 - IDC_TEXTBOXDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) - 60, IDC_TEXTBOXWIDTH,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_KEY2_EDIT,
                 GetModuleHandle(NULL), NULL);

  // Create a textbox for optional password
  CreateWindowEx(0, "EDIT", "",
                 WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL |
                     ES_PASSWORD,
                 (WINDOWWIDTH - IDC_TEXTBOXWIDTH) / 2 - IDC_TEXTBOXDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) - 20, IDC_TEXTBOXWIDTH,
                 IDC_ANYHEIGHT, // Position and dimensions
                 hwnd, (HMENU)IDC_PASSWORD_EDIT, GetModuleHandle(NULL), NULL);

  // Create Text box for password confirmation
  CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
               (WINDOWWIDTH - IDC_TEXTBOXWIDTH) / 2 - IDC_TEXTBOXDECALAGE,
               ((WINDOWHEIGHT - 20) / 2) + 20, IDC_TEXTBOXWIDTH, IDC_ANYHEIGHT,
               hwnd, (HMENU)IDC_PASSWORDCONFIRM_EDIT, GetModuleHandle(NULL),
               NULL);

  // Create static text for key 1
  CreateWindowEx(0, "STATIC", "Segment 1:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                 (WINDOWWIDTH - IDC_LABELWIDTH) / 2 - IDC_LABELDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) - 100, IDC_LABELWIDTH,
                 IDC_ANYHEIGHT, // Position and dimensions
                 hwnd, (HMENU)IDC_KEY1_STATIC, GetModuleHandle(NULL), NULL);

  // Create static text for key 2
  CreateWindowEx(0, "STATIC", "Segment 2:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                 (WINDOWWIDTH - IDC_LABELWIDTH) / 2 - IDC_LABELDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) - 60, IDC_LABELWIDTH,
                 IDC_ANYHEIGHT, // Position and dimensions
                 hwnd, (HMENU)IDC_KEY2_STATIC, GetModuleHandle(NULL), NULL);

  // Create static text for optional password
  CreateWindowEx(
      0, "STATIC", "Password:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
      (WINDOWWIDTH - IDC_LABELWIDTH) / 2 - IDC_LABELDECALAGE,
      ((WINDOWHEIGHT - 20) / 2) - 20, IDC_LABELWIDTH,
      IDC_ANYHEIGHT, // Position and dimensions // Position and dimensions
      hwnd, (HMENU)IDC_PASSWORD_STATIC, GetModuleHandle(NULL), NULL);

  // Create static text for optional password
  CreateWindowEx(0, "STATIC", "Confirm:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                 (WINDOWWIDTH - IDC_LABELWIDTH) / 2 - IDC_LABELDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) + 20, IDC_LABELWIDTH,
                 IDC_ANYHEIGHT, // Position and dimensions
                 hwnd, (HMENU)IDC_PASSWORDCONFIRM_STATIC, GetModuleHandle(NULL),
                 NULL);

  // Create a button to generate keys
  CreateWindowEx(
      0, "BUTTON", "Générer segments", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      (WINDOWWIDTH - IDC_BUTTONWIDTH) / 2 + IDC_BUTTONDECALAGE,
      ((WINDOWHEIGHT - 20) / 2) - 100, IDC_BUTTONWIDTH,
      IDC_ANYHEIGHT, // Position and dimensions
      hwnd, (HMENU)IDC_GENERATEKEYS_BUTTON, GetModuleHandle(NULL), NULL);

  // Create a button to import keys
  CreateWindowEx(
      0, "BUTTON", "Importer sauvegarde", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      (WINDOWWIDTH - IDC_BUTTONWIDTH) / 2 + IDC_BUTTONDECALAGE,
      ((WINDOWHEIGHT - 20) / 2) - 60, IDC_BUTTONWIDTH, IDC_ANYHEIGHT, hwnd,
      (HMENU)IDC_IMPORTKEYS_BUTTON, GetModuleHandle(NULL), NULL);

  // Create a button to confirm and send the keys to cybercomputer
  CreateWindowEx(
      0, "BUTTON", "Importer clé HSM", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      (WINDOWWIDTH - IDC_BUTTONWIDTH) / 2 + IDC_BUTTONDECALAGE,
      ((WINDOWHEIGHT - 20) / 2) - 20, IDC_BUTTONWIDTH, IDC_ANYHEIGHT, hwnd,
      (HMENU)IDC_GETNFCKEYS_BUTTON, GetModuleHandle(NULL), NULL);

  // Create a button to confirm and send the keys to cybercomputer
  CreateWindowEx(0, "BUTTON", "Confirmer", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 (WINDOWWIDTH - IDC_BUTTONWIDTH) / 2 + IDC_BUTTONDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) + 60, IDC_BUTTONWIDTH, IDC_ANYHEIGHT,
                 hwnd, (HMENU)IDC_CONFIRM_BUTTON, GetModuleHandle(NULL), NULL);

  // Create a button to confirm and send the keys to cybercomputer
  CreateWindowEx(0, "BUTTON", "Exporter clé HSM",
                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 (WINDOWWIDTH - IDC_BUTTONWIDTH) / 2 + IDC_BUTTONDECALAGE,
                 ((WINDOWHEIGHT - 20) / 2) + 20, IDC_BUTTONWIDTH, IDC_ANYHEIGHT,
                 hwnd, (HMENU)IDC_WRITEKEYS_BUTTON, GetModuleHandle(NULL),
                 NULL);

  // Create a checkbox
  CreateWindowEx(0, "BUTTON", "Dual cards", WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
                 180, 200, 100, IDC_ANYHEIGHT, hwnd,
                 (HMENU)IDC_DUALCARDS_CHECKBOX, GetModuleHandle(NULL), NULL);


  // Create a backup checkbox and sets it to be checked by default
  SendMessage(CreateWindowEx(0, "BUTTON", "Export de la clé",
                             WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 20, 200, 160,
                             IDC_ANYHEIGHT, hwnd, (HMENU)IDC_BACKUP_CHECKBOX,
                             GetModuleHandle(NULL), NULL),
              BM_SETCHECK, (WPARAM)BST_CHECKED, 0);

  std::string editmsgbckp = "Backup location: " + backupLocation;
  CreateWindowEx(0, "EDIT", editmsgbckp.c_str(),
                 WS_CHILD | WS_VISIBLE | SS_LEFT | ES_READONLY, 45, 230, 440,
                 IDC_ANYHEIGHT,
                 hwnd, (HMENU)IDC_BACKUP_STATIC, GetModuleHandle(NULL), NULL);

  CreateWindowEx(0, "BUTTON", "...", WS_CHILD | WS_VISIBLE, 20, 230, 20,
                 IDC_ANYHEIGHT, hwnd, (HMENU)IDC_BACKUP_BUTTON,
                 GetModuleHandle(NULL), NULL);

  // Match the textbox backgrouds to the window background
  hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}

void encryptionPasswordInterface(HWND hwnd, HBRUSH hbrBackground) {
  CreateWindowEx(0, "STATIC",
                 "Create encryption password. Do not forget it or your backup "
                 "will be un recoverable!",
                 WS_CHILD | WS_VISIBLE | SS_CENTER, (WINDOWWIDTH - 400) / 2,
                 ((WINDOWHEIGHT - 20) / 2) - 80, 400, IDC_ANYHEIGHT * 2, hwnd,
                 (HMENU)IDC_ENCRYPTIONPASSWORD_STATIC, GetModuleHandle(NULL),
                 NULL);

  // Create Text box for password input
  CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
               (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 - 20, 200,
               IDC_ANYHEIGHT, hwnd, (HMENU)IDC_ENCRYPTIONPASSWORD_EDIT,
               GetModuleHandle(NULL), NULL);

  // Create Text box for password confirmation
  CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
               (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 + 20, 200,
               IDC_ANYHEIGHT, hwnd, (HMENU)IDC_ENCRYPTIONPASSWORDCONFIRM_EDIT,
               GetModuleHandle(NULL), NULL);

  CreateWindowEx(0, "BUTTON", "Submit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      (WINDOWWIDTH - 80) / 2 - 60, ((WINDOWHEIGHT - 20) / 2 + 60), 80,
               IDC_ANYHEIGHT, hwnd, (HMENU)IDC_ENCRYPTIONSUBMIT_BUTTON,
               GetModuleHandle(NULL), NULL);

  CreateWindowEx(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      (WINDOWWIDTH - 80) / 2 + 60, ((WINDOWHEIGHT - 20) / 2 + 60), 80,
      IDC_ANYHEIGHT, hwnd, (HMENU)IDC_EXPORTKEYSCANCEL_BUTTON,
      GetModuleHandle(NULL), NULL);
  // Match the textbox backgrouds to the window background
  hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}

void destroyEncryptionPasswordInterface(HWND hwnd) {
    DestroyWindow(GetDlgItem(hwnd, IDC_ENCRYPTIONPASSWORD_STATIC));
    DestroyWindow(GetDlgItem(hwnd, IDC_ENCRYPTIONPASSWORD_EDIT));
    DestroyWindow(GetDlgItem(hwnd, IDC_ENCRYPTIONPASSWORDCONFIRM_EDIT));
    DestroyWindow(GetDlgItem(hwnd, IDC_ENCRYPTIONSUBMIT_BUTTON));
    DestroyWindow(GetDlgItem(hwnd, IDC_EXPORTKEYSCANCEL_BUTTON));
}

void decryptionPasswordInterface(HWND hwnd, HBRUSH hbrBackground) {
    CreateWindowEx(0, "STATIC",
        "Type your backup password",
        WS_CHILD | WS_VISIBLE | SS_CENTER, (WINDOWWIDTH - 400) / 2,
        ((WINDOWHEIGHT - 20) / 2) - 60, 400, IDC_ANYHEIGHT, hwnd,
        (HMENU)IDC_DECRYPTIONPASSWORD_STATIC, GetModuleHandle(NULL),
        NULL);

    // Create Text box for password input
    CreateWindowEx(0,"EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
        (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 - 20, 200,
        IDC_ANYHEIGHT, hwnd, (HMENU)IDC_DECRYPTIONPASSWORD_EDIT,
        GetModuleHandle(NULL), NULL);

    // Create Text box for password confirmation
    CreateWindowEx(0,"EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
        (WINDOWWIDTH - 200) / 2, (WINDOWHEIGHT - 20) / 2 + 20, 200,
        IDC_ANYHEIGHT, hwnd, (HMENU)IDC_DECRYPTIONPASSWORDCONFIRM_EDIT,
        GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, "BUTTON", "Submit", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (WINDOWWIDTH - 80) / 2 - 60, ((WINDOWHEIGHT - 20) / 2 + 60), 80,
        IDC_ANYHEIGHT, hwnd, (HMENU)IDC_DECRYPTIONSUBMIT_BUTTON,
        GetModuleHandle(NULL), NULL);

    CreateWindowEx(0, "BUTTON" , "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        (WINDOWWIDTH - 80) / 2 +  60, ((WINDOWHEIGHT - 20) / 2 + 60), 80,
        IDC_ANYHEIGHT, hwnd, (HMENU)IDC_IMPORTKEYSCANCEL_BUTTON,
        GetModuleHandle(NULL), NULL);

    // Match the textbox backgrouds to the window background
    hbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
}

void destroyDecryptionPasswordInterface(HWND hwnd) {
    DestroyWindow(GetDlgItem(hwnd, IDC_DECRYPTIONPASSWORD_STATIC));
    DestroyWindow(GetDlgItem(hwnd, IDC_DECRYPTIONPASSWORD_EDIT));
    DestroyWindow(GetDlgItem(hwnd, IDC_DECRYPTIONPASSWORDCONFIRM_EDIT));
    DestroyWindow(GetDlgItem(hwnd, IDC_DECRYPTIONSUBMIT_BUTTON));
    DestroyWindow(GetDlgItem(hwnd, IDC_IMPORTKEYSCANCEL_BUTTON));
}

void destroykeyManagementInterface(HWND hwnd) {
  DestroyWindow(GetDlgItem(hwnd, IDC_BACKUP_BUTTON));
  DestroyWindow(GetDlgItem(hwnd, IDC_BACKUP_STATIC));
  DestroyWindow(GetDlgItem(hwnd, IDC_BACKUP_CHECKBOX));
  DestroyWindow(GetDlgItem(hwnd, IDC_DUALCARDS_CHECKBOX));
  DestroyWindow(GetDlgItem(hwnd, IDC_WRITEKEYS_BUTTON));
  DestroyWindow(GetDlgItem(hwnd, IDC_CONFIRM_BUTTON));
  DestroyWindow(GetDlgItem(hwnd, IDC_GETNFCKEYS_BUTTON));
  DestroyWindow(GetDlgItem(hwnd, IDC_IMPORTKEYS_BUTTON));

  DestroyWindow(GetDlgItem(hwnd, IDC_GENERATEKEYS_BUTTON));
  DestroyWindow(GetDlgItem(hwnd, IDC_PASSWORDCONFIRM_EDIT));
  DestroyWindow(GetDlgItem(hwnd, IDC_KEY1_EDIT));
  DestroyWindow(GetDlgItem(hwnd, IDC_PASSWORD_EDIT));
  DestroyWindow(GetDlgItem(hwnd, IDC_KEY2_EDIT));
  DestroyWindow(GetDlgItem(hwnd, IDC_PASSWORDCONFIRM_STATIC));
  DestroyWindow(GetDlgItem(hwnd, IDC_PASSWORD_STATIC));
  DestroyWindow(GetDlgItem(hwnd, IDC_KEY2_STATIC));
  DestroyWindow(GetDlgItem(hwnd, IDC_KEY1_STATIC));
}