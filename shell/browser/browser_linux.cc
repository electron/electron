// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

#include <fcntl.h>
#include <stdlib.h>

#include <string_view>

#if BUILDFLAG(IS_LINUX)
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#endif

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "electron/electron_version.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/login_item_settings_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/thread_restrictions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/glib/glib_cast.h"
#include "ui/base/glib/scoped_gobject.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gtk/gtk_compat.h"
#include "ui/gtk/gtk_util.h"

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

[[nodiscard]] ScopedGObject<GAppInfo> GetAppInfoForProtocol(const GURL& url) {
  const auto scheme = std::string{url.scheme()};  // gio can't use string_views
  return TakeGObject(g_app_info_get_default_for_uri_scheme(scheme.c_str()));
}

[[nodiscard]] std::optional<base::FilePath> ResolveExecutablePath(
    const char* const executable) {
  if (executable == nullptr || *executable == '\0')
    return {};

  if (auto path = base::FilePath::FromUTF8Unsafe(executable); path.IsAbsolute())
    return path;

  gchar* const found = g_find_program_in_path(executable);
  if (!found)
    return {};

  const auto path = base::FilePath::FromUTF8Unsafe(found);
  g_free(found);
  return path;
}

[[nodiscard]] gfx::Image GetApplicationIcon(GAppInfo* const app_info) {
  constexpr int kIconSize = 32;

  GIcon* const icon = g_app_info_get_icon(app_info);
  if (!icon)
    return {};

  // Note: this gtk3/gtk4 + icon theme lookup + snapshot control flow
  // is copied from GtkUi::GetIconForContentType(). We couldn't use it
  // here because it gets its GIcon from a different place than us.
  SkBitmap bitmap;
  if (gtk::GtkCheckVersion(4)) {
    const auto icon_paintable = gtk::Gtk4IconThemeLookupByGicon(
        gtk::GetDefaultIconTheme(), icon, kIconSize, 1, GTK_TEXT_DIR_NONE,
        static_cast<GtkIconLookupFlags>(0));
    if (!icon_paintable)
      return {};

    auto* const paintable =
        GlibCast<GdkPaintable>(icon_paintable.get(), gdk_paintable_get_type());
    auto* const snapshot = gtk_snapshot_new();
    gdk_paintable_snapshot(paintable, snapshot, kIconSize, kIconSize);
    auto* const node = gtk_snapshot_free_to_node(snapshot);
    GdkTexture* const texture = gtk::GetTextureFromRenderNode(node);
    if (!texture) {
      gsk_render_node_unref(node);
      return {};
    }

    bitmap.allocN32Pixels(gdk_texture_get_width(texture),
                          gdk_texture_get_height(texture));
    gdk_texture_download(texture, static_cast<guchar*>(bitmap.getAddr(0, 0)),
                         bitmap.rowBytes());
    gsk_render_node_unref(node);
  } else {
    const auto icon_info = gtk::Gtk3IconThemeLookupByGiconForScale(
        gtk::GetDefaultIconTheme(), icon, kIconSize, 1,
        static_cast<GtkIconLookupFlags>(GTK_ICON_LOOKUP_FORCE_SIZE));
    if (!icon_info)
      return {};

    auto* const surface =
        gtk_icon_info_load_surface(icon_info.get(), nullptr, nullptr);
    if (!surface)
      return {};

    DCHECK_EQ(cairo_surface_get_type(surface), CAIRO_SURFACE_TYPE_IMAGE);
    DCHECK_EQ(cairo_image_surface_get_format(surface), CAIRO_FORMAT_ARGB32);

    const SkImageInfo image_info =
        SkImageInfo::Make(cairo_image_surface_get_width(surface),
                          cairo_image_surface_get_height(surface),
                          kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
    if (!bitmap.installPixels(
            image_info, cairo_image_surface_get_data(surface),
            image_info.minRowBytes(),
            [](void*, void* surface_ptr) {
              cairo_surface_destroy(
                  reinterpret_cast<cairo_surface_t*>(surface_ptr));
            },
            surface)) {
      return {};
    }
  }

  gfx::ImageSkia image_skia = gfx::ImageSkia::CreateFrom1xBitmap(bitmap);
  if (image_skia.isNull())
    return {};

  image_skia.MakeThreadSafe();
  return gfx::Image(image_skia);
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
  auto app_info = GetAppInfoForProtocol(url);
  if (!app_info)
    return {};

  const char* const name = g_app_info_get_display_name(app_info);
  return base::UTF8ToUTF16(name);
}

v8::Local<v8::Promise> Browser::GetApplicationInfoForProtocol(
    v8::Isolate* const isolate,
    const GURL& url) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  const v8::Local<v8::Promise> handle = promise.GetHandle();

  auto app_info = GetAppInfoForProtocol(url);
  if (!app_info) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return handle;
  }

  const char* const executable = g_app_info_get_executable(app_info);
  const auto app_path = ResolveExecutablePath(executable);
  if (!app_path) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return handle;
  }

  const char* const app_display_name = g_app_info_get_display_name(app_info);
  if (!app_display_name || app_display_name[0] == '\0') {
    promise.RejectWithErrorMessage("Unable to retrieve display name of app");
    return handle;
  }

  const gfx::Image app_icon = GetApplicationIcon(app_info);
  if (app_icon.IsEmpty()) {
    promise.RejectWithErrorMessage("Failed to get file icon.");
    return handle;
  }

  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("name", base::UTF8ToUTF16(app_display_name));
  dict.Set("path", app_path->value());
  dict.Set("icon", app_icon);
  promise.Resolve(dict);
  return handle;
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
