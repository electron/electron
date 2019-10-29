// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_download_item.h"

#include <map>

#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/filename_util.h"
#include "shell/browser/atom_browser_main_parts.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

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

namespace electron {

namespace api {

namespace {

std::map<uint32_t, v8::Global<v8::Value>> g_download_item_objects;

}  // namespace

DownloadItem::DownloadItem(v8::Isolate* isolate,
                           download::DownloadItem* download_item)
    : download_item_(download_item) {
  download_item_->AddObserver(this);
  Init(isolate);
  AttachAsUserData(download_item);
}

DownloadItem::~DownloadItem() {
  if (download_item_) {
    // Destroyed by either garbage collection or destroy().
    download_item_->RemoveObserver(this);
    download_item_->Remove();
  }

  // Remove from the global map.
  g_download_item_objects.erase(weak_map_id());
}

void DownloadItem::OnDownloadUpdated(download::DownloadItem* item) {
  if (download_item_->IsDone()) {
    Emit("done", item->GetState());
    // Destroy the item once item is downloaded.
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  GetDestroyClosure());
  } else {
    Emit("updated", item->GetState());
  }
}

void DownloadItem::OnDownloadDestroyed(download::DownloadItem* download_item) {
  download_item_ = nullptr;
  // Destroy the native class immediately when downloadItem is destroyed.
  delete this;
}

void DownloadItem::Pause() {
  download_item_->Pause();
}

bool DownloadItem::IsPaused() const {
  return download_item_->IsPaused();
}

void DownloadItem::Resume() {
  download_item_->Resume(true /* user_gesture */);
}

bool DownloadItem::CanResume() const {
  return download_item_->CanResume();
}

void DownloadItem::Cancel() {
  download_item_->Cancel(true);
}

int64_t DownloadItem::GetReceivedBytes() const {
  return download_item_->GetReceivedBytes();
}

int64_t DownloadItem::GetTotalBytes() const {
  return download_item_->GetTotalBytes();
}

std::string DownloadItem::GetMimeType() const {
  return download_item_->GetMimeType();
}

bool DownloadItem::HasUserGesture() const {
  return download_item_->HasUserGesture();
}

std::string DownloadItem::GetFilename() const {
  return base::UTF16ToUTF8(
      net::GenerateFileName(GetURL(), GetContentDisposition(), std::string(),
                            download_item_->GetSuggestedFilename(),
                            GetMimeType(), "download")
          .LossyDisplayName());
}

std::string DownloadItem::GetContentDisposition() const {
  return download_item_->GetContentDisposition();
}

const GURL& DownloadItem::GetURL() const {
  return download_item_->GetURL();
}

const std::vector<GURL>& DownloadItem::GetURLChain() const {
  return download_item_->GetUrlChain();
}

download::DownloadItem::DownloadState DownloadItem::GetState() const {
  return download_item_->GetState();
}

bool DownloadItem::IsDone() const {
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
  return download_item_->GetLastModifiedTime();
}

std::string DownloadItem::GetETag() const {
  return download_item_->GetETag();
}

double DownloadItem::GetStartTime() const {
  return download_item_->GetStartTime().ToDoubleT();
}

// static
void DownloadItem::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "DownloadItem"));
  gin_helper::Destroyable::MakeDestroyable(isolate, prototype);
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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

// static
gin::Handle<DownloadItem> DownloadItem::Create(v8::Isolate* isolate,
                                               download::DownloadItem* item) {
  auto* existing = TrackableObject::FromWrappedClass(isolate, item);
  if (existing)
    return gin::CreateHandle(isolate, static_cast<DownloadItem*>(existing));

  auto handle = gin::CreateHandle(isolate, new DownloadItem(isolate, item));

  // Reference this object in case it got garbage collected.
  g_download_item_objects[handle->weak_map_id()] =
      v8::Global<v8::Value>(isolate, handle.ToV8());
  return handle;
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary(isolate, exports)
      .Set("DownloadItem", electron::api::DownloadItem::GetConstructor(isolate)
                               ->GetFunction(context)
                               .ToLocalChecked());
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_download_item, Initialize)
