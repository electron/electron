# Native Code and Electron: Objective-C (macOS)

This tutorial builds on the [general introduction to Native Code and Electron](./native-code-and-electron.md) and focuses on creating a native addon for macOS using Objective-C, Objective-C++, and Cocoa frameworks. To illustrate how you can embed native macOS code in your Electron app, we'll be building a basic native macOS GUI (using AppKit) that communicates with Electron's JavaScript.

Specifically, we'll be integrating with two macOS frameworks:

* AppKit - The primary UI framework for macOS applications that provides components like windows, buttons, text fields, and more.
* Foundation - A framework that provides data management, file system interaction, and other essential services.

This tutorial will be most useful to those who already have some familiarity with Objective-C and Cocoa development. You should understand basic concepts like delegates, NSObjects, and the target-action pattern commonly used in macOS development.

> [!NOTE]
> If you're not already familiar with these concepts, Apple's [documentation on Objective-C](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Introduction/Introduction.html) is an excellent starting point.

## Requirements

Just like our general introduction to Native Code and Electron, this tutorial assumes you have Node.js and npm installed, as well as the basic tools necessary for compiling native code on macOS. You'll need:

* Xcode installed (available from the Mac App Store)
* Xcode Command Line Tools (can be installed by running `xcode-select --install` in Terminal)

## 1) Creating a package

You can re-use the package we created in our [Native Code and Electron](./native-code-and-electron.md) tutorial. This tutorial will not be repeating the steps described there. Let's first setup our basic addon folder structure:

```txt
my-native-objc-addon/
├── binding.gyp
├── include/
│   └── objc_code.h
├── js/
│   └── index.js
├── package.json
└── src/
    ├── objc_addon.mm
    └── objc_code.mm
```

Our `package.json` should look like this:

```json title='package.json'
{
  "name": "objc-macos",
  "version": "1.0.0",
  "description": "A demo module that exposes Objective-C code to Electron",
  "main": "js/index.js",
  "author": "Your Name",
  "scripts": {
    "clean": "rm -rf build",
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

For a macOS-specific addon using Objective-C, we need to modify our `binding.gyp` file to include the appropriate frameworks and compiler flags. We need to:

1. Ensure our addon is only compiled on macOS
2. Include the necessary macOS frameworks (Foundation and AppKit)
3. Configure the compiler for Objective-C/C++ support

```json title='binding.gyp'
{
  "targets": [
    {
      "target_name": "objc_addon",
      "conditions": [
        ['OS=="mac"', {
          "sources": [
            "src/objc_addon.mm",
            "src/objc_code.mm"
          ],
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "include"
          ],
          "libraries": [
            "-framework Foundation",
            "-framework AppKit"
          ],
          "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
          ],
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "11.0",
            "CLANG_ENABLE_OBJC_ARC": "YES",
            "OTHER_CFLAGS": [
              "-ObjC++",
              "-std=c++17"
            ]
          },
          "defines": [
            "NODE_ADDON_API_CPP_EXCEPTIONS"
          ]
        }]
      ]
    }
  ]
}
```

Note the key macOS-specific settings:

* `.mm` extension for source files: This indicates Objective-C++ files which can mix Objective-C and C++.
* `libraries`: This section includes the Foundation and AppKit frameworks
* `xcode_settings` includes:
  * `CLANG_ENABLE_OBJC_ARC`: "YES" enables Automatic Reference Counting for easier memory management
  * `OTHER_CFLAGS`: `-ObjC++` to properly handle Objective-C++ compilation
  * `MACOSX_DEPLOYMENT_TARGET`: This flag specifies the minimum macOS version supported. You'll likely want this to match the lowest version of macOS you support with your app.

## 3) Defining the Objective-C Interface

Let's define our interface in `include/objc_code.h`:

```cpp title='include/objc_code.h'
#pragma once
#include <string>
#include <functional>

