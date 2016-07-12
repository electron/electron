// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_
#define ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include "atom/renderer/content_settings_observer.h"
#include "base/values.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/renderer/render_process_observer.h"

#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/callback.h"

class GURL;

namespace base {
class DictionaryValue;
}

namespace mate {

template<>
struct Converter<ContentSetting> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   ContentSetting val) {
    std::string setting;
    switch (val) {
      case CONTENT_SETTING_ALLOW: setting = "allow"; break;
      case CONTENT_SETTING_BLOCK: setting = "block"; break;
      case CONTENT_SETTING_ASK: setting = "ask"; break;
      case CONTENT_SETTING_SESSION_ONLY: setting = "session"; break;
      default: setting = "default"; break;
    }
    return mate::ConvertToV8(isolate, setting);
  }
};

}  // namespace mate

namespace atom {

class ContentSettingsManager : public content::RenderProcessObserver {
 public:
  ContentSettingsManager();
  ~ContentSettingsManager() override;

  static ContentSettingsManager* GetInstance();

  const base::DictionaryValue* content_settings() const
    { return content_settings_.get(); };

  ContentSetting GetSetting(
      GURL primary_url,
      GURL secondary_url,
      std::string resource_id,
      bool incognito);

  std::vector<std::string> GetContentTypes();

  void AddObserver(ContentSettingsObserver* observer);

  void RemoveObserver(ContentSettingsObserver* observer);

 private:
  ContentSetting GetContentSettingFromRules(
    const GURL& primary_url,
    const GURL& secondary_url,
    const std::string& content_type,
    const bool& enabled_per_settings);

  // content::RenderThreadObserver:
  bool OnControlMessageReceived(const IPC::Message& message) override;

  void OnUpdateContentSettings(
      const base::DictionaryValue& content_settings);

  ContentSettingsObserverList observers_;

  std::unique_ptr<base::DictionaryValue> content_settings_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsManager);
};

}  // namespace atom

#endif  // ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_
