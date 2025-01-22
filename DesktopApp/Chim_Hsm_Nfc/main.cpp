#include "framework.h"
#include "main.h"
#include "resource.h"

 // Function to initialize console
void InitConsole() {
    // Attach to an existing console
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        // If no console to attach to, create a new one
        AllocConsole();
    }

    // Redirect std::cout and std::cerr to the console
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCout, "CONOUT$", "w", stderr);

    // Redirect std::cin to console input
    FILE* pCin;
    freopen_s(&pCin, "CONIN$", "r", stdin);

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();

    // Sync stdio with iostreams
    std::ios::sync_with_stdio(true);
}


// Entry point of the Windows application
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  // InitConsole();
  // 
  // Initialise the serial communication, exit app if the device not connected.
  if (setup_reader_serial(&passwordProtected)) {
    return true;
  }

  
  // Register the window class
  const char CLASS_NAME[] = "Sample Window Class";

  WNDCLASS wc = {};
  // Pointer to the window procedure for this window class
  wc.lpfnWndProc = WindowProc;
  // Handle to the instance that contains the window procedure
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));;
  wc.lpszClassName = CLASS_NAME;            // Name of the window class
  wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Default cursor

  RegisterClass(&wc); // Register the window class

  // Screen dimensions
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  // Calculate centered position
  int xPos = (screenWidth - WINDOWWIDTH) / 2;
  int yPos = (screenHeight - WINDOWHEIGHT) / 2;

  // Create the window
  HWND hwnd = CreateWindowEx(
      0,                          // Optional window styles, 0 for no styles
      CLASS_NAME,                 // Window class name to use
      "CHIM HSM NFC", // Window title
      WS_OVERLAPPEDWINDOW,       // Standard window style for overlapped windows
      xPos, yPos,                // Let the system decide the default position
      WINDOWWIDTH, WINDOWHEIGHT, // Let the system decide the default size
      NULL,                      // No parent window
      NULL,                      // No menus
      hInstance,                 // Instance handle
      NULL                       // No additional application data
  );

  if (hwnd == NULL) {
    return 0; // If window creation failed, exit the application
  }

  // Show the window
  ShowWindow(hwnd, nCmdShow); // Display the window on the screen

  // Run the message loop
  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) { // Get messages from the message queue
    TranslateMessage(&msg); // Translate virtual-key msgs into char msgs
    DispatchMessage(&msg);  // Send message to the window procedure
  }

  // Exit the application with the message's wParam value
  return (int)msg.wParam;
}
