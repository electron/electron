// X11 backend: injects input with the XTEST extension, which the X server
// processes exactly like events from a physical device (pointer grabs, input
// shapes and window stacking all apply). Xvfb enables XTEST by default.
//
// libX11 and libXtst are loaded at runtime and the handful of functions used
// are declared here, so the addon builds without the X11 development headers
// (the Linux arm64 test image has none) and still loads, reporting itself as
// unavailable, when there is no X display (the Wayland test leg).
//
// The addon keeps one Display connection of its own for the life of the
// process and serializes all use of it with a mutex, so Xlib never sees
// concurrent calls and XInitThreads() is not needed. It is never closed: when
// the last client of an X server disconnects the server resets, which among
// other things puts the pointer back in the middle of the screen. The addon
// never installs an Xlib error handler, which is process wide and would
// replace GTK's.

#include "impl.h"

#include <dlfcn.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <mutex>

namespace mouse_input {

namespace {

struct Display;  // Opaque Xlib display.
using Window = unsigned long;
using Bool = int;
constexpr Window kNone = 0;
constexpr unsigned long kCurrentTime = 0;

struct XLib {
  Display* (*OpenDisplay)(const char*) = nullptr;
  int (*CloseDisplay)(Display*) = nullptr;
  int (*Sync)(Display*, Bool) = nullptr;
  Window (*DefaultRootWindow)(Display*) = nullptr;
  Bool (*QueryPointer)(Display*,
                       Window,
                       Window*,
                       Window*,
                       int*,
                       int*,
                       int*,
                       int*,
                       unsigned int*) = nullptr;
  Bool (*TestQueryExtension)(Display*, int*, int*, int*, int*) = nullptr;
  int (*TestFakeMotionEvent)(Display*, int, int, int, unsigned long) = nullptr;
  int (*TestFakeButtonEvent)(Display*,
                             unsigned int,
                             Bool,
                             unsigned long) = nullptr;
  std::string load_error;

  bool ok() const { return load_error.empty(); }
};

template <typename T>
bool Resolve(void* library, const char* name, T* out, std::string* error) {
  *out = reinterpret_cast<T>(::dlsym(library, name));
  if (!*out && error->empty())
    *error = std::string("dlsym(") + name + ") failed";
  return *out != nullptr;
}

const XLib& GetXLib() {
  static const XLib* xlib = [] {
    auto* lib = new XLib();
    void* x11 = ::dlopen("libX11.so.6", RTLD_NOW | RTLD_LOCAL);
    void* xtst = ::dlopen("libXtst.so.6", RTLD_NOW | RTLD_LOCAL);
    if (!x11 || !xtst) {
      const char* reason = ::dlerror();
      lib->load_error = reason ? reason : "dlopen failed";
      return lib;
    }
    std::string& e = lib->load_error;
    Resolve(x11, "XOpenDisplay", &lib->OpenDisplay, &e);
    Resolve(x11, "XCloseDisplay", &lib->CloseDisplay, &e);
    Resolve(x11, "XSync", &lib->Sync, &e);
    Resolve(x11, "XDefaultRootWindow", &lib->DefaultRootWindow, &e);
    Resolve(x11, "XQueryPointer", &lib->QueryPointer, &e);
    Resolve(xtst, "XTestQueryExtension", &lib->TestQueryExtension, &e);
    Resolve(xtst, "XTestFakeMotionEvent", &lib->TestFakeMotionEvent, &e);
    Resolve(xtst, "XTestFakeButtonEvent", &lib->TestFakeButtonEvent, &e);
    return lib;
  }();
  return *xlib;
}

// Error codes reported in InjectResult::error.
constexpr int64_t kErrorNoLibrary = 1;
constexpr int64_t kErrorNoDisplay = 2;
constexpr int64_t kErrorNoXTest = 3;

// Holds the addon's display connection, opening it on first use, and the
// lock that serializes access to it.
class Connection {
 public:
  Connection() : lock_(Mutex()) {
    const XLib& x = GetXLib();
    if (!x.ok()) {
      error_ = kErrorNoLibrary;
      return;
    }
    static Display* display = nullptr;
    // XOpenDisplay(nullptr) uses $DISPLAY. Retried while it fails.
    if (!display)
      display = x.OpenDisplay(nullptr);
    display_ = display;
    if (!display_)
      error_ = kErrorNoDisplay;
  }
  Connection(const Connection&) = delete;
  Connection& operator=(const Connection&) = delete;

  Display* display() const { return display_; }
  int64_t error() const { return error_; }

  bool HasXTest() const {
    int event_base, error_base, major, minor;
    return display_ && GetXLib().TestQueryExtension(
                           display_, &event_base, &error_base, &major, &minor);
  }

  // Waits until the server has processed everything sent so far.
  void Sync() const { GetXLib().Sync(display_, 0); }

 private:
  static std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
  }

  std::lock_guard<std::mutex> lock_;
  Display* display_ = nullptr;
  int64_t error_ = 0;
};

InjectResult Fail(int expected, int64_t error) {
  InjectResult result;
  result.expected = expected;
  result.error = error;
  return result;
}

unsigned int ButtonNumber(Button button) {
  switch (button) {
    case Button::kLeft:
      return 1;
    case Button::kMiddle:
      return 2;
    case Button::kRight:
      return 3;
  }
  return 1;
}

}  // namespace

