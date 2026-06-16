// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/renderer_startup_data.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_paths.h"
#include "crypto/hash.h"
#include "shell/browser/preload_code_cache.h"
#include "shell/browser/session_preferences.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/node_includes.h"
#include "shell/common/thread_restrictions.h"
#include "url/gurl.h"

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

// Reads one preload script into a PreloadScriptData. When |served| is
// non-null the script participates in the preload code cache: the source is
// hashed once, the hash both gates the (same-site) cache lookup and is
// appended to |served| so it can be recorded as the trust anchor that
// SetPreloadCodeCache writes from the frame are validated against. Service
// worker realms pass null — they have no code-cache producer.
mojom::PreloadScriptDataPtr ReadPreloadScript(
    const std::string& id,
    const base::FilePath& path,
    const GURL& site,
    std::vector<preload_code_cache::ServedPreload>* served) {
  auto ps = mojom::PreloadScriptData::New();
  ps->id = id;
  ps->file_path = path.AsUTF8Unsafe();
  std::string contents;
  if (!asar::ReadFileToString(path, &contents)) {
    ps->error =
        "ENOENT: no such file or directory, open '" + ps->file_path + "'";
    return ps;
  }
  ps->contents.assign(contents.begin(), contents.end());
  if (served) {
    const preload_code_cache::SourceHash hash =
        crypto::hash::Sha256(ps->contents);
    std::vector<uint8_t> cache = preload_code_cache::Get(id, site, hash);
    if (!cache.empty())
      ps->code_cache = std::move(cache);
    served->emplace_back(id, hash);
  }
  return ps;
}

// Session-registered preloads of |type| plus the env/execPath snapshot.
mojom::RendererStartupDataPtr BuildInternal(
    content::BrowserContext* browser_context,
    PreloadScript::ScriptType type,
    const GURL& site,
    std::vector<preload_code_cache::ServedPreload>* served) {
  auto data = mojom::RendererStartupData::New();

  auto* session_prefs = SessionPreferences::FromBrowserContext(browser_context);
  if (session_prefs) {
    for (const auto& script : session_prefs->preload_scripts()) {
      if (script.script_type != type)
        continue;
      if (!script.file_path.IsAbsolute())
        continue;
      data->preload_scripts.push_back(
          ReadPreloadScript(script.id, script.file_path, site, served));
    }
  }

  data->environment = CaptureBrowserEnvironment();
  data->helper_exec_path = GetHelperExecPath().AsUTF8Unsafe();
  return data;
}

}  // namespace

mojom::RendererStartupDataPtr Build(content::BrowserContext* browser_context,
                                    PreloadScript::ScriptType type) {
  return BuildInternal(browser_context, type, GURL(), nullptr);
}

mojom::RendererStartupDataPtr BuildForFrame(content::RenderFrameHost* rfh) {
  const GURL site = preload_code_cache::SiteForFrame(rfh);
  std::vector<preload_code_cache::ServedPreload> served;
  auto data =
      BuildInternal(rfh->GetBrowserContext(),
                    PreloadScript::ScriptType::kWebFrame, site, &served);

  // The per-WebContents webPreferences.preload runs last. May be absent for a
  // WebContents that never went through a BrowserWindow/webContents
  // constructor (extension pages, devtools); such a WebContents has no per-WC
  // preload but still gets session preloads + env.
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  auto* web_prefs = WebContentsPreferences::From(web_contents);
  std::optional<base::FilePath> preload;
  if (web_prefs)
    preload = web_prefs->GetPreloadPath();
  if (preload && preload->IsAbsolute()) {
    data->preload_scripts.push_back(ReadPreloadScript(
        preload_code_cache::IdForWebPreferencesPreload(*preload), *preload,
        site, &served));
  }

  // Remember exactly what was served to this frame; SetPreloadCodeCache()
  // only accepts cache writes that match this record, so the renderer never
  // gets to pick the source a cache entry is validated against.
  preload_code_cache::RecordServedPreloads(rfh, std::move(served));
  return data;
}

}  // namespace electron::renderer_startup_data
