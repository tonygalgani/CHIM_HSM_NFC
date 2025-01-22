#pragma once

#include <chrono>
#include <iostream>
#include <ostream>
#include <string>
#include <windows.h>


// Serial communication prototypes
bool initializeSerialPort(HANDLE &hSerial, const char *portName);
bool setup_reader_serial(bool *passwordProtected);
std::string findNFCDevice();
bool create_admin_password(char *password);
bool admin_password_verification(char *password);
bool keyRecovery(std::string *key1, std::string *key2);
bool writeKeys(char *key1, char *key2, char dualCards);