InjectResult Move(int x, int y) {
  Connection c;
  if (!c.display())
    return Fail(1, c.error());
  InjectResult result;
  result.expected = 1;
  // Screen -1 is the screen the pointer is on.
  if (GetXLib().TestFakeMotionEvent(c.display(), -1, x, y, kCurrentTime))
    result.sent = 1;
  else
    result.error = kErrorNoXTest;
  c.Sync();
  return result;
}

InjectResult Press(Button button, bool down) {
  Connection c;
  if (!c.display())
    return Fail(1, c.error());
  InjectResult result;
  result.expected = 1;
  if (GetXLib().TestFakeButtonEvent(c.display(), ButtonNumber(button),
                                    down ? 1 : 0, kCurrentTime))
    result.sent = 1;
  else
    result.error = kErrorNoXTest;
  c.Sync();
  return result;
}

InjectResult Wheel(int delta, bool horizontal) {
  // X11 has no wheel events: each notch is a press and release of button 4
  // (up) / 5 (down) / 6 (left) / 7 (right).
  const int notches = std::abs(delta) / 120;
  unsigned int button;
  if (horizontal)
    button = delta > 0 ? 7 : 6;
  else
    button = delta > 0 ? 4 : 5;
  Connection c;
  if (!c.display())
    return Fail(notches * 2, c.error());
  InjectResult result;
  result.expected = notches * 2;
  for (int i = 0; i < notches; ++i) {
    for (Bool press : {1, 0}) {
      if (GetXLib().TestFakeButtonEvent(c.display(), button, press,
                                        kCurrentTime))
        ++result.sent;
      else
        result.error = kErrorNoXTest;
    }
  }
  c.Sync();
  return result;
}

bool GetCursorPos(int* x, int* y, int64_t* error) {
  Connection c;
  if (!c.display()) {
    *error = c.error();
    return false;
  }
  Window root = kNone, child = kNone;
  int win_x, win_y;
  unsigned int mask;
  if (!GetXLib().QueryPointer(c.display(),
                              GetXLib().DefaultRootWindow(c.display()), &root,
                              &child, x, y, &win_x, &win_y, &mask)) {
    // The pointer is on another screen.
    *error = kErrorNoDisplay;
    return false;
  }
  return true;
}

// The X server hit tests the pointer against input shapes, and XQueryPointer
// reports the child of the queried window that contains the pointer as hit
// tested, so descend from the root while the pointer is at (x, y). This moves
// the pointer there first.
bool IsWindowAtPoint(const void* handle,
                     size_t handle_size,
                     int x,
                     int y,
                     bool* supported) {
  // BrowserWindow.getNativeWindowHandle() is the XID, as a 32 bit
  // gfx::AcceleratedWidget or a 64 bit Window depending on the build.
  Window target = kNone;
  if (handle_size == sizeof(uint32_t)) {
    uint32_t id;
    std::memcpy(&id, handle, sizeof(id));
    target = id;
  } else if (handle_size == sizeof(uint64_t)) {
    uint64_t id;
    std::memcpy(&id, handle, sizeof(id));
    target = static_cast<Window>(id);
  }
  if (target == kNone)
    return false;

  Connection c;
  if (!c.display() || !c.HasXTest()) {
    *supported = false;
    return false;
  }
  GetXLib().TestFakeMotionEvent(c.display(), -1, x, y, kCurrentTime);
  c.Sync();

  // Only windows known to exist are queried: the root, then the child it
  // reports. Without a window manager (Xvfb in CI) Electron's window is a
  // child of the root; with one it is inside a frame window. The depth limit
  // stops at the web contents' own child windows, which are not of interest.
  Window window = GetXLib().DefaultRootWindow(c.display());
  for (int depth = 0; depth < 4 && window != kNone; ++depth) {
    Window root = kNone, child = kNone;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    if (!GetXLib().QueryPointer(c.display(), window, &root, &child, &root_x,
                                &root_y, &win_x, &win_y, &mask))
      return false;
    if (child == target)
      return true;
    window = child;
  }
  return false;
}

Diagnostics GetDiagnostics() {
  Diagnostics d;
  const XLib& x = GetXLib();
  d.emplace_back("backend", StringValue("xtest"));
  const char* display_env = std::getenv("DISPLAY");
  d.emplace_back("display", StringValue(display_env ? display_env : ""));
  const char* session_type = std::getenv("XDG_SESSION_TYPE");
  d.emplace_back("sessionType", StringValue(session_type ? session_type : ""));
  d.emplace_back("libraryLoaded", BoolValue(x.ok()));
  if (!x.ok()) {
    d.emplace_back("libraryError", StringValue(x.load_error));
    d.emplace_back("available", BoolValue(false));
    return d;
  }
  Connection c;
  d.emplace_back("displayOpened", BoolValue(c.display() != nullptr));
  const bool has_xtest = c.HasXTest();
  d.emplace_back("xtest", BoolValue(has_xtest));
  d.emplace_back("available", BoolValue(has_xtest));
  return d;
}

}  // namespace mouse_input