namespace objc_code {

std::string hello_world(const std::string& input);
void hello_gui();

// Callback function types
using TodoCallback = std::function<void(const std::string&)>;

// Callback setters
void setTodoAddedCallback(TodoCallback callback);

} // namespace objc_code
```

This header:

* Includes a basic hello_world function from the general tutorial
* Adds a `hello_gui` function to create a native macOS GUI
* Defines callback types for Todo operations
* Provides setter functions for these callbacks

## 4) Implementing the Objective-C Code

Now, let's implement our Objective-C code in `src/objc_code.mm`. This is where we'll create our native macOS GUI using AppKit.

We'll always add code to the bottom of our file. To make this tutorial easier to follow, we'll start with the basic structure and add features incrementally - step by step.

### Setting Up the Basic Structure

```objc title='src/objc_code.mm'
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <string>
#import <functional>
#import "../include/objc_code.h"

using TodoCallback = std::function<void(const std::string&)>;

static TodoCallback g_todoAddedCallback;

// More code to follow later...
```

This imports the required frameworks and defines our callback type. The static `g_todoAddedCallback` variable will store our JavaScript callback function.

### Defining the Window Controller Interface

At the bottom of `objc_code.mm`, add the following code to define our window controller class interface:

```objc title='src/objc_code.mm'
// Previous code...

// Forward declaration of our custom classes
@interface TodoWindowController : NSWindowController
@property (strong) NSTextField *textField;
@property (strong) NSDatePicker *datePicker;
@property (strong) NSButton *addButton;
@property (strong) NSTableView *tableView;
@property (strong) NSMutableArray<NSDictionary*> *todos;
@end

// More code to follow later...
```

This declares our TodoWindowController class which will manage the window and UI components:

* A text field (`NSTextField`) for entering todo text
* A date picker (`NSDatePicker`) for selecting the date
* An "Add" button (`NSButton`)
* A table view to display the todos (`NSTableView`)
* An array to store the todo items (`NSMutableArray`)

### Implementing the Window Controller

At the bottom of `objc_code.mm`, add the following code to start implementing the window controller with an initialization method:

```objc title='src/objc_code.mm'
// Previous code...

// Controller for the main window
@implementation TodoWindowController

- (instancetype)init {
    self = [super initWithWindowNibName:@""];
    if (self) {
        // Create an array to store todos
        _todos = [NSMutableArray array];
        [self setupWindow];
    }
    return self;
}

// More code to follow later...
```

This initializes our controller. We're not using a nib file, so we pass an empty string to `initWithWindowNibName`. We create an empty array to store our todos and call the `setupWindow` method, which we'll implement next.

At this point, our full file looks like this:

```objc title='src/objc_code.mm'
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <string>
#import <functional>
#import "../include/objc_code.h"

using TodoCallback = std::function<void(const std::string&)>;

static TodoCallback g_todoAddedCallback;

// Forward declaration of our custom classes
@interface TodoWindowController : NSWindowController
@property (strong) NSTextField *textField;
@property (strong) NSDatePicker *datePicker;
@property (strong) NSButton *addButton;
@property (strong) NSTableView *tableView;
@property (strong) NSMutableArray<NSDictionary*> *todos;
@end

// Controller for the main window
@implementation TodoWindowController

- (instancetype)init {
    self = [super initWithWindowNibName:@""];
    if (self) {
        // Create an array to store todos
        _todos = [NSMutableArray array];
        [self setupWindow];
    }
    return self;
}

// More code to follow later...
```

### Creating the Window and Basic UI

Now, we'll add a `setupWindow()` method. This method will look a little overwhelming on first sight, but it really just instantiates a number of UI controls and then adds them to our window.

```objc title='src/objc_code.mm'
// Previous code...

