// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_message_filter.h"

#include <stdint.h>
#include <memory>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/l10n_file_util.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/message_bundle.h"

using content::BrowserThread;

namespace electron {

const uint32_t kExtensionFilteredMessageClasses[] = {
    ExtensionMsgStart,
};

ElectronExtensionMessageFilter::ElectronExtensionMessageFilter(
    int render_process_id,
    content::BrowserContext* browser_context)
    : BrowserMessageFilter(kExtensionFilteredMessageClasses,
                           std::size(kExtensionFilteredMessageClasses)),
      render_process_id_(render_process_id),
      browser_context_(browser_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

ElectronExtensionMessageFilter::~ElectronExtensionMessageFilter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

bool ElectronExtensionMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ElectronExtensionMessageFilter, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(ExtensionHostMsg_GetMessageBundle,
                                    OnGetExtMessageBundle)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void ElectronExtensionMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message,
    BrowserThread::ID* thread) {
  switch (message.type()) {
    case ExtensionHostMsg_GetMessageBundle::ID:
      *thread = BrowserThread::UI;
      break;
    default:
      break;
  }
}

void ElectronExtensionMessageFilter::OnDestruct() const {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    delete this;
  } else {
    content::GetUIThreadTaskRunner({})->DeleteSoon(FROM_HERE, this);
  }
}

void ElectronExtensionMessageFilter::OnGetExtMessageBundle(
    const std::string& extension_id,
    IPC::Message* reply_msg) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const extensions::ExtensionSet& extension_set =
      extensions::ExtensionRegistry::Get(browser_context_)
          ->enabled_extensions();
  const extensions::Extension* extension = extension_set.GetByID(extension_id);

  if (!extension) {  // The extension has gone.
    ExtensionHostMsg_GetMessageBundle::WriteReplyParams(
        reply_msg, extensions::MessageBundle::SubstitutionMap());
    Send(reply_msg);
    return;
  }

  const std::string& default_locale =
      extensions::LocaleInfo::GetDefaultLocale(extension);
  if (default_locale.empty()) {
    // A little optimization: send the answer here to avoid an extra thread hop.
    std::unique_ptr<extensions::MessageBundle::SubstitutionMap> dictionary_map(
        extensions::l10n_file_util::
            LoadNonLocalizedMessageBundleSubstitutionMap(extension_id));
    ExtensionHostMsg_GetMessageBundle::WriteReplyParams(reply_msg,
                                                        *dictionary_map);
    Send(reply_msg);
    return;
  }

  std::vector<base::FilePath> paths_to_load;
  paths_to_load.push_back(extension->path());

  auto imports = extensions::SharedModuleInfo::GetImports(extension);
  // Iterate through the imports in reverse.  This will allow later imported
  // modules to override earlier imported modules, as the list order is
  // maintained from the definition in manifest.json of the imports.
  for (auto it = imports.rbegin(); it != imports.rend(); ++it) {
    const extensions::Extension* imported_extension =
        extension_set.GetByID(it->extension_id);
    if (!imported_extension) {
      NOTREACHED() << "Missing shared module " << it->extension_id;
      continue;
    }
    paths_to_load.push_back(imported_extension->path());
  }

  // This blocks tab loading. Priority is inherited from the calling context.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &ElectronExtensionMessageFilter::OnGetExtMessageBundleAsync, this,
          paths_to_load, extension_id, default_locale,
          extension_l10n_util::GetGzippedMessagesPermissionForExtension(
              extension),
          reply_msg));
}

void ElectronExtensionMessageFilter::OnGetExtMessageBundleAsync(
    const std::vector<base::FilePath>& extension_paths,
    const std::string& main_extension_id,
    const std::string& default_locale,
    extension_l10n_util::GzippedMessagesPermission gzip_permission,
    IPC::Message* reply_msg) {
  std::unique_ptr<extensions::MessageBundle::SubstitutionMap> dictionary_map(
      extensions::l10n_file_util::LoadMessageBundleSubstitutionMapFromPaths(
          extension_paths, main_extension_id, default_locale, gzip_permission));

  ExtensionHostMsg_GetMessageBundle::WriteReplyParams(reply_msg,
                                                      *dictionary_map);
  Send(reply_msg);
}

}  // namespace electron
