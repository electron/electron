// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_POINTER_TAGS_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_POINTER_TAGS_H_

#include "gin/public/wrappable_pointer_tags.h"
#include "gin/public/wrapper_info.h"

namespace electron {

// Electron-specific WrappablePointerTag values that extend gin's tag range.
enum ElectronWrappablePointerTag : uint16_t {
  kElectronApp = gin::kLastPointerTag + 1,  // electron::api::App
  kElectronDataPipeHolder,                  // electron::api::DataPipeHolder
  kElectronDebugger,                        // electron::api::Debugger
  kElectronEvent,                           // gin_helper::internal::Event
  kElectronExtensions,                      // electron::api::Extensions
  kElectronMenu,                            // electron::api::Menu
  kElectronNetLog,                          // electron::api::NetLog
  kElectronPowerMonitor,                    // electron::api::PowerMonitor
  kElectronPowerSaveBlocker,                // electron::api::PowerSaveBlocker
  kElectronProtocol,                        // electron::api::Protocol
  kElectronReplyChannel,          // gin_helper::internal::ReplyChannel
  kElectronScreen,                // electron::api::Screen
  kElectronServiceWorkerContext,  // electron::api::ServiceWorkerContext
  kElectronSession,               // electron::api::Session
  kElectronTray,                  // electron::api::Tray
  kElectronWebRequest,            // electron::api::WebRequest
  kLastElectronPointerTag = kElectronWebRequest,
};

// Constructs a gin::WrapperInfo from an ElectronWrappablePointerTag,
constexpr gin::WrapperInfo MakeWrapperInfo(ElectronWrappablePointerTag tag) {
  return {{gin::kEmbedderNativeGin},
          static_cast<gin::WrappablePointerTag>(tag)};
}

static_assert(static_cast<uint16_t>(kElectronApp) >
                  static_cast<uint16_t>(gin::kLastPointerTag),
              "Electron tags must start after gin's last tag.");

// Ensure the full range (gin + Electron) fits within v8's allowed tag space.
static_assert(
    kLastElectronPointerTag <
        static_cast<uint16_t>(v8::CppHeapPointerTag::kZappedEntryTag),
    "The defined Electron type tags exceed the range of allowed tags. "
    "Reduce the number of tags or adjust gin::kFirstPointerTag so that "
    "all values fit.");

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_POINTER_TAGS_H_