- (void)setupWindow {
    // Create a window
    NSRect frame = NSMakeRect(0, 0, 400, 300);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                         backing:NSBackingStoreBuffered
                                         defer:NO];
    [window setTitle:@"Todo List"];
    [window center];
    self.window = window;

    // Set up the content view with auto layout
    NSView *contentView = [window contentView];

    // Create text field
    _textField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 260, 200, 24)];
    [_textField setPlaceholderString:@"Enter a todo..."];
    [contentView addSubview:_textField];

    // Create date picker
    _datePicker = [[NSDatePicker alloc] initWithFrame:NSMakeRect(230, 260, 100, 24)];
    [_datePicker setDatePickerStyle:NSDatePickerStyleTextField];
    [_datePicker setDatePickerElements:NSDatePickerElementFlagYearMonthDay];
    [contentView addSubview:_datePicker];

    // Create add button
    _addButton = [[NSButton alloc] initWithFrame:NSMakeRect(340, 260, 40, 24)];
    [_addButton setTitle:@"Add"];
    [_addButton setBezelStyle:NSBezelStyleRounded];
    [_addButton setTarget:self];
    [_addButton setAction:@selector(addTodo:)];
    [contentView addSubview:_addButton];

    // More UI elements to follow in the next step...
}

// More code to follow later...
```

This method:

1. Creates a window with a title and standard window controls
2. Centers the window on the screen
3. Creates a text field for entering todo text
4. Adds a date picker configured to show only date (no time)
5. Adds an "Add" button that will call the `addTodo:` method when clicked

We're still missing the table view to display our todos. Let's add that to the bottom of our `setupWindow()` method, right where it says `More UI elements to follow in the next step...` in the code above.

```objc title='src/objc_code.mm'
// Previous code...

- (void)setupWindow {
  // Previous setupWindow() code...

  // Create a scroll view for the table
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 360, 230)];
    [scrollView setBorderType:NSBezelBorder];
    [scrollView setHasVerticalScroller:YES];
    [contentView addSubview:scrollView];

    // Create table view
    _tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 360, 230)];

    // Add a column for the todo text
    NSTableColumn *textColumn = [[NSTableColumn alloc] initWithIdentifier:@"text"];
    [textColumn setWidth:240];
    [textColumn setTitle:@"Todo"];
    [_tableView addTableColumn:textColumn];

    // Add a column for the date
    NSTableColumn *dateColumn = [[NSTableColumn alloc] initWithIdentifier:@"date"];
    [dateColumn setWidth:100];
    [dateColumn setTitle:@"Date"];
    [_tableView addTableColumn:dateColumn];

    // Set the table's delegate and data source
    [_tableView setDataSource:self];
    [_tableView setDelegate:self];

    // Add the table to the scroll view
    [scrollView setDocumentView:_tableView];
}

// More code to follow later...
```

This extends our `setupWindow` method to:

1. Create a scroll view to contain the table
2. Create a table view with two columns: one for the todo text and one for the date
3. Set up the data source and delegate to this class
4. Add the table to the scroll view

This concludes the UI elements in `setupWindow()`, so we can now move on to business logic.

### Implementing the "Add Todo" Functionality

Next, let's implement the `addTodo:` method to handle adding new todos. We'll need to do two sets of operations here: First, we need to handle our native UI and perform operations like getting the data out of our UI elements or resetting them. Then, we also need notify our JavaScript world about the newly added todo.

In the interest of keeping this tutorial easy to follow, we'll do this in two steps.

```objc title='src/objc_code.mm'
// Previous code...

// Action method for the Add button
- (void)addTodo:(id)sender {
    NSString *text = [_textField stringValue];
    if ([text length] > 0) {
        NSDate *date = [_datePicker dateValue];

        // Create a unique ID
        NSUUID *uuid = [NSUUID UUID];

        // Create a dictionary to store the todo
        NSDictionary *todo = @{
            @"id": [uuid UUIDString],
            @"text": text,
            @"date": date
        };

        // Add to our array
        [_todos addObject:todo];

        // Reload the table
        [_tableView reloadData];

        // Reset the text field
        [_textField setStringValue:@""];

        // Next, we'll notify our JavaScript world here...
    }
}

// More code to follow later...
```

This method:

1. Gets the text from the text field
2. If the text is not empty, creates a new todo with a unique ID, the entered text, and the selected date
3. Adds the todo to our array
4. Reloads the table to show the new todo
5. Clears the text field for the next entry

Now, let's extend the `addTodo:` method to notify JavaScript when a todo is added. We'll do that at the bottom of the method, where it currently reads "Next, we'll notify our JavaScript world here...".

```objc title='src/objc_code.mm'
// Previous code...

