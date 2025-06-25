# Native Code and Electron: C++ (Linux)

This tutorial builds on the [general introduction to Native Code and Electron](./native-code-and-electron.md) and focuses on creating a native addon for Linux using C++ and GTK3. To illustrate how you can embed native Linux code in your Electron app, we'll be building a basic native GTK3 GUI that communicates with Electron's JavaScript.

Specifically, we'll be using GTK3 for our GUI interface, which provides:

* A comprehensive set of UI widgets like buttons, entry fields, and lists
* Cross-desktop compatibility across various Linux distributions
* Integration with the native theming and accessibility features of Linux desktops

> [!NOTE]
> We specifically use GTK3 because that's what Chromium (and by extension, Electron) uses internally. Using GTK4 would cause runtime conflicts since both GTK3 and GTK4 would be loaded in the same process. If and when Chromium upgrades to GTK4, you will likely be able to easily upgrade your native code to GTK4, too.

This tutorial will be most useful to those who already have some familiarity with GTK development on Linux. You should have experience with basic GTK concepts like widgets, signals, and the main event loop. In the interest of brevity, we're not spending too much time explaining the individual GTK elements we're using or the code we're writing for them. This allows this tutorial to be really helpful for those who already know GTK development and want to use their skills with Electron - without having to also be an entire GTK documentation.

> [!NOTE]
> If you're not already familiar with these concepts, the [GTK3 documentation](https://docs.gtk.org/gtk3/) and [GTK3 tutorials](https://docs.gtk.org/gtk3/getting_started.html) are excellent resources to get started. The [GNOME Developer Documentation](https://developer.gnome.org/) also provides comprehensive guides for GTK development.

## Requirements

Just like our general introduction to Native Code and Electron, this tutorial assumes you have Node.js and npm installed, as well as the basic tools necessary for compiling native code. Since this tutorial discusses writing native code that interacts with GTK3, you'll need:

* A Linux distribution with GTK3 development files installed
* The [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) tool
* G++ compiler and build tools

On Ubuntu/Debian, you can install these with:

```sh
sudo apt-get install build-essential pkg-config libgtk-3-dev
```

On Fedora/RHEL/CentOS:

```sh
sudo dnf install gcc-c++ pkgconfig gtk3-devel
```

## 1) Creating a package

You can re-use the package we created in our [Native Code and Electron](./native-code-and-electron.md) tutorial. This tutorial will not be repeating the steps described there. Let's first setup our basic addon folder structure:

```txt
cpp-linux/
├── binding.gyp          # Configuration file for node-gyp to build the native addon
├── include/
│   └── cpp_code.h       # Header file with declarations for our C++ native code
├── js/
│   └── index.js         # JavaScript interface that loads and exposes our native addon
├── package.json         # Node.js package configuration and dependencies
└── src/
    ├── cpp_addon.cc     # C++ code that bridges Node.js/Electron with our native code
    └── cpp_code.cc      # Implementation of our native C++ functionality using GTK3
```

Our package.json should look like this:

```json title='package.json'
{
  "name": "cpp-linux",
  "version": "1.0.0",
  "description": "A demo module that exposes C++ code to Electron",
  "main": "js/index.js",
  "scripts": {
    "clean": "rm -rf build",
    "build-electron": "electron-rebuild",
    "build": "node-gyp configure && node-gyp build"
  },
  "license": "MIT",
  "dependencies": {
    "node-addon-api": "^8.3.0",
    "bindings": "^1.5.0"
  }
}
```

## 2) Setting up the build configuration

For a Linux-specific addon using GTK3, we need to configure our `binding.gyp` file correctly to ensure our addon is only compiled on Linux systems - doing ideally nothing on other platforms. This involves using conditional compilation flags, leveraging `pkg-config` to automatically locate and include the GTK3 libraries and header paths on the user's system, and setting appropriate compiler flags to enable features like exception handling and threading support. The configuration will ensure that our native code can properly interface with both the Node.js/Electron runtime and the GTK3 libraries that provide the native GUI capabilities.

```json title='binding.gyp'
{
  "targets": [
    {
      "target_name": "cpp_addon",
      "conditions": [
        ['OS=="linux"', {
          "sources": [
            "src/cpp_addon.cc",
            "src/cpp_code.cc"
          ],
          "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")",
            "include",
            "<!@(pkg-config --cflags-only-I gtk+-3.0 | sed s/-I//g)"
          ],
          "libraries": [
            "<!@(pkg-config --libs gtk+-3.0)",
            "-luuid"
          ],
          "cflags": [
            "-fexceptions",
            "<!@(pkg-config --cflags gtk+-3.0)",
            "-pthread"
          ],
          "cflags_cc": [
            "-fexceptions",
            "<!@(pkg-config --cflags gtk+-3.0)",
            "-pthread"
          ],
          "ldflags": [
            "-pthread"
          ],
          "cflags!": ["-fno-exceptions"],
          "cflags_cc!": ["-fno-exceptions"],
          "defines": ["NODE_ADDON_API_CPP_EXCEPTIONS"],
          "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
          ]
        }]
      ]
    }
  ]
}
```

Let's examine the key parts of this configuration, starting with the `pkg-config` integration. The `<!@` syntax in a `binding.gyp` file is a command expansion operator. It executes the command inside the parentheses and uses the command's output as the value at that position. So, wherever you see `<!@` with `pkg-config` inside, know that we're calling a `pkg-config` command and using the output as our value. The `sed` command strips the `-I` prefix from the include paths to make them compatible with GYP's format.

## 3) Defining the C++ interface

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
void setTodoUpdatedCallback(TodoCallback callback);
void setTodoDeletedCallback(TodoCallback callback);

} // namespace cpp_code
```

This header defines:

* A basic `hello_world` function
* A `hello_gui` function to create a GTK3 GUI
* Callback types for Todo operations (add, update, delete)
* Setter functions for the callback

## 4) Implementing GTK3 GUI Code

Now, let's implement our GTK3 GUI in `src/cpp_code.cc`. We'll break this into manageable sections. We'll start with a number of includes as well as the basic setup.

### Basic Setup and Data Structures

```cpp title='src/cpp_code.cc'
#include <gtk/gtk.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <uuid/uuid.h>
#include <ctime>
#include <thread>
#include <memory>

using TodoCallback = std::function<void(const std::string &)>;

namespace cpp_code
{
  // Basic functions
  std::string hello_world(const std::string &input)
  {
    return "Hello from C++! You said: " + input;
  }

  // Data structures
  struct TodoItem
  {
    uuid_t id;
    std::string text;
    int64_t date;

