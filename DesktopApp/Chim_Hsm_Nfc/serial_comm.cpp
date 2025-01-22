#include "serial_comm.h"

std::string NFCDevicePort;

std::string line;
char readBuff[256];

DWORD bytesRead;
HANDLE hSerial;
DWORD bytesWritten;
bool found = false;
bool dualCards = false;

// Faire un enum
const int KEY_LENGTH = 32;
const char WRITE_KEYS_CODE = 'e';
const char CONTINUE_PROCESS_CODE = '~';
const int TIMEOUT_SECONDS = 2;

/**
 * Initiates operations by sending a character code to the board and waits for a
 * confirmation that a card is detected, ensuring the device is ready to proceed
 * with further operations.
 *
 * @return bool Returns true if initialization fails, false if successful.
 */
bool start();
bool waitForMessage(const std::string &expectedMessage);
bool writeToFileHandle(const char *buffer, DWORD bufferSize);

// Function to find the NFC Device on available serial ports
std::string findNFCDevice() {
  HANDLE hSerial = INVALID_HANDLE_VALUE;
  // Iterate over possible COM ports
  for (int i = 1; i <= 256; i++) {
    std::string portName = "\\\\.\\COM" + std::to_string(i); // Format port name
    if (initializeSerialPort(hSerial, portName.c_str())) {
      // If port is initialized, NFC Device might be connected
      CloseHandle(hSerial); // Close the handle to the port
      return portName;      // Return the COM port name
    }
  }
  return ""; // Return an empty string if no NFC Device found
}

/**
 * Initiates operations by sending a character code to the board and waits for a
 * confirmation that a card is detected, ensuring the device is ready to proceed
 * with further operations.
 *
 * @return bool Returns true if initialization fails, false if successful.
 */
bool start() {
  //  Send a character to start operations within the board
  if (!writeToFileHandle(&CONTINUE_PROCESS_CODE, 1)) {
    // std::cerr << "Failed to send initCode\n";
    CloseHandle(hSerial);
    return true;
  }
  // Wait for confirmation from the board that the card is deteced.
  if (!waitForMessage("Found a card!")) {
    // std::cerr << "Failed to Find a card\n";
    return true;
  }
  return false; // Everyting is good.
}

// Function to initialize the serial port with specified settings
bool initializeSerialPort(HANDLE& hSerial, const char* portName) {
    // std::cout << "Creating serial file... \n";

    hSerial = CreateFile(portName,                     // Port name
        GENERIC_READ | GENERIC_WRITE, // Read/Write access
        0,                            // No sharing
        NULL,                         // No security attributes
        OPEN_EXISTING,                // Opens an existing device
        0,                            // No special attributes (using 0 here is more common)
        NULL);                        // No template file

    // Check if the handle is valid
    if (hSerial == INVALID_HANDLE_VALUE) {
        // std::cerr << "Error opening serial port: " << GetLastError() << "\n";
        return false; // Return false if could not open the port
    }

    DCB dcbSerialParams = { 0 }; // Initializing DCB structure
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    // Get the current settings of the serial port
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        // std::cerr << "Error getting current serial port state: " << GetLastError() << "\n";
        CloseHandle(hSerial); // Close handle if getting state fails
        return false;
    }

    // Configure the baud rate and other settings for the serial communication
    dcbSerialParams.BaudRate = CBR_9600;   // Set baud rate to 9600
    dcbSerialParams.ByteSize = 8;          // 8 bit data
    dcbSerialParams.StopBits = ONESTOPBIT; // One stop bit
    dcbSerialParams.Parity = NOPARITY;     // No parity bit

    // Set the new state with the configured settings
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        // std::cerr << "Error setting serial port state: " << GetLastError() << "\n";
        CloseHandle(hSerial); // Close handle if setting state fails
        return false;
    }

    // Setting timeouts (optional but recommended)
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;          // 50 ms
    timeouts.ReadTotalTimeoutConstant = 50;     // 50 ms
    timeouts.ReadTotalTimeoutMultiplier = 10;   // 10 ms per byte
    timeouts.WriteTotalTimeoutConstant = 50;    // 50 ms
    timeouts.WriteTotalTimeoutMultiplier = 10;  // 10 ms per byte

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        // std::cerr << "Error setting timeouts: " << GetLastError() << "\n";
        CloseHandle(hSerial); // Close handle if setting timeouts fails
        return false;
    }

    // std::cout << "Serial port initialized successfully.\n";
    return true; // Return true if serial port is initialized successfully
}

