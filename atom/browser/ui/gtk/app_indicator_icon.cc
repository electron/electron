// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/gtk/app_indicator_icon.h"

#include <gtk/gtk.h>
#include <dlfcn.h>

#include "base/file_util.h"
#include "base/guid.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/ui/gtk/menu_gtk.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/image/image.h"

namespace {

typedef enum {
  APP_INDICATOR_CATEGORY_APPLICATION_STATUS,
  APP_INDICATOR_CATEGORY_COMMUNICATIONS,
  APP_INDICATOR_CATEGORY_SYSTEM_SERVICES,
  APP_INDICATOR_CATEGORY_HARDWARE,
  APP_INDICATOR_CATEGORY_OTHER
} AppIndicatorCategory;

typedef enum {
  APP_INDICATOR_STATUS_PASSIVE,
  APP_INDICATOR_STATUS_ACTIVE,
  APP_INDICATOR_STATUS_ATTENTION
} AppIndicatorStatus;

typedef AppIndicator* (*app_indicator_new_func)(const gchar* id,
                                                const gchar* icon_name,
                                                AppIndicatorCategory category);

typedef AppIndicator* (*app_indicator_new_with_path_func)(
    const gchar* id,
    const gchar* icon_name,
    AppIndicatorCategory category,
    const gchar* icon_theme_path);

typedef void (*app_indicator_set_status_func)(AppIndicator* self,
                                              AppIndicatorStatus status);

typedef void (*app_indicator_set_attention_icon_full_func)(
    AppIndicator* self,
    const gchar* icon_name,
    const gchar* icon_desc);

typedef void (*app_indicator_set_menu_func)(AppIndicator* self, GtkMenu* menu);

typedef void (*app_indicator_set_icon_full_func)(AppIndicator* self,
                                                 const gchar* icon_name,
                                                 const gchar* icon_desc);

typedef void (*app_indicator_set_icon_theme_path_func)(
    AppIndicator* self,
    const gchar* icon_theme_path);

bool g_attempted_load = false;
bool g_opened = false;

// Retrieved functions from libappindicator.
app_indicator_new_func app_indicator_new = NULL;
app_indicator_new_with_path_func app_indicator_new_with_path = NULL;
app_indicator_set_status_func app_indicator_set_status = NULL;
app_indicator_set_attention_icon_full_func
    app_indicator_set_attention_icon_full = NULL;
app_indicator_set_menu_func app_indicator_set_menu = NULL;
app_indicator_set_icon_full_func app_indicator_set_icon_full = NULL;
app_indicator_set_icon_theme_path_func app_indicator_set_icon_theme_path = NULL;

void EnsureMethodsLoaded() {
  if (g_attempted_load)
    return;

  g_attempted_load = true;

  void* indicator_lib = dlopen("libappindicator.so", RTLD_LAZY);
  if (!indicator_lib) {
    indicator_lib = dlopen("libappindicator.so.1", RTLD_LAZY);
  }
  if (!indicator_lib) {
    indicator_lib = dlopen("libappindicator.so.0", RTLD_LAZY);
  }
  if (!indicator_lib) {
    return;
  }

  g_opened = true;

  app_indicator_new = reinterpret_cast<app_indicator_new_func>(
      dlsym(indicator_lib, "app_indicator_new"));

  app_indicator_new_with_path =
      reinterpret_cast<app_indicator_new_with_path_func>(
          dlsym(indicator_lib, "app_indicator_new_with_path"));

  app_indicator_set_status = reinterpret_cast<app_indicator_set_status_func>(
      dlsym(indicator_lib, "app_indicator_set_status"));

  app_indicator_set_attention_icon_full =
      reinterpret_cast<app_indicator_set_attention_icon_full_func>(
          dlsym(indicator_lib, "app_indicator_set_attention_icon_full"));

  app_indicator_set_menu = reinterpret_cast<app_indicator_set_menu_func>(
      dlsym(indicator_lib, "app_indicator_set_menu"));

  app_indicator_set_icon_full =
      reinterpret_cast<app_indicator_set_icon_full_func>(
          dlsym(indicator_lib, "app_indicator_set_icon_full"));

  app_indicator_set_icon_theme_path =
      reinterpret_cast<app_indicator_set_icon_theme_path_func>(
          dlsym(indicator_lib, "app_indicator_set_icon_theme_path"));
}

base::FilePath CreateTempImageFile(gfx::ImageSkia* image_ptr,
                                   int icon_change_count,
                                   std::string id) {
  scoped_ptr<gfx::ImageSkia> image(image_ptr);

  scoped_refptr<base::RefCountedMemory> png_data =
      gfx::Image(*image.get()).As1xPNGBytes();
  if (png_data->size() == 0) {
    // If the bitmap could not be encoded to PNG format, skip it.
    LOG(WARNING) << "Could not encode icon";
    return base::FilePath();
  }

  base::FilePath temp_dir;
  base::FilePath new_file_path;

  // Create a new temporary directory for each image since using a single
  // temporary directory seems to have issues when changing icons in quick
  // succession.
  if (!file_util::CreateNewTempDirectory(base::FilePath::StringType(),
                                         &temp_dir))
    return base::FilePath();
  new_file_path =
      temp_dir.Append(id + base::StringPrintf("_%d.png", icon_change_count));
  int bytes_written =
      file_util::WriteFile(
          new_file_path,
          reinterpret_cast<const char*>(png_data->front()),
          png_data->size());

  if (bytes_written != static_cast<int>(png_data->size()))
    return base::FilePath();
  return new_file_path;
}

void DeleteTempImagePath(const base::FilePath& icon_file_path) {
  if (icon_file_path.empty())
    return;
  base::DeleteFile(icon_file_path, true);
}

}  // namespace