    std::string toJson() const
    {
      char uuid_str[37];
      uuid_unparse(id, uuid_str);
      return "{"
             "\"id\":\"" +
             std::string(uuid_str) + "\","
                                     "\"text\":\"" +
             text + "\","
                    "\"date\":" +
             std::to_string(date) +
             "}";
    }

    static std::string formatDate(int64_t timestamp)
    {
      char date_str[64];
      time_t unix_time = timestamp / 1000;
      strftime(date_str, sizeof(date_str), "%Y-%m-%d", localtime(&unix_time));
      return date_str;
    }
  };
```

In this section:

* We include necessary headers for GTK3, standard library components, and UUID generation.
* Define a `TodoCallback` type to handle communication back to JavaScript.
* Create a `TodoItem` struct to store our todo data with:
  * A UUID for unique identification
  * Text content and a timestamp
  * A method to convert to JSON for sending to JavaScript
  * A static helper to format dates for display

The `toJson()` method is particularly important as it's what allows our C++ objects to be serialized for transmission to JavaScript. There are probably better ways to do that, but this tutorial is about combining C++ for native Linux UI development with Electron, so we'll give ourselves a pass for not writing better JSON serialization code here. There are many libraries to work with JSON in C++ with different trade-offs. See https://www.json.org/json-en.html for a list.

Notably, we haven't actually added any user interface yet - which we'll do in the next step. GTK code tends to be verbose, so bear with us - despite the length.

### Global state and forward declarations

Below the code already in your `src/cpp_code.cc`, add the following:

```cpp title='src/cpp_code.cc'
  // Forward declarations
  static void update_todo_row_label(GtkListBoxRow *row, const TodoItem &todo);
  static GtkWidget *create_todo_dialog(GtkWindow *parent, const TodoItem *existing_todo);

  // Global state
  namespace
  {
    TodoCallback g_todoAddedCallback;
    TodoCallback g_todoUpdatedCallback;
    TodoCallback g_todoDeletedCallback;
    GMainContext *g_gtk_main_context = nullptr;
    GMainLoop *g_main_loop = nullptr;
    std::thread *g_gtk_thread = nullptr;
    std::vector<TodoItem> g_todos;
  }
```

Here we:

* Forward-declare helper functions we'll use later
* Set up global state in an anonymous namespace, including:
  * Callbacks for the `add`, `update`, and `delete` todo operations
  * GTK main context and loop pointers for thread management
  * A pointer to the GTK thread itself
  * A vector to store our todos

These global variables keep track of application state and allow different parts of our code to interact with each other. The thread management variables (`g_gtk_main_context`, `g_main_loop`, and `g_gtk_thread`) are particularly important because GTK requires running in its own event loop. Since our code will be called from Node.js/Electron's main thread, we need to run GTK in a separate thread to avoid blocking the JavaScript event loop. This separation ensures that our native UI remains responsive while still allowing bidirectional communication with the Electron application. The callbacks enable us to send events back to JavaScript when the user interacts with our native GTK interface.

### Helper Functions

Moving on, we're adding more code below the code we've already written. In this section, we're adding three static helper methods - and also start setting up some actual native user interface. We'll add a helper function that'll notify a callback in a thread-safe way, a function to update a row label, and a function to create the whole "Add Todo" dialog.

```cpp title='src/cpp_code.cc'
  // Helper functions
  static void notify_callback(const TodoCallback &callback, const std::string &json)
  {
    if (callback && g_gtk_main_context)
    {
      g_main_context_invoke(g_gtk_main_context, [](gpointer data) -> gboolean
                            {
            auto* cb_data = static_cast<std::pair<TodoCallback, std::string>*>(data);
            cb_data->first(cb_data->second);
            delete cb_data;
            return G_SOURCE_REMOVE; }, new std::pair<TodoCallback, std::string>(callback, json));
    }
  }

  static void update_todo_row_label(GtkListBoxRow *row, const TodoItem &todo)
  {
    auto *label = gtk_label_new((todo.text + " - " + TodoItem::formatDate(todo.date)).c_str());
    auto *old_label = GTK_WIDGET(gtk_container_get_children(GTK_CONTAINER(row))->data);
    gtk_container_remove(GTK_CONTAINER(row), old_label);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_widget_show_all(GTK_WIDGET(row));
  }

