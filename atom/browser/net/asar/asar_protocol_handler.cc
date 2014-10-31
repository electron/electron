// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_protocol_handler.h"

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "atom/common/asar/archive.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_error_job.h"
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

AsarProtocolHandler::~AsarProtocolHandler() {
}

Archive* AsarProtocolHandler::GetOrCreateAsarArchive(
    const base::FilePath& path) const {
  if (!archives_.contains(path)) {
    scoped_ptr<Archive> archive(new Archive(path));
    if (!archive->Init())
      return nullptr;

    archives_.set(path, archive.Pass());
  }

  return archives_.get(path);
}

net::URLRequestJob* AsarProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  base::FilePath full_path;
  net::FileURLToFilePath(request->url(), &full_path);

  // Create asar:// job when the path contains "xxx.asar/", otherwise treat the
  // URL request as file://.
  base::FilePath asar_path, relative_path;
  if (!GetAsarPath(full_path, &asar_path, &relative_path))
    return new net::URLRequestFileJob(request, network_delegate, full_path,
                                      file_task_runner_);

  Archive* archive = GetOrCreateAsarArchive(asar_path);
  if (!archive)
    return new net::URLRequestErrorJob(request, network_delegate,
                                       net::ERR_FILE_NOT_FOUND);

  return new URLRequestAsarJob(request, network_delegate, archive,
                               relative_path, file_task_runner_);
}

bool AsarProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace asar
