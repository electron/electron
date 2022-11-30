// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_download_item.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/filename_util.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "url/gurl.h"

namespace gin {

template <>
struct Converter<download::DownloadItem::DownloadState> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      download::DownloadItem::DownloadState state) {
    std::string download_state;
    switch (state) {
      case download::DownloadItem::IN_PROGRESS:
        download_state = "progressing";
        break;
      case download::DownloadItem::COMPLETE:
        download_state = "completed";
        break;
      case download::DownloadItem::CANCELLED:
        download_state = "cancelled";
        break;
      case download::DownloadItem::INTERRUPTED:
        download_state = "interrupted";
        break;
      default:
        break;
    }
    return ConvertToV8(isolate, download_state);
  }
};

}  // namespace gin

namespace electron::api {

namespace {

// Ordinarily base::SupportsUserData only supports strong links, where the
// thing to which the user data is attached owns the user data. But we can't
// make the api::DownloadItem owned by the DownloadItem, since it's owned by
// V8. So this makes a weak link. The lifetimes of download::DownloadItem and
// api::DownloadItem are fully independent, and either one may be destroyed
// before the other.
struct UserDataLink : base::SupportsUserData::Data {
  explicit UserDataLink(base::WeakPtr<DownloadItem> item)
      : download_item(item) {}

  base::WeakPtr<DownloadItem> download_item;
};

const void* kElectronApiDownloadItemKey = &kElectronApiDownloadItemKey;

}  // namespace

gin::WrapperInfo DownloadItem::kWrapperInfo = {gin::kEmbedderNativeGin};

// static
DownloadItem* DownloadItem::FromDownloadItem(download::DownloadItem* item) {
  // ^- say that 7 times fast in a row
  auto* data = static_cast<UserDataLink*>(
      item->GetUserData(kElectronApiDownloadItemKey));
  return data ? data->download_item.get() : nullptr;
}

DownloadItem::DownloadItem(v8::Isolate* isolate, download::DownloadItem* item)
    : download_item_(item), isolate_(isolate) {
  download_item_->AddObserver(this);
  download_item_->SetUserData(
      kElectronApiDownloadItemKey,
      std::make_unique<UserDataLink>(weak_factory_.GetWeakPtr()));
}

DownloadItem::~DownloadItem() {
  if (download_item_) {
    // Destroyed by either garbage collection or destroy().
    download_item_->RemoveObserver(this);
    download_item_->Remove();
  }
}

bool DownloadItem::CheckAlive() const {
  if (!download_item_) {
    gin_helper::ErrorThrower(isolate_).ThrowError(
        "DownloadItem used after being destroyed");
    return false;
  }
  return true;
}

void DownloadItem::OnDownloadUpdated(download::DownloadItem* item) {
  if (!CheckAlive())
    return;
  if (download_item_->IsDone()) {
    Emit("done", item->GetState());
    Unpin();
  } else {
    Emit("updated", item->GetState());
  }
}

void DownloadItem::OnDownloadDestroyed(download::DownloadItem* /*item*/) {
  download_item_ = nullptr;
  Unpin();
}

void DownloadItem::Pause() {
  if (!CheckAlive())
    return;
  download_item_->Pause();
}

bool DownloadItem::IsPaused() const {
  if (!CheckAlive())
    return false;
  return download_item_->IsPaused();
}

void DownloadItem::Resume() {
  if (!CheckAlive())
    return;
  download_item_->Resume(true /* user_gesture */);
}

bool DownloadItem::CanResume() const {
  if (!CheckAlive())
    return false;
  return download_item_->CanResume();
}

void DownloadItem::Cancel() {
  if (!CheckAlive())
    return;
  download_item_->Cancel(true);
}

int64_t DownloadItem::GetReceivedBytes() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetReceivedBytes();
}

int64_t DownloadItem::GetTotalBytes() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetTotalBytes();
}

std::string DownloadItem::GetMimeType() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetMimeType();
}

bool DownloadItem::HasUserGesture() const {
  if (!CheckAlive())
    return false;
  return download_item_->HasUserGesture();
}

