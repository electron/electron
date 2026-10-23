// Copyright (c) 2026 Anthropic PBC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_extension_tab_registry.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_map.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_web_contents_observer.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/mojom/frame.mojom.h"

namespace extensions {

namespace {

const void* const kRegistryKey = &kRegistryKey;

// The stored details are copied into every message channel a tab opens, and
// into every chrome.tabs result.
constexpr size_t kMaxDetailsSize = 64 * 1024;

// Tab properties Electron reads from the WebContents. The app cannot set them.
constexpr auto kDerivedKeys = base::MakeFixedFlatSet<std::string_view>({
    "audible",
    "height",
    "id",
    "incognito",
    "mutedInfo",
    "pendingUrl",
    "selected",
    "status",
    "title",
    "url",
    "width",
});

// Every property tabs.json declares on Tab, with its minimum if it has one.
// A details key named here is that typed field; any other key is custom.
using TabSchema = base::flat_map<std::string, std::optional<int>>;

const TabSchema& GetTabSchema() {
  static const base::NoDestructor<TabSchema> schema([] {
    TabSchema result;
    const base::DictValue* tabs =
        ExtensionAPI::GetSharedInstance()->GetSchema("tabs");
    CHECK(tabs);
    const base::ListValue* types = tabs->FindList("types");
    CHECK(types);
    for (const base::Value& type : *types) {
      const base::DictValue* dict = type.GetIfDict();
      const std::string* id = dict ? dict->FindString("id") : nullptr;
      if (!id || (*id != "Tab" && *id != "tabs.Tab"))
        continue;
      const base::DictValue* properties = dict->FindDict("properties");
      CHECK(properties);
      for (const auto [name, property] : *properties) {
        result.emplace(name, property.is_dict()
                                 ? property.GetDict().FindInt("minimum")
                                 : std::nullopt);
      }
    }
    CHECK(!result.empty());
    return result;
  }());
  return *schema;
}

// A dictionary electron_api::tabs::Tab::FromValue() accepts: every property
// tabs.json requires, overlaid with what the app registered.
base::DictValue BuildTabDict(int window_id,
                             int index,
                             bool active,
                             const base::DictValue& details) {
  base::DictValue dict;
  dict.Set("autoDiscardable", false);
  dict.Set("discarded", false);
  dict.Set("frozen", false);
  dict.Set("groupId", -1);
  dict.Set("highlighted", active);
  dict.Set("incognito", false);
  dict.Set("lastAccessed", 0.0);
  dict.Set("pinned", false);
  dict.Set("selected", false);
  dict.Set("splitViewId", -1);
  dict.Merge(details.Clone());
  dict.Set("active", active);
  dict.Set("index", index);
  dict.Set("windowId", window_id);
  return dict;
}

std::string Quote(std::string_view key) {
  return base::StrCat({"'", key, "'"});
}

// The renderer matches extension views to a window by the window ID each
// frame was last told (ExtensionFrameHelper::browser_window_id()).
void SendBrowserWindowId(content::WebContents* web_contents, int window_id) {
  auto* observer =
      ExtensionWebContentsObserver::GetForWebContents(web_contents);
  if (!observer)
    return;
  web_contents->ForEachRenderFrameHost(
      [observer, window_id](content::RenderFrameHost* render_frame_host) {
        if (auto* local_frame = observer->GetLocalFrame(render_frame_host))
          local_frame->UpdateBrowserWindowId(window_id);
      });
}

}  // namespace

struct ElectronExtensionTabRegistry::ParsedDetails {
  int window_id = -1;
  int index = -1;
  bool active = false;
  base::DictValue details;
};

ElectronExtensionTabData::ElectronExtensionTabData(
    content::WebContents* web_contents,
    base::WeakPtr<ElectronExtensionTabRegistry> registry,
    mojom::ViewType view_type)
    : content::WebContentsUserData<ElectronExtensionTabData>(*web_contents),
      registry_(std::move(registry)),
      view_type_(view_type) {}

ElectronExtensionTabData::~ElectronExtensionTabData() {
  if (registry_)
    registry_->OnViewDestroyed(&GetWebContents());
}

electron_api::tabs::Tab ElectronExtensionTabData::CreateTab() const {
  CHECK(is_tab());
  std::optional<electron_api::tabs::Tab> tab =
      electron_api::tabs::Tab::FromValue(
          BuildTabDict(window_id_, index_, active_, details_));
  // ParseDetails() accepted these details with the same schema.
  CHECK(tab);
  // Tab::Populate() copies every key into |additional_properties| and
  // Tab::ToValue() merges that over the typed fields, so anything left there
  // under a schema name would override what the caller writes afterwards.
  // Keep only the custom keys.
  for (const auto& [name, minimum] : GetTabSchema())
    tab->additional_properties.Remove(name);
  return std::move(*tab);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(ElectronExtensionTabData);

// static
ElectronExtensionTabRegistry* ElectronExtensionTabRegistry::Get(
    content::BrowserContext* context) {
  auto* registry = static_cast<ElectronExtensionTabRegistry*>(
      context->GetUserData(kRegistryKey));
  if (!registry) {
    auto owned = std::make_unique<ElectronExtensionTabRegistry>(context);
    registry = owned.get();
    context->SetUserData(kRegistryKey, std::move(owned));
  }
  return registry;
}

ElectronExtensionTabRegistry::ElectronExtensionTabRegistry(
    content::BrowserContext* context)
    : context_(context) {}

ElectronExtensionTabRegistry::~ElectronExtensionTabRegistry() = default;

// static
base::expected<ElectronExtensionTabRegistry::ParsedDetails, std::string>
ElectronExtensionTabRegistry::ParseDetails(
    mojom::ViewType view_type,
    const ElectronExtensionTabData* current,
    const base::DictValue& details) {
  const bool is_tab = view_type == mojom::ViewType::kTabContents;

  ParsedDetails parsed;
  if (current) {
    parsed.window_id = current->window_id();
    parsed.index = current->index();
    parsed.active = current->active();
    parsed.details = current->details().Clone();
  }

  for (const auto [key, value] : details) {
    if (key == "type") {
      if (current) {
        return base::unexpected(
            "'type' cannot be changed. Remove the view and add it again.");
      }
      continue;
    }
    if (kDerivedKeys.contains(key)) {
      return base::unexpected(
          base::StrCat({Quote(key), " is set by Electron and cannot be set"}));
    }
    if (key == "windowId") {
      const std::optional<int> window_id = value.GetIfInt();
      if (!window_id || *window_id < 0) {
        return base::unexpected("'windowId' must be a non-negative integer");
      }
      parsed.window_id = *window_id;
      continue;
    }
    if (!is_tab) {
      return base::unexpected(base::StrCat(
          {Quote(key), " cannot be set on a view that is not a tab"}));
    }
    if (key == "index") {
      const std::optional<int> index = value.GetIfInt();
      if (!index || *index < 0)
        return base::unexpected("'index' must be a non-negative integer");
      parsed.index = *index;
    } else if (key == "active") {
      if (!value.is_none() && !value.is_bool())
        return base::unexpected("'active' must be a boolean");
      parsed.active = value.is_bool() && value.GetBool();
    } else if (value.is_none()) {
      parsed.details.Remove(key);
    } else {
      parsed.details.Set(key, value.Clone());
    }
  }

  if (parsed.window_id < 0)
    return base::unexpected("'windowId' is required");
  if (!is_tab)
    return parsed;
  if (parsed.index < 0)
    return base::unexpected("'index' is required for a tab");

  const TabSchema& schema = GetTabSchema();
  for (const auto [key, value] : parsed.details) {
    const auto property = schema.find(key);
    if (property == schema.end())
      continue;
    base::DictValue probe = BuildTabDict(0, 0, false, base::DictValue());
    probe.Set(key, value.Clone());
    const std::optional<int> minimum = property->second;
    if (!electron_api::tabs::Tab::FromValue(probe) ||
        (minimum && value.is_int() && value.GetInt() < *minimum)) {
      return base::unexpected(base::StrCat({"Invalid value for ", Quote(key)}));
    }
  }
  if (!electron_api::tabs::Tab::FromValue(BuildTabDict(
          parsed.window_id, parsed.index, parsed.active, parsed.details))) {
    return base::unexpected("Invalid tab details");
  }

  const std::optional<std::string> json = base::WriteJson(parsed.details);
  if (!json)
    return base::unexpected("Tab details must be JSON-serializable");
  if (json->size() > kMaxDetailsSize)
    return base::unexpected("Tab details are too large");

  return parsed;
}

base::expected<void, std::string> ElectronExtensionTabRegistry::AddView(
    content::WebContents* web_contents,
    const base::DictValue& details) {
  if (web_contents->GetBrowserContext() != context_)
    return base::unexpected("webContents belongs to another session");
  if (ElectronExtensionTabData::FromWebContents(web_contents))
    return base::unexpected("webContents is already registered");
  // Chromium sets the view type of the hosts it creates. Relabelling one
  // would change how its renderer and DevTools treat it.
  if (GetViewType(web_contents) != mojom::ViewType::kInvalid) {
    return base::unexpected(
        "webContents is an extension background page or offscreen document");
  }

  mojom::ViewType view_type = mojom::ViewType::kTabContents;
  if (const base::Value* type = details.Find("type")) {
    const std::string* name = type->GetIfString();
    if (name && *name == "tab") {
      view_type = mojom::ViewType::kTabContents;
    } else if (name && *name == "popup") {
      view_type = mojom::ViewType::kExtensionPopup;
    } else if (name && *name == "side-panel") {
      view_type = mojom::ViewType::kExtensionSidePanel;
    } else {
      return base::unexpected("'type' must be 'tab', 'popup' or 'side-panel'");
    }
  }

  auto parsed = ParseDetails(view_type, nullptr, details);
  if (!parsed.has_value())
    return base::unexpected(std::move(parsed.error()));

  ElectronExtensionTabData::CreateForWebContents(
      web_contents, weak_factory_.GetWeakPtr(), view_type);
  views_.insert(web_contents);
  Apply(ElectronExtensionTabData::FromWebContents(web_contents),
        std::move(parsed.value()));
  SetViewType(web_contents, view_type);
  return base::ok();
}

base::expected<void, std::string> ElectronExtensionTabRegistry::UpdateView(
    content::WebContents* web_contents,
    const base::DictValue& details) {
  auto* data = ElectronExtensionTabData::FromWebContents(web_contents);
  if (!data || !views_.contains(web_contents))
    return base::unexpected("webContents is not registered");

  auto parsed = ParseDetails(data->view_type(), data, details);
  if (!parsed.has_value())
    return base::unexpected(std::move(parsed.error()));

  Apply(data, std::move(parsed.value()));
  return base::ok();
}

void ElectronExtensionTabRegistry::RemoveView(
    content::WebContents* web_contents) {
  if (!views_.contains(web_contents))
    return;
  web_contents->RemoveUserData(ElectronExtensionTabData::UserDataKey());
  SetViewType(web_contents, mojom::ViewType::kInvalid);
  SendBrowserWindowId(web_contents, extension_misc::kUnknownWindowId);
}

content::WebContents* ElectronExtensionTabRegistry::GetActiveTab(
    int window_id) const {
  for (content::WebContents* web_contents : views_) {
    const auto* data = ElectronExtensionTabData::FromWebContents(web_contents);
    if (data && data->is_tab() && data->active() &&
        data->window_id() == window_id) {
      return web_contents;
    }
  }
  return nullptr;
}

void ElectronExtensionTabRegistry::Apply(ElectronExtensionTabData* data,
                                         ParsedDetails parsed) {
  const bool window_changed = data->window_id_ != parsed.window_id;
  data->window_id_ = parsed.window_id;
  data->index_ = parsed.index;
  data->active_ = parsed.active;
  data->details_ = std::move(parsed.details);

  // A window has at most one active tab.
  if (data->active_) {
    for (content::WebContents* web_contents : views_) {
      auto* other = ElectronExtensionTabData::FromWebContents(web_contents);
      if (other && other != data && other->window_id_ == data->window_id_)
        other->active_ = false;
    }
  }

  if (window_changed)
    SendBrowserWindowId(&data->GetWebContents(), data->window_id_);
}

void ElectronExtensionTabRegistry::OnViewDestroyed(
    content::WebContents* web_contents) {
  views_.erase(web_contents);
}

}  // namespace extensions