// Action method for the Add button
- (void)addTodo:(id)sender {
    NSString *text = [_textField stringValue];
    if ([text length] > 0) {
        // Previous addTodo() code...

        // Call the callback if it exists
        if (g_todoAddedCallback) {
            // Convert the todo to JSON
            NSError *error;
            NSData *jsonData = [NSJSONSerialization dataWithJSONObject:@{
                @"id": [uuid UUIDString],
                @"text": text,
                @"date": @((NSTimeInterval)[date timeIntervalSince1970] * 1000)
            } options:0 error:&error];

            if (!error) {
                NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
                std::string cppJsonString = [jsonString UTF8String];
                g_todoAddedCallback(cppJsonString);
            }
        }
    }
}

// More code to follow later...
```

This adds code to do a whole bunch of conversions (so that N-API can eventually turn this data into structures ready for V8 and the JavaScript world) - and then calls our JavaScript callback. Specifically, it does the following:

1. Check if a callback function has been registered
2. Convert the todo to JSON format
3. Convert the date to milliseconds since epoch (JavaScript date format)
4. Convert the JSON to a C++ string
5. Call the callback function with the JSON string

We're now done with our `addTodo:` method and can move on to the next step: The data source for the Table View.

### Implementing the Table View Data Source

Let's implement the table view data source methods to display our todos:

```objc title='src/objc_code.mm'
// Previous code...

// NSTableViewDataSource methods
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [_todos count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSDictionary *todo = _todos[row];
    NSString *identifier = [tableColumn identifier];

    if ([identifier isEqualToString:@"text"]) {
        return todo[@"text"];
    } else if ([identifier isEqualToString:@"date"]) {
        NSDate *date = todo[@"date"];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        return [formatter stringFromDate:date];
    }

    return nil;
}

@end

// More code to follow later...
```

These methods:

* Return the number of todos for the table view
* Provide the text or formatted date for each cell in the table

### Implementing the C++ Functions

Lastly, we need to implement the C++ namespace functions that were declared in our header file:

```objc title='src/objc_code.mm'
// Previous code...

namespace objc_code {

std::string hello_world(const std::string& input) {
    return "Hello from Objective-C! You said: " + input;
}

void setTodoAddedCallback(TodoCallback callback) {
    g_todoAddedCallback = callback;
}

void hello_gui() {
    // Create and run the GUI on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        // Create our window controller
        TodoWindowController *windowController = [[TodoWindowController alloc] init];

        // Show the window
        [windowController showWindow:nil];

        // Keep a reference to prevent it from being deallocated
        // Note: in a real app, you'd store this reference more carefully
        static TodoWindowController *staticController = nil;
        staticController = windowController;
    });
}

} // namespace objc_code
```

These functions:

1. Implement the `hello_world` function that returns a greeting string
2. Provide a way to set the callback function for todo additions
3. Implement the `hello_gui` function that creates and shows our native UI
4. Lastly, we also keep a static reference to prevent the window controller from being deallocated

Note that we're using GCD (Grand Central Dispatch) to dispatch to the main thread, which is required for UI operations. We're not dedicating more time to thread safety in this tutorial, but here's a quick reminder: In macOS/iOS, all UI updates must happen on the main thread. The main thread is the primary execution path where the application runs its event loop and processes user interface events. In our code, when JavaScript calls the `hello_gui()` function, the call might be coming from a Node.js worker thread, not the main thread. Using GCD, we safely redirect the window creation code to the main thread, ensuring proper UI behavior.

This is a common pattern in macOS/iOS development - any code that touches the UI needs to be executed on the main thread, and GCD provides a clean way to ensure this happens.

The final version of `objc_code.mm` looks like this:

```objc title='src/objc_code.mm'
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <string>
#import <functional>
#import "../include/objc_code.h"

using TodoCallback = std::function<void(const std::string&)>;

static TodoCallback g_todoAddedCallback;

// Forward declaration of our custom classes
@interface TodoWindowController : NSWindowController
@property (strong) NSTextField *textField;
@property (strong) NSDatePicker *datePicker;
@property (strong) NSButton *addButton;
@property (strong) NSTableView *tableView;
@property (strong) NSMutableArray<NSDictionary*> *todos;
@end

