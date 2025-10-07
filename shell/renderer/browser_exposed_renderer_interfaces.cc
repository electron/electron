// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/browser_exposed_renderer_interfaces.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "electron/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "shell/renderer/renderer_client_base.h"

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/renderer/spellcheck.h"
#endif

namespace {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
void BindSpellChecker(
    electron::RendererClientBase* client,
    mojo::PendingReceiver<spellcheck::mojom::SpellChecker> receiver) {
  if (client->GetSpellCheck())
    client->GetSpellCheck()->BindReceiver(std::move(receiver));
}
#endif

}  // namespace

void ExposeElectronRendererInterfacesToBrowser(
    electron::RendererClientBase* client,
    mojo::BinderMap* binders) {
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  binders->Add<spellcheck::mojom::SpellChecker>(
      base::BindRepeating(&BindSpellChecker, client),
      base::SequencedTaskRunner::GetCurrentDefault());
#endif
}
