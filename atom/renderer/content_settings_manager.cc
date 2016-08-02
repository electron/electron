// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/content_settings_manager.h"

#include <string>
#include <vector>
#include "atom/common/api/api_messages.h"
#include "base/values.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/render_thread.h"
#include "url/url_constants.h"
#include "url/gurl.h"

namespace atom {

ContentSettingsManager::ContentSettingsManager() {
  content::RenderThread::Get()->AddObserver(this);
}

ContentSettingsManager::~ContentSettingsManager() {
  if (content::RenderThread::Get())
    content::RenderThread::Get()->RemoveObserver(this);
}

// static
ContentSettingsManager* ContentSettingsManager::GetInstance() {
  static base::LazyInstance<ContentSettingsManager> manager =
    LAZY_INSTANCE_INITIALIZER;
  return manager.Pointer();
}

bool ContentSettingsManager::OnControlMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ContentSettingsManager, message)
    IPC_MESSAGE_HANDLER(AtomMsg_UpdateContentSettings, OnUpdateContentSettings)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ContentSettingsManager::OnUpdateContentSettings(
    const base::DictionaryValue& content_settings) {
  content_settings_ = content_settings.CreateDeepCopy();
  FOR_EACH_OBSERVER(
    ContentSettingsObserver,
    observers_,
    OnContentSettingsChanged(content_settings));
}

void ContentSettingsManager::AddObserver(
    ContentSettingsObserver* observer) {
  observers_.AddObserver(observer);
}

void ContentSettingsManager::RemoveObserver(
    ContentSettingsObserver* observer) {
  observers_.RemoveObserver(observer);
}

ContentSetting ContentSettingsManager::GetSetting(
    GURL primary_url,
    GURL secondary_url,
    std::string content_type,
    bool incognito) {
  return GetContentSettingFromRules(primary_url,
                                    secondary_url,
                                    content_type,
                                    content_type != "runInsecureContent"
                                      ? true : false);
}

std::vector<std::string> ContentSettingsManager::GetContentTypes() {
  std::vector<std::string> content_types;
  const base::DictionaryValue* settings = nullptr;
  content_settings_->GetAsDictionary(&settings);
  for (base::DictionaryValue::Iterator it(*settings);
      !it.IsAtEnd();
      it.Advance()) {
    content_types.push_back(it.key());
  }
  return content_types;
}

ContentSetting ContentSettingsManager::GetContentSettingFromRules(
    const GURL& primary_url,
    const GURL& secondary_url,
    const std::string& content_type,
    const bool& default_value) {
  ContentSetting result = default_value
    ? ContentSetting::CONTENT_SETTING_ALLOW
    : ContentSetting::CONTENT_SETTING_BLOCK;

  const base::ListValue* rules = nullptr;
  if (!content_settings_->GetList(content_type, &rules))
    return result;

  // all rules are evaluated in order and the
  // most specific matching rule will apply
  for (base::ListValue::const_iterator it = rules->begin();
       it != rules->end(); ++it) {
    base::DictionaryValue* rule;
    std::string pattern_string;
    std::string setting_string;
    if (!(*it)->GetAsDictionary(&rule) ||
        !rule->GetString("primaryPattern", &pattern_string) ||
        !rule->GetString("setting", &setting_string)) {
      // skip invalid entries
      // TODO(bridiver) should also send an ipc error message
      ++it;
    } else {
      std::string secondary_pattern_string;
      rule->GetString("secondaryPattern", &secondary_pattern_string);

      ContentSetting setting = result;
      if (setting_string != "block" && setting_string != "deny") {
        setting = ContentSetting::CONTENT_SETTING_ALLOW;
      } else {
        setting = ContentSetting::CONTENT_SETTING_BLOCK;
      }

      if (secondary_pattern_string == "[firstParty]") {
        secondary_pattern_string = "[*.]" + primary_url.HostNoBrackets();
      }

      if (ContentSettingsPattern::FromString(
            pattern_string).Matches(primary_url) &&
          // if there is a secondary resource pattern it has to match as well
          (secondary_pattern_string.empty() ||
            ContentSettingsPattern::FromString(
              secondary_pattern_string).Matches(secondary_url)))
        result = setting;
    }
  }
  return result;
}
}  // namespace atom
