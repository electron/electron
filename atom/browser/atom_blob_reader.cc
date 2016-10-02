// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_blob_reader.h"

#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "storage/browser/blob/blob_data_handle.h"
#include "storage/browser/blob/blob_reader.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/fileapi/file_system_context.h"

#include "atom/common/node_includes.h"

using content::BrowserThread;

namespace atom {

namespace {

void RunCallbackInUI(
    const AtomBlobReader::CompletionCallback& callback,
    char* blob_data,
    int size) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  if (blob_data) {
    v8::Local<v8::Value> buffer = node::Buffer::New(isolate,
        blob_data, static_cast<size_t>(size)).ToLocalChecked();
    callback.Run(buffer);
  } else {
    callback.Run(v8::Null(isolate));
  }
}

}  // namespace

AtomBlobReader::AtomBlobReader(
    content::ChromeBlobStorageContext* blob_context,
    storage::FileSystemContext* file_system_context)
    : blob_context_(blob_context),
      file_system_context_(file_system_context) {
}

AtomBlobReader::~AtomBlobReader() {
}

void AtomBlobReader::StartReading(
    const std::string& uuid,
    const AtomBlobReader::CompletionCallback& completion_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto blob_data_handle =
      blob_context_->context()->GetBlobDataFromUUID(uuid);
  auto callback = base::Bind(&RunCallbackInUI,
                             completion_callback);
  if (!blob_data_handle) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(callback, nullptr, 0));
    return;
  }

  auto blob_reader = blob_data_handle->CreateReader(
      file_system_context_.get(),
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE).get());
  BlobReadHelper* blob_read_helper =
      new BlobReadHelper(std::move(blob_reader), callback);
  blob_read_helper->Read();
}

AtomBlobReader::BlobReadHelper::BlobReadHelper(
    std::unique_ptr<storage::BlobReader> blob_reader,
    const BlobReadHelper::CompletionCallback& callback)
    : blob_reader_(std::move(blob_reader)),
      completion_callback_(callback) {
}

AtomBlobReader::BlobReadHelper::~BlobReadHelper() {
}

void AtomBlobReader::BlobReadHelper::Read() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  storage::BlobReader::Status size_status = blob_reader_->CalculateSize(
      base::Bind(&AtomBlobReader::BlobReadHelper::DidCalculateSize,
                 base::Unretained(this)));
  if (size_status != storage::BlobReader::Status::IO_PENDING)
    DidCalculateSize(net::OK);
}

void AtomBlobReader::BlobReadHelper::DidCalculateSize(int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (result != net::OK) {
    DidReadBlobData(nullptr, 0);
    return;
  }

  uint64_t total_size = blob_reader_->total_size();
  int bytes_read = 0;
  scoped_refptr<net::IOBuffer> blob_data =
      new net::IOBuffer(static_cast<size_t>(total_size));
  auto callback = base::Bind(&AtomBlobReader::BlobReadHelper::DidReadBlobData,
                             base::Unretained(this),
                             base::RetainedRef(blob_data));
  storage::BlobReader::Status read_status = blob_reader_->Read(
      blob_data.get(),
      total_size,
      &bytes_read,
      callback);
  if (read_status != storage::BlobReader::Status::IO_PENDING)
    callback.Run(bytes_read);
}

void AtomBlobReader::BlobReadHelper::DidReadBlobData(
    const scoped_refptr<net::IOBuffer>& blob_data,
    int size) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  char* data = new char[size];
  memcpy(data, blob_data->data(), size);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(completion_callback_, data, size));
  delete this;
}

}  // namespace atom
