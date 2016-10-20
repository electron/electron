// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BLOB_READER_H_
#define ATOM_BROWSER_ATOM_BLOB_READER_H_

#include <string>

#include "base/callback.h"

namespace content {
class ChromeBlobStorageContext;
}

namespace net {
class IOBuffer;
}

namespace storage {
class BlobDataHandle;
class BlobReader;
class FileSystemContext;
}

namespace v8 {
template <class T>
class Local;
class Value;
}

namespace atom {

// A class to keep track of the blob context. All methods,
// except Ctor are expected to be called on IO thread.
class AtomBlobReader {
 public:
  using CompletionCallback = base::Callback<void(v8::Local<v8::Value>)>;

  AtomBlobReader(content::ChromeBlobStorageContext* blob_context,
                 storage::FileSystemContext* file_system_context);
  ~AtomBlobReader();

  void StartReading(
      const std::string& uuid,
      const AtomBlobReader::CompletionCallback& callback);

 private:
  // A self-destroyed helper class to read the blob data.
  // Must be accessed on IO thread.
  class BlobReadHelper {
   public:
    using CompletionCallback = base::Callback<void(char*, int)>;

    BlobReadHelper(std::unique_ptr<storage::BlobReader> blob_reader,
                   const BlobReadHelper::CompletionCallback& callback);
    ~BlobReadHelper();

    void Read();

   private:
    void DidCalculateSize(int result);
    void DidReadBlobData(const scoped_refptr<net::IOBuffer>& blob_data,
                         int bytes_read);

    std::unique_ptr<storage::BlobReader> blob_reader_;
    BlobReadHelper::CompletionCallback completion_callback_;

    DISALLOW_COPY_AND_ASSIGN(BlobReadHelper);
  };

  scoped_refptr<content::ChromeBlobStorageContext> blob_context_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;

  DISALLOW_COPY_AND_ASSIGN(AtomBlobReader);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BLOB_READER_H_
