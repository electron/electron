// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_
#define ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_

#include <memory>
#include <string>

#include "atom/browser/net/js_asker.h"
#include "atom/common/asar/archive.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request_job.h"

namespace base {
class TaskRunner;
}

namespace net {
class FileStream;
}

namespace asar {

// Createa a request job according to the file path.
net::URLRequestJob* CreateJobFromPath(
    const base::FilePath& full_path,
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const scoped_refptr<base::TaskRunner> file_task_runner);

class URLRequestAsarJob : public net::URLRequestJob {
 public:
  URLRequestAsarJob(net::URLRequest* request,
                    net::NetworkDelegate* network_delegate);

  void Initialize(const scoped_refptr<base::TaskRunner> file_task_runner,
                  const base::FilePath& file_path);

 protected:
  ~URLRequestAsarJob() override;

  void InitializeAsarJob(const scoped_refptr<base::TaskRunner> file_task_runner,
                         std::shared_ptr<Archive> archive,
                         const base::FilePath& file_path,
                         const Archive::FileInfo& file_info);
  void InitializeFileJob(const scoped_refptr<base::TaskRunner> file_task_runner,
                         const base::FilePath& file_path);

  // net::URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool IsRedirectResponse(GURL* location,
                          int* http_status_code,
                          bool* insecure_scheme_was_upgraded) override;
  std::unique_ptr<net::SourceStream> SetUpSourceStream() override;
  bool GetMimeType(std::string* mime_type) const override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  int GetResponseCode() const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;

 private:
  // The type of this job.
  enum JobType {
    TYPE_ERROR,
    TYPE_ASAR,
    TYPE_FILE,
  };

  // Meta information about the file. It's used as a member in the
  // URLRequestFileJob and also passed between threads because disk access is
  // necessary to obtain it.
  struct FileMetaInfo {
    // Size of the file.
    int64_t file_size = 0;
    // Mime type associated with the file.
    std::string mime_type;
    // Result returned from GetMimeTypeFromFile(), i.e. flag showing whether
    // obtaining of the mime type was successful.
    bool mime_type_result = false;
    // Flag showing whether the file exists.
    bool file_exists = false;
    // Flag showing whether the file name actually refers to a directory.
    bool is_directory = false;
    // Path to the file.
    base::FilePath file_path;

    FileMetaInfo();
  };

  // Fetches file info on a background thread.
  static void FetchMetaInfo(const base::FilePath& file_path,
                            JobType type,
                            FileMetaInfo* meta_info);

  // Callback after fetching file info on a background thread.
  void DidFetchMetaInfo(const FileMetaInfo* meta_info);

  // Callback after opening file on a background thread.
  void DidOpen(int result);

  // Callback after seeking to the beginning of |byte_range_| in the file
  // on a background thread.
  void DidSeek(int64_t result);

  // Callback after data is asynchronously read from the file into |buf|.
  void DidRead(scoped_refptr<net::IOBuffer> buf, int result);

  JobType type_ = TYPE_ERROR;

  std::shared_ptr<Archive> archive_;
  base::FilePath file_path_;
  Archive::FileInfo file_info_;

  std::unique_ptr<net::FileStream> stream_;
  FileMetaInfo meta_info_;
  scoped_refptr<base::TaskRunner> file_task_runner_;

  net::HttpByteRange byte_range_;
  int64_t remaining_bytes_ = 0;
  int64_t seek_offset_ = 0;

  net::Error range_parse_result_ = net::OK;

  base::WeakPtrFactory<URLRequestAsarJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestAsarJob);
};

}  // namespace asar

#endif  // ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_
