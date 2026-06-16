// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/renderer_startup_data.h"

#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/content_paths.h"
#include "shell/browser/preload_code_cache.h"
#include "shell/browser/session_preferences.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/node_includes.h"
#include "shell/common/thread_restrictions.h"

namespace electron::renderer_startup_data {

namespace {

// Browser process.env, captured fresh per push so a `process.env.FOO = ...`
// in main is visible to the next renderer — same semantics as the legacy
// `{ ...process.env }` spread, same per-navigation timing.
base::flat_map<std::string, std::string> CaptureBrowserEnvironment() {
  base::flat_map<std::string, std::string> env;
  uv_env_item_t* items = nullptr;
  int count = 0;
  if (uv_os_environ(&items, &count) == 0) {
    // SAFETY: uv_os_environ() guarantees `items` points to `count` contiguous
    // uv_env_item_t entries.
    const auto span =
        UNSAFE_BUFFERS(base::span(items, base::checked_cast<size_t>(count)));
    for (const auto& item : span)
      env.insert_or_assign(item.name, item.value);
    uv_os_free_environ(items, count);
  }
  return env;
}

base::FilePath GetHelperExecPath() {
  base::FilePath path;
  base::PathService::Get(content::CHILD_PROCESS_EXE, &path);
  return path;
}

}  // namespace

mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type,
                                    const GURL& consumer_site) {
  auto data = mojom::RendererStartupData::New();

  auto* session_prefs = SessionPreferences::FromBrowserContext(browser_context);
  if (session_prefs) {
    for (const auto& script : session_prefs->preload_scripts()) {
      if (script.script_type != type)
        continue;
      if (!script.file_path.IsAbsolute())
        continue;
      auto ps = mojom::PreloadScriptData::New();
      ps->id = script.id;
      ps->file_path = script.file_path.AsUTF8Unsafe();
      std::string contents;
      if (asar::ReadFileToString(script.file_path, &contents)) {
        ps->contents.assign(contents.begin(), contents.end());
        std::vector<uint8_t> cache =
            preload_code_cache::Get(script.id, consumer_site, ps->contents);
        if (!cache.empty())
          ps->code_cache = std::move(cache);
      } else {
        ps->contents.clear();
        ps->error =
            "ENOENT: no such file or directory, open '" + ps->file_path + "'";
      }
      data->preload_scripts.push_back(std::move(ps));
    }
  }

  data->environment = CaptureBrowserEnvironment();
  data->helper_exec_path = GetHelperExecPath().AsUTF8Unsafe();
  return data;
}

}  // namespace electron::renderer_startup_data