  static GtkWidget *create_todo_dialog(GtkWindow *parent, const TodoItem *existing_todo = nullptr)
  {
    auto *dialog = gtk_dialog_new_with_buttons(
        existing_todo ? "Edit Todo" : "Add Todo",
        parent,
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        nullptr);

    auto *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    auto *entry = gtk_entry_new();
    if (existing_todo)
    {
      gtk_entry_set_text(GTK_ENTRY(entry), existing_todo->text.c_str());
    }
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    auto *calendar = gtk_calendar_new();
    if (existing_todo)
    {
      time_t unix_time = existing_todo->date / 1000;
      struct tm *timeinfo = localtime(&unix_time);
      gtk_calendar_select_month(GTK_CALENDAR(calendar), timeinfo->tm_mon, timeinfo->tm_year + 1900);
      gtk_calendar_select_day(GTK_CALENDAR(calendar), timeinfo->tm_mday);
    }
    gtk_container_add(GTK_CONTAINER(content_area), calendar);

    gtk_widget_show_all(dialog);
    return dialog;
  }
```

These helper functions are crucial for our application:

* `notify_callback`: Safely invokes JavaScript callbacks from the GTK thread using `g_main_context_invoke`, which schedules function execution in the GTK main context. As a reminder, the GTK main context is the environment where GTK operations must be performed to ensure thread safety, as GTK is not thread-safe and all UI operations must happen on the main thread.
* `update_todo_row_label`: Updates a row in the todo list with new text and formatted date.
* `create_todo_dialog`: Creates a dialog for adding or editing todos with:
  * A text entry field for the todo text
  * A calendar widget for selecting the date
  * Appropriate buttons for saving or canceling

### Event handlers

Our native user interface has events - and those events must be handled. The only Electron-specific thing in this code is that we're notifying our JS callbacks.

```cpp title='src/cpp_code.cc'
  static void edit_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));
    auto *row = gtk_list_box_get_selected_row(list);
    if (!row)
      return;

    gint index = gtk_list_box_row_get_index(row);
    auto size = static_cast<gint>(g_todos.size());
    if (index < 0 || index >= size)
      return;

    auto *dialog = create_todo_dialog(
        GTK_WINDOW(gtk_builder_get_object(builder, "window")),
        &g_todos[index]);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
      auto *entry = GTK_ENTRY(gtk_container_get_children(
                                  GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))))
                                  ->data);
      auto *calendar = GTK_CALENDAR(gtk_container_get_children(
                                        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))))
                                        ->next->data);

      const char *new_text = gtk_entry_get_text(entry);

      guint year, month, day;
      gtk_calendar_get_date(calendar, &year, &month, &day);
      GDateTime *datetime = g_date_time_new_local(year, month + 1, day, 0, 0, 0);
      gint64 new_date = g_date_time_to_unix(datetime) * 1000;
      g_date_time_unref(datetime);

      g_todos[index].text = new_text;
      g_todos[index].date = new_date;

      update_todo_row_label(row, g_todos[index]);
      notify_callback(g_todoUpdatedCallback, g_todos[index].toJson());
    }

    gtk_widget_destroy(dialog);
  }

  static void delete_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));
    auto *row = gtk_list_box_get_selected_row(list);
    if (!row)
      return;

    gint index = gtk_list_box_row_get_index(row);
    auto size = static_cast<gint>(g_todos.size());
    if (index < 0 || index >= size)
      return;

    std::string json = g_todos[index].toJson();
    gtk_container_remove(GTK_CONTAINER(list), GTK_WIDGET(row));
    g_todos.erase(g_todos.begin() + index);
    notify_callback(g_todoDeletedCallback, json);
  }

  static void on_add_clicked(GtkButton *button, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *entry = GTK_ENTRY(gtk_builder_get_object(builder, "todo_entry"));
    auto *calendar = GTK_CALENDAR(gtk_builder_get_object(builder, "todo_calendar"));
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));

    const char *text = gtk_entry_get_text(entry);
    if (strlen(text) > 0)
    {
      TodoItem todo;
      uuid_generate(todo.id);
      todo.text = text;

      guint year, month, day;
      gtk_calendar_get_date(calendar, &year, &month, &day);
      GDateTime *datetime = g_date_time_new_local(year, month + 1, day, 0, 0, 0);
      todo.date = g_date_time_to_unix(datetime) * 1000;
      g_date_time_unref(datetime);

      g_todos.push_back(todo);

      auto *row = gtk_list_box_row_new();
      auto *label = gtk_label_new((todo.text + " - " + TodoItem::formatDate(todo.date)).c_str());
      gtk_container_add(GTK_CONTAINER(row), label);
      gtk_container_add(GTK_CONTAINER(list), row);
      gtk_widget_show_all(row);

      gtk_entry_set_text(entry, "");

      notify_callback(g_todoAddedCallback, todo.toJson());
    }
  }

  static void on_row_activated(GtkListBox *list_box, GtkListBoxRow *row, gpointer user_data)
  {
    GMenu *menu = g_menu_new();
    g_menu_append(menu, "Edit", "app.edit");
    g_menu_append(menu, "Delete", "app.delete");

    auto *popover = gtk_popover_new_from_model(GTK_WIDGET(row), G_MENU_MODEL(menu));
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_RIGHT);
    gtk_popover_popup(GTK_POPOVER(popover));

    g_object_unref(menu);
  }
```

These event handlers manage user interactions:

`edit_action`: Handles editing a todo by:

* Getting the selected row
* Creating a dialog with the current todo data
* Updating the todo if the user confirms
* Notifying JavaScript via callback

`delete_action`: Removes a todo and notifies JavaScript.

`on_add_clicked`: Adds a new todo when the user clicks the Add button:

* Gets text and date from input fields
* Creates a new TodoItem with a unique ID
* Adds it to the list and the underlying data store
* Notifies JavaScript

`on_row_activated`: Shows a popup menu when a todo is clicked, with options to edit or delete.

### GTK application setup

Now, we'll need to setup our GTK application. This might be counter-intuitive, given that we already have a GTK application running. The activation code here is necessary because this is native C++ code running alongside Electron, not within it. While Electron does have its own main process and renderer processes, this GTK application operates as a native OS window that's launched from the Electron application but runs in its own process or thread. The `hello_gui()` function specifically starts the GTK application with its own thread (`g_gtk_thread`), application loop, and UI context.

```cpp title='src/cpp_code.cc'
  static gboolean init_gtk_app(gpointer user_data)
  {
    auto *app = static_cast<GtkApplication *>(user_data);
    g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);
    if (g_main_loop)
    {
      g_main_loop_quit(g_main_loop);
    }
    return G_SOURCE_REMOVE;
  }

  static void activate_handler(GtkApplication *app, gpointer user_data)
  {
    auto *builder = gtk_builder_new();

    const GActionEntry app_actions[] = {
        {"edit", edit_action, nullptr, nullptr, nullptr, {0, 0, 0}},
        {"delete", delete_action, nullptr, nullptr, nullptr, {0, 0, 0}}};
    g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions,
                                    G_N_ELEMENTS(app_actions), builder);

    gtk_builder_add_from_string(builder,
                                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                "<interface>"
                                "  <object class=\"GtkWindow\" id=\"window\">"
                                "    <property name=\"title\">Todo List</property>"
                                "    <property name=\"default-width\">400</property>"
                                "    <property name=\"default-height\">500</property>"
                                "    <child>"
                                "      <object class=\"GtkBox\">"
                                "        <property name=\"visible\">true</property>"
                                "        <property name=\"orientation\">vertical</property>"
                                "        <property name=\"spacing\">6</property>"
                                "        <property name=\"margin\">12</property>"
                                "        <child>"
                                "          <object class=\"GtkBox\">"
                                "            <property name=\"visible\">true</property>"
                                "            <property name=\"spacing\">6</property>"
                                "            <child>"
                                "              <object class=\"GtkEntry\" id=\"todo_entry\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"hexpand\">true</property>"
                                "                <property name=\"placeholder-text\">Enter todo item...</property>"
                                "              </object>"
                                "            </child>"
                                "            <child>"
                                "              <object class=\"GtkCalendar\" id=\"todo_calendar\">"
                                "                <property name=\"visible\">true</property>"
                                "              </object>"
                                "            </child>"
                                "            <child>"
                                "              <object class=\"GtkButton\" id=\"add_button\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"label\">Add</property>"
                                "              </object>"
                                "            </child>"
                                "          </object>"
                                "        </child>"
                                "        <child>"
                                "          <object class=\"GtkScrolledWindow\">"
                                "            <property name=\"visible\">true</property>"
                                "            <property name=\"vexpand\">true</property>"
                                "            <child>"
                                "              <object class=\"GtkListBox\" id=\"todo_list\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"selection-mode\">single</property>"
                                "              </object>"
                                "            </child>"
                                "          </object>"
                                "        </child>"
                                "      </object>"
                                "    </child>"
                                "  </object>"
                                "</interface>",
                                -1, nullptr);

    auto *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
    auto *button = GTK_BUTTON(gtk_builder_get_object(builder, "add_button"));
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));

    gtk_window_set_application(window, app);

    g_signal_connect(button, "clicked", G_CALLBACK(on_add_clicked), builder);
    g_signal_connect(list, "row-activated", G_CALLBACK(on_row_activated), nullptr);

    gtk_widget_show_all(GTK_WIDGET(window));
  }
