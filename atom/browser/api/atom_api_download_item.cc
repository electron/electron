// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_download_item.h"

#include <map>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/node_includes.h"
#include "base/memory/linked_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

namespace mate {

template<>
struct Converter<content::DownloadItem::DownloadState> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   content::DownloadItem::DownloadState state) {
    std::string download_state;
    switch (state) {
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
// The wrapDownloadItem funtion which is implemented in JavaScript
using WrapDownloadItemCallback = base::Callback<void(v8::Local<v8::Value>)>;
WrapDownloadItemCallback g_wrap_download_item;

char kDownloadItemSavePathKey[] = "DownloadItemSavePathKey";

std::map<uint32, linked_ptr<v8::Global<v8::Value>>> g_download_item_objects;
}  // namespace

DownloadItem::SavePathData::SavePathData(const base::FilePath& path) :
  path_(path) {
}

const base::FilePath& DownloadItem::SavePathData::path() {
  return path_;
}

DownloadItem::DownloadItem(content::DownloadItem* download_item) :
    download_item_(download_item) {
  download_item_->AddObserver(this);
}

DownloadItem::~DownloadItem() {
  Destroy();
}

void DownloadItem::Destroy() {
  if (download_item_) {
    download_item_->RemoveObserver(this);
    auto iter = g_download_item_objects.find(download_item_->GetId());
    if (iter != g_download_item_objects.end())
      g_download_item_objects.erase(iter);
    download_item_ = nullptr;
  }
}

bool DownloadItem::IsDestroyed() const {
  return download_item_ == nullptr;
}

void DownloadItem::OnDownloadUpdated(content::DownloadItem* item) {
  download_item_->IsDone() ? Emit("done", item->GetState()) : Emit("updated");
}

void DownloadItem::OnDownloadDestroyed(content::DownloadItem* download) {
  Destroy();
}

int64 DownloadItem::GetReceivedBytes() {
  return download_item_->GetReceivedBytes();
}

int64 DownloadItem::GetTotalBytes() {
  return download_item_->GetTotalBytes();
}

const GURL& DownloadItem::GetUrl() {
  return download_item_->GetURL();
}

std::string DownloadItem::GetMimeType() {
  return download_item_->GetMimeType();
}

bool DownloadItem::HasUserGesture() {
  return download_item_->HasUserGesture();
}

std::string DownloadItem::GetFilename() {
  return base::UTF16ToUTF8(net::GenerateFileName(GetUrl(),
                           GetContentDisposition(),
                           std::string(),
                           download_item_->GetSuggestedFilename(),
                           GetMimeType(),
                           std::string()).LossyDisplayName());
}

std::string DownloadItem::GetContentDisposition() {
  return download_item_->GetContentDisposition();
}

void DownloadItem::SetSavePath(const base::FilePath& path) {
  download_item_->SetUserData(UserDataKey(), new SavePathData(path));
}

void DownloadItem::Pause() {
  download_item_->Pause();
}

void DownloadItem::Resume() {
  download_item_->Resume();
}

void DownloadItem::Cancel() {
  download_item_->Cancel(true);
}

mate::ObjectTemplateBuilder DownloadItem::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("pause", &DownloadItem::Pause)
      .SetMethod("resume", &DownloadItem::Resume)
      .SetMethod("cancel", &DownloadItem::Cancel)
      .SetMethod("getReceivedBytes", &DownloadItem::GetReceivedBytes)
      .SetMethod("getTotalBytes", &DownloadItem::GetTotalBytes)
      .SetMethod("getUrl", &DownloadItem::GetUrl)
      .SetMethod("getMimeType", &DownloadItem::GetMimeType)
      .SetMethod("hasUserGesture", &DownloadItem::HasUserGesture)
      .SetMethod("getFilename", &DownloadItem::GetFilename)
      .SetMethod("getContentDisposition", &DownloadItem::GetContentDisposition)
      .SetMethod("setSavePath", &DownloadItem::SetSavePath);
}

// static
mate::Handle<DownloadItem> DownloadItem::Create(
    v8::Isolate* isolate, content::DownloadItem* item) {
  auto handle = mate::CreateHandle(isolate, new DownloadItem(item));
  g_wrap_download_item.Run(handle.ToV8());
  g_download_item_objects[item->GetId()] = make_linked_ptr(
      new v8::Global<v8::Value>(isolate, handle.ToV8()));
  return handle;
}

// static
void* DownloadItem::UserDataKey() {
  return &kDownloadItemSavePathKey;
}

void ClearWrapDownloadItem() {
  g_wrap_download_item.Reset();
}

void SetWrapDownloadItem(const WrapDownloadItemCallback& callback) {
  g_wrap_download_item = callback;

  // Cleanup the wrapper on exit.
  atom::AtomBrowserMainParts::Get()->RegisterDestructionCallback(
      base::Bind(ClearWrapDownloadItem));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("_setWrapDownloadItem", &atom::api::SetWrapDownloadItem);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_download_item, Initialize);