// Controller for the main window
@implementation TodoWindowController

- (instancetype)init {
    self = [super initWithWindowNibName:@""];
    if (self) {
        // Create an array to store todos
        _todos = [NSMutableArray array];
        [self setupWindow];
    }
    return self;
}

- (void)setupWindow {
    // Create a window
    NSRect frame = NSMakeRect(0, 0, 400, 300);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                         backing:NSBackingStoreBuffered
                                         defer:NO];
    [window setTitle:@"Todo List"];
    [window center];
    self.window = window;

    // Set up the content view with auto layout
    NSView *contentView = [window contentView];

    // Create text field
    _textField = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 260, 200, 24)];
    [_textField setPlaceholderString:@"Enter a todo..."];
    [contentView addSubview:_textField];

    // Create date picker
    _datePicker = [[NSDatePicker alloc] initWithFrame:NSMakeRect(230, 260, 100, 24)];
    [_datePicker setDatePickerStyle:NSDatePickerStyleTextField];
    [_datePicker setDatePickerElements:NSDatePickerElementFlagYearMonthDay];
    [contentView addSubview:_datePicker];

    // Create add button
    _addButton = [[NSButton alloc] initWithFrame:NSMakeRect(340, 260, 40, 24)];
    [_addButton setTitle:@"Add"];
    [_addButton setBezelStyle:NSBezelStyleRounded];
    [_addButton setTarget:self];
    [_addButton setAction:@selector(addTodo:)];
    [contentView addSubview:_addButton];

    // Create a scroll view for the table
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(20, 20, 360, 230)];
    [scrollView setBorderType:NSBezelBorder];
    [scrollView setHasVerticalScroller:YES];
    [contentView addSubview:scrollView];

    // Create table view
    _tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 360, 230)];

    // Add a column for the todo text
    NSTableColumn *textColumn = [[NSTableColumn alloc] initWithIdentifier:@"text"];
    [textColumn setWidth:240];
    [textColumn setTitle:@"Todo"];
    [_tableView addTableColumn:textColumn];

    // Add a column for the date
    NSTableColumn *dateColumn = [[NSTableColumn alloc] initWithIdentifier:@"date"];
    [dateColumn setWidth:100];
    [dateColumn setTitle:@"Date"];
    [_tableView addTableColumn:dateColumn];

    // Set the table's delegate and data source
    [_tableView setDataSource:self];
    [_tableView setDelegate:self];

    // Add the table to the scroll view
    [scrollView setDocumentView:_tableView];
}

// Action method for the Add button
- (void)addTodo:(id)sender {
    NSString *text = [_textField stringValue];
    if ([text length] > 0) {
        NSDate *date = [_datePicker dateValue];

        // Create a unique ID
        NSUUID *uuid = [NSUUID UUID];

        // Create a dictionary to store the todo
        NSDictionary *todo = @{
            @"id": [uuid UUIDString],
            @"text": text,
            @"date": date
        };

        // Add to our array
        [_todos addObject:todo];

        // Reload the table
        [_tableView reloadData];

        // Reset the text field
        [_textField setStringValue:@""];

        // Call the callback if it exists
        if (g_todoAddedCallback) {
            // Convert the todo to JSON
            NSError *error;
            NSData *jsonData = [NSJSONSerialization dataWithJSONObject:@{
                @"id": [uuid UUIDString],
                @"text": text,
                @"date": @((NSTimeInterval)[date timeIntervalSince1970] * 1000)
            } options:0 error:&error];

            if (!error) {
                NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
                std::string cppJsonString = [jsonString UTF8String];
                g_todoAddedCallback(cppJsonString);
            }
        }
    }
}

// NSTableViewDataSource methods
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [_todos count];
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSDictionary *todo = _todos[row];
    NSString *identifier = [tableColumn identifier];

    if ([identifier isEqualToString:@"text"]) {
        return todo[@"text"];
    } else if ([identifier isEqualToString:@"date"]) {
        NSDate *date = todo[@"date"];
        NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
        [formatter setDateStyle:NSDateFormatterShortStyle];
        return [formatter stringFromDate:date];
    }

    return nil;
}