```

Let's take a closer look at the code above:

* `init_gtk_app`: Runs the GTK application main loop.
* `activate_handler`: Sets up the application UI when activated:
  * Creates a GtkBuilder for loading the UI
  * Registers edit and delete actions
  * Defines the UI layout using GTK's XML markup language
  * Connects signals to our event handlers

The UI layout is defined inline using XML, which is a common pattern in GTK applications. It creates a main window, input controls (text entry, calendar, and add button), a list box for displaying todos, and proper layout containers and scrolling.

### Main GUI function and thread management

Now that we have everything wired, up, we can add our two core GUI functions: `hello_gui()` (which we'll call from JavaScript) and `cleanup_gui()` to get rid of everything. You'll be hopefully delighted to hear that our careful setup of GTK app, context, and threads makes this straightforward:

```cpp title='src/cpp_code.cc'
  void hello_gui()
  {
    if (g_gtk_thread != nullptr)
    {
      g_print("GTK application is already running.\n");
      return;
    }

    if (!gtk_init_check(0, nullptr))
    {
      g_print("Failed to initialize GTK.\n");
      return;
    }

    g_gtk_main_context = g_main_context_new();
    g_main_loop = g_main_loop_new(g_gtk_main_context, FALSE);

    g_gtk_thread = new std::thread([]()
                                   {
        GtkApplication* app = gtk_application_new("com.example.todo", G_APPLICATION_NON_UNIQUE);
        g_signal_connect(app, "activate", G_CALLBACK(activate_handler), nullptr);

        g_idle_add_full(G_PRIORITY_DEFAULT, init_gtk_app, app, nullptr);

        if (g_main_loop) {
            g_main_loop_run(g_main_loop);
        } });

    g_gtk_thread->detach();
  }

  void cleanup_gui()
  {
    if (g_main_loop && g_main_loop_is_running(g_main_loop))
    {
      g_main_loop_quit(g_main_loop);
    }

    if (g_main_loop)
    {
      g_main_loop_unref(g_main_loop);
      g_main_loop = nullptr;
    }

    if (g_gtk_main_context)
    {
      g_main_context_unref(g_gtk_main_context);
      g_gtk_main_context = nullptr;
    }

    g_gtk_thread = nullptr;
  }
```

These functions manage the GTK application lifecycle:

* `hello_gui`: The entry point exposed to JavaScript that checks if GTK is already running, initializes GTK, creates a new main context and loop, launches a thread to run the GTK application, and detaches the thread so it runs independently.
* `cleanup_gui`: Properly cleans up GTK resources when the application closes.

Running GTK in a separate thread is crucial for Electron integration, as it prevents the GTK main loop from blocking Node.js's event loop.

### Callback management

Previously, we setup global variables to hold our callbacks. Now, we'll add functions that assign those callbacks. These callbacks form the bridge between our native GTK code and JavaScript, allowing bidirectional communication.

```cpp title='src/cpp_code.cc'
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
```

### Putting `cpp_code.cc` together

We've now finished the GTK and native part of our addon - that is, the code that's most concerned with interacting with the operating system (and by contrast, less so with bridging the native C++ and JavaScript worlds). After adding all the sections above, your `src/cpp_code.cc` should look like this:

```cpp title='src/cpp_code.cc'
#include <gtk/gtk.h>
#include <string>
#include <functional>
#include <chrono>
#include <vector>
#include <uuid/uuid.h>
#include <ctime>
#include <thread>
#include <memory>

using TodoCallback = std::function<void(const std::string &)>;

namespace cpp_code
{

  // Basic functions
  std::string hello_world(const std::string &input)
  {
    return "Hello from C++! You said: " + input;
  }

  // Data structures
  struct TodoItem
  {
    uuid_t id;
    std::string text;
    int64_t date;

    std::string toJson() const
    {
      char uuid_str[37];
      uuid_unparse(id, uuid_str);
      return "{"
             "\"id\":\"" +
             std::string(uuid_str) + "\","
                                     "\"text\":\"" +
             text + "\","
                    "\"date\":" +
             std::to_string(date) +
             "}";
    }

    static std::string formatDate(int64_t timestamp)
    {
      char date_str[64];
      time_t unix_time = timestamp / 1000;
      strftime(date_str, sizeof(date_str), "%Y-%m-%d", localtime(&unix_time));
      return date_str;
    }
  };

  // Forward declarations
  static void update_todo_row_label(GtkListBoxRow *row, const TodoItem &todo);
  static GtkWidget *create_todo_dialog(GtkWindow *parent, const TodoItem *existing_todo);

  // Global state
  namespace
  {
    TodoCallback g_todoAddedCallback;
    TodoCallback g_todoUpdatedCallback;
    TodoCallback g_todoDeletedCallback;
    GMainContext *g_gtk_main_context = nullptr;
    GMainLoop *g_main_loop = nullptr;
    std::thread *g_gtk_thread = nullptr;
    std::vector<TodoItem> g_todos;
  }

  // Helper functions
  static void notify_callback(const TodoCallback &callback, const std::string &json)
  {
    if (callback && g_gtk_main_context)
    {
      g_main_context_invoke(g_gtk_main_context, [](gpointer data) -> gboolean
                            {
            auto* cb_data = static_cast<std::pair<TodoCallback, std::string>*>(data);
            cb_data->first(cb_data->second);
            delete cb_data;
            return G_SOURCE_REMOVE; }, new std::pair<TodoCallback, std::string>(callback, json));
    }
  }

  static void update_todo_row_label(GtkListBoxRow *row, const TodoItem &todo)
  {
    auto *label = gtk_label_new((todo.text + " - " + TodoItem::formatDate(todo.date)).c_str());
    auto *old_label = GTK_WIDGET(gtk_container_get_children(GTK_CONTAINER(row))->data);
    gtk_container_remove(GTK_CONTAINER(row), old_label);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_widget_show_all(GTK_WIDGET(row));
  }

  static GtkWidget *create_todo_dialog(GtkWindow *parent, const TodoItem *existing_todo = nullptr)
  {
    auto *dialog = gtk_dialog_new_with_buttons(
        existing_todo ? "Edit Todo" : "Add Todo",
        parent,
        GTK_DIALOG_MODAL,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        nullptr);

    auto *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    auto *entry = gtk_entry_new();
    if (existing_todo)
    {
      gtk_entry_set_text(GTK_ENTRY(entry), existing_todo->text.c_str());
    }
    gtk_container_add(GTK_CONTAINER(content_area), entry);

    auto *calendar = gtk_calendar_new();
    if (existing_todo)
    {
      time_t unix_time = existing_todo->date / 1000;
      struct tm *timeinfo = localtime(&unix_time);
      gtk_calendar_select_month(GTK_CALENDAR(calendar), timeinfo->tm_mon, timeinfo->tm_year + 1900);
      gtk_calendar_select_day(GTK_CALENDAR(calendar), timeinfo->tm_mday);
    }
    gtk_container_add(GTK_CONTAINER(content_area), calendar);

    gtk_widget_show_all(dialog);
    return dialog;
  }

