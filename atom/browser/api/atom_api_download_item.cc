// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_download_item.h"

#include <map>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<content::DownloadItem::DownloadState> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::DownloadItem::DownloadState state) {
    std::string download_state;
    switch (state) {
      case content::DownloadItem::IN_PROGRESS:
        download_state = "progressing";
        break;
      case content::DownloadItem::COMPLETE:
        download_state = "completed";
        break;
      case content::DownloadItem::CANCELLED:
        download_state = "cancelled";
        break;
      case content::DownloadItem::INTERRUPTED:
        download_state = "interrupted";
        break;
      default:
        break;
    }
    return ConvertToV8(isolate, download_state);
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

std::map<uint32_t, v8::Global<v8::Object>> g_download_item_objects;

}  // namespace

DownloadItem::DownloadItem(v8::Isolate* isolate,
                           content::DownloadItem* download_item)
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

void DownloadItem::OnDownloadUpdated(content::DownloadItem* item) {
  if (download_item_->IsDone()) {
    Emit("done", item->GetState());

    // Destroy the item once item is downloaded.
    base::MessageLoop::current()->PostTask(FROM_HERE, GetDestroyClosure());
  } else {
    Emit("updated", item->GetState());
  }
}

void DownloadItem::OnDownloadDestroyed(content::DownloadItem* download_item) {
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
  download_item_->Resume();
}

bool DownloadItem::CanResume() const {
  return download_item_->CanResume();
}

void DownloadItem::Cancel() {
  download_item_->Cancel(true);
  download_item_->Remove();
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
  return base::UTF16ToUTF8(net::GenerateFileName(GetURL(),
                           GetContentDisposition(),
                           std::string(),
                           download_item_->GetSuggestedFilename(),
                           GetMimeType(),
                           std::string()).LossyDisplayName());
}

std::string DownloadItem::GetContentDisposition() const {
  return download_item_->GetContentDisposition();
}

const GURL& DownloadItem::GetURL() const {
  return download_item_->GetURL();
}

content::DownloadItem::DownloadState DownloadItem::GetState() const {
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

// static
void DownloadItem::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "DownloadItem"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
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
      .SetMethod("getState", &DownloadItem::GetState)
      .SetMethod("isDone", &DownloadItem::IsDone)
      .SetMethod("setSavePath", &DownloadItem::SetSavePath)
      .SetMethod("getSavePath", &DownloadItem::GetSavePath);
}

// static
mate::Handle<DownloadItem> DownloadItem::Create(
    v8::Isolate* isolate, content::DownloadItem* item) {
  auto existing = TrackableObject::FromWrappedClass(isolate, item);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<DownloadItem*>(existing));

  auto handle = mate::CreateHandle(isolate, new DownloadItem(isolate, item));

  // Reference this object in case it got garbage collected.
  g_download_item_objects[handle->weak_map_id()] =
      v8::Global<v8::Object>(isolate, handle.ToV8());
  return handle;
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary(isolate, exports)
      .Set("DownloadItem",
           atom::api::DownloadItem::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_download_item, Initialize);
