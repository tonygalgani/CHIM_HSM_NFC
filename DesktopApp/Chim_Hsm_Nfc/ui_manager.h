#pragma once

#include "key_manager.h"
#include "serial_comm.h"

// Textbox IDs
#define IDC_KEY1_EDIT 101
#define IDC_KEY2_EDIT 102
#define IDC_PASSWORD_EDIT 103
#define IDC_PASSWORDCONFIRM_EDIT 104
#define IDC_ADMINPASSWORD_EDIT 105
#define IDC_ADMINPASSWORDCONFIRM_EDIT 106
#define IDC_ENCRYPTIONPASSWORD_EDIT 107
#define IDC_ENCRYPTIONPASSWORDCONFIRM_EDIT 108
#define IDC_DECRYPTIONPASSWORD_EDIT 109
#define IDC_DECRYPTIONPASSWORDCONFIRM_EDIT 110

// Button IDs
#define IDC_GENERATEKEYS_BUTTON 201
#define IDC_IMPORTKEYS_BUTTON 202
#define IDC_GETNFCKEYS_BUTTON 203
#define IDC_CONFIRM_BUTTON 204
#define IDC_SUBMIT_BUTTON 205
#define IDC_WRITEKEYS_BUTTON 206
#define IDC_BACKUP_BUTTON 207
#define IDC_ENCRYPTIONSUBMIT_BUTTON 208
#define IDC_DECRYPTIONSUBMIT_BUTTON 209
#define IDC_IMPORTKEYSCANCEL_BUTTON 210
#define IDC_EXPORTKEYSCANCEL_BUTTON 211

// Checkbox IDs
#define IDC_BACKUP_CHECKBOX 301
#define IDC_DUALCARDS_CHECKBOX 302

// Text IDs
#define IDC_ADMINPASSWORD_STATIC 401
#define IDC_BACKUP_STATIC 402
#define IDC_KEY1_STATIC 403
#define IDC_KEY2_STATIC 404
#define IDC_PASSWORD_STATIC 405
#define IDC_PASSWORDCONFIRM_STATIC 406
#define IDC_ENCRYPTIONPASSWORD_STATIC 407
#define IDC_DECRYPTIONPASSWORD_STATIC 408

// Windows' dimensions
#define WINDOWWIDTH 500
#define WINDOWHEIGHT 295
// Components' dimensions
#define IDC_ANYHEIGHT 20
#define IDC_TEXTBOXWIDTH 200
#define IDC_LABELWIDTH 80
#define IDC_BUTTONWIDTH 150
#define IDC_TEXTBOXDECALAGE 50
#define IDC_LABELDECALAGE 200
#define IDC_BUTTONDECALAGE 140

// main prototypes:
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void createAdminInterface(HWND hwnd, HBRUSH hdrBackground);
void keyManagementInterface(HWND hwnd, HBRUSH hbrBackground);
void encryptionPasswordInterface(HWND hwnd, HBRUSH hbrBackground);
void decryptionPasswordInterface(HWND hwnd, HBRUSH hbrBackground);
void destroykeyManagementInterface(HWND hwnd);
void destroyEncryptionPasswordInterface(HWND hwnd);
void destroyDecryptionPasswordInterface(HWND hwnd);

bool encryption(std::string passphrase, std::string keys, std::string& encodedSalt, std::string& encodedKeys);
bool decryption(std::string passphrase, std::string encodedSalt, std::string encodedKeys, std::string& decryptedKeys);