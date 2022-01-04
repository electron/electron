// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/task/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/extension_l10n_util.h"

namespace content {
class BrowserContext;
}

namespace extensions {
struct Message;
}

namespace electron {

// This class filters out incoming Electron-specific IPC messages from the
// extension process on the IPC thread.
class ElectronExtensionMessageFilter : public content::BrowserMessageFilter {
 public:
  ElectronExtensionMessageFilter(int render_process_id,
                                 content::BrowserContext* browser_context);

  // disable copy
  ElectronExtensionMessageFilter(const ElectronExtensionMessageFilter&) =
      delete;
  ElectronExtensionMessageFilter& operator=(
      const ElectronExtensionMessageFilter&) = delete;

  // content::BrowserMessageFilter methods:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OverrideThreadForMessage(const IPC::Message& message,
                                content::BrowserThread::ID* thread) override;
  void OnDestruct() const override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<ElectronExtensionMessageFilter>;

  ~ElectronExtensionMessageFilter() override;

  void OnGetExtMessageBundle(const std::string& extension_id,
                             IPC::Message* reply_msg);
  void OnGetExtMessageBundleAsync(
      const std::vector<base::FilePath>& extension_paths,
      const std::string& main_extension_id,
      const std::string& default_locale,
      extension_l10n_util::GzippedMessagesPermission gzip_permission,
      IPC::Message* reply_msg);

  const int render_process_id_;

  // The BrowserContext associated with our renderer process.  This should only
  // be accessed on the UI thread! Furthermore since this class is refcounted it
  // may outlive |browser_context_|, so make sure to NULL check if in doubt;
  // async calls and the like.
  content::BrowserContext* browser_context_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_MESSAGE_FILTER_H_
