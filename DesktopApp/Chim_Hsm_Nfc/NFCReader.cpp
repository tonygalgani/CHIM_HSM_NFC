#include <windows.h>
#include <iostream>
#include <string>

// Prototype declaration of the dialog procedure.
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// This function creates and displays the dialog box using the DialogBoxParam function.
void ShowPasswordDialog(HINSTANCE hInstance) {
    // Create and display the dialog box and specify the DialogProc as the message handler.
    // Replace IDD_PASSWORD_DIALOG with the actual resource ID of the dialog box.
    DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PASSWORD_DIALOG), NULL, DialogProc, 0);
}

// The dialog procedure handles messages for the dialog box.
// This includes command messages from button clicks and initialization messages.
INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: // Message sent when the dialog is initialized
            return (INT_PTR)TRUE; // Return TRUE to direct the system to set the keyboard focus.

        case WM_COMMAND: // Message sent when a control (like a button) is interacted with
            if (LOWORD(wParam) == IDOK) { // Check if OK button was pressed
                char password[100]; // Buffer to store the password input by the user
                // Get the text from the password input field (replace IDC_PASSWORD with the actual control ID)
                GetDlgItemText(hwndDlg, IDC_PASSWORD, password, sizeof(password));
                // Here you can add code to process the password, e.g., authentication checks

                EndDialog(hwndDlg, IDOK); // Close the dialog box with IDOK result
                return (INT_PTR)TRUE; // Return TRUE if the message is handled
            } else if (LOWORD(wParam) == IDCANCEL) { // Check if Cancel button was pressed
                EndDialog(hwndDlg, IDCANCEL); // Close the dialog box with IDCANCEL result
                return (INT_PTR)TRUE; // Return TRUE if the message is handled
            }
            break;
    }
    return (INT_PTR)FALSE; // Return FALSE for unhandled messages
}

// Function to initialize the serial port with specified settings
bool initializeSerialPort(HANDLE& hSerial, const char* portName) {
    // Try to open the serial port
    hSerial = CreateFile(portName,                            // Port name
                         GENERIC_READ | GENERIC_WRITE,        // Read/Write access
                         0,                                   // No sharing
                         NULL,                                // No security attributes
                         OPEN_EXISTING,                       // Opens an existing device
                         FILE_ATTRIBUTE_NORMAL,               // No special attributes
                         NULL);                               // No template file

    // Check if the handle is valid
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;  // Return false if could not open the port
    }

    DCB dcbSerialParams = {0};  // Initializing DCB structure
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    // Get the current settings of the serial port
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);  // Close handle if getting state fails
        return false;
    }

    // Configure the baud rate and other settings for the serial communication
    dcbSerialParams.BaudRate = CBR_9600;  // Set baud rate to 9600
    dcbSerialParams.ByteSize = 8;         // 8 bit data
    dcbSerialParams.StopBits = ONESTOPBIT;// One stop bit
    dcbSerialParams.Parity = NOPARITY;    // No parity bit

    // Set the new state with the configured settings
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);  // Close handle if setting state fails
        return false;
    }

    return true;  // Return true if serial port is initialized successfully
}

// Function to find the Arduino on available serial ports
std::string findArduino() {
    HANDLE hSerial = INVALID_HANDLE_VALUE;
    // Iterate over possible COM ports
    for (int i = 1; i <= 256; i++) {
        std::string portName = "\\\\.\\COM" + std::to_string(i); // Format port name
        if (initializeSerialPort(hSerial, portName.c_str())) {
            // If port is initialized, Arduino might be connected
            CloseHandle(hSerial); // Close the handle to the port
            return portName;      // Return the COM port name
        }
    }
    return "";  // Return an empty string if no Arduino found
}

int main() {
    // Find the Arduino's serial port
    std::string arduinoPort = findArduino();
    if (arduinoPort.empty()) {
        std::cerr << "Arduino not found\n";
        return 1;  // Exit if Arduino not found
    }

    std::cout << "Arduino found on port: " << arduinoPort << std::endl;

    HANDLE hSerial;
    // Initialize the serial port for communication
    if (!initializeSerialPort(hSerial, arduinoPort.c_str())) {
        std::cerr << "Failed to initialize the Arduino port\n";
        return 1;  // Exit if initialization fails
    }

// Send a character to start operations
    char initChar = 'A'; // You can change this character as needed
    DWORD bytesWritten;
    if (!WriteFile(hSerial, &initChar, 1, &bytesWritten, NULL)) {
        std::cerr << "Failed to send init character\n";
        CloseHandle(hSerial);
        return 1;
    }

    // Read data from serial until "ID =" is found
    char readBuff[256];
    DWORD bytesRead;
    bool found = false;
    std::string line;

    while (!found) {
        if (ReadFile(hSerial, readBuff, sizeof(readBuff) - 1, &bytesRead, NULL) && bytesRead != 0) {
            readBuff[bytesRead] = '\0'; // Null terminate the string
            line += readBuff;
            size_t pos = line.find("authentication keys:");
            if (pos != std::string::npos) {
                std::cout << "Found authentication keys:";
                size_t endLine = line.find('\n', pos);
                if (endLine != std::string::npos) {
                    std::cout << line.substr(pos, endLine - pos) << std::endl;
                    found = true;  // Stop reading after the ID line is found
                }
            }
        }
    }
    
if (!WriteFile(hSerial, &initChar, 1, &bytesWritten, NULL)) {
        std::cerr << "Failed to send restart character\n";
        CloseHandle(hSerial);
        return 1;
    }

// WinMain: Entry point for a Windows application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize the application and create the main window

    // Optionally, handle command line arguments with lpCmdLine

    // Show the password dialog using the instance handle
    ShowPasswordDialog(hInstance);

    // Run the message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}
    // Close the serial port handle
    CloseHandle(hSerial);
    return 0;  // Program completed successfully
}