  static void edit_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));
    auto *row = gtk_list_box_get_selected_row(list);
    if (!row)
      return;

    gint index = gtk_list_box_row_get_index(row);
    auto size = static_cast<gint>(g_todos.size());
    if (index < 0 || index >= size)
      return;

    auto *dialog = create_todo_dialog(
        GTK_WINDOW(gtk_builder_get_object(builder, "window")),
        &g_todos[index]);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
      auto *entry = GTK_ENTRY(gtk_container_get_children(
                                  GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))))
                                  ->data);
      auto *calendar = GTK_CALENDAR(gtk_container_get_children(
                                        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))))
                                        ->next->data);

      const char *new_text = gtk_entry_get_text(entry);

      guint year, month, day;
      gtk_calendar_get_date(calendar, &year, &month, &day);
      GDateTime *datetime = g_date_time_new_local(year, month + 1, day, 0, 0, 0);
      gint64 new_date = g_date_time_to_unix(datetime) * 1000;
      g_date_time_unref(datetime);

      g_todos[index].text = new_text;
      g_todos[index].date = new_date;

      update_todo_row_label(row, g_todos[index]);
      notify_callback(g_todoUpdatedCallback, g_todos[index].toJson());
    }

    gtk_widget_destroy(dialog);
  }

  static void delete_action(GSimpleAction *action, GVariant *parameter, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));
    auto *row = gtk_list_box_get_selected_row(list);
    if (!row)
      return;

    gint index = gtk_list_box_row_get_index(row);
    auto size = static_cast<gint>(g_todos.size());
    if (index < 0 || index >= size)
      return;

    std::string json = g_todos[index].toJson();
    gtk_container_remove(GTK_CONTAINER(list), GTK_WIDGET(row));
    g_todos.erase(g_todos.begin() + index);
    notify_callback(g_todoDeletedCallback, json);
  }

  static void on_add_clicked(GtkButton *button, gpointer user_data)
  {
    auto *builder = static_cast<GtkBuilder *>(user_data);
    auto *entry = GTK_ENTRY(gtk_builder_get_object(builder, "todo_entry"));
    auto *calendar = GTK_CALENDAR(gtk_builder_get_object(builder, "todo_calendar"));
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));

    const char *text = gtk_entry_get_text(entry);
    if (strlen(text) > 0)
    {
      TodoItem todo;
      uuid_generate(todo.id);
      todo.text = text;

      guint year, month, day;
      gtk_calendar_get_date(calendar, &year, &month, &day);
      GDateTime *datetime = g_date_time_new_local(year, month + 1, day, 0, 0, 0);
      todo.date = g_date_time_to_unix(datetime) * 1000;
      g_date_time_unref(datetime);

      g_todos.push_back(todo);

      auto *row = gtk_list_box_row_new();
      auto *label = gtk_label_new((todo.text + " - " + TodoItem::formatDate(todo.date)).c_str());
      gtk_container_add(GTK_CONTAINER(row), label);
      gtk_container_add(GTK_CONTAINER(list), row);
      gtk_widget_show_all(row);

      gtk_entry_set_text(entry, "");

      notify_callback(g_todoAddedCallback, todo.toJson());
    }
  }

  static void on_row_activated(GtkListBox *list_box, GtkListBoxRow *row, gpointer user_data)
  {
    GMenu *menu = g_menu_new();
    g_menu_append(menu, "Edit", "app.edit");
    g_menu_append(menu, "Delete", "app.delete");

    auto *popover = gtk_popover_new_from_model(GTK_WIDGET(row), G_MENU_MODEL(menu));
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_RIGHT);
    gtk_popover_popup(GTK_POPOVER(popover));

    g_object_unref(menu);
  }

  static gboolean init_gtk_app(gpointer user_data)
  {
    auto *app = static_cast<GtkApplication *>(user_data);
    g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);
    if (g_main_loop)
    {
      g_main_loop_quit(g_main_loop);
    }
    return G_SOURCE_REMOVE;
  }

  static void activate_handler(GtkApplication *app, gpointer user_data)
  {
    auto *builder = gtk_builder_new();

    const GActionEntry app_actions[] = {
        {"edit", edit_action, nullptr, nullptr, nullptr, {0, 0, 0}},
        {"delete", delete_action, nullptr, nullptr, nullptr, {0, 0, 0}}};
    g_action_map_add_action_entries(G_ACTION_MAP(app), app_actions,
                                    G_N_ELEMENTS(app_actions), builder);

    gtk_builder_add_from_string(builder,
                                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                "<interface>"
                                "  <object class=\"GtkWindow\" id=\"window\">"
                                "    <property name=\"title\">Todo List</property>"
                                "    <property name=\"default-width\">400</property>"
                                "    <property name=\"default-height\">500</property>"
                                "    <child>"
                                "      <object class=\"GtkBox\">"
                                "        <property name=\"visible\">true</property>"
                                "        <property name=\"orientation\">vertical</property>"
                                "        <property name=\"spacing\">6</property>"
                                "        <property name=\"margin\">12</property>"
                                "        <child>"
                                "          <object class=\"GtkBox\">"
                                "            <property name=\"visible\">true</property>"
                                "            <property name=\"spacing\">6</property>"
                                "            <child>"
                                "              <object class=\"GtkEntry\" id=\"todo_entry\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"hexpand\">true</property>"
                                "                <property name=\"placeholder-text\">Enter todo item...</property>"
                                "              </object>"
                                "            </child>"
                                "            <child>"
                                "              <object class=\"GtkCalendar\" id=\"todo_calendar\">"
                                "                <property name=\"visible\">true</property>"
                                "              </object>"
                                "            </child>"
                                "            <child>"
                                "              <object class=\"GtkButton\" id=\"add_button\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"label\">Add</property>"
                                "              </object>"
                                "            </child>"
                                "          </object>"
                                "        </child>"
                                "        <child>"
                                "          <object class=\"GtkScrolledWindow\">"
                                "            <property name=\"visible\">true</property>"
                                "            <property name=\"vexpand\">true</property>"
                                "            <child>"
                                "              <object class=\"GtkListBox\" id=\"todo_list\">"
                                "                <property name=\"visible\">true</property>"
                                "                <property name=\"selection-mode\">single</property>"
                                "              </object>"
                                "            </child>"
                                "          </object>"
                                "        </child>"
                                "      </object>"
                                "    </child>"
                                "  </object>"
                                "</interface>",
                                -1, nullptr);

    auto *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
    auto *button = GTK_BUTTON(gtk_builder_get_object(builder, "add_button"));
    auto *list = GTK_LIST_BOX(gtk_builder_get_object(builder, "todo_list"));

    gtk_window_set_application(window, app);

    g_signal_connect(button, "clicked", G_CALLBACK(on_add_clicked), builder);
    g_signal_connect(list, "row-activated", G_CALLBACK(on_row_activated), nullptr);

    gtk_widget_show_all(GTK_WIDGET(window));
  }

  void hello_gui()
  {
    if (g_gtk_thread != nullptr)
    {
      g_print("GTK application is already running.\n");
      return;
    }

    if (!gtk_init_check(0, nullptr))
    {
      g_print("Failed to initialize GTK.\n");
      return;
    }

    g_gtk_main_context = g_main_context_new();
    g_main_loop = g_main_loop_new(g_gtk_main_context, FALSE);

    g_gtk_thread = new std::thread([]()
                                   {
        GtkApplication* app = gtk_application_new("com.example.todo", G_APPLICATION_NON_UNIQUE);
        g_signal_connect(app, "activate", G_CALLBACK(activate_handler), nullptr);

        g_idle_add_full(G_PRIORITY_DEFAULT, init_gtk_app, app, nullptr);

        if (g_main_loop) {
            g_main_loop_run(g_main_loop);
        } });

    g_gtk_thread->detach();
  }

  void cleanup_gui()
  {
    if (g_main_loop && g_main_loop_is_running(g_main_loop))
    {
      g_main_loop_quit(g_main_loop);
    }

    if (g_main_loop)
    {
      g_main_loop_unref(g_main_loop);
      g_main_loop = nullptr;
    }

    if (g_gtk_main_context)
    {
      g_main_context_unref(g_gtk_main_context);
      g_gtk_main_context = nullptr;
    }

    g_gtk_thread = nullptr;
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

} // namespace cpp_code
```

## 5) Creating the Node.js addon bridge

Now let's implement the bridge between our C++ code and Node.js in `src/cpp_addon.cc`. Let's start by creating a basic skeleton for our addon:

```cpp title='src/cpp_addon.cc'
#include <napi.h>
#include <string>
#include "cpp_code.h"