namespace atom {

AppIndicatorIcon::AppIndicatorIcon()
    : icon_(NULL),
      id_(base::GenerateGUID()),
      icon_change_count_(0),
      weak_factory_(this) {
}

AppIndicatorIcon::~AppIndicatorIcon() {
  if (icon_) {
    app_indicator_set_status(icon_, APP_INDICATOR_STATUS_PASSIVE);
    // if (gtk_menu_)
    //   DestroyMenu();
    g_object_unref(icon_);
    content::BrowserThread::GetBlockingPool()->PostTask(
        FROM_HERE,
        base::Bind(&DeleteTempImagePath, icon_file_path_.DirName()));
  }
}

bool AppIndicatorIcon::CouldOpen() {
  EnsureMethodsLoaded();
  return g_opened;
}

void AppIndicatorIcon::SetImage(const gfx::ImageSkia& image) {
  if (!g_opened)
    return;

  ++icon_change_count_;

  // We create a deep copy of the image since it may have been freed by the time
  // it's accessed in the other thread.
  scoped_ptr<gfx::ImageSkia> safe_image(image.DeepCopy());
  base::PostTaskAndReplyWithResult(
      content::BrowserThread::GetBlockingPool()
          ->GetTaskRunnerWithShutdownBehavior(
                base::SequencedWorkerPool::SKIP_ON_SHUTDOWN).get(),
      FROM_HERE,
      base::Bind(&CreateTempImageFile,
                 safe_image.release(),
                 icon_change_count_,
                 id_),
      base::Bind(&AppIndicatorIcon::SetImageFromFile,
                 weak_factory_.GetWeakPtr()));
}

void AppIndicatorIcon::SetPressedImage(const gfx::ImageSkia& image) {
  // Ignore pressed images, since the standard on Linux is to not highlight
  // pressed status icons.
}

void AppIndicatorIcon::SetToolTip(const std::string& tool_tip) {
  // App indicator doesn't have tooltips:
  // https://bugs.launchpad.net/indicator-application/+bug/527458
}

void AppIndicatorIcon::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset(new MenuGtk(NULL, menu_model));
  app_indicator_set_menu(icon_, GTK_MENU(menu_->widget()));
}

void AppIndicatorIcon::SetImageFromFile(const base::FilePath& icon_file_path) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  if (icon_file_path.empty())
    return;

  base::FilePath old_path = icon_file_path_;
  icon_file_path_ = icon_file_path;

  std::string icon_name =
      icon_file_path_.BaseName().RemoveExtension().value();
  std::string icon_dir = icon_file_path_.DirName().value();
  if (!icon_) {
    icon_ =
        app_indicator_new_with_path(id_.c_str(),
                                    icon_name.c_str(),
                                    APP_INDICATOR_CATEGORY_APPLICATION_STATUS,
                                    icon_dir.c_str());
    app_indicator_set_status(icon_, APP_INDICATOR_STATUS_ACTIVE);
  } else {
    // Currently we are creating a new temp directory every time the icon is
    // set. So we need to set the directory each time.
    app_indicator_set_icon_theme_path(icon_, icon_dir.c_str());
    app_indicator_set_icon_full(icon_, icon_name.c_str(), "icon");

    // Delete previous icon directory.
    content::BrowserThread::GetBlockingPool()->PostTask(
        FROM_HERE,
        base::Bind(&DeleteTempImagePath, old_path.DirName()));
  }
}

}  // namespace atom
