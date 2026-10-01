// macOS backend: posts Quartz events at the HID event tap, the point where
// events from physical devices enter the system. They move the cursor, are
// routed by the window server (so windows that ignore mouse events are skipped)
// and reach AppKit as ordinary NSEvents.
//
// Posting events needs the Accessibility (kTCCServicePostEvent) permission of
// the responsible process. Without it CGEventPost() drops events silently, so
// getDiagnostics() reports CGPreflightPostEventAccess() and specs should also
// check that a move actually moves the cursor.

#include "impl.h"

#include <ApplicationServices/ApplicationServices.h>

#include <atomic>
#include <cmath>
#include <cstdlib>

namespace mouse_input {

namespace {

// Buttons currently held down through this addon. While one is, moves are
// posted as drags so AppKit sees the same sequence as from a real mouse.
std::atomic<int> g_pressed_buttons{0};

int ButtonBit(Button button) {
  switch (button) {
    case Button::kLeft:
      return 1;
    case Button::kRight:
      return 2;
    case Button::kMiddle:
      return 4;
  }
  return 1;
}

CGPoint CurrentLocation() {
  CGEventRef event = CGEventCreate(nullptr);
  CGPoint location = event ? CGEventGetLocation(event) : CGPointZero;
  if (event)
    CFRelease(event);
  return location;
}

// Posts |event| (taking ownership) and reports it the way SendInput() would.
InjectResult Post(CGEventRef event) {
  InjectResult result;
  result.expected = 1;
  if (!event) {
    result.error = 1;
    return result;
  }
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
  result.sent = 1;
  return result;
}

}  // namespace

InjectResult Move(int x, int y) {
  const int pressed = g_pressed_buttons.load();
  CGEventType type = kCGEventMouseMoved;
  CGMouseButton button = kCGMouseButtonLeft;
  if (pressed & ButtonBit(Button::kLeft)) {
    type = kCGEventLeftMouseDragged;
  } else if (pressed & ButtonBit(Button::kRight)) {
    type = kCGEventRightMouseDragged;
    button = kCGMouseButtonRight;
  } else if (pressed & ButtonBit(Button::kMiddle)) {
    type = kCGEventOtherMouseDragged;
    button = kCGMouseButtonCenter;
  }
  return Post(
      CGEventCreateMouseEvent(nullptr, type, CGPointMake(x, y), button));
}

InjectResult Press(Button button, bool down) {
  CGEventType type = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
  CGMouseButton cg_button = kCGMouseButtonLeft;
  switch (button) {
    case Button::kLeft:
      type = down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
      cg_button = kCGMouseButtonLeft;
      break;
    case Button::kRight:
      type = down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
      cg_button = kCGMouseButtonRight;
      break;
    case Button::kMiddle:
      type = down ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
      cg_button = kCGMouseButtonCenter;
      break;
  }
  CGEventRef event =
      CGEventCreateMouseEvent(nullptr, type, CurrentLocation(), cg_button);
  if (event)
    CGEventSetIntegerValueField(event, kCGMouseEventClickState, 1);
  if (down)
    g_pressed_buttons |= ButtonBit(button);
  else
    g_pressed_buttons &= ~ButtonBit(button);
  return Post(event);
}

InjectResult Wheel(int delta, bool horizontal) {
  const int32_t lines = delta / 120;
  // Positive wheel1 scrolls up; positive wheel2 scrolls left, so negate it to
  // match Windows' MOUSEEVENTF_HWHEEL, where positive scrolls right.
  CGEventRef event = horizontal
                         ? CGEventCreateScrollWheelEvent(
                               nullptr, kCGScrollEventUnitLine, 2, 0, -lines)
                         : CGEventCreateScrollWheelEvent(
                               nullptr, kCGScrollEventUnitLine, 1, lines);
  return Post(event);
}

bool GetCursorPos(int* x, int* y, int64_t* error) {
  const CGPoint location = CurrentLocation();
  *x = static_cast<int>(std::lround(location.x));
  *y = static_cast<int>(std::lround(location.y));
  return true;
}

// There is no public API that hit tests the way the window server routes
// events (CGWindowList and -[NSWindow windowNumberAtPoint:...] do not account
// for ignoresMouseEvents), so specs check click-through by clicking instead.
bool IsWindowAtPoint(const void* handle,
                     size_t handle_size,
                     int x,
                     int y,
                     bool* supported) {
  *supported = false;
  return false;
}

Diagnostics GetDiagnostics() {
  Diagnostics d;
  d.emplace_back("backend", StringValue("quartz"));
  d.emplace_back("postEventAccess", BoolValue(CGPreflightPostEventAccess()));
  d.emplace_back("accessibilityTrusted", BoolValue(AXIsProcessTrusted()));
  const CGPoint location = CurrentLocation();
  d.emplace_back("cursorX", IntValue(std::lround(location.x)));
  d.emplace_back("cursorY", IntValue(std::lround(location.y)));
  const CGRect main_display = CGDisplayBounds(CGMainDisplayID());
  d.emplace_back("mainDisplayWidth",
                 IntValue(std::lround(main_display.size.width)));
  d.emplace_back("mainDisplayHeight",
                 IntValue(std::lround(main_display.size.height)));
  // Posting without the permission is not an error, the events just vanish;
  // the specs confirm with a real move.
  d.emplace_back("available", BoolValue(true));
  return d;
}

}  // namespace mouse_input
