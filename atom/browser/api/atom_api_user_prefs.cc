// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_user_prefs.h"

#include "atom/common/native_mate_converters/value_converter.h"
#include "base/values.h"
#include "content/public/browser/browser_thread.h"

#if defined(ENABLE_EXTENSIONS)
#include "components/syncable_prefs/pref_service_syncable.h"
#include "components/pref_registry/pref_registry_syncable.h"
#endif

#include "atom/common/native_mate_converters/v8_value_converter.h"

namespace mate {

template<>
struct Converter<const base::DictionaryValue*> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const base::DictionaryValue* val) {
    std::unique_ptr<atom::V8ValueConverter>
        converter(new atom::V8ValueConverter);
    return converter->ToV8Value(val, isolate->GetCurrentContext());
  }
};

template<>
struct Converter<const base::ListValue*> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const base::ListValue* val) {
    std::unique_ptr<atom::V8ValueConverter>
        converter(new atom::V8ValueConverter);
    return converter->ToV8Value(val, isolate->GetCurrentContext());
  }
};

}  // namespace mate

namespace atom {

namespace api {

UserPrefs::UserPrefs(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context) {
  Init(isolate);
}

UserPrefs::~UserPrefs() {
}

void UserPrefs::RegisterStringPref(const std::string& path,
    const std::string& default_value, bool overlay) {
  browser_context()->pref_registry()->RegisterStringPref(path, default_value);
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

void UserPrefs::RegisterDictionaryPref(const std::string& path,
    const base::DictionaryValue& default_value, bool overlay) {
  std::unique_ptr<base::DictionaryValue> copied(
      default_value.CreateDeepCopy());
  browser_context()->pref_registry()->
      RegisterDictionaryPref(path, copied.release());
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

void UserPrefs::RegisterListPref(const std::string& path,
    const base::ListValue& default_value, bool overlay) {
  std::unique_ptr<base::ListValue> copied(
      default_value.CreateDeepCopy());
  browser_context()->pref_registry()->
      RegisterListPref(path, copied.release());
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

void UserPrefs::RegisterBooleanPref(const std::string& path,
    bool default_value, bool overlay) {
  browser_context()->pref_registry()->RegisterBooleanPref(path, default_value);
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

void UserPrefs::RegisterIntegerPref(const std::string& path,
    int default_value, bool overlay) {
  browser_context()->pref_registry()->RegisterIntegerPref(path, default_value);
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

void UserPrefs::RegisterDoublePref(const std::string& path,
    double default_value, bool overlay) {
  browser_context()->pref_registry()->RegisterDoublePref(path, default_value);
  if (overlay)
    browser_context()->AddOverlayPref(path);
}

std::string UserPrefs::GetStringPref(const std::string& path) {
  return browser_context()->user_prefs()->GetString(path);
}

const base::DictionaryValue* UserPrefs::GetDictionaryPref(
      const std::string& path) {
  return browser_context()->user_prefs()->GetDictionary(path);
}

const base::ListValue* UserPrefs::GetListPref(const std::string& path) {
  return browser_context()->user_prefs()->GetList(path);
}

bool UserPrefs::GetBooleanPref(const std::string& path) {
  return browser_context()->user_prefs()->GetBoolean(path);
}

int UserPrefs::GetIntegerPref(const std::string& path) {
  return browser_context()->user_prefs()->GetInteger(path);
}

double UserPrefs::GetDoublePref(const std::string& path) {
  return browser_context()->user_prefs()->GetDouble(path);
}

void UserPrefs::SetStringPref(const std::string& path,
    const std::string& value) {
  browser_context()->user_prefs()->SetString(path, value);
}

void UserPrefs::SetDictionaryPref(const std::string& path,
    const base::DictionaryValue& value) {
  browser_context()->user_prefs()->Set(path, value);
}

void UserPrefs::SetListPref(const std::string& path,
    const base::ListValue& value) {
  browser_context()->user_prefs()->Set(path, value);
}

void UserPrefs::SetBooleanPref(const std::string& path,
    bool value) {
  browser_context()->user_prefs()->SetBoolean(path, value);
}

void UserPrefs::SetIntegerPref(const std::string& path,
    int value) {
  browser_context()->user_prefs()->SetInteger(path, value);
}

void UserPrefs::SetDoublePref(const std::string& path,
    double value) {
  browser_context()->user_prefs()->SetDouble(path, value);
}

double UserPrefs::GetDefaultZoomLevel() {
  return browser_context()->GetZoomLevelPrefs()->GetDefaultZoomLevelPref();
}

void UserPrefs::SetDefaultZoomLevel(double zoom) {
  if (!browser_context()->IsOffTheRecord())
    browser_context()->GetZoomLevelPrefs()->SetDefaultZoomLevelPref(zoom);
}

// static
mate::Handle<UserPrefs> UserPrefs::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new UserPrefs(isolate, browser_context));
}

// static
void UserPrefs::BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("registerStringPref", &UserPrefs::RegisterStringPref)
      .SetMethod("registerDictionaryPref", &UserPrefs::RegisterDictionaryPref)
      .SetMethod("registerListPref", &UserPrefs::RegisterListPref)
      .SetMethod("registerBooleanPref", &UserPrefs::RegisterBooleanPref)
      .SetMethod("registerIntegerPref", &UserPrefs::RegisterIntegerPref)
      .SetMethod("registerDoublePref", &UserPrefs::RegisterDoublePref)
      // .SetMethod("registerFilePathPref", &UserPrefs::RegisterFilePathPref)

      .SetMethod("getStringPref", &UserPrefs::GetStringPref)
      .SetMethod("getDictionaryPref", &UserPrefs::GetDictionaryPref)
      .SetMethod("getListPref", &UserPrefs::GetListPref)
      .SetMethod("getBooleanPref", &UserPrefs::GetBooleanPref)
      .SetMethod("getIntegerPref", &UserPrefs::GetIntegerPref)
      .SetMethod("getDoublePref", &UserPrefs::GetDoublePref)
      // .SetMethod("getFilePathPref", &UserPrefs::GetFilePathPref)

      .SetMethod("setStringPref", &UserPrefs::SetStringPref)
      .SetMethod("setDictionaryPref", &UserPrefs::SetDictionaryPref)
      .SetMethod("setListPref", &UserPrefs::SetListPref)
      .SetMethod("setBooleanPref", &UserPrefs::SetBooleanPref)
      .SetMethod("setIntegerPref", &UserPrefs::SetIntegerPref)
      .SetMethod("setDoublePref", &UserPrefs::SetDoublePref)
      // .SetMethod("setFilePathPref", &UserPrefs::SetFilePathPref)

      .SetMethod("getDefaultZoomLevel", &UserPrefs::GetDefaultZoomLevel)
      .SetMethod("setDefaultZoomLevel", &UserPrefs::SetDefaultZoomLevel);
}

}  // namespace api

}  // namespace atom