@end

namespace objc_code {

std::string hello_world(const std::string& input) {
    return "Hello from Objective-C! You said: " + input;
}

void setTodoAddedCallback(TodoCallback callback) {
    g_todoAddedCallback = callback;
}

void hello_gui() {
    // Create and run the GUI on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        // Create our window controller
        TodoWindowController *windowController = [[TodoWindowController alloc] init];

        // Show the window
        [windowController showWindow:nil];

        // Keep a reference to prevent it from being deallocated
        // Note: in a real app, you'd store this reference more carefully
        static TodoWindowController *staticController = nil;
        staticController = windowController;
    });
}

} // namespace objc_code
```

## 5) Creating the Node.js Addon Bridge

We now have working Objective-C code. To make sure it can be safely and properly called from the JavaScript world, we need to build a bridge between Objective-C and C++, which we can do with Objective-C++. We'll do that in `src/objc_addon.mm`.

Bear with us: This bridge code always ends up being pretty verbose and might seem difficult to follow. As far as modern desktop development goes, it's fairly low-level, so be patient with yourself - it might take a little bit before the bridging really "clicks".

### Basic Class Definition

```objc title='src/objc_addon.mm'
#include <napi.h>
#include <string>
#include "../include/objc_code.h"

class ObjcAddon : public Napi::ObjectWrap<ObjcAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "ObjcMacosAddon", {
            InstanceMethod("helloWorld", &ObjcAddon::HelloWorld),
            InstanceMethod("helloGui", &ObjcAddon::HelloGui),
            InstanceMethod("on", &ObjcAddon::On)
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("ObjcMacosAddon", func);
        return exports;
    }

    struct CallbackData {
        std::string eventType;
        std::string payload;
        ObjcAddon* addon;
    };

    // More code to follow later...
    // Specifically, we'll add ObjcAddon here in the next step
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return ObjcAddon::Init(env, exports);
}