std::string DownloadItem::GetFilename() const {
  if (!CheckAlive())
    return "";
  return base::UTF16ToUTF8(
      net::GenerateFileName(GetURL(), GetContentDisposition(), std::string(),
                            download_item_->GetSuggestedFilename(),
                            GetMimeType(), "download")
          .LossyDisplayName());
}

std::string DownloadItem::GetContentDisposition() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetContentDisposition();
}

const GURL& DownloadItem::GetURL() const {
  if (!CheckAlive())
    return GURL::EmptyGURL();
  return download_item_->GetURL();
}

v8::Local<v8::Value> DownloadItem::GetURLChain() const {
  if (!CheckAlive())
    return v8::Local<v8::Value>();
  return gin::ConvertToV8(isolate_, download_item_->GetUrlChain());
}

download::DownloadItem::DownloadState DownloadItem::GetState() const {
  if (!CheckAlive())
    return download::DownloadItem::IN_PROGRESS;
  return download_item_->GetState();
}

bool DownloadItem::IsDone() const {
  if (!CheckAlive())
    return false;
  return download_item_->IsDone();
}

void DownloadItem::SetSavePath(const base::FilePath& path) {
  save_path_ = path;
}

base::FilePath DownloadItem::GetSavePath() const {
  return save_path_;
}

file_dialog::DialogSettings DownloadItem::GetSaveDialogOptions() const {
  return dialog_options_;
}

void DownloadItem::SetSaveDialogOptions(
    const file_dialog::DialogSettings& options) {
  dialog_options_ = options;
}

std::string DownloadItem::GetLastModifiedTime() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetLastModifiedTime();
}

std::string DownloadItem::GetETag() const {
  if (!CheckAlive())
    return "";
  return download_item_->GetETag();
}

double DownloadItem::GetStartTime() const {
  if (!CheckAlive())
    return 0;
  return download_item_->GetStartTime().ToDoubleT();
}

// static
gin::ObjectTemplateBuilder DownloadItem::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<DownloadItem>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("pause", &DownloadItem::Pause)
      .SetMethod("isPaused", &DownloadItem::IsPaused)
      .SetMethod("resume", &DownloadItem::Resume)
      .SetMethod("canResume", &DownloadItem::CanResume)
      .SetMethod("cancel", &DownloadItem::Cancel)
      .SetMethod("getReceivedBytes", &DownloadItem::GetReceivedBytes)
      .SetMethod("getTotalBytes", &DownloadItem::GetTotalBytes)
      .SetMethod("getMimeType", &DownloadItem::GetMimeType)
      .SetMethod("hasUserGesture", &DownloadItem::HasUserGesture)
      .SetMethod("getFilename", &DownloadItem::GetFilename)
      .SetMethod("getContentDisposition", &DownloadItem::GetContentDisposition)
      .SetMethod("getURL", &DownloadItem::GetURL)
      .SetMethod("getURLChain", &DownloadItem::GetURLChain)
      .SetMethod("getState", &DownloadItem::GetState)
      .SetMethod("isDone", &DownloadItem::IsDone)
      .SetMethod("setSavePath", &DownloadItem::SetSavePath)
      .SetMethod("getSavePath", &DownloadItem::GetSavePath)
      .SetProperty("savePath", &DownloadItem::GetSavePath,
                   &DownloadItem::SetSavePath)
      .SetMethod("setSaveDialogOptions", &DownloadItem::SetSaveDialogOptions)
      .SetMethod("getSaveDialogOptions", &DownloadItem::GetSaveDialogOptions)
      .SetMethod("getLastModifiedTime", &DownloadItem::GetLastModifiedTime)
      .SetMethod("getETag", &DownloadItem::GetETag)
      .SetMethod("getStartTime", &DownloadItem::GetStartTime);
}

const char* DownloadItem::GetTypeName() {
  return "DownloadItem";
}

// static
gin::Handle<DownloadItem> DownloadItem::FromOrCreate(
    v8::Isolate* isolate,
    download::DownloadItem* item) {
  DownloadItem* existing = FromDownloadItem(item);
  if (existing)
    return gin::CreateHandle(isolate, existing);

  auto handle = gin::CreateHandle(isolate, new DownloadItem(isolate, item));

  handle->Pin(isolate);

  return handle;
}

}  // namespace electron::api
