// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/extensions/atom_extensions_client.h"

#include <string>
#include "atom/grit/atom_resources.h"  // NOLINT: This file is generated
#include "chrome/grit/common_resources.h"  // NOLINT: This file is generated
#include "chrome/grit/extensions_api_resources.h"  // NOLINT: This file is generated
#include "extensions/grit/extensions_resources.h"  // NOLINT: This file is generated
#include "extensions/common/extension_api.h"
#include "extensions/common/features/json_feature_provider_source.h"


namespace extensions {

namespace {

static base::LazyInstance<AtomExtensionsClient> g_client =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

AtomExtensionsClient::AtomExtensionsClient() :
  ChromeExtensionsClient() {
}

AtomExtensionsClient::~AtomExtensionsClient() {
}

const std::string AtomExtensionsClient::GetProductName() {
  return ATOM_PRODUCT_NAME;
}

scoped_ptr<JSONFeatureProviderSource>
AtomExtensionsClient::CreateFeatureProviderSource(
    const std::string& name) const {
  scoped_ptr<JSONFeatureProviderSource> source(
      new JSONFeatureProviderSource(name));
  if (name == "api") {
    source->LoadJSON(IDR_EXTENSION_API_FEATURES);
    source->LoadJSON(IDR_ATOM_API_FEATURES);
  } else if (name == "manifest") {
    source->LoadJSON(IDR_EXTENSION_MANIFEST_FEATURES);
  } else if (name == "permission") {
    source->LoadJSON(IDR_EXTENSION_PERMISSION_FEATURES);
    source->LoadJSON(IDR_CHROME_EXTENSION_PERMISSION_FEATURES);
  } else if (name == "behavior") {
    source->LoadJSON(IDR_EXTENSION_BEHAVIOR_FEATURES);
  } else {
    NOTREACHED();
    source.reset();
  }
  return source;
}

bool AtomExtensionsClient::IsScriptableURL(
    const GURL& url, std::string* error) const {
  return true;
}

void AtomExtensionsClient::RegisterAPISchemaResources(
    ExtensionAPI* api) const {
  api->RegisterSchemaResource("ipc", IDR_ATOM_EXTENSION_API_JSON_IPC);
  api->RegisterSchemaResource("commands", IDR_EXTENSION_API_JSON_COMMANDS);
  api->RegisterSchemaResource("declarativeContent",
                              IDR_EXTENSION_API_JSON_DECLARATIVE_CONTENT);
  api->RegisterSchemaResource("pageAction", IDR_EXTENSION_API_JSON_PAGEACTION);
  api->RegisterSchemaResource("privacy", IDR_EXTENSION_API_JSON_PRIVACY);
  api->RegisterSchemaResource("proxy", IDR_EXTENSION_API_JSON_PROXY);
  api->RegisterSchemaResource("ttsEngine", IDR_EXTENSION_API_JSON_TTSENGINE);
  api->RegisterSchemaResource("tts", IDR_EXTENSION_API_JSON_TTS);
  api->RegisterSchemaResource("types", IDR_EXTENSION_API_JSON_TYPES);
  api->RegisterSchemaResource("types.private",
                              IDR_EXTENSION_API_JSON_TYPES_PRIVATE);
}

std::string AtomExtensionsClient::GetWebstoreBaseURL() const {
  return "about:blank";
}

std::string AtomExtensionsClient::GetWebstoreUpdateURL() const {
  return "about:blank";
}

// static
AtomExtensionsClient* AtomExtensionsClient::GetInstance() {
  return g_client.Pointer();
}

}  // namespace extensions