bool setup_reader_serial(bool *passwordProtected) {
  // Find the NFC Device's serial port
  NFCDevicePort = findNFCDevice();
  if (NFCDevicePort.empty()) {
    // std::cerr << "NFC Device not found\n";
    return true; // Exit if NFC Device not found
  }

  // std::cout << "NFC Device found on port: " << NFCDevicePort << std::endl;

  // Initialize the serial port for communication
  if (!initializeSerialPort(hSerial, NFCDevicePort.c_str())) {
    // std::cerr << "Failed to initialize the NFC Device port\n";
    return true; // Exit if initialization fails
  }

  // Send a character to start operations within the board and wait for the card
  // to be present.
  start();

  // Send 'a' code to tell the device to check password protection.
  char AdminPasswordCode = 'a';
  if (!WriteFile(hSerial, &AdminPasswordCode, 1, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send AdminPasswordCode\n";
    CloseHandle(hSerial);
    return true;
  }

  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);

  //  Read until the device returns passwordProtected value.
  line.clear();
  found = false;
  while (!found) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << "passwordProtectd=" << std::endl;
      return false;
    }
    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0'; // Null terminate the string
      line += readBuff;
      size_t pos = line.find("passwordProtected=");
      if (pos != std::string::npos) {
        // std::cout << "Found password protection value" << std::endl;
        size_t endLine = line.find('\r', pos);
        if (endLine != std::string::npos) {
          std::string value = line.substr(pos + 18, endLine - (pos + 18));
          *passwordProtected = (value == "true");
          // std::cout << "protected:" << *passwordProtected << std::endl;
          found = true; // Stop reading after this to choose key retrieving
                        // operation.
        }
      }
    }
  }

  return false;
}

bool create_admin_password(char *password) {

  // Send a character to start operations within the board and wait for the card
  // to be present.
  start();

  // Send a character to set admin password operation.
  char CreateAdminPasswordCode = 'b';
  if (!WriteFile(hSerial, &CreateAdminPasswordCode, 1, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send CreateAdminPasswordCode\n";
    CloseHandle(hSerial);
    return true;
  }

  Sleep(60);

  // Send password.
  if (!WriteFile(hSerial, password, 32, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send password\n";
    CloseHandle(hSerial);
    return true;
  }

  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);
  //  Read until the device returns password is created.
  while (!found) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << "passwordCreation=" << std::endl;
      return false;
    }
    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0'; // Null terminate the string
      line += readBuff;
      size_t pos = line.find("passwordCreation=");
      if (pos != std::string::npos) {
        // std::cout << "Found password correct value" << std::endl;
        size_t endLine = line.find('\r', pos);
        if (endLine != std::string::npos) {
          std::string value = line.substr(pos + 17, endLine - (pos + 17));
          if (value == "true")
            return true;
          else
            return false;
        }
      }
    }
  }
  return false;
}

bool admin_password_verification(char *password) {

  // Send a character to start operations within the board and wait for the card
  // to be present.
  start();

  // Send a character to set admin password operation.
  char AdminPasswordVerifCode = 'c';
  if (!WriteFile(hSerial, &AdminPasswordVerifCode, 1, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send AdminPasswordVerifCode\n";
    CloseHandle(hSerial);
    return true;
  }

  Sleep(60);

  // Send password.
  if (!WriteFile(hSerial, password, 32, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send password\n";
    CloseHandle(hSerial);
    return true;
  }

  line.clear();
  found = false;
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);
  //  Read until the device returns password is created.
  while (!found) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << "passwordCorrect=" << std::endl;
      return false;
    }

    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0'; // Null terminate the string
      line += readBuff;
     // std::cout << line << std::endl;
      size_t pos = line.find("passwordCorrect=");
      if (pos != std::string::npos) {
        // std::cout << "Found password correct value" << std::endl;
        size_t endLine = line.find('\r', pos);
        if (endLine != std::string::npos) {
          std::string value = line.substr(pos + 16, endLine - (pos + 16));
          // std::cout << line.substr(pos, endLine - pos) << std::endl;
          if (value == "true")
            return false;
          else
            return true;
        }
      }
    }
  }
  return false;
}

