// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_DEVTOOLS_FILE_SYSTEM_INDEXER_H_
#define BROWSER_DEVTOOLS_FILE_SYSTEM_INDEXER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file_proxy.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace base {
class FilePath;
class FileEnumerator;
class Time;
}

namespace content {
class WebContents;
}

namespace brightray {

class DevToolsFileSystemIndexer
    : public base::RefCountedThreadSafe<DevToolsFileSystemIndexer> {
 public:
  typedef base::Callback<void(int)> TotalWorkCallback;
  typedef base::Callback<void(int)> WorkedCallback;
  typedef base::Callback<void()> DoneCallback;
  typedef base::Callback<void(const std::vector<std::string>&)> SearchCallback;

  class FileSystemIndexingJob : public base::RefCounted<FileSystemIndexingJob> {
   public:
    void Stop();

   private:
    friend class base::RefCounted<FileSystemIndexingJob>;
    friend class DevToolsFileSystemIndexer;
    FileSystemIndexingJob(const base::FilePath& file_system_path,
                          const TotalWorkCallback& total_work_callback,
                          const WorkedCallback& worked_callback,
                          const DoneCallback& done_callback);
    virtual ~FileSystemIndexingJob();

    void Start();
    void StopOnFileThread();
    void CollectFilesToIndex();
    void IndexFiles();
    void StartFileIndexing(base::File::Error error);
    void ReadFromFile();
    void OnRead(base::File::Error error,
                const char* data,
                int bytes_read);
    void FinishFileIndexing(bool success);
    void CloseFile();
    void CloseCallback(base::File::Error error);
    void ReportWorked();

    base::FilePath file_system_path_;
    TotalWorkCallback total_work_callback_;
    WorkedCallback worked_callback_;
    DoneCallback done_callback_;
    std::unique_ptr<base::FileEnumerator> file_enumerator_;
    typedef std::map<base::FilePath, base::Time> FilePathTimesMap;
    FilePathTimesMap file_path_times_;
    FilePathTimesMap::const_iterator indexing_it_;
    base::FileProxy current_file_;
    int64_t current_file_offset_;
    typedef int32_t Trigram;
    std::vector<Trigram> current_trigrams_;
    // The index in this vector is the trigram id.
    std::vector<bool> current_trigrams_set_;
    base::TimeTicks last_worked_notification_time_;
    int files_indexed_;
    bool stopped_;
  };

  DevToolsFileSystemIndexer();

  // Performs file system indexing for given |file_system_path| and sends
  // progress callbacks.
  scoped_refptr<FileSystemIndexingJob> IndexPath(
      const std::string& file_system_path,
      const TotalWorkCallback& total_work_callback,
      const WorkedCallback& worked_callback,
      const DoneCallback& done_callback);

  // Performs trigram search for given |query| in |file_system_path|.
  void SearchInPath(const std::string& file_system_path,
                    const std::string& query,
                    const SearchCallback& callback);

 private:
  friend class base::RefCountedThreadSafe<DevToolsFileSystemIndexer>;

  virtual ~DevToolsFileSystemIndexer();

  void SearchInPathOnFileThread(const std::string& file_system_path,
                                const std::string& query,
                                const SearchCallback& callback);

  DISALLOW_COPY_AND_ASSIGN(DevToolsFileSystemIndexer);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_FILE_SYSTEM_INDEXER_H_
