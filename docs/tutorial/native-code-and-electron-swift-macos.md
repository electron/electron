# Native Code and Electron: Swift (macOS)

This tutorial builds on the [general introduction to Native Code and Electron](./native-code-and-electron.md) and focuses on creating a native addon for macOS using Swift.

Swift is a modern, powerful language designed for safety and performance. While you can't use Swift directly with the Node.js N-API as used by Electron, you can create a bridge using Objective-C++ to connect Swift with JavaScript in your Electron application.

To illustrate how you can embed native macOS code in your Electron app, we'll be building a basic native macOS GUI (using SwiftUI) that communicates with Electron's JavaScript.

This tutorial will be most useful to those who already have some familiarity with Objective-C, Swift, and SwiftUI development. You should understand basic concepts like Swift syntax, optionals, closures, SwiftUI views, property wrappers, and the Objective-C/Swift interoperability mechanisms such as the `@objc` attribute and bridging headers.

> [!NOTE]
> If you're not already familiar with these concepts, Apple's [Swift Programming Language Guide](https://docs.swift.org/swift-book/), [SwiftUI Documentation](https://developer.apple.com/documentation/swiftui/), and [Swift and Objective-C Interoperability Guide](https://developer.apple.com/documentation/swift/importing-swift-into-objective-c) are excellent starting points.

## Requirements

Just like our [general introduction to Native Code and Electron](./native-code-and-electron.md), this tutorial assumes you have Node.js and npm installed, as well as the basic tools necessary for compiling native code on macOS. You'll need:

* Xcode installed (available from the Mac App Store)
* Xcode Command Line Tools (can be installed by running `xcode-select --install` in Terminal)

## 1) Creating a package

You can re-use the package we created in our [Native Code and Electron](./native-code-and-electron.md) tutorial. This tutorial will not be repeating the steps described there. Let's first setup our basic addon folder structure:

```txt
swift-native-addon/
├── binding.gyp            # Build configuration
├── include/
│   └── SwiftBridge.h      # Objective-C header for the bridge
├── js/
│   └── index.js           # JavaScript interface
├── package.json           # Package configuration
└── src/
    ├── SwiftCode.swift    # Swift implementation
    ├── SwiftBridge.m      # Objective-C bridge implementation
    └── swift_addon.mm     # Node.js addon implementation
```

Our `package.json` should look like this:

```json title='package.json'
{
  "name": "swift-macos",
  "version": "1.0.0",
  "description": "A demo module that exposes Swift code to Electron",
  "main": "js/index.js",
  "scripts": {
    "clean": "rm -rf build",
    "build-electron": "electron-rebuild",
    "build": "node-gyp configure && node-gyp build"
  },
  "license": "MIT",
  "dependencies": {
    "bindings": "^1.5.0",
    "node-addon-api": "^8.3.0"
  },
  "devDependencies": {
    "node-gyp": "^11.1.0"
  }
}
```

## 2) Setting Up the Build Configuration

In our other tutorials focusing on other native languages, we could use `node-gyp` to build the entirety of our code. With Swift, things are a bit more tricky: We need to first build and then link our Swift code. This is because Swift has its own compilation model and runtime requirements that don't directly integrate with node-gyp's C/C++ focused build system.

The process involves:

1. Compiling Swift code separately into a static library (.a file)
2. Creating an Objective-C bridge that exposes Swift functionality
3. Linking the compiled Swift library with our Node.js addon
4. Managing Swift runtime dependencies

This two-step compilation process ensures that Swift's advanced language features and runtime are properly handled while still allowing us to expose the functionality to JavaScript through Node.js's native addon system.

Let's start by adding a basic structure:

```json title='binding.gyp'
{
  "targets": [{
    "target_name": "swift_addon",
    "conditions": [
      ['OS=="mac"', {
        "sources": [
          "src/swift_addon.mm",
          "src/SwiftBridge.m",
          "src/SwiftCode.swift"
        ],
        "include_dirs": [
          "<!@(node -p \"require('node-addon-api').include\")",
          "include",
          "build_swift"
        ],
        "dependencies": [
          "<!(node -p \"require('node-addon-api').gyp\")"
        ],
        "libraries": [
          "<(PRODUCT_DIR)/libSwiftCode.a"
        ],
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "xcode_settings": {
          "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
          "CLANG_ENABLE_OBJC_ARC": "YES",
          "SWIFT_OBJC_BRIDGING_HEADER": "include/SwiftBridge.h",
          "SWIFT_VERSION": "5.0",
          "SWIFT_OBJC_INTERFACE_HEADER_NAME": "swift_addon-Swift.h",
          "MACOSX_DEPLOYMENT_TARGET": "11.0",
          "OTHER_CFLAGS": [
            "-ObjC++",
            "-fobjc-arc"
          ],
          "OTHER_LDFLAGS": [
            "-Wl,-rpath,@loader_path",
            "-Wl,-install_name,@rpath/libSwiftCode.a"
          ],
          "HEADER_SEARCH_PATHS": [
            "$(SRCROOT)/include",
            "$(CONFIGURATION_BUILD_DIR)",
            "$(SRCROOT)/build/Release",
            "$(SRCROOT)/build_swift"
          ]
        },
        "actions": []
      }]
    ]
  }]
}
```

We include our Objective-C++ files (`sources`), specify the necessary macOS frameworks, and set up C++ exceptions and ARC. We also set various Xcode flags:

* `GCC_ENABLE_CPP_EXCEPTIONS`: Enables C++ exception handling in the native code.
* `CLANG_ENABLE_OBJC_ARC`: Enables Automatic Reference Counting for Objective-C memory management.
* `SWIFT_OBJC_BRIDGING_HEADER`: Specifies the header file that bridges Swift and Objective-C code.
* `SWIFT_VERSION`: Sets the Swift language version to 5.0.
* `SWIFT_OBJC_INTERFACE_HEADER_NAME`: Names the automatically generated header that exposes Swift code to Objective-C.
* `MACOSX_DEPLOYMENT_TARGET`: Sets the minimum macOS version (11.0/Big Sur) required.
* `OTHER_CFLAGS`: Additional compiler flags: `-ObjC++` specifies Objective-C++ mode. `-fobjc-arc` enables ARC at the compiler level.

Then, with `OTHER_LDFLAGS`, we set Linker flags:

* `-Wl,-rpath,@loader_path`: Sets runtime search path for libraries
* `-Wl,-install_name,@rpath/libSwiftCode.a`: Configures library install name
* `HEADER_SEARCH_PATHS`: Directories to search for header files during compilation.

You might also notice that we added a currently empty `actions` array to the JSON. In the next step, we'll be compiling Swift.

### Setting up the Swift Build Configuration

We'll add two actions: One to compile our Swift code (so that it can be linked) and another one to copy it to a folder to use. Replace the `actions` array above with the following JSON:

```json title='binding.gyp'
{
  // ...other code
  "actions": [
    {
      "action_name": "build_swift",
      "inputs": [
        "src/SwiftCode.swift"
      ],
      "outputs": [
        "build_swift/libSwiftCode.a",
        "build_swift/swift_addon-Swift.h"
      ],
      "action": [
        "swiftc",
        "src/SwiftCode.swift",
        "-emit-objc-header-path", "./build_swift/swift_addon-Swift.h",
        "-emit-library", "-o", "./build_swift/libSwiftCode.a",
        "-emit-module", "-module-name", "swift_addon",
        "-module-link-name", "SwiftCode"
      ]
    },
    {
      "action_name": "copy_swift_lib",
      "inputs": [
        "<(module_root_dir)/build_swift/libSwiftCode.a"
      ],
      "outputs": [
        "<(PRODUCT_DIR)/libSwiftCode.a"
      ],
      "action": [
        "sh",
        "-c",
        "cp -f <(module_root_dir)/build_swift/libSwiftCode.a <(PRODUCT_DIR)/libSwiftCode.a && install_name_tool -id @rpath/libSwiftCode.a <(PRODUCT_DIR)/libSwiftCode.a"
      ]
    }
  ]
  // ...other code
}
```

These actions:

* Compile the Swift code to a static library using `swiftc`
* Generate an Objective-C header from the Swift code
* Copy the compiled Swift library to the output directory
* Fix the library path with `install_name_tool` to ensure the dynamic linker can find the library at runtime by setting the correct install name

## 3) Creating the Objective-C Bridge Header

We'll need to setup a bridge between the Swift code and the native Node.js C++ addon. Let's start by creating a header file for the bridge in `include/SwiftBridge.h`:

```objc title='include/SwiftBridge.h'
#ifndef SwiftBridge_h
#define SwiftBridge_h

#import <Foundation/Foundation.h>

@interface SwiftBridge : NSObject
+ (NSString*)helloWorld:(NSString*)input;
+ (void)helloGui;

+ (void)setTodoAddedCallback:(void(^)(NSString* todoJson))callback;
+ (void)setTodoUpdatedCallback:(void(^)(NSString* todoJson))callback;
+ (void)setTodoDeletedCallback:(void(^)(NSString* todoId))callback;
@end

#endif
```

This header defines the Objective-C interface that we'll use to bridge between our Swift code and the Node.js addon. It includes:

* A simple `helloWorld` method that takes a string input and returns a string
* A `helloGui` method that will display a native SwiftUI interface
* Methods to set callbacks for todo operations (add, update, delete)

## 4) Implementing the Objective-C Bridge

Now, let's create the Objective-C bridge itself in `src/SwiftBridge.m`:

```objc title='src/SwiftBridge.m'
#import "SwiftBridge.h"
#import "swift_addon-Swift.h"
#import <Foundation/Foundation.h>

@implementation SwiftBridge

static void (^todoAddedCallback)(NSString*);
static void (^todoUpdatedCallback)(NSString*);
static void (^todoDeletedCallback)(NSString*);

+ (NSString*)helloWorld:(NSString*)input {
    return [SwiftCode helloWorld:input];
}

+ (void)helloGui {
    [SwiftCode helloGui];
}

+ (void)setTodoAddedCallback:(void(^)(NSString*))callback {
    todoAddedCallback = callback;
    [SwiftCode setTodoAddedCallback:callback];
}

+ (void)setTodoUpdatedCallback:(void(^)(NSString*))callback {
    todoUpdatedCallback = callback;
    [SwiftCode setTodoUpdatedCallback:callback];
}

+ (void)setTodoDeletedCallback:(void(^)(NSString*))callback {
    todoDeletedCallback = callback;
    [SwiftCode setTodoDeletedCallback:callback];
}

@end
```

This bridge:

* Imports the Swift-generated header (`swift_addon-Swift.h`)
* Implements the methods defined in our header
* Simply forwards calls to the Swift code
* Stores the callbacks for later use in static variables, allowing them to persist throughout the application's lifecycle. This ensures that the JavaScript callbacks can be invoked at any time when todo items are added, updated, or deleted.

## 5) Implementing the Swift Code

Now, let's implement our Objective-C code in `src/SwiftCode.swift`. This is where we'll create our native macOS GUI using SwiftUI.

To make this tutorial easier to follow, we'll start with the basic structure and add features incrementally - step by step.

### Setting Up the Basic Structure

Let's start with the basic structure. Here, we're just setting up variables, some basic callback methods, and a simple helper method we'll use later to convert data into formats ready for the JavaScript world.

```swift title='src/SwiftCode.swift'
import Foundation
import SwiftUI

@objc
public class SwiftCode: NSObject {
    private static var windowController: NSWindowController?
    private static var todoAddedCallback: ((String) -> Void)?
    private static var todoUpdatedCallback: ((String) -> Void)?
    private static var todoDeletedCallback: ((String) -> Void)?

    @objc
    public static func helloWorld(_ input: String) -> String {
        return "Hello from Swift! You said: \(input)"
    }

    @objc
    public static func setTodoAddedCallback(_ callback: @escaping (String) -> Void) {
        todoAddedCallback = callback
    }

    @objc
    public static func setTodoUpdatedCallback(_ callback: @escaping (String) -> Void) {
        todoUpdatedCallback = callback
    }

    @objc
    public static func setTodoDeletedCallback(_ callback: @escaping (String) -> Void) {
        todoDeletedCallback = callback
    }

    private static func encodeToJson<T: Encodable>(_ item: T) -> String? {
        let encoder = JSONEncoder()

        // Encode date as milliseconds since 1970, which is what the JS side expects
        encoder.dateEncodingStrategy = .custom { date, encoder in
            let milliseconds = Int64(date.timeIntervalSince1970 * 1000)
            var container = encoder.singleValueContainer()
            try container.encode(milliseconds)
        }

        guard let jsonData = try? encoder.encode(item),
              let jsonString = String(data: jsonData, encoding: .utf8) else {
            return nil
        }
        return jsonString
    }

    // More code to follow...
}
```

This first part of our Swift code:

1. Declares a class with the `@objc` attribute, making it accessible from Objective-C
2. Implements the `helloWorld` method
3. Adds callback setters for todo operations
4. Includes a helper method to encode Swift objects to JSON strings

### Implementing `helloGui()`

Let's continue with the `helloGui` method and the SwiftUI implementation. This is where we start adding user interface elements to the screen.

```swift title='src/SwiftCode.swift'
// Other code...

@objc
public class SwiftCode: NSObject {
    // Other code...

        @objc
    public static func helloGui() -> Void {
        let contentView = NSHostingView(rootView: ContentView(
            onTodoAdded: { todo in
                if let jsonString = encodeToJson(todo) {
                    todoAddedCallback?(jsonString)
                }
            },
            onTodoUpdated: { todo in
                if let jsonString = encodeToJson(todo) {
                    todoUpdatedCallback?(jsonString)
                }
            },
            onTodoDeleted: { todoId in
                todoDeletedCallback?(todoId.uuidString)
            }
        ))
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 500, height: 500),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )

        window.title = "Todo List"
        window.contentView = contentView
        window.center()

        windowController = NSWindowController(window: window)
        windowController?.showWindow(nil)

        NSApp.activate(ignoringOtherApps: true)
    }
}
```

This helloGui method:

1. Creates a SwiftUI view hosted in an `NSHostingView`. This is a crucial bridging component that allows SwiftUI views to be used in AppKit applications. The `NSHostingView` acts as a container that wraps our SwiftUI `ContentView` and handles the translation between SwiftUI's declarative UI system and AppKit's imperative UI system. This enables us to leverage SwiftUI's modern UI framework while still integrating with the traditional macOS window management system.
2. Sets up callbacks to notify JavaScript when todo items change. We'll setup the actual callbacks later, for now we'll just call them if one is available.
3. Creates and displays a native macOS window.
4. Activates the app to bring the window to the front.

### Implementing the Todo Item

Next, we'll define a `TodoItem` model with an ID, text, and date.

```swift title='src/SwiftCode.swift'
// Other code...

@objc
public class SwiftCode: NSObject {
    // Other code...

    private struct TodoItem: Identifiable, Codable {
        let id: UUID
        var text: String
        var date: Date

        init(id: UUID = UUID(), text: String, date: Date) {
            self.id = id
            self.text = text
            self.date = date
        }
    }
}
```

### Implementing the View

Next, we can implement the actual view. Swift is fairly verbose here, so the code below might look scary if you're new to Swift. The many lines of code obfuscate the simplicity in it - we're just setting up some UI elements. Nothing here is specific to Electron.

```swift title='src/SwiftCode.swift'
// Other code...

@objc
public class SwiftCode: NSObject {
    // Other code...

    private struct ContentView: View {
        @State private var todos: [TodoItem] = []
        @State private var newTodo: String = ""
        @State private var newTodoDate: Date = Date()
        @State private var editingTodo: UUID?
        @State private var editedText: String = ""
        @State private var editedDate: Date = Date()

        let onTodoAdded: (TodoItem) -> Void
        let onTodoUpdated: (TodoItem) -> Void
        let onTodoDeleted: (UUID) -> Void

        private func todoTextField(_ text: Binding<String>, placeholder: String, maxWidth: CGFloat? = nil) -> some View {
            TextField(placeholder, text: text)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(maxWidth: maxWidth ?? .infinity)
        }

        private func todoDatePicker(_ date: Binding<Date>) -> some View {
            DatePicker("Due date", selection: date, displayedComponents: [.date])
                .datePickerStyle(CompactDatePickerStyle())
                .labelsHidden()
                .frame(width: 100)
                .textFieldStyle(RoundedBorderTextFieldStyle())
        }

        var body: some View {
            VStack(spacing: 16) {
                HStack(spacing: 12) {
                    todoTextField($newTodo, placeholder: "New todo")
                    todoDatePicker($newTodoDate)
                    Button(action: {
                        if !newTodo.isEmpty {
                            let todo = TodoItem(text: newTodo, date: newTodoDate)
                            todos.append(todo)
                            onTodoAdded(todo)
                            newTodo = ""
                            newTodoDate = Date()
                        }
                    }) {
                        Text("Add")
                            .frame(width: 50)
                    }
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)

                List {
                    ForEach(todos) { todo in
                        if editingTodo == todo.id {
                            HStack(spacing: 12) {
                                todoTextField($editedText, placeholder: "Edit todo", maxWidth: 250)
                                todoDatePicker($editedDate)
                                Button(action: {
                                    if let index = todos.firstIndex(where: { $0.id == todo.id }) {
                                        let updatedTodo = TodoItem(id: todo.id, text: editedText, date: editedDate)
                                        todos[index] = updatedTodo
                                        onTodoUpdated(updatedTodo)
                                        editingTodo = nil
                                    }
                                }) {
                                    Text("Save")
                                        .frame(width: 60)
                                }
                            }
                            .padding(.vertical, 4)
                        } else {
                            HStack(spacing: 12) {
                                Text(todo.text)
                                    .lineLimit(1)
                                    .truncationMode(.tail)
                                Spacer()
                                Text(todo.date.formatted(date: .abbreviated, time: .shortened))
                                    .foregroundColor(.gray)
                                Button(action: {
                                    editingTodo = todo.id
                                    editedText = todo.text
                                    editedDate = todo.date
                                }) {
                                    Image(systemName: "pencil")
                                }
                                .buttonStyle(BorderlessButtonStyle())
                                Button(action: {
                                    todos.removeAll(where: { $0.id == todo.id })
                                    onTodoDeleted(todo.id)
                                }) {
                                    Image(systemName: "trash")
                                        .foregroundColor(.red)
                                }
                                .buttonStyle(BorderlessButtonStyle())
                            }
                            .padding(.vertical, 4)
                        }
                    }
                }
            }
        }
    }
}
```

This part of the code:

* Creates a SwiftUI view with a form to add new todos, featuring a text field for the todo description, a date picker for setting due dates, and an Add button that validates input, creates a new TodoItem, adds it to the local array, triggers the `onTodoAdded` callback to notify JavaScript, and then resets the input fields for the next entry.
* Implements a list to display todos with edit and delete capabilities
* Calls the appropriate callbacks when todos are added, updated, or deleted

The final file should look as follows:

```swift title='src/SwiftCode.swift'
import Foundation
import SwiftUI

@objc
public class SwiftCode: NSObject {
    private static var windowController: NSWindowController?
    private static var todoAddedCallback: ((String) -> Void)?
    private static var todoUpdatedCallback: ((String) -> Void)?
    private static var todoDeletedCallback: ((String) -> Void)?

    @objc
    public static func helloWorld(_ input: String) -> String {
        return "Hello from Swift! You said: \(input)"
    }

    @objc
    public static func setTodoAddedCallback(_ callback: @escaping (String) -> Void) {
        todoAddedCallback = callback
    }

    @objc
    public static func setTodoUpdatedCallback(_ callback: @escaping (String) -> Void) {
        todoUpdatedCallback = callback
    }

    @objc
    public static func setTodoDeletedCallback(_ callback: @escaping (String) -> Void) {
        todoDeletedCallback = callback
    }

    private static func encodeToJson<T: Encodable>(_ item: T) -> String? {
        let encoder = JSONEncoder()

        // Encode date as milliseconds since 1970, which is what the JS side expects
        encoder.dateEncodingStrategy = .custom { date, encoder in
            let milliseconds = Int64(date.timeIntervalSince1970 * 1000)
            var container = encoder.singleValueContainer()
            try container.encode(milliseconds)
        }

        guard let jsonData = try? encoder.encode(item),
              let jsonString = String(data: jsonData, encoding: .utf8) else {
            return nil
        }
        return jsonString
    }

    @objc
    public static func helloGui() -> Void {
        let contentView = NSHostingView(rootView: ContentView(
            onTodoAdded: { todo in
                if let jsonString = encodeToJson(todo) {
                    todoAddedCallback?(jsonString)
                }
            },
            onTodoUpdated: { todo in
                if let jsonString = encodeToJson(todo) {
                    todoUpdatedCallback?(jsonString)
                }
            },
            onTodoDeleted: { todoId in
                todoDeletedCallback?(todoId.uuidString)
            }
        ))
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 500, height: 500),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )

        window.title = "Todo List"
        window.contentView = contentView
        window.center()

        windowController = NSWindowController(window: window)
        windowController?.showWindow(nil)

        NSApp.activate(ignoringOtherApps: true)
    }

    private struct TodoItem: Identifiable, Codable {
        let id: UUID
        var text: String
        var date: Date

        init(id: UUID = UUID(), text: String, date: Date) {
            self.id = id
            self.text = text
            self.date = date
        }
    }

    private struct ContentView: View {
        @State private var todos: [TodoItem] = []
        @State private var newTodo: String = ""
        @State private var newTodoDate: Date = Date()
        @State private var editingTodo: UUID?
        @State private var editedText: String = ""
        @State private var editedDate: Date = Date()

        let onTodoAdded: (TodoItem) -> Void
        let onTodoUpdated: (TodoItem) -> Void
        let onTodoDeleted: (UUID) -> Void

        private func todoTextField(_ text: Binding<String>, placeholder: String, maxWidth: CGFloat? = nil) -> some View {
            TextField(placeholder, text: text)
                .textFieldStyle(RoundedBorderTextFieldStyle())
                .frame(maxWidth: maxWidth ?? .infinity)
        }

        private func todoDatePicker(_ date: Binding<Date>) -> some View {
            DatePicker("Due date", selection: date, displayedComponents: [.date])
                .datePickerStyle(CompactDatePickerStyle())
                .labelsHidden()
                .frame(width: 100)
                .textFieldStyle(RoundedBorderTextFieldStyle())
        }

        var body: some View {
            VStack(spacing: 16) {
                HStack(spacing: 12) {
                    todoTextField($newTodo, placeholder: "New todo")
                    todoDatePicker($newTodoDate)
                    Button(action: {
                        if !newTodo.isEmpty {
                            let todo = TodoItem(text: newTodo, date: newTodoDate)
                            todos.append(todo)
                            onTodoAdded(todo)
                            newTodo = ""
                            newTodoDate = Date()
                        }
                    }) {
                        Text("Add")
                            .frame(width: 50)
                    }
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 8)

                List {
                    ForEach(todos) { todo in
                        if editingTodo == todo.id {
                            HStack(spacing: 12) {
                                todoTextField($editedText, placeholder: "Edit todo", maxWidth: 250)
                                todoDatePicker($editedDate)
                                Button(action: {
                                    if let index = todos.firstIndex(where: { $0.id == todo.id }) {
                                        let updatedTodo = TodoItem(id: todo.id, text: editedText, date: editedDate)
                                        todos[index] = updatedTodo
                                        onTodoUpdated(updatedTodo)
                                        editingTodo = nil
                                    }
                                }) {
                                    Text("Save")
                                        .frame(width: 60)
                                }
                            }
                            .padding(.vertical, 4)
                        } else {
                            HStack(spacing: 12) {
                                Text(todo.text)
                                    .lineLimit(1)
                                    .truncationMode(.tail)
                                Spacer()
                                Text(todo.date.formatted(date: .abbreviated, time: .shortened))
                                    .foregroundColor(.gray)
                                Button(action: {
                                    editingTodo = todo.id
                                    editedText = todo.text
                                    editedDate = todo.date
                                }) {
                                    Image(systemName: "pencil")
                                }
                                .buttonStyle(BorderlessButtonStyle())
                                Button(action: {
                                    todos.removeAll(where: { $0.id == todo.id })
                                    onTodoDeleted(todo.id)
                                }) {
                                    Image(systemName: "trash")
                                        .foregroundColor(.red)
                                }
                                .buttonStyle(BorderlessButtonStyle())
                            }
                            .padding(.vertical, 4)
                        }
                    }
                }
            }
        }
    }
}
```

## 6) Creating the Node.js Addon Bridge

We now have working Objective-C code, which in turn is able to call working Swift code. To make sure it can be safely and properly called from the JavaScript world, we need to build a bridge between Objective-C and C++, which we can do with Objective-C++. We'll do that in `src/swift_addon.mm`.

```objc title='src/swift_addon.mm'
#import <Foundation/Foundation.h>
#import "SwiftBridge.h"
#include <napi.h>

class SwiftAddon : public Napi::ObjectWrap<SwiftAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "SwiftAddon", {
            InstanceMethod("helloWorld", &SwiftAddon::HelloWorld),
            InstanceMethod("helloGui", &SwiftAddon::HelloGui),
            InstanceMethod("on", &SwiftAddon::On)
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("SwiftAddon", func);
        return exports;
    }

    // More code to follow...
```

This first part:

1. Defines a C++ class that inherits from `Napi::ObjectWrap`
2. Creates a static `Init` method to register our class with Node.js
3. Defines three methods: `helloWorld`, `helloGui`, and `on`

### Callback Mechanism

Next, let's implement the callback mechanism:

```objc title='src/swift_addon.mm'
// Previous code...

    struct CallbackData {
        std::string eventType;
        std::string payload;
        SwiftAddon* addon;
    };

    SwiftAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<SwiftAddon>(info)
        , env_(info.Env())
        , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
        , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
        , tsfn_(nullptr) {

        napi_status status = napi_create_threadsafe_function(
            env_,
            nullptr,
            nullptr,
            Napi::String::New(env_, "SwiftCallback"),
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

                auto addon = static_cast<SwiftAddon*>(context);
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
```

This part:

1. Defines a struct to pass data between threads
2. Sets up a constructor for our addon
3. Creates a threadsafe function to handle callbacks from Swift

Let's continue with setting up the Swift callbacks:

```objc title='src/swift_addon.mm'
// Previous code...

        auto makeCallback = [this](const char* eventType) {
            return ^(NSString* payload) {
                if (tsfn_ != nullptr) {
                    auto* data = new CallbackData{
                        eventType,
                        std::string([payload UTF8String]),
                        this
                    };
                    napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
                }
            };
        };

        [SwiftBridge setTodoAddedCallback:makeCallback("todoAdded")];
        [SwiftBridge setTodoUpdatedCallback:makeCallback("todoUpdated")];
        [SwiftBridge setTodoDeletedCallback:makeCallback("todoDeleted")];
    }

    ~SwiftAddon() {
        if (tsfn_ != nullptr) {
            napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
            tsfn_ = nullptr;
        }
    }
```

This part:

1. Creates a helper function to generate Objective-C blocks that will be used as callbacks for Swift events. This lambda function `makeCallback` takes an event type string and returns an Objective-C block that captures the event type and payload. When Swift calls this block, it creates a CallbackData structure with the event information and passes it to the threadsafe function, which safely bridges between Swift's thread and Node.js's event loop.
2. Sets up the carefully constructed callbacks for todo operations
3. Implements a destructor to clean up resources

### Instance Methods

Finally, let's implement the instance methods:

```objc title='src/swift_addon.mm'
// Previous code...

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
        NSString* nsInput = [NSString stringWithUTF8String:input.c_str()];
        NSString* result = [SwiftBridge helloWorld:nsInput];

        return Napi::String::New(env, [result UTF8String]);
    }

    void HelloGui(const Napi::CallbackInfo& info) {
        [SwiftBridge helloGui];
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
    return SwiftAddon::Init(env, exports);
}

NODE_API_MODULE(swift_addon, Init)
```

This final part does multiple things:

1. The code defines private member variables for the environment, event emitter, callback storage, and thread-safe function that are essential for the addon's operation.
2. The HelloWorld method implementation takes a string input from JavaScript, passes it to the Swift code, and returns the processed result back to the JavaScript environment.
3. The `HelloGui` method implementation provides a simple wrapper that calls the Swift UI creation function to display the native macOS window.
4. The `On` method implementation allows JavaScript code to register callback functions that will be invoked when specific events occur in the native Swift code.
5. The code sets up the module initialization process that registers the addon with Node.js and makes its functionality available to JavaScript.

The final and full `src/swift_addon.mm` should look like:

```objc title='src/swift_addon.mm'
#import <Foundation/Foundation.h>
#import "SwiftBridge.h"
#include <napi.h>

class SwiftAddon : public Napi::ObjectWrap<SwiftAddon> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "SwiftAddon", {
            InstanceMethod("helloWorld", &SwiftAddon::HelloWorld),
            InstanceMethod("helloGui", &SwiftAddon::HelloGui),
            InstanceMethod("on", &SwiftAddon::On)
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("SwiftAddon", func);
        return exports;
    }

    struct CallbackData {
        std::string eventType;
        std::string payload;
        SwiftAddon* addon;
    };

    SwiftAddon(const Napi::CallbackInfo& info)
        : Napi::ObjectWrap<SwiftAddon>(info)
        , env_(info.Env())
        , emitter(Napi::Persistent(Napi::Object::New(info.Env())))
        , callbacks(Napi::Persistent(Napi::Object::New(info.Env())))
        , tsfn_(nullptr) {

        napi_status status = napi_create_threadsafe_function(
            env_,
            nullptr,
            nullptr,
            Napi::String::New(env_, "SwiftCallback"),
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

                auto addon = static_cast<SwiftAddon*>(context);
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

        auto makeCallback = [this](const char* eventType) {
            return ^(NSString* payload) {
                if (tsfn_ != nullptr) {
                    auto* data = new CallbackData{
                        eventType,
                        std::string([payload UTF8String]),
                        this
                    };
                    napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
                }
            };
        };

        [SwiftBridge setTodoAddedCallback:makeCallback("todoAdded")];
        [SwiftBridge setTodoUpdatedCallback:makeCallback("todoUpdated")];
        [SwiftBridge setTodoDeletedCallback:makeCallback("todoDeleted")];
    }

    ~SwiftAddon() {
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
        NSString* nsInput = [NSString stringWithUTF8String:input.c_str()];
        NSString* result = [SwiftBridge helloWorld:nsInput];

        return Napi::String::New(env, [result UTF8String]);
    }

    void HelloGui(const Napi::CallbackInfo& info) {
        [SwiftBridge helloGui];
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
    return SwiftAddon::Init(env, exports);
}

NODE_API_MODULE(swift_addon, Init)
```

## 6) Creating a JavaScript Wrapper

You're so close! We now have working Objective-C, Swift, and thread-safe ways to expose methods and events to JavaScript. In this final step, let's create a JavaScript wrapper in `js/index.js` to provide a more friendly API:

```js title='js/index.js' @ts-expect-error=[10]
const EventEmitter = require('node:events')

class SwiftAddon extends EventEmitter {
  constructor () {
    super()

    if (process.platform !== 'darwin') {
      throw new Error('This module is only available on macOS')
    }

    const native = require('bindings')('swift_addon')
    this.addon = new native.SwiftAddon()

    this.addon.on('todoAdded', (payload) => {
      this.emit('todoAdded', this.parse(payload))
    })

    this.addon.on('todoUpdated', (payload) => {
      this.emit('todoUpdated', this.parse(payload))
    })

    this.addon.on('todoDeleted', (payload) => {
      this.emit('todoDeleted', this.parse(payload))
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
  module.exports = new SwiftAddon()
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

You've now built a complete native Node.js addon for macOS using Swift and SwiftUI. This provides a foundation for building more complex macOS-specific features in your Electron apps, giving you the best of both worlds: the ease of web technologies with the power of native macOS code.

The approach demonstrated here allows you to:

* Setting up a project structure that bridges Swift, Objective-C, and JavaScript
* Implementing Swift code with SwiftUI for native UI
* Creating an Objective-C bridge to connect Swift with Node.js
* Setting up bidirectional communication using callbacks and events
* Configuring a custom build process to compile Swift code

For more information on developing with Swift and Swift, refer to Apple's developer documentation:

* [Swift Programming Language](https://developer.apple.com/swift/)
* [SwiftUI Framework](https://developer.apple.com/documentation/swiftui)
* [macOS Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/macos)
* [Swift and Objective-C Interoperability Guide](https://developer.apple.com/documentation/swift/importing-swift-into-objective-c)