// Class to wrap our C++ code will go here

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  // We'll add code here later
  return exports;
}

NODE_API_MODULE(cpp_addon, Init)
```

This is the minimal structure required for a Node.js addon using `node-addon-api`. The `Init` function is called when the addon is loaded, and the `NODE_API_MODULE` macro registers our initializer. This basic skeleton doesn't do anything yet, but it provides the entry point for Node.js to load our native code.

### Create a class to wrap our C++ code

Let's create a class that will wrap our C++ code and expose it to JavaScript. In our previous step, we've added a comment reading "Class to wrap our C++ code will go here" - replace it with the code below.

```cpp title='src/cpp_addon.cc'
class CppAddon : public Napi::ObjectWrap<CppAddon>
{
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports)
  {
    Napi::Function func = DefineClass(env, "CppLinuxAddon", {
      InstanceMethod("helloWorld", &CppAddon::HelloWorld),
      InstanceMethod("helloGui", &CppAddon::HelloGui),
      InstanceMethod("on", &CppAddon::On)
    });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("CppLinuxAddon", func);
    return exports;
  }

  CppAddon(const Napi::CallbackInfo &info)
      : Napi::ObjectWrap<CppAddon>(info),
        env_(info.Env()),
        emitter(Napi::Persistent(Napi::Object::New(info.Env()))),
        callbacks(Napi::Persistent(Napi::Object::New(info.Env()))),
        tsfn_(nullptr)
  {
    // We'll implement the constructor together with a callback struct later
  }

  ~CppAddon()
  {
    if (tsfn_ != nullptr)
    {
      napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
      tsfn_ = nullptr;
    }
  }

private:
  Napi::Env env_;
  Napi::ObjectReference emitter;
  Napi::ObjectReference callbacks;
  napi_threadsafe_function tsfn_;

  // Method implementations will go here
};
```

Here, we create a C++ class that inherits from `Napi::ObjectWrap<CppAddon>`:

`static Napi::Object Init` defines our JavaScript interface with three methods:

* `helloWorld`: A simple function to test the bridge
* `helloGui`: The function to launch our GTK3 UI
* `on`: A method to register event callbacks

The constructor initializes:

* `emitter`: An object that will emit events to JavaScript
* `callbacks`: A map of registered JavaScript callback functions
* `tsfn_`: A thread-safe function handle (crucial for GTK3 thread communication)

The destructor properly cleans up the thread-safe function when the object is garbage collected.

### Implement basic functionality - HelloWorld

Next, we'll add our two main methods, `HelloWorld()` and `HelloGui()`. We'll add these to our `private` scope, right where we have a comment reading "Method implementations will go here".

```cpp title='src/cpp_addon.cc'
Napi::Value HelloWorld(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsString())
  {
    Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string input = info[0].As<Napi::String>();
  std::string result = cpp_code::hello_world(input);

  return Napi::String::New(env, result);
}

void HelloGui(const Napi::CallbackInfo &info)
{
  cpp_code::hello_gui();
}