NODE_API_MODULE(objc_addon, Init)
```

This code:

1. Defines an ObjcAddon class that inherits from Napi::ObjectWrap
2. Creates a static Init method that registers our JavaScript methods
3. Defines a CallbackData structure for passing data between threads
4. Sets up the Node API module initialization

### Constructor and Threadsafe Function Setup

Next, let's implement the constructor that sets up our threadsafe callback mechanism:

```objc title='src/objc_addon.mm'
ObjcAddon(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<ObjcAddon>(info)
    , env_(info.Env())
    , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
    , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
    , tsfn_(nullptr) {

    napi_status status = napi_create_threadsafe_function(
        env_,
        nullptr,
        nullptr,
        Napi::String::New(env_, "ObjcCallback"),
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

            auto addon = static_cast<ObjcAddon*>(context);
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

    // Set up the callbacks
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

    objc_code::setTodoAddedCallback(makeCallback("todoAdded"));
}

~ObjcAddon() {
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
```

This code:

* Sets up the constructor with member initialization
* Creates a threadsafe function using N-API, which allows safe callbacks from any thread
* Defines a lambda to create callback functions for different event types
* Registers the "todoAdded" callback with our Objective-C code
* Implements a destructor to clean up resources when the addon is destroyed

The threadsafe function is important because UI events in Objective-C might happen on a different thread than the JavaScript event loop. This mechanism safely bridges those thread boundaries.

### Implementing JavaScript Methods

Finally, let's implement the methods that JavaScript will call:

```objc title='src/objc_addon.mm'
Napi::Value HelloWorld(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string input = info[0].As<Napi::String>();
    std::string result = objc_code::hello_world(input);

    return Napi::String::New(env, result);
}

void HelloGui(const Napi::CallbackInfo& info) {
    objc_code::hello_gui();
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
```

Let's take a look at what we've added in this step:

* `HelloWorld()`: Takes a string input, calls our Objective-C function, and returns the result
* `HelloGui()`:  A simple wrapper around the Objective-C `hello_gui` function
* `On`: Allows JavaScript to register event listeners that will be called when native events occur

The `On` method is particularly important as it creates the event system that our JavaScript code will use to receive notifications from the native UI.

Together, these three components form a complete bridge between our Objective-C code and the JavaScript world, allowing bidirectional communication. Here's what the finished file should look like:

```objc title='src/objc_addon.mm'
#include <napi.h>
#include <string>
#include "../include/objc_code.h"

class ObjcAddon : public Napi::ObjectWrap<ObjcAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "ObjcMacosAddon", {
            InstanceMethod("helloWorld", &ObjcAddon::HelloWorld),
            InstanceMethod("helloGui", &ObjcAddon::HelloGui),
            InstanceMethod("on", &ObjcAddon::On)
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("ObjcMacosAddon", func);
        return exports;
    }

    struct CallbackData {
        std::string eventType;
        std::string payload;
        ObjcAddon* addon;
    };

    ObjcAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<ObjcAddon>(info)
        , env_(info.Env())
        , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
        , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
        , tsfn_(nullptr) {

        napi_status status = napi_create_threadsafe_function(
            env_,
            nullptr,
            nullptr,
            Napi::String::New(env_, "ObjcCallback"),
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

                auto addon = static_cast<ObjcAddon*>(context);
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

        // Set up the callbacks
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

        objc_code::setTodoAddedCallback(makeCallback("todoAdded"));
    }

    ~ObjcAddon() {
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
        std::string result = objc_code::hello_world(input);

        return Napi::String::New(env, result);
    }

    void HelloGui(const Napi::CallbackInfo& info) {
        objc_code::hello_gui();
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
    return ObjcAddon::Init(env, exports);
}

NODE_API_MODULE(objc_addon, Init)
```

## 6) Creating a JavaScript Wrapper

You're so close! We now have working Objective-C and thread-safe ways to expose methods and events to JavaScript. In this final step, let's create a JavaScript wrapper in `js/index.js` to provide a more friendly API:

```js title='js/index.js' @ts-expect-error=[10]
const EventEmitter = require('node:events')

class ObjcMacosAddon extends EventEmitter {
  constructor () {
    super()

    if (process.platform !== 'darwin') {
      throw new Error('This module is only available on macOS')
    }

    const native = require('bindings')('objc_addon')
    this.addon = new native.ObjcMacosAddon()

    this.addon.on('todoAdded', (payload) => {
      this.emit('todoAdded', this.parse(payload))
    })
  }

  helloWorld (input = '') {
    return this.addon.helloWorld(input)
  }

  helloGui () {
    this.addon.helloGui()
  }

  parse (payload) {
    const parsed = JSON.parse(payload)

    return { ...parsed, date: new Date(parsed.date) }
  }
}

if (process.platform === 'darwin') {
  module.exports = new ObjcMacosAddon()
} else {
  module.exports = {}
}
```

This wrapper:

1. Extends EventEmitter to provide event support
2. Checks if we're running on macOS
3. Loads the native addon
4. Sets up event listeners and forwards them
5. Provides a clean API for our functions
6. Parses JSON payloads and converts timestamps to JavaScript Date objects

## 7) Building and Testing the Addon

With all files in place, you can build the addon:

```sh
npm run build
```

Please note that you _cannot_ call this script from Node.js directly, since Node.js doesn't set up an "app" in the eyes of macOS. Electron does though, so you can test your code by requiring and calling it from Electron.

## Conclusion

You've now built a complete native Node.js addon for macOS using Objective-C and AppKit. This provides a foundation for building more complex macOS-specific features in your Electron apps, giving you the best of both worlds: the ease of web technologies with the power of native macOS code.

The approach demonstrated here allows you to:

* Create native macOS UIs using AppKit
* Implement bidirectional communication between JavaScript and Objective-C
* Leverage macOS-specific features and frameworks
* Integrate with existing Objective-C codebases

For more information on developing with Objective-C and Cocoa, refer to Apple's developer documentation:

* [Objective-C Programming](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Introduction/Introduction.html)
* [AppKit Framework](https://developer.apple.com/documentation/appkit)
* [macOS Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/macos)
