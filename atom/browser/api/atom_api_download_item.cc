// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_download_item.h"

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

namespace {
// The wrapDownloadItem funtion which is implemented in JavaScript
using WrapDownloadItemCallback = base::Callback<void(v8::Local<v8::Value>)>;
WrapDownloadItemCallback g_wrap_download_item;
}  // namespace

DownloadItem::DownloadItem(content::DownloadItem* download_item) :
    download_item_(download_item) {
  download_item_->AddObserver(this);
}

DownloadItem::~DownloadItem() {
  download_item_->RemoveObserver(this);
}

void DownloadItem::OnDownloadUpdated(content::DownloadItem* item) {
  if (download_item_ == item)
    download_item_->IsDone() ? Emit("completed") : Emit("updated");
}

void DownloadItem::OnDownloadDestroyed(content::DownloadItem* download) {
  if (download_item_ == download) {
    download_item_->RemoveObserver(this);
  }
}

bool DownloadItem::IsDestroyed() const {
  return download_item_ == nullptr;
}

void DownloadItem::Destroy() {
  download_item_ = nullptr;
}

int64 DownloadItem::GetReceivedBytes() {
  return download_item_->GetReceivedBytes();
}

int64 DownloadItem::GetTotalBytes() {
  return download_item_->GetTotalBytes();
}

const GURL& DownloadItem::GetURL() {
  return download_item_->GetURL();
}

std::string DownloadItem::GetMimeType() {
  return download_item_->GetMimeType();
}

bool DownloadItem::HasUserGesture() {
  return download_item_->HasUserGesture();
}

std::string DownloadItem::GetSuggestedFilename() {
  return download_item_->GetSuggestedFilename();
}

std::string DownloadItem::GetContentDisposition() {
  return download_item_->GetContentDisposition();
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
      .SetMethod("getReceiveBytes", &DownloadItem::GetReceivedBytes)
      .SetMethod("getTotalBytes", &DownloadItem::GetTotalBytes)
      .SetMethod("getURL", &DownloadItem::GetURL)
      .SetMethod("getMimeType", &DownloadItem::GetMimeType)
      .SetMethod("hasUserGesture", &DownloadItem::HasUserGesture)
      .SetMethod("getSuggestedFilename", &DownloadItem::GetSuggestedFilename)
      .SetMethod("getContentDisposition", &DownloadItem::GetContentDisposition);
}

void SetWrapDownloadItem(const WrapDownloadItemCallback& callback) {
  g_wrap_download_item = callback;
}

void ClearWrapDownloadItem() {
  g_wrap_download_item.Reset();
}

mate::Handle<DownloadItem> DownloadItem::Create(
    v8::Isolate* isolate, content::DownloadItem* item) {
  auto handle = mate::CreateHandle(isolate, new DownloadItem(item));
  g_wrap_download_item.Run(handle.ToV8());
  return handle;
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("_setWrapDownloadItem", &atom::api::SetWrapDownloadItem);
  dict.SetMethod("_clearWrapDownloadItem", &atom::api::ClearWrapDownloadItem);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_download_item, Initialize);
