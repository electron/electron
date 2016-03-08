// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/global_shortcut_listener_mac.h"

#include <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#include <IOKit/hidsystem/ev_keymap.h>

#import "base/mac/foundation_util.h"
#include "chrome/common/extensions/command.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/events/event.h"
#import "ui/events/keycodes/keyboard_code_conversion_mac.h"

using content::BrowserThread;
using extensions::GlobalShortcutListenerMac;

namespace {

// The media keys subtype. No official docs found, but widely known.
// http://lists.apple.com/archives/cocoa-dev/2007/Aug/msg00499.html
const int kSystemDefinedEventMediaKeysSubtype = 8;

ui::KeyboardCode MediaKeyCodeToKeyboardCode(int key_code) {
  switch (key_code) {
    case NX_KEYTYPE_PLAY:
      return ui::VKEY_MEDIA_PLAY_PAUSE;
    case NX_KEYTYPE_PREVIOUS:
    case NX_KEYTYPE_REWIND:
      return ui::VKEY_MEDIA_PREV_TRACK;
    case NX_KEYTYPE_NEXT:
    case NX_KEYTYPE_FAST:
      return ui::VKEY_MEDIA_NEXT_TRACK;
  }
  return ui::VKEY_UNKNOWN;
}

}  // namespace

namespace extensions {

// static
GlobalShortcutListener* GlobalShortcutListener::GetInstance() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  static GlobalShortcutListenerMac* instance =
      new GlobalShortcutListenerMac();
  return instance;
}

GlobalShortcutListenerMac::GlobalShortcutListenerMac()
    : is_listening_(false),
      hot_key_id_(0),
      event_tap_(NULL),
      event_tap_source_(NULL),
      event_handler_(NULL) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

GlobalShortcutListenerMac::~GlobalShortcutListenerMac() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // By this point, UnregisterAccelerator should have been called for all
  // keyboard shortcuts. Still we should clean up.
  if (is_listening_)
    StopListening();

  // If keys are still registered, make sure we stop the tap. Again, this
  // should never happen.
  if (IsAnyMediaKeyRegistered())
    StopWatchingMediaKeys();

  if (IsAnyHotKeyRegistered())
    StopWatchingHotKeys();
}

void GlobalShortcutListenerMac::StartListening() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DCHECK(!accelerator_ids_.empty());
  DCHECK(!id_accelerators_.empty());
  DCHECK(!is_listening_);

  is_listening_ = true;
}

void GlobalShortcutListenerMac::StopListening() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  DCHECK(accelerator_ids_.empty());  // Make sure the set is clean.
  DCHECK(id_accelerators_.empty());
  DCHECK(is_listening_);

  is_listening_ = false;
}

void GlobalShortcutListenerMac::OnHotKeyEvent(EventHotKeyID hot_key_id) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  // This hot key should be registered.
  DCHECK(id_accelerators_.find(hot_key_id.id) != id_accelerators_.end());
  // Look up the accelerator based on this hot key ID.
  const ui::Accelerator& accelerator = id_accelerators_[hot_key_id.id];
  NotifyKeyPressed(accelerator);
}

bool GlobalShortcutListenerMac::OnMediaKeyEvent(int media_key_code) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  ui::KeyboardCode key_code = MediaKeyCodeToKeyboardCode(media_key_code);
  // Create an accelerator corresponding to the keyCode.
  ui::Accelerator accelerator(key_code, 0);
  // Look for a match with a bound hot_key.
  if (accelerator_ids_.find(accelerator) != accelerator_ids_.end()) {
    // If matched, callback to the event handling system.
    NotifyKeyPressed(accelerator);
    return true;
  }
  return false;
}

bool GlobalShortcutListenerMac::RegisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(accelerator_ids_.find(accelerator) == accelerator_ids_.end());

  if (Command::IsMediaKey(accelerator)) {
    if (!IsAnyMediaKeyRegistered()) {
      // If this is the first media key registered, start the event tap.
      StartWatchingMediaKeys();
    }
  } else {
    // Register hot_key if they are non-media keyboard shortcuts.
    if (!RegisterHotKey(accelerator, hot_key_id_))
      return false;

    if (!IsAnyHotKeyRegistered()) {
      StartWatchingHotKeys();
    }
  }

  // Store the hotkey-ID mappings we will need for lookup later.
  id_accelerators_[hot_key_id_] = accelerator;
  accelerator_ids_[accelerator] = hot_key_id_;
  ++hot_key_id_;
  return true;
}