bool keyRecovery(std::string *key1, std::string *key2) {

  // Send a character to start operations within the board and wait for the card
  // to be present.
  start();

  // Send a character to set admin password operation.
  char keyRecoveryEEPROMnfcCode = 'd';
  if (!WriteFile(hSerial, &keyRecoveryEEPROMnfcCode, 1, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send keyRecoveryEEPROMnfcCode\n";
    CloseHandle(hSerial);
    return true;
  }

  Sleep(60);

  line.clear();
  found = false;
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);
  //  Read until the device returns password is created.
  while (!found) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << "DualCards=" << std::endl;
      return false;
    }

    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0'; // Null terminate the string
      line += readBuff;
      // std::cout << line << std::endl;
      size_t pos = line.find("DualCards=");
      if (pos != std::string::npos) {
        // std::cout << "Found keys" << std::endl;
        size_t endLine = line.find('\r', pos);
        if (endLine != std::string::npos) {
          std::string value = line.substr(pos + 10, endLine - (pos + 10));
          if (value == "true")
            dualCards = true;
          else
            dualCards = false;
          found = true;
        }
      }
    }
  }
  int secondCardPlaced;
  // std::cout << "dualcards:" << dualCards << std::endl;
  if (dualCards) {
    secondCardPlaced = MessageBox(NULL,
                                  "Dual cards detected.\n\rPlace the second "
                                  "card on reader and click ok.",
                                  "", MB_OK);

    while (secondCardPlaced != IDOK)
      ;
  }

  char continueProcessCode = '~';
  if (!WriteFile(hSerial, &continueProcessCode, 1, &bytesWritten, NULL)) {
    // std::cerr << "Failed to send continueProcessCode\n";
    CloseHandle(hSerial);
    return true;
  }
  line.clear();
  found = false;
  start_time = std::chrono::steady_clock::now();
  timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);
  //  Read until the device returns the received keys
  while (!found) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << "Key segments: " << std::endl;
      return false;
    }
    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0'; // Null terminate the string
      line += readBuff;
      // std::cout << line << std::endl;
      size_t pos = line.find("Key segments: ");
      if (pos != std::string::npos) {
        // std::cout << "Found keys" << std::endl;
        size_t endLine = line.find('\r', pos);
        if (endLine != std::string::npos) {
          *key1 = line.substr(pos + 14, 32);
          *key2 = line.substr(pos + 46, 32);
          // std::cout << "key1:" << *key1 << std::endl;
          // std::cout << "key2:" << *key2 << std::endl;
          found = true;
        }
      }
    }
  }
  return false;
}

bool writeKeys(char *key1, char *key2, char dualCards) {
  start();

  if (!writeToFileHandle(&WRITE_KEYS_CODE, 1)) {
    // std::cerr << "Failed to send writeKeysCode\n";
    CloseHandle(hSerial);
    return true;
  }

  Sleep(60);

  if (!writeToFileHandle(&dualCards, 1)) {
    // std::cerr << "Failed to send dual card flag\n";
    return true;
  }

  if (!writeToFileHandle(key1, KEY_LENGTH) ||
      !writeToFileHandle(key2, KEY_LENGTH)) {
    // std::cerr << "Failed to send keys\n";
    CloseHandle(hSerial);
    return true;
  }

  if (!waitForMessage("First card written."))
    return true;

  if (dualCards == '1') {
    MessageBox(NULL, "Place the second card on reader and click OK.", "",
               MB_OK);
    if (!writeToFileHandle(&CONTINUE_PROCESS_CODE, 1)) {
      // std::cerr << "Failed to send continueProcessCode\n";
      CloseHandle(hSerial);
      return true;
    }
    waitForMessage("Second card written.");
    MessageBox(NULL, "Both cards written", "Successful", MB_OK);
  } else {
    waitForMessage("EEPROM written.");
    MessageBox(NULL, "Card and EEPROM written", "Successful", MB_OK);
  }

  return false;
}

bool waitForMessage(const std::string &expectedMessage) {
  std::string line;
  char readBuff[256];
  DWORD bytesRead;
  auto start_time = std::chrono::steady_clock::now();
  auto timeout_duration = std::chrono::seconds(TIMEOUT_SECONDS);

  while (true) {
    if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
      // std::cerr << "Operation timed out waiting for: " << expectedMessage  << std::endl;
      return false;
    }

    if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) &&
        bytesRead != 0) {
      readBuff[bytesRead] = '\0';
      line += readBuff;
      // std::cout << line << std::endl;
      size_t pos = line.find(expectedMessage);
      if (pos != std::string::npos) {
        // std::cout << "Message received: " << expectedMessage << std::endl;
        return true;
      }
    }
  }
}

bool writeToFileHandle(const char *buffer, DWORD bufferSize) {
  DWORD bytesWritten;
  return WriteFile(hSerial, buffer, bufferSize, &bytesWritten, NULL) &&
         bytesWritten == bufferSize;
}
//  line.clear();
//   found = false;
//   // Read until the device informs the second card has been detected.
//   while (!found) {
//     if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL)
//     &&
//         bytesRead != 0) {
//       readBuff[bytesRead] = '\0'; // Null terminate the string
//       line += readBuff;
//       std::cout << line << std::endl;
//       size_t pos = line.find("Kthxbye.");
//       if (pos != std::string::npos) {

//         size_t endLine = line.find('\r', pos);
//         if (endLine != std::string::npos) {
//           found = true;
//         }
//       }
//     }
//   }