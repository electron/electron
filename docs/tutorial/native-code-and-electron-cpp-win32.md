# Native Code and Electron: C++ (Windows)

This tutorial builds on the [general introduction to Native Code and Electron](./native-code-and-electron.md) and focuses on creating a native addon for Windows using C++ and the [Win32 API](https://learn.microsoft.com/en-us/windows/win32/). To illustrate how you can embed native Win32 code in your Electron app, we'll be building a basic native Windows GUI (using the Windows Common Controls) that communicates with Electron's JavaScript.

Specifically, we'll be integrating with two commonly used native Windows libraries:

* `comctl32.lib`, which contains common controls and user interface components. It provides various UI elements like buttons, scrollbars, toolbars, status bars, progress bars, and tree views. As far as GUI development on Windows goes, this library is very low-level and basic - more modern frameworks like WinUI or WPF are advanced and alternatives but require a lot more C++ and Windows version considerations than are useful for this tutorial. This way, we can avoid the many perils of building native interfaces for multiple Windows versions!
* `shcore.lib`, a library that provides high-DPI awareness functionality and other Shell-related features around managing displays and UI elements.

This tutorial will be most useful to those who already have some familiarity with native C++ GUI development on Windows. You should have experience with basic window classes and procedures, like `WNDCLASSEXW` and `WindowProc` functions. You should also be familiar with the Windows message loop, which is the heart of any native application - our code will be using `GetMessage`, `TranslateMessage`, and `DispatchMessage` to handle messages. Lastly, we'll be using (but not explaining) standard Win32 controls like `WC_EDITW` or `WC_BUTTONW`.

> [!NOTE]
> If you're not familiar with C++ GUI development on Windows, we recommend Microsoft's excellent documentation and guides, particular for beginners. "[Get Started with Win32 and C++](https://learn.microsoft.com/en-us/windows/win32/learnwin32/learn-to-program-for-windows)" is a great introduction.

## Requirements

Just like our [general introduction to Native Code and Electron](./native-code-and-electron.md), this tutorial assumes you have Node.js and npm installed, as well as the basic tools necessary for compiling native code. Since this tutorial discusses writing native code that interacts with Windows, we recommend that you follow this tutorial on Windows with both Visual Studio and the "Desktop development with C++ workload" installed. For details, see the [Visual Studio Installation instructions](https://learn.microsoft.com/en-us/visualstudio/install/install-visual-studio).

## 1) Creating a package

You can re-use the package we created in our [Native Code and Electron](./native-code-and-electron.md) tutorial. This tutorial will not be repeating the steps described there. Let's first setup our basic addon folder structure:

```txt
my-native-win32-addon/
├── binding.gyp
├── include/
│   └── cpp_code.h
├── js/
│   └── index.js
├── package.json
└── src/
    ├── cpp_addon.cc
    └── cpp_code.cc
```

Our `package.json` should look like this:

```json title='package.json'
{
  "name": "cpp-win32",
  "version": "1.0.0",
  "description": "A demo module that exposes C++ code to Electron",
  "main": "js/index.js",
  "author": "Your Name",
  "scripts": {
    "clean": "rm -rf build_swift && rm -rf build",
    "build-electron": "electron-rebuild",
    "build": "node-gyp configure && node-gyp build"
  },
  "license": "MIT",
  "dependencies": {
    "bindings": "^1.5.0",
    "node-addon-api": "^8.3.0"
  }
}
```

## 2) Setting Up the Build Configuration

For a Windows-specific addon, we need to modify our `binding.gyp` file to include Windows libraries and set appropriate compiler flags. In short, we need to do the following three things:

1. We need to ensure our addon is only compiled on Windows, since we'll be writing platform-specific code.
2. We need to include the Windows-specific libraries. In our tutorial, we'll be targeting `comctl32.lib` and `shcore.lib`.
3. We need to configure the compiler and define C++ macros.

```json title='binding.gyp'
{
  "targets": [
    {
      "target_name": "cpp_addon",
      "conditions": [
        ['OS=="win"', {
          "sources": [
            "src/cpp_addon.cc",
            "src/cpp_code.cc"
          ],
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "include"
          ],
          "libraries": [
            "comctl32.lib",
            "shcore.lib"
          ],
          "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "DebugInformationFormat": "OldStyle",
              "AdditionalOptions": [
                "/FS"
              ]
            },
            "VCLinkerTool": {
              "GenerateDebugInformation": "true"
            }
          },
          "defines": [
            "NODE_ADDON_API_CPP_EXCEPTIONS",
            "WINVER=0x0A00",
            "_WIN32_WINNT=0x0A00"
          ]
        }]
      ]
    }
  ]
}
```

If you're curious about the details about this config, you can read on - otherwise, feel free to just copy them and move on to the next step, where we define the C++ interface.

### Microsoft Visual Studio Build Configurations

`msvs_settings` provide Visual Studio-specific settings.

#### `VCCLCompilerTool` Settings

```json title='binding.gyp'
"VCCLCompilerTool": {
  "ExceptionHandling": 1,
  "DebugInformationFormat": "OldStyle",
  "AdditionalOptions": [
    "/FS"
  ]
}
```

* `ExceptionHandling: 1`: This enables C++ exception handling with the /EHsc compiler flag. This is important because it enables the compiler to catch C++ exceptions, ensures proper stack unwinding when exceptions occur, and is required for Node-API to properly handle exceptions between JavaScript and C++.
* `DebugInformationFormat: "OldStyle"`: This specifies the format of debugging information, using the older, more compatible PDB (Program Database) format. This supports compatibility with various debugging tools and works better with incremental builds.
* `AdditionalOptions: ["/FS"]`: This adds the File Serialization flag, forcing serialized access to PDB files during compilation. It prevents build errors in parallel builds where multiple compiler processes try to access the same PDB file.

#### `VCLinkerTool` Settings

```json title='binding.gyp'
"VCLinkerTool": {
  "GenerateDebugInformation": "true"
}
```

* `GenerateDebugInformation: "true"`: This tells the linker to include debug information, which allows source-level debugging in tools that use symbols. Most importantly, this will allow us to get human-readable stack traces if the addon crashes.

### Preprocessor macros (`defines`):

* `NODE_ADDON_API_CPP_EXCEPTIONS`: This macro enables C++ exception handling in the Node Addon API. By default, Node-API uses a return-value error handling pattern, but this define allows the C++ wrapper to throw and catch C++ exceptions, which makes the code more idiomatic C++ and easier to work with.
* `WINVER=0x0A00`: This defines the minimum Windows version that the code is targeting. The value `0x0A00` corresponds to Windows 10. Setting this tells the compiler that the code can use features available in Windows 10, and it won't attempt to maintain backward compatibility with earlier Windows versions. Make sure to set this to the lowest version of Windows you intend to support with your Electron app.
* `_WIN32_WINNT=0x0A00` - Similar to `WINVER`, this defines the minimum version of the Windows NT kernel that the code will run on. Again, 0x0A00 corresponds to Windows 10. This is commonly set to the same value as `WINVER`.

## 3) Defining the C++ Interface

Let's define our header in `include/cpp_code.h`:

```cpp title='include/cpp_code.h'
#pragma once
#include <string>
#include <functional>

namespace cpp_code {

std::string hello_world(const std::string& input);
void hello_gui();

// Callback function types
using TodoCallback = std::function<void(const std::string&)>;

// Callback setters
void setTodoAddedCallback(TodoCallback callback);

} // namespace cpp_code
```

This header:

* Includes the basic `hello_world` function from the general tutorial
* Adds a `hello_gui` function to create a Win32 GUI
* Defines callback types for Todo operations (add). To keep this tutorial somewhat brief, we'll only be implementing one callback.
* Provides setter functions for these callbacks

## 4) Implementing Win32 GUI Code

Now, let's implement our Win32 GUI in `src/cpp_code.cc`. This is a larger file, so we'll review it in sections. First, let's include necessary headers and define basic structures.

```cpp title='src/cpp_code.cc'
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <thread>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using TodoCallback = std::function<void(const std::string &)>;

static TodoCallback g_todoAddedCallback;

struct TodoItem
{
  GUID id;
  std::wstring text;
  int64_t date;

  std::string toJson() const
  {
    OLECHAR *guidString;
    StringFromCLSID(id, &guidString);
    std::wstring widGuid(guidString);
    CoTaskMemFree(guidString);

    // Convert wide string to narrow for JSON
    std::string guidStr(widGuid.begin(), widGuid.end());
    std::string textStr(text.begin(), text.end());

    return "{"
           "\"id\":\"" + guidStr + "\","
           "\"text\":\"" + textStr + "\","
           "\"date\":" + std::to_string(date) +
           "}";
  }
};

namespace cpp_code
{
  // More code to follow later...
}
```

In this section:

* We include necessary Win32 headers
* We set up pragma comments to link against required libraries
* We define callback variables for Todo operations
* We create a `TodoItem` struct with a method to convert to JSON

Next, let's implement the basic functions and helper methods:

```cpp title='src/cpp_code.cc'
namespace cpp_code
{
  std::string hello_world(const std::string &input)
  {
    return "Hello from C++! You said: " + input;
  }

  void setTodoAddedCallback(TodoCallback callback)
  {
    g_todoAddedCallback = callback;
  }

  // Window procedure function that handles window messages
  // hwnd: Handle to the window
  // uMsg: Message code
  // wParam: Additional message-specific information
  // lParam: Additional message-specific information
  LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  // Helper function to scale a value based on DPI
  int Scale(int value, UINT dpi)
  {
    return MulDiv(value, dpi, 96); // 96 is the default DPI
  }

  // Helper function to convert SYSTEMTIME to milliseconds since epoch
  int64_t SystemTimeToMillis(const SYSTEMTIME &st)
  {
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart - 116444736000000000ULL) / 10000;
  }

  // More code to follow later...
}
```

In this section, we've added a function that allows us to set the callback for an added todo item. We also added two helper functions that we need when working with JavaScript: One to scale our UI elements depending on the display's DPI - and another one to convert a Windows `SYSTEMTIME` to milliseconds since epoch, which is how JavaScript keeps track of time.

Now, let's get to the part you probably came to this tutorial for - creating a GUI thread and drawing native pixels on screen.  We'll do that by adding a `void hello_gui()` function to our `cpp_code` namespace. There are a few considerations we need to make:

* We need to create a new thread for the GUI to avoid blocking the Node.js event loop. The Windows message loop that processes GUI events runs in an infinite loop, which would prevent Node.js from processing other events if run on the main thread. By running the GUI on a separate thread, we allow both the native Windows interface and Node.js to remain responsive. This separation also helps prevent potential deadlocks that could occur if GUI operations needed to wait for JavaScript callbacks. You don't need to do that for simpler Windows API interactions - but since you need to check the message loop, you do need to setup your own thread for GUI.
* Then, within our thread, we need to run a message loop to handle any Windows messages.
* We need to setup DPI awareness for proper display scaling.
* We need to register a window class, create a window, and add various UI controls.

In the code below, we haven't added any actual controls yet. We're doing that on purpose to look at our added code in smaller portions here.

```cpp title='src/cpp_code.cc'
void hello_gui() {
  // Launch GUI in a separate thread
  std::thread guiThread([]() {
    // Enable Per-Monitor DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"TodoApp";
    RegisterClassExW(&wc);

    // Get the DPI for the monitor
    UINT dpi = GetDpiForSystem();

    // Create window
    HWND hwnd = CreateWindowExW(
      0, L"TodoApp", L"Todo List",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      Scale(500, dpi), Scale(500, dpi),
      nullptr, nullptr,
      GetModuleHandle(nullptr), nullptr
    );

    if (hwnd == nullptr) {
      return;
    }

    // Controls go here! The window is currently empty,
    // we'll add controls in the next step.

    ShowWindow(hwnd, SW_SHOW);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // Clean up
    DeleteObject(hFont);
  });

  // Detach the thread so it runs independently
  guiThread.detach();
}
```

Now that we have a thread, a window, and a message loop, we can add some controls. Nothing we're doing here is unique to writing Windows C++ for Electron - you can simply copy & paste the code below into the `Controls go here!` section inside our `hello_gui()` function.

We're specifically adding buttons, a date picker, and a list.

```cpp title='src/cpp_code.cc'
void hello_gui() {
    // ...
    // All the code above "Controls go here!"

    // Create the modern font with DPI-aware size
    HFONT hFont = CreateFontW(
      -Scale(14, dpi),              // Height (scaled)
      0,                            // Width
      0,                            // Escapement
      0,                            // Orientation
      FW_NORMAL,                    // Weight
      FALSE,                        // Italic
      FALSE,                        // Underline
      FALSE,                        // StrikeOut
      DEFAULT_CHARSET,              // CharSet
      OUT_DEFAULT_PRECIS,           // OutPrecision
      CLIP_DEFAULT_PRECIS,          // ClipPrecision
      CLEARTYPE_QUALITY,            // Quality
      DEFAULT_PITCH | FF_DONTCARE,  // Pitch and Family
      L"Segoe UI"                   // Font face name
    );

    // Create input controls with scaled positions and sizes
    HWND hEdit = CreateWindowExW(0, WC_EDITW, L"",
      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
      Scale(10, dpi), Scale(10, dpi),
      Scale(250, dpi), Scale(25, dpi),
      hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Create date picker
    HWND hDatePicker = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"",
      WS_CHILD | WS_VISIBLE | DTS_SHORTDATECENTURYFORMAT,
      Scale(270, dpi), Scale(10, dpi),
      Scale(100, dpi), Scale(25, dpi),
      hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
    SendMessageW(hDatePicker, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hButton = CreateWindowExW(0, WC_BUTTONW, L"Add",
      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
      Scale(380, dpi), Scale(10, dpi),
      Scale(50, dpi), Scale(25, dpi),
      hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
    SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hListBox = CreateWindowExW(0, WC_LISTBOXW, L"",
      WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
      Scale(10, dpi), Scale(45, dpi),
      Scale(460, dpi), Scale(400, dpi),
      hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
    SendMessageW(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Store menu handle in window's user data
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hContextMenu);

    // All the code below "Controls go here!"
    // ...
}
```

Now that we have a user interface that allows users to add todos, we need to store them - and add a helper function that'll potentially call our JavaScript callback. Right below the `void hello_gui() { ... }` function, we'll add the following:

```cpp title='src/cpp_code.cc'
  // Global vector to store todos
  static std::vector<TodoItem> g_todos;

  void NotifyCallback(const TodoCallback &callback, const std::string &json)
  {
    if (callback)
    {
      callback(json);
      // Process pending messages
      MSG msg;
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
```

We'll also need a function that turns a todo into something we can display. We don't need anything fancy - given the name of the todo and a `SYSTEMTIME` timestamp, we'll return a simple string. Add it right below the function above:

```cpp title='src/cpp_code.cc'
  std::wstring FormatTodoDisplay(const std::wstring &text, const SYSTEMTIME &st)
  {
    wchar_t dateStr[64];
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, dateStr, 64);
    return text + L" - " + dateStr;
  }
```

When a user adds a todo, we want to reset the controls back to an empty state. To do so, add a helper function below the code we just added:

```cpp title='src/cpp_code.cc'
  void ResetControls(HWND hwnd)
  {
    HWND hEdit = GetDlgItem(hwnd, 1);
    HWND hDatePicker = GetDlgItem(hwnd, 4);
    HWND hAddButton = GetDlgItem(hwnd, 2);

    // Clear text
    SetWindowTextW(hEdit, L"");

    // Reset date to current
    SYSTEMTIME currentTime;
    GetLocalTime(&currentTime);
    DateTime_SetSystemtime(hDatePicker, GDT_VALID, &currentTime);
  }
```

Then, we'll need to implement the window procedure to handle Windows messages. Like a lot of our code here, there is very little specific to Electron in this code - so as a Win32 C++ developer, you'll recognize this function. The only thing that is unique is that we want to potentially notify the JavaScript callback about an added todo. We've previously implemented the `NotifyCallback()` function, which we will be using here. Add this code right below the function above:

```cpp title='src/cpp_code.cc'
  LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg)
    {
      case WM_COMMAND:
      {
        HWND hListBox = GetDlgItem(hwnd, 3);
        int cmd = LOWORD(wParam);

        switch (cmd)
        {
          case 2: // Add button
          {
            wchar_t buffer[256];
            GetDlgItemTextW(hwnd, 1, buffer, 256);

            if (wcslen(buffer) > 0)
            {
              SYSTEMTIME st;
              HWND hDatePicker = GetDlgItem(hwnd, 4);
              DateTime_GetSystemtime(hDatePicker, &st);

              TodoItem todo;
              CoCreateGuid(&todo.id);
              todo.text = buffer;
              todo.date = SystemTimeToMillis(st);

              g_todos.push_back(todo);

              std::wstring displayText = FormatTodoDisplay(buffer, st);
              SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());

              ResetControls(hwnd);
              NotifyCallback(g_todoAddedCallback, todo.toJson());
            }
            break;
          }
        }
        break;
      }

      case WM_DESTROY:
      {
        PostQuitMessage(0);
        return 0;
      }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
```

We now have successfully implemented the Win32 C++ code. Most of this should look and feel to you like code you'd write with or without Electron. In the next step, we'll be building the bridge between C++ and JavaScript. Here's the complete implementation:

```cpp title='src/cpp_code.cc'
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <thread>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using TodoCallback = std::function<void(const std::string &)>;

static TodoCallback g_todoAddedCallback;
static TodoCallback g_todoUpdatedCallback;
static TodoCallback g_todoDeletedCallback;

struct TodoItem
{
  GUID id;
  std::wstring text;
  int64_t date;

  std::string toJson() const
  {
    OLECHAR *guidString;
    StringFromCLSID(id, &guidString);
    std::wstring widGuid(guidString);
    CoTaskMemFree(guidString);

    // Convert wide string to narrow for JSON
    std::string guidStr(widGuid.begin(), widGuid.end());
    std::string textStr(text.begin(), text.end());

    return "{"
           "\"id\":\"" + guidStr + "\","
           "\"text\":\"" + textStr + "\","
           "\"date\":" + std::to_string(date) +
           "}";
  }
};

namespace cpp_code
{

  std::string hello_world(const std::string &input)
  {
    return "Hello from C++! You said: " + input;
  }

  void setTodoAddedCallback(TodoCallback callback)
  {
    g_todoAddedCallback = callback;
  }

  void setTodoUpdatedCallback(TodoCallback callback)
  {
    g_todoUpdatedCallback = callback;
  }

  void setTodoDeletedCallback(TodoCallback callback)
  {
    g_todoDeletedCallback = callback;
  }

  LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  // Helper function to scale a value based on DPI
  int Scale(int value, UINT dpi)
  {
    return MulDiv(value, dpi, 96); // 96 is the default DPI
  }

  // Helper function to convert SYSTEMTIME to milliseconds since epoch
  int64_t SystemTimeToMillis(const SYSTEMTIME &st)
  {
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return (uli.QuadPart - 116444736000000000ULL) / 10000;
  }

  void ResetControls(HWND hwnd)
  {
    HWND hEdit = GetDlgItem(hwnd, 1);
    HWND hDatePicker = GetDlgItem(hwnd, 4);
    HWND hAddButton = GetDlgItem(hwnd, 2);

    // Clear text
    SetWindowTextW(hEdit, L"");

    // Reset date to current
    SYSTEMTIME currentTime;
    GetLocalTime(&currentTime);
    DateTime_SetSystemtime(hDatePicker, GDT_VALID, &currentTime);
  }

  void hello_gui() {
    // Launch GUI in a separate thread
    std::thread guiThread([]() {
      // Enable Per-Monitor DPI awareness
      SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

      // Initialize Common Controls
      INITCOMMONCONTROLSEX icex;
      icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
      icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
      InitCommonControlsEx(&icex);

      // Register window class
      WNDCLASSEXW wc = {};
      wc.cbSize = sizeof(WNDCLASSEXW);
      wc.lpfnWndProc = WindowProc;
      wc.hInstance = GetModuleHandle(nullptr);
      wc.lpszClassName = L"TodoApp";
      RegisterClassExW(&wc);

      // Get the DPI for the monitor
      UINT dpi = GetDpiForSystem();

      // Create window
      HWND hwnd = CreateWindowExW(
        0, L"TodoApp", L"Todo List",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        Scale(500, dpi), Scale(500, dpi),
        nullptr, nullptr,
        GetModuleHandle(nullptr), nullptr
      );

      if (hwnd == nullptr) {
        return;
      }

      // Create the modern font with DPI-aware size
      HFONT hFont = CreateFontW(
        -Scale(14, dpi),              // Height (scaled)
        0,                            // Width
        0,                            // Escapement
        0,                            // Orientation
        FW_NORMAL,                    // Weight
        FALSE,                        // Italic
        FALSE,                        // Underline
        FALSE,                        // StrikeOut
        DEFAULT_CHARSET,              // CharSet
        OUT_DEFAULT_PRECIS,           // OutPrecision
        CLIP_DEFAULT_PRECIS,          // ClipPrecision
        CLEARTYPE_QUALITY,            // Quality
        DEFAULT_PITCH | FF_DONTCARE,  // Pitch and Family
        L"Segoe UI"                   // Font face name
      );

      // Create input controls with scaled positions and sizes
      HWND hEdit = CreateWindowExW(0, WC_EDITW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        Scale(10, dpi), Scale(10, dpi),
        Scale(250, dpi), Scale(25, dpi),
        hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
      SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

      // Create date picker
      HWND hDatePicker = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | DTS_SHORTDATECENTURYFORMAT,
        Scale(270, dpi), Scale(10, dpi),
        Scale(100, dpi), Scale(25, dpi),
        hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
      SendMessageW(hDatePicker, WM_SETFONT, (WPARAM)hFont, TRUE);

      HWND hButton = CreateWindowExW(0, WC_BUTTONW, L"Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        Scale(380, dpi), Scale(10, dpi),
        Scale(50, dpi), Scale(25, dpi),
        hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
      SendMessageW(hButton, WM_SETFONT, (WPARAM)hFont, TRUE);

      HWND hListBox = CreateWindowExW(0, WC_LISTBOXW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        Scale(10, dpi), Scale(45, dpi),
        Scale(460, dpi), Scale(400, dpi),
        hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
      SendMessageW(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

      ShowWindow(hwnd, SW_SHOW);

      // Message loop
      MSG msg = {};
      while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

      // Clean up
      DeleteObject(hFont);
    });

    // Detach the thread so it runs independently
    guiThread.detach();
  }

  // Global vector to store todos
  static std::vector<TodoItem> g_todos;

  void NotifyCallback(const TodoCallback &callback, const std::string &json)
  {
    if (callback)
    {
      callback(json);
      // Process pending messages
      MSG msg;
      while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }

  std::wstring FormatTodoDisplay(const std::wstring &text, const SYSTEMTIME &st)
  {
    wchar_t dateStr[64];
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, dateStr, 64);
    return text + L" - " + dateStr;
  }

  LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg)
    {
      case WM_COMMAND:
      {
        HWND hListBox = GetDlgItem(hwnd, 3);
        int cmd = LOWORD(wParam);

        switch (cmd)
        {
          case 2: // Add button
          {
            wchar_t buffer[256];
            GetDlgItemTextW(hwnd, 1, buffer, 256);

            if (wcslen(buffer) > 0)
            {
              SYSTEMTIME st;
              HWND hDatePicker = GetDlgItem(hwnd, 4);
              DateTime_GetSystemtime(hDatePicker, &st);

              TodoItem todo;
              CoCreateGuid(&todo.id);
              todo.text = buffer;
              todo.date = SystemTimeToMillis(st);

              g_todos.push_back(todo);

              std::wstring displayText = FormatTodoDisplay(buffer, st);
              SendMessageW(hListBox, LB_ADDSTRING, 0, (LPARAM)displayText.c_str());

              ResetControls(hwnd);
              NotifyCallback(g_todoAddedCallback, todo.toJson());
            }
            break;
          }
        }
        break;
      }

      case WM_DESTROY:
      {
        PostQuitMessage(0);
        return 0;
      }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }

} // namespace cpp_code
```

## 5) Creating the Node.js Addon Bridge

Now let's implement the bridge between our C++ code and Node.js in `src/cpp_addon.cc`. Let's start by creating a basic skeleton for our addon:

```cpp title='src/cpp_addon.cc'
#include <napi.h>
#include <string>
#include "cpp_code.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // We'll add code here later
    return exports;
}

NODE_API_MODULE(cpp_addon, Init)
```

This is the minimal structure required for a Node.js addon using `node-addon-api`. The `Init` function is called when the addon is loaded, and the `NODE_API_MODULE` macro registers our initializer.

### Create a Class to Wrap Our C++ Code

Let's create a class that will wrap our C++ code and expose it to JavaScript:

```cpp title='src/cpp_addon.cc'
#include <napi.h>
#include <string>
#include "cpp_code.h"

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "CppWin32Addon", {
            // We'll add methods here later
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("CppWin32Addon", func);
        return exports;
    }

    CppAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<CppAddon>(info) {
        // Constructor logic will go here
    }

private:
    // Will add private members and methods later
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return CppAddon::Init(env, exports);
}

NODE_API_MODULE(cpp_addon, Init)
```

This creates a class that inherits from `Napi::ObjectWrap`, which allows us to wrap our C++ object for use in JavaScript. The `Init` function sets up the class and exports it to JavaScript.

### Implement Basic Functionality - HelloWorld

Now let's add our first method, the `HelloWorld` function:

```cpp title='src/cpp_addon.cc'
// ... previous code

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "CppWin32Addon", {
            InstanceMethod("helloWorld", &CppAddon::HelloWorld),
        });

        // ... rest of Init function
    }

    CppAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<CppAddon>(info) {
        // Constructor logic will go here
    }

private:
    Napi::Value HelloWorld(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
            return env.Null();
        }

        std::string input = info[0].As<Napi::String>();
        std::string result = cpp_code::hello_world(input);

        return Napi::String::New(env, result);
    }
};

// ... rest of the file
```

This adds the `HelloWorld` method to our class and registers it with `DefineClass`. The method validates inputs, calls our C++ function, and returns the result to JavaScript.

```cpp title='src/cpp_addon.cc'
// ... previous code

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "CppWin32Addon", {
            InstanceMethod("helloWorld", &CppAddon::HelloWorld),
            InstanceMethod("helloGui", &CppAddon::HelloGui),
        });

        // ... rest of Init function
    }

    // ... constructor

private:
    // ... HelloWorld method

    void HelloGui(const Napi::CallbackInfo& info) {
        cpp_code::hello_gui();
    }
};

// ... rest of the file
```

This simple method calls our `hello_gui` function from the C++ code, which launches the Win32 GUI window in a separate thread.

### Setting Up the Event System

Now comes the complex part - setting up the event system so our C++ code can call back to JavaScript. We need to:

1. Add private members to store callbacks
2. Create a threadsafe function for cross-thread communication
3. Add an `On` method to register JavaScript callbacks
4. Set up C++ callbacks that will trigger the JavaScript callbacks

```cpp title='src/cpp_addon.cc'
// ... previous code

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    // ... previous public methods

private:
    Napi::Env env_;
    Napi::ObjectReference emitter;
    Napi::ObjectReference callbacks;
    napi_threadsafe_function tsfn_;

    // ... existing private methods
};

// ... rest of the file
```

Now, let's enhance our constructor to initialize these members:

```cpp title='src/cpp_addon.cc'
// ... previous code

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    // CallbackData struct to pass data between threads
    struct CallbackData {
        std::string eventType;
        std::string payload;
        CppAddon* addon;
    };

    CppAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<CppAddon>(info)
        , env_(info.Env())
        , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
        , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
        , tsfn_(nullptr) {

        // We'll add threadsafe function setup here in the next step
    }

    // Add destructor to clean up
    ~CppAddon() {
        if (tsfn_ != nullptr) {
            napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
            tsfn_ = nullptr;
        }
    }

    // ... rest of the class
};

// ... rest of the file
```

Now let's add the threadsafe function setup to our constructor:

```cpp title='src/cpp_addon.cc'
// ... existing constructor code
CppAddon(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<CppAddon>(info)
    , env_(info.Env())
    , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
    , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
    , tsfn_(nullptr) {

    napi_status status = napi_create_threadsafe_function(
        env_,
        nullptr,
        nullptr,
        Napi::String::New(env_, "CppCallback"),
        0,
        1,
        nullptr,
        nullptr,
        this,
        [](napi_env env, napi_value js_callback, void* context, void* data) {
            auto* callbackData = static_cast<CallbackData*>(data);
            if (!callbackData) return;

            Napi::Env napi_env(env);
            Napi::HandleScope scope(napi_env);

            auto addon = static_cast<CppAddon*>(context);
            if (!addon) {
                delete callbackData;
                return;
            }

            try {
                auto callback = addon->callbacks.Value().Get(callbackData->eventType).As<Napi::Function>();
                if (callback.IsFunction()) {
                    callback.Call(addon->emitter.Value(), {Napi::String::New(napi_env, callbackData->payload)});
                }
            } catch (...) {}

            delete callbackData;
        },
        &tsfn_
    );

    if (status != napi_ok) {
        Napi::Error::New(env_, "Failed to create threadsafe function").ThrowAsJavaScriptException();
        return;
    }

    // We'll add callback setup in the next step
}
```

This creates a threadsafe function that allows our C++ code to call JavaScript from any thread. When called, it retrieves the appropriate JavaScript callback and invokes it with the provided payload.

Now let's add the callbacks setup:

```cpp title='src/cpp_addon.cc'
// ... existing constructor code after threadsafe function setup

// Set up the callbacks here
auto makeCallback = [this](const std::string& eventType) {
    return [this, eventType](const std::string& payload) {
        if (tsfn_ != nullptr) {
            auto* data = new CallbackData{
                eventType,
                payload,
                this
            };
            napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
        }
    };
};

cpp_code::setTodoAddedCallback(makeCallback("todoAdded"));
```

This creates a function that generates callbacks for each event type. The callbacks capture the event type and, when called, create a `CallbackData` object and pass it to our threadsafe function.

Finally, let's add the `On` method to allow JavaScript to register callback functions:

```cpp title='src/cpp_addon.cc'
// ... in the class definition, add On to DefineClass
static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "CppWin32Addon", {
        InstanceMethod("helloWorld", &CppAddon::HelloWorld),
        InstanceMethod("helloGui", &CppAddon::HelloGui),
        InstanceMethod("on", &CppAddon::On)
    });

    // ... rest of Init function
}

// ... and add the implementation in the private section
Napi::Value On(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "Expected (string, function) arguments").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    callbacks.Value().Set(info[0].As<Napi::String>(), info[1].As<Napi::Function>());
    return env.Undefined();
}
```

This allows JavaScript to register callbacks for specific event types.

### Putting the bridge together

Now we have all the pieces in place.

Here's the complete implementation:

```cpp title='src/cpp_addon.cc'
#include <napi.h>
#include <string>
#include "cpp_code.h"

class CppAddon : public Napi::ObjectWrap<CppAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "CppWin32Addon", {
            InstanceMethod("helloWorld", &CppAddon::HelloWorld),
            InstanceMethod("helloGui", &CppAddon::HelloGui),
            InstanceMethod("on", &CppAddon::On)
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("CppWin32Addon", func);
        return exports;
    }

    struct CallbackData {
        std::string eventType;
        std::string payload;
        CppAddon* addon;
    };

    CppAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<CppAddon>(info)
        , env_(info.Env())
        , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
        , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
        , tsfn_(nullptr) {

        napi_status status = napi_create_threadsafe_function(
            env_,
            nullptr,
            nullptr,
            Napi::String::New(env_, "CppCallback"),
            0,
            1,
            nullptr,
            nullptr,
            this,
            [](napi_env env, napi_value js_callback, void* context, void* data) {
                auto* callbackData = static_cast<CallbackData*>(data);
                if (!callbackData) return;

                Napi::Env napi_env(env);
                Napi::HandleScope scope(napi_env);

                auto addon = static_cast<CppAddon*>(context);
                if (!addon) {
                    delete callbackData;
                    return;
                }

                try {
                    auto callback = addon->callbacks.Value().Get(callbackData->eventType).As<Napi::Function>();
                    if (callback.IsFunction()) {
                        callback.Call(addon->emitter.Value(), {Napi::String::New(napi_env, callbackData->payload)});
                    }
                } catch (...) {}

                delete callbackData;
            },
            &tsfn_
        );

        if (status != napi_ok) {
            Napi::Error::New(env_, "Failed to create threadsafe function").ThrowAsJavaScriptException();
            return;
        }

        // Set up the callbacks here
        auto makeCallback = [this](const std::string& eventType) {
            return [this, eventType](const std::string& payload) {
                if (tsfn_ != nullptr) {
                    auto* data = new CallbackData{
                        eventType,
                        payload,
                        this
                    };
                    napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
                }
            };
        };

        cpp_code::setTodoAddedCallback(makeCallback("todoAdded"));
    }

    ~CppAddon() {
        if (tsfn_ != nullptr) {
            napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
            tsfn_ = nullptr;
        }
    }

private:
    Napi::Env env_;
    Napi::ObjectReference emitter;
    Napi::ObjectReference callbacks;
    napi_threadsafe_function tsfn_;

    Napi::Value HelloWorld(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
            return env.Null();
        }

        std::string input = info[0].As<Napi::String>();
        std::string result = cpp_code::hello_world(input);

        return Napi::String::New(env, result);
    }

    void HelloGui(const Napi::CallbackInfo& info) {
        cpp_code::hello_gui();
    }

    Napi::Value On(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();

        if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
            Napi::TypeError::New(env, "Expected (string, function) arguments").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        callbacks.Value().Set(info[0].As<Napi::String>(), info[1].As<Napi::Function>());
        return env.Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return CppAddon::Init(env, exports);
}

NODE_API_MODULE(cpp_addon, Init)
```

## 6) Creating a JavaScript Wrapper

Let's finish things off by adding a JavaScript wrapper in `js/index.js`. As we could all see, C++ requires a lot of boilerplate code that might be easier or faster to write in JavaScript - and you will find that many production applications end up transforming data or requests in JavaScript before invoking native code. We, for instance, turn our timestamp into a proper JavaScript date.

```cpp title='js/index.js'
const EventEmitter = require('events')

class CppWin32Addon extends EventEmitter {
  constructor() {
    super()

    if (process.platform !== 'win32') {
      throw new Error('This module is only available on Windows')
    }

    const native = require('bindings')('cpp_addon')
    this.addon = new native.CppWin32Addon();

    this.addon.on('todoAdded', (payload) => {
      this.emit('todoAdded', this.#parse(payload))
    });

    this.addon.on('todoUpdated', (payload) => {
      this.emit('todoUpdated', this.#parse(payload))
    });

    this.addon.on('todoDeleted', (payload) => {
      this.emit('todoDeleted', this.#parse(payload))
    });
  }

  helloWorld(input = "") {
    return this.addon.helloWorld(input)
  }

  helloGui() {
    this.addon.helloGui()
  }

  #parse(payload) {
    const parsed = JSON.parse(payload)

    return { ...parsed, date: new Date(parsed.date) }
  }
}

if (process.platform === 'win32') {
  module.exports = new CppWin32Addon()
} else {
  module.exports = {}
}
```

## 7) Building and Testing the Addon

With all files in place, you can build the addon:

```psh
npm run build
```

## Conclusion

You've now built a complete native Node.js addon for Windows using C++ and the Win32 API. Some of things we've done here are:

1. Creating a native Windows GUI from C++
2. Implementing a Todo list application with Add, Edit, and Delete functionality
3. Bidirectional communication between C++ and JavaScript
4. Using Win32 controls and Windows-specific features
5. Safely calling back into JavaScript from C++ threads

This provides a foundation for building more complex Windows-specific features in your Electron apps, giving you the best of both worlds: the ease of web technologies with the power of native code.

For more information on working with Win32 API, refer to the [Microsoft C++, C, and Assembler documentation](https://learn.microsoft.com/en-us/cpp/?view=msvc-170) and the [Windows API reference](https://learn.microsoft.com/en-us/windows/win32/api/).