void GlobalShortcutListenerMac::UnregisterAcceleratorImpl(
    const ui::Accelerator& accelerator) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(accelerator_ids_.find(accelerator) != accelerator_ids_.end());

  // Unregister the hot_key if it's a keyboard shortcut.
  if (!Command::IsMediaKey(accelerator))
    UnregisterHotKey(accelerator);

  // Remove hot_key from the mappings.
  KeyId key_id = accelerator_ids_[accelerator];
  id_accelerators_.erase(key_id);
  accelerator_ids_.erase(accelerator);

  if (Command::IsMediaKey(accelerator)) {
    // If we unregistered a media key, and now no media keys are registered,
    // stop the media key tap.
    if (!IsAnyMediaKeyRegistered())
      StopWatchingMediaKeys();
  } else {
    // If we unregistered a hot key, and no more hot keys are registered, remove
    // the hot key handler.
    if (!IsAnyHotKeyRegistered()) {
      StopWatchingHotKeys();
    }
  }
}

bool GlobalShortcutListenerMac::RegisterHotKey(
    const ui::Accelerator& accelerator, KeyId hot_key_id) {
  EventHotKeyID event_hot_key_id;

  // Signature uniquely identifies the application that owns this hot_key.
  event_hot_key_id.signature = base::mac::CreatorCodeForApplication();
  event_hot_key_id.id = hot_key_id;

  // Translate ui::Accelerator modifiers to cmdKey, altKey, etc.
  int modifiers = 0;
  modifiers |= (accelerator.IsShiftDown() ? shiftKey : 0);
  modifiers |= (accelerator.IsCtrlDown() ? controlKey : 0);
  modifiers |= (accelerator.IsAltDown() ? optionKey : 0);
  modifiers |= (accelerator.IsCmdDown() ? cmdKey : 0);

  int key_code = ui::MacKeyCodeForWindowsKeyCode(accelerator.key_code(), 0,
      NULL, NULL);

  // Register the event hot key.
  EventHotKeyRef hot_key_ref;
  OSStatus status = RegisterEventHotKey(key_code, modifiers, event_hot_key_id,
      GetApplicationEventTarget(), 0, &hot_key_ref);
  if (status != noErr)
    return false;

  id_hot_key_refs_[hot_key_id] = hot_key_ref;
  return true;
}

void GlobalShortcutListenerMac::UnregisterHotKey(
    const ui::Accelerator& accelerator) {
  // Ensure this accelerator is already registered.
  DCHECK(accelerator_ids_.find(accelerator) != accelerator_ids_.end());
  // Get the ref corresponding to this accelerator.
  KeyId key_id = accelerator_ids_[accelerator];
  EventHotKeyRef ref = id_hot_key_refs_[key_id];
  // Unregister the event hot key.
  UnregisterEventHotKey(ref);

  // Remove the event from the mapping.
  id_hot_key_refs_.erase(key_id);
}

void GlobalShortcutListenerMac::StartWatchingMediaKeys() {
  // Make sure there's no existing event tap.
  DCHECK(event_tap_ == NULL);
  DCHECK(event_tap_source_ == NULL);

  // Add an event tap to intercept the system defined media key events.
  event_tap_ = CGEventTapCreate(kCGSessionEventTap,
      kCGHeadInsertEventTap,
      kCGEventTapOptionDefault,
      CGEventMaskBit(NX_SYSDEFINED),
      EventTapCallback,
      this);
  if (event_tap_ == NULL) {
    LOG(ERROR) << "Error: failed to create event tap.";
    return;
  }

  event_tap_source_ = CFMachPortCreateRunLoopSource(kCFAllocatorSystemDefault,
      event_tap_, 0);
  if (event_tap_source_ == NULL) {
    LOG(ERROR) << "Error: failed to create new run loop source.";
    return;
  }

  CFRunLoopAddSource(CFRunLoopGetCurrent(), event_tap_source_,
      kCFRunLoopCommonModes);
}

void GlobalShortcutListenerMac::StopWatchingMediaKeys() {
  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), event_tap_source_,
      kCFRunLoopCommonModes);
  // Ensure both event tap and source are initialized.
  DCHECK(event_tap_ != NULL);
  DCHECK(event_tap_source_ != NULL);

  // Invalidate the event tap.
  CFMachPortInvalidate(event_tap_);
  CFRelease(event_tap_);
  event_tap_ = NULL;

  // Release the event tap source.
  CFRelease(event_tap_source_);
  event_tap_source_ = NULL;
}

