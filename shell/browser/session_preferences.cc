// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/session_preferences.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"
#include "gin/weak_cell.h"
#include "shell/browser/api/electron_api_utility_process.h"
#include "v8/include/cppgc/persistent.h"

namespace electron {

// static
int SessionPreferences::kLocatorKey = 0;

// static
void SessionPreferences::CreateForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(context);
  context->SetUserData(&kLocatorKey,
                       base::WrapUnique(new SessionPreferences{}));
}

SessionPreferences::SessionPreferences() = default;
SessionPreferences::~SessionPreferences() = default;

// static
SessionPreferences* SessionPreferences::FromBrowserContext(
    content::BrowserContext* context) {
  return static_cast<SessionPreferences*>(context->GetUserData(&kLocatorKey));
}

gin::WeakCell<api::UtilityProcessWrapper>*
SessionPreferences::GetLocalAIHandler() const {
  return local_ai_handler_.Get();
}

void SessionPreferences::SetLocalAIHandler(
    gin::WeakCell<api::UtilityProcessWrapper>* handler) {
  local_ai_handler_ = handler;
  ai_handler_changed_callbacks_.Notify();
}

base::CallbackListSubscription SessionPreferences::AddAIHandlerChangedCallback(
    base::RepeatingClosure callback) {
  return ai_handler_changed_callbacks_.Add(std::move(callback));
}

bool SessionPreferences::HasServiceWorkerPreloadScript() {
  const auto& preloads = preload_scripts();
  auto it = std::find_if(
      preloads.begin(), preloads.end(), [](const PreloadScript& script) {
        return script.script_type == PreloadScript::ScriptType::kServiceWorker;
      });
  return it != preloads.end();
}

}  // namespace electron