// On() method implementation will go here
```

`HelloWorld()`:

* Validates the input argument (must be a string)
* Calls our C++ hello_world function
* Returns the result as a JavaScript string

`HelloGui()`:

* Simply calls our C++ hello_gui function without arguments
* Returns nothing (void) as the function just launches the UI
* These methods form the direct bridge between JavaScript calls and our native C++ functions.

You might be wondering what `Napi::CallbackInfo` is or where it comes from. This is a class provided by the Node-API (N-API) C++ wrapper, specifically from the [`node-addon-api`](https://github.com/nodejs/node-addon-api) package. It encapsulates all the information about a JavaScript function call, including:

* The arguments passed from JavaScript
* The JavaScript execution environment (via `info.Env()`)
* The `this` value of the function call
* The number of arguments (via `info.Length()`)

This class is fundamental to the Node.js native addon development as it serves as the bridge between JavaScript function calls and C++ method implementations. Every native method that can be called from JavaScript receives a `CallbackInfo` object as its parameter, allowing the C++ code to access and validate the JavaScript arguments before processing them. You can see us using it in `HelloWorld()` to get function parameters and other information about the function call. Our `HelloGui()` function doesn't use it, but if it did, it'd follow the same pattern.

### Setting up the event system

Now we'll tackle the tricky part of native development: setting up the event system. Previously, we added native callbacks to our `cpp_code.cc` code - and in our bridge code in `cpp_addon.cc`, we'll need to find a way to have those callbacks ultimately trigger a JavaScript method.

Let's start with the `On()` method, which we'll call from JavaScript. In our previously written code, you'll find a comment reading `On() method implementation will go here`. Replace it with the following method:

```cpp title='src/cpp_addon.cc'
Napi::Value On(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
  {
    Napi::TypeError::New(env, "Expected (string, function) arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  callbacks.Value().Set(info[0].As<Napi::String>(), info[1].As<Napi::Function>());
  return env.Undefined();
}
```

This method allows JavaScript to register callbacks for different event types and stores the JavaScript function in our `callbacks` map for later use. So far, so good - but now we need to let `cpp_code.cc` know about these callbacks. We also need to figure out a way to coordinate our threads, because the actual `cpp_code.cc` will be doing most of its work on its own thread.

In our code, find the section where we're declaring the constructor `CppAddon(const Napi::CallbackInfo &info)`, which you'll find in the `public` section. It should have a comment reading `We'll implement the constructor together with a callback struct later`. Then, replace that part with the following code:

```cpp title='src/cpp_addon.cc'
  struct CallbackData
  {
    std::string eventType;
    std::string payload;
    CppAddon *addon;
  };

  CppAddon(const Napi::CallbackInfo &info)
      : Napi::ObjectWrap<CppAddon>(info),
        env_(info.Env()),
        emitter(Napi::Persistent(Napi::Object::New(info.Env()))),
        callbacks(Napi::Persistent(Napi::Object::New(info.Env()))),
        tsfn_(nullptr)
  {
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
        [](napi_env env, napi_value js_callback, void *context, void *data)
        {
          auto *callbackData = static_cast<CallbackData *>(data);
          if (!callbackData)
            return;

          Napi::Env napi_env(env);
          Napi::HandleScope scope(napi_env);

          auto addon = static_cast<CppAddon *>(context);
          if (!addon)
          {
            delete callbackData;
            return;
          }

          try
          {
            auto callback = addon->callbacks.Value().Get(callbackData->eventType).As<Napi::Function>();
            if (callback.IsFunction())
            {
              callback.Call(addon->emitter.Value(), {Napi::String::New(napi_env, callbackData->payload)});
            }
          }
          catch (...)
          {
          }

          delete callbackData;
        },
        &tsfn_);

    if (status != napi_ok)
    {
      Napi::Error::New(env_, "Failed to create threadsafe function").ThrowAsJavaScriptException();
      return;
    }

    // Set up the callbacks here
    auto makeCallback = [this](const std::string &eventType)
    {
      return [this, eventType](const std::string &payload)
      {
        if (tsfn_ != nullptr)
        {
          auto *data = new CallbackData{
              eventType,
              payload,
              this};
          napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
        }
      };
    };

    cpp_code::setTodoAddedCallback(makeCallback("todoAdded"));
    cpp_code::setTodoUpdatedCallback(makeCallback("todoUpdated"));
    cpp_code::setTodoDeletedCallback(makeCallback("todoDeleted"));
  }
```

This is the most complex part of our bridge: implementing bidirectional communication. There are a few things worth noting going on here, so let's take them step by step:

`CallbackData` struct:

* Holds the event type, JSON payload, and a reference to our addon.

In the constructor:

* We create a thread-safe function (`napi_create_threadsafe_function`) which is crucial for calling into JavaScript from the GTK3 thread
* The thread-safe function callback unpacks the data and calls the appropriate JavaScript callback
* We create a lambda `makeCallback` that produces callback functions for different event types
* We register these callbacks with our C++ code using the setter functions

Let's talk about `napi_create_threadsafe_function`. The orchestration of different threads is maybe the most difficult part about native addon development - and in our experience, the place where developers are most likely to give up. `napi_create_threadsafe_function` is provided by the N-API and allows you to safely call JavaScript functions from any thread. This is essential when working with GUI frameworks like GTK3 that run on their own thread. Here's why it's important:

1. **Thread Safety**: JavaScript in Electron runs on a single thread (exceptions apply, but this is a generally useful rule). Without thread-safe functions, calling JavaScript from another thread would cause crashes or race conditions.
1. **Queue Management**: It automatically queues function calls and executes them on the JavaScript thread.
1. **Resource Management**: It handles proper reference counting to ensure objects aren't garbage collected while still needed.

In our code, we're using it to bridge the gap between GTK3's event loop and Node.js's event loop, allowing events from our GUI to safely trigger JavaScript callbacks.

For developers wanting to learn more, you can refer to the [official N-API documentation](https://nodejs.org/api/n-api.html#n_api_napi_create_threadsafe_function) for detailed information about thread-safe functions, the [node-addon-api wrapper documentation](https://github.com/nodejs/node-addon-api/blob/main/doc/threadsafe_function.md) for the C++ wrapper implementation, and the [Node.js Threading Model article](https://nodejs.org/en/docs/guides/dont-block-the-event-loop/) to understand how Node.js handles concurrency and why thread-safe functions are necessary.

### Putting `cpp_addon.cc` together

We've now finished the bridge part our addon - that is, the code that's most concerned with being the bridge between your JavaScript and C++ code (and by contrast, less so actually interacting with the operating system or GTK). After adding all the sections above, your `src/cpp_addon.cc` should look like this:

```cpp title='src/cpp_addon.cc'
#include <napi.h>
#include <string>
#include "cpp_code.h"

class CppAddon : public Napi::ObjectWrap<CppAddon>
{
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports)
  {
    Napi::Function func = DefineClass(env, "CppLinuxAddon", {
      InstanceMethod("helloWorld", &CppAddon::HelloWorld),
      InstanceMethod("helloGui", &CppAddon::HelloGui),
      InstanceMethod("on", &CppAddon::On)
    });

    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("CppLinuxAddon", func);
    return exports;
  }

  struct CallbackData
  {
    std::string eventType;
    std::string payload;
    CppAddon *addon;
  };

  CppAddon(const Napi::CallbackInfo &info)
      : Napi::ObjectWrap<CppAddon>(info),
        env_(info.Env()),
        emitter(Napi::Persistent(Napi::Object::New(info.Env()))),
        callbacks(Napi::Persistent(Napi::Object::New(info.Env()))),
        tsfn_(nullptr)
  {
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
        [](napi_env env, napi_value js_callback, void *context, void *data)
        {
          auto *callbackData = static_cast<CallbackData *>(data);
          if (!callbackData)
            return;

          Napi::Env napi_env(env);
          Napi::HandleScope scope(napi_env);

          auto addon = static_cast<CppAddon *>(context);
          if (!addon)
          {
            delete callbackData;
            return;
          }

          try
          {
            auto callback = addon->callbacks.Value().Get(callbackData->eventType).As<Napi::Function>();
            if (callback.IsFunction())
            {
              callback.Call(addon->emitter.Value(), {Napi::String::New(napi_env, callbackData->payload)});
            }
          }
          catch (...)
          {
          }

          delete callbackData;
        },
        &tsfn_);

    if (status != napi_ok)
    {
      Napi::Error::New(env_, "Failed to create threadsafe function").ThrowAsJavaScriptException();
      return;
    }

    // Set up the callbacks here
    auto makeCallback = [this](const std::string &eventType)
    {
      return [this, eventType](const std::string &payload)
      {
        if (tsfn_ != nullptr)
        {
          auto *data = new CallbackData{
              eventType,
              payload,
              this};
          napi_call_threadsafe_function(tsfn_, data, napi_tsfn_blocking);
        }
      };
    };

    cpp_code::setTodoAddedCallback(makeCallback("todoAdded"));
    cpp_code::setTodoUpdatedCallback(makeCallback("todoUpdated"));
    cpp_code::setTodoDeletedCallback(makeCallback("todoDeleted"));
  }

  ~CppAddon()
  {
    if (tsfn_ != nullptr)
    {
      napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
      tsfn_ = nullptr;
    }
  }

private:
  Napi::Env env_;
  Napi::ObjectReference emitter;
  Napi::ObjectReference callbacks;
  napi_threadsafe_function tsfn_;

  Napi::Value HelloWorld(const Napi::CallbackInfo &info)
  {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString())
    {
      Napi::TypeError::New(env, "Expected string argument").ThrowAsJavaScriptException();
      return env.Null();
    }

    std::string input = info[0].As<Napi::String>();
    std::string result = cpp_code::hello_world(input);

    return Napi::String::New(env, result);
  }

  void HelloGui(const Napi::CallbackInfo &info)
  {
    cpp_code::hello_gui();
  }

  Napi::Value On(const Napi::CallbackInfo &info)
  {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction())
    {
      Napi::TypeError::New(env, "Expected (string, function) arguments").ThrowAsJavaScriptException();
      return env.Undefined();
    }

    callbacks.Value().Set(info[0].As<Napi::String>(), info[1].As<Napi::Function>());
    return env.Undefined();
  }
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return CppAddon::Init(env, exports);
}

NODE_API_MODULE(cpp_addon, Init)
```

## 6) Creating a JavaScript wrapper

Let's finish things off by adding a JavaScript wrapper in `js/index.js`. As we could all see, C++ requires a lot of boilerplate code that might be easier or faster to write in JavaScript - and you will find that many production applications end up transforming data or requests in JavaScript before invoking native code. We, for instance, turn our timestamp into a proper JavaScript date.

```cpp title='js/index.js'
const EventEmitter = require('events');

class CppLinuxAddon extends EventEmitter {
  constructor() {
    super()

    if (process.platform !== 'linux') {
      throw new Error('This module is only available on Linux');
    }

    const native = require('bindings')('cpp_addon')
    this.addon = new native.CppLinuxAddon()

    // Set up event forwarding
    this.addon.on('todoAdded', (payload) => {
      this.emit('todoAdded', this.parse(payload))
    });

    this.addon.on('todoUpdated', (payload) => {
      this.emit('todoUpdated', this.parse(payload))
    })

    this.addon.on('todoDeleted', (payload) => {
      this.emit('todoDeleted', this.parse(payload))
    })
  }

  helloWorld(input = "") {
    return this.addon.helloWorld(input)
  }

  helloGui() {
    return this.addon.helloGui()
  }

  // Parse JSON and convert date to JavaScript Date object
  parse(payload) {
    const parsed = JSON.parse(payload)

    return { ...parsed, date: new Date(parsed.date) }
  }
}

if (process.platform === 'linux') {
  module.exports = new CppLinuxAddon()
} else {
  // Return empty object on non-Linux platforms
  module.exports = {}
}
```

This wrapper:

* Extends EventEmitter for native event handling
* Only loads on Linux platforms
* Forwards events from C++ to JavaScript
* Provides clean methods to call into C++
* Converts JSON data into proper JavaScript objects

## 7) Building and testing the addon

With all files in place, you can build the addon:

```sh
npm run build
```

If the build completes, you can now add the addon to your Electron app and `import` or `require` it there.

## Usage Example

Once you've built the addon, you can use it in your Electron application. Here's a complete example:

```js @ts-expect-error=[2]
// In your Electron main process or renderer process
import cppLinux from 'cpp-linux'

// Test the basic functionality
console.log(cppLinux.helloWorld('Hi!'))
// Output: "Hello from C++! You said: Hi!"

// Set up event listeners for GTK GUI interactions
cppLinux.on('todoAdded', (todo) => {
  console.log('New todo added:', todo)
  // todo: { id: "uuid-string", text: "Todo text", date: Date object }
})

cppLinux.on('todoUpdated', (todo) => {
  console.log('Todo updated:', todo)
})

cppLinux.on('todoDeleted', (todo) => {
  console.log('Todo deleted:', todo)
})

// Launch the native GTK GUI
cppLinux.helloGui()
```

When you run this code:

1. The `helloWorld()` call will return a greeting from C++
2. The event listeners will be triggered when users interact with the GTK3 GUI
3. The `helloGui()` call will open a native GTK3 window with:
   * A text entry field for todo items
   * A calendar widget for selecting dates
   * An "Add" button to create new todos
   * A scrollable list showing all todos
   * Right-click context menus for editing and deleting todos

All interactions with the native GTK3 interface will trigger the corresponding JavaScript events, allowing your Electron application to respond to native GUI actions in real-time.

## Conclusion

You've now built a complete native Node.js addon for Linux using C++ and GTK3. This addon:

1. Provides a bidirectional bridge between JavaScript and C++
1. Creates a native GTK3 GUI that runs in its own thread
1. Implements a simple Todo application with add functionality
1. Uses GTK3, which is compatible with Electron's Chromium runtime
1. Handles callbacks from C++ to JavaScript safely

This foundation can be extended to implement more complex Linux-specific features in your Electron applications. You can access system features, integrate with Linux-specific libraries, or create performant native UIs while maintaining the flexibility and ease of development that Electron provides.
For more information on GTK3 development, refer to the [GTK3 Documentation](https://docs.gtk.org/gtk3/) and the [GLib/GObject documentation](https://docs.gtk.org/gobject/). You may also find the [Node.js N-API documentation](https://nodejs.org/api/n-api.html) and [node-addon-api](https://github.com/nodejs/node-addon-api) helpful for extending your native addons.