void GlobalShortcutListenerMac::StartWatchingHotKeys() {
  DCHECK(!event_handler_);
  EventHandlerUPP hot_key_function = NewEventHandlerUPP(HotKeyHandler);
  EventTypeSpec event_type;
  event_type.eventClass = kEventClassKeyboard;
  event_type.eventKind = kEventHotKeyPressed;
  InstallApplicationEventHandler(
      hot_key_function, 1, &event_type, this, &event_handler_);
}

void GlobalShortcutListenerMac::StopWatchingHotKeys() {
  DCHECK(event_handler_);
  RemoveEventHandler(event_handler_);
  event_handler_ = NULL;
}

bool GlobalShortcutListenerMac::IsAnyMediaKeyRegistered() {
  // Iterate through registered accelerators, looking for media keys.
  AcceleratorIdMap::iterator it;
  for (it = accelerator_ids_.begin(); it != accelerator_ids_.end(); ++it) {
    if (Command::IsMediaKey(it->first))
      return true;
  }
  return false;
}

bool GlobalShortcutListenerMac::IsAnyHotKeyRegistered() {
  AcceleratorIdMap::iterator it;
  for (it = accelerator_ids_.begin(); it != accelerator_ids_.end(); ++it) {
    if (!Command::IsMediaKey(it->first))
      return true;
  }
  return false;
}

// Processed events should propagate if they aren't handled by any listeners.
// For events that don't matter, this handler should return as quickly as
// possible.
// Returning event causes the event to propagate to other applications.
// Returning NULL prevents the event from propagating.
// static
CGEventRef GlobalShortcutListenerMac::EventTapCallback(
    CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
  GlobalShortcutListenerMac* shortcut_listener =
      static_cast<GlobalShortcutListenerMac*>(refcon);

  // Handle the timeout case by re-enabling the tap.
  if (type == kCGEventTapDisabledByTimeout) {
    CGEventTapEnable(shortcut_listener->event_tap_, TRUE);
    return event;
  }

  // Convert the CGEvent to an NSEvent for access to the data1 field.
  NSEvent* ns_event = [NSEvent eventWithCGEvent:event];
  if (ns_event == nil) {
    return event;
  }

  // Ignore events that are not system defined media keys.
  if (type != NX_SYSDEFINED ||
      [ns_event type] != NSSystemDefined ||
      [ns_event subtype] != kSystemDefinedEventMediaKeysSubtype) {
    return event;
  }

  NSInteger data1 = [ns_event data1];
  // Ignore media keys that aren't previous, next and play/pause.
  // Magical constants are from http://weblog.rogueamoeba.com/2007/09/29/
  int key_code = (data1 & 0xFFFF0000) >> 16;
  if (key_code != NX_KEYTYPE_PLAY && key_code != NX_KEYTYPE_NEXT &&
      key_code != NX_KEYTYPE_PREVIOUS && key_code != NX_KEYTYPE_FAST &&
      key_code != NX_KEYTYPE_REWIND) {
    return event;
  }

  int key_flags = data1 & 0x0000FFFF;
  bool is_key_pressed = ((key_flags & 0xFF00) >> 8) == 0xA;

  // If the key wasn't pressed (eg. was released), ignore this event.
  if (!is_key_pressed)
    return event;

  // Now we have a media key that we care about. Send it to the caller.
  bool was_handled = shortcut_listener->OnMediaKeyEvent(key_code);

  // Prevent event from proagating to other apps if handled by Chrome.
  if (was_handled)
    return NULL;

  // By default, pass the event through.
  return event;
}

// static
OSStatus GlobalShortcutListenerMac::HotKeyHandler(
    EventHandlerCallRef next_handler, EventRef event, void* user_data) {
  // Extract the hotkey from the event.
  EventHotKeyID hot_key_id;
  OSStatus result = GetEventParameter(event, kEventParamDirectObject,
      typeEventHotKeyID, NULL, sizeof(hot_key_id), NULL, &hot_key_id);
  if (result != noErr)
    return result;

  GlobalShortcutListenerMac* shortcut_listener =
      static_cast<GlobalShortcutListenerMac*>(user_data);
  shortcut_listener->OnHotKeyEvent(hot_key_id);
  return noErr;
}

}  // namespace extensions
