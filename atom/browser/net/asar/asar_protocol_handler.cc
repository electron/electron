// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_protocol_handler.h"

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "net/base/filename_util.h"
#include "net/url_request/url_request_file_job.h"

namespace asar {

namespace {

const base::FilePath::CharType kAsarExtension[] = FILE_PATH_LITERAL(".asar");

// Get the relative path in asar archive.
bool GetAsarPath(const base::FilePath& full_path,
                 base::FilePath* asar_path,
                 base::FilePath* relative_path) {
  base::FilePath iter = full_path;
  while (true) {
    base::FilePath dirname = iter.DirName();
    if (iter.MatchesExtension(kAsarExtension))
      break;
    else if (iter == dirname)
      return false;
    iter = dirname;
  }

  base::FilePath tail;
  if (!iter.AppendRelativePath(full_path, &tail))
    return false;

  *asar_path = iter;
  *relative_path = tail;
  return true;
}

}  // namespace

AsarProtocolHandler::AsarProtocolHandler(
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : file_task_runner_(file_task_runner) {}

AsarProtocolHandler::~AsarProtocolHandler() {}

net::URLRequestJob* AsarProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  base::FilePath full_path;
  net::FileURLToFilePath(request->url(), &full_path);

  // Create asar:// job when the path contains "xxx.asar/".
  base::FilePath asar_path, relative_path;
  if (GetAsarPath(full_path, &asar_path, &relative_path))
    return new URLRequestAsarJob(request, network_delegate, asar_path,
                                 relative_path, file_task_runner_);
  else
    return new net::URLRequestFileJob(request, network_delegate, full_path,
                                      file_task_runner_);
}

bool AsarProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace asar
