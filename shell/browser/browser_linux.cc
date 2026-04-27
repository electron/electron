// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

#if BUILDFLAG(IS_LINUX)
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#endif

#include "base/environment.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "electron/electron_version.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_converters/login_item_settings_converter.h"

#if BUILDFLAG(IS_LINUX)
#include "shell/browser/linux/unity_service.h"
#endif

namespace electron {

namespace {

bool SetDefaultWebClient(const std::string& protocol) {
  const auto env = base::Environment::Create();
  const std::optional<std::string> desktop_name = env->GetVar("CHROME_DESKTOP");
  if (!desktop_name)
    return false;

  GDesktopAppInfo* const app_info =
      g_desktop_app_info_new(desktop_name->c_str());
  if (!app_info)
    return false;

  const std::string content_type = "x-scheme-handler/" + protocol;
  GError* error = nullptr;
  const bool success = g_app_info_set_as_default_for_type(
      G_APP_INFO(app_info), content_type.c_str(), &error);
  if (error != nullptr)
    LOG(ERROR) << error->message;
  g_clear_error(&error);
  g_object_unref(app_info);
  return success;
}

}  // namespace

void Browser::AddRecentDocument(const base::FilePath& path) {}

std::vector<std::string> Browser::GetRecentDocuments() {
  return std::vector<std::string>();
}

void Browser::ClearRecentDocuments() {}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         gin::Arguments* args) {
  if (!IsValidProtocolScheme(protocol))
    return false;

  return SetDefaultWebClient(protocol);
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      gin::Arguments* args) {
  if (!IsValidProtocolScheme(protocol))
    return false;

  const auto env = base::Environment::Create();
  const std::optional<std::string> desktop_name = env->GetVar("CHROME_DESKTOP");
  if (!desktop_name)
    return false;

  GAppInfo* app_info = g_app_info_get_default_for_uri_scheme(protocol.c_str());
  if (!app_info)
    return false;

  const char* const app_id = g_app_info_get_id(app_info);
  const bool is_default = app_id && app_id == desktop_name.value();
  g_object_unref(app_info);
  return is_default;
}

// Todo implement
bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            gin::Arguments* args) {
  return false;
}

std::u16string Browser::GetApplicationNameForProtocol(const GURL& url) {
  const auto scheme = std::string{url.scheme()};  // gio can't use string_view
  auto* app_info = g_app_info_get_default_for_uri_scheme(scheme.c_str());
  if (!app_info)
    return {};

  const char* const name = g_app_info_get_display_name(app_info);
  const std::u16string u16name = base::UTF8ToUTF16(name);
  g_object_unref(app_info);
  return u16name;
}

bool Browser::SetBadgeCount(std::optional<int> count) {
  if (IsUnityRunning() && count.has_value()) {
    unity::SetDownloadCount(count.value());
    badge_count_ = count.value();
    return true;
  } else {
    return false;
  }
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {}

v8::Local<v8::Value> Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  LoginItemSettings settings;
  return gin::ConvertToV8(JavascriptEnvironment::GetIsolate(), settings);
}

std::string Browser::GetExecutableFileVersion() const {
  return GetApplicationVersion();
}

std::string Browser::GetExecutableFileProductName() const {
  return GetApplicationName();
}

bool Browser::IsUnityRunning() {
  return unity::IsRunning();
}

bool Browser::IsEmojiPanelSupported() {
  return false;
}

void Browser::ShowAboutPanel() {
  const auto& opts = about_panel_options_;

  GtkWidget* dialogWidget = gtk_about_dialog_new();
  GtkAboutDialog* dialog = GTK_ABOUT_DIALOG(dialogWidget);

  const std::string* str;
  const base::ListValue* list;

  if ((str = opts.FindString("applicationName"))) {
    gtk_about_dialog_set_program_name(dialog, str->c_str());
  }
  if ((str = opts.FindString("applicationVersion"))) {
    gtk_about_dialog_set_version(dialog, str->c_str());
  }
  if ((str = opts.FindString("copyright"))) {
    gtk_about_dialog_set_copyright(dialog, str->c_str());
  }
  if ((str = opts.FindString("website"))) {
    gtk_about_dialog_set_website(dialog, str->c_str());
  }
  if ((str = opts.FindString("iconPath"))) {
    GError* error = nullptr;
    constexpr int width = 64;   // width of about panel icon in pixels
    constexpr int height = 64;  // height of about panel icon in pixels

    // set preserve_aspect_ratio to true
    GdkPixbuf* icon =
        gdk_pixbuf_new_from_file_at_size(str->c_str(), width, height, &error);
    if (error != nullptr) {
      LOG(INFO) << error->message;
      g_clear_error(&error);
    } else {
      gtk_about_dialog_set_logo(dialog, icon);
      g_clear_object(&icon);
    }
  }

  if ((list = opts.FindList("authors"))) {
    std::vector<const char*> cstrs;
    for (const auto& authorVal : *list) {
      if (authorVal.is_string()) {
        cstrs.push_back(authorVal.GetString().c_str());
      }
    }
    if (cstrs.empty()) {
      LOG(WARNING) << "No author strings found in 'authors' array";
    } else {
      cstrs.push_back(nullptr);  // null-terminated char* array
      gtk_about_dialog_set_authors(dialog, cstrs.data());
    }
  }

  // destroy the widget when it closes
  g_signal_connect_swapped(dialogWidget, "response",
                           G_CALLBACK(gtk_widget_destroy), dialogWidget);

  gtk_widget_show_all(dialogWidget);
}

void Browser::SetAboutPanelOptions(base::DictValue options) {
  about_panel_options_ = std::move(options);
}

}  // namespace electron
