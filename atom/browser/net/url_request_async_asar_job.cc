// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_async_asar_job.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace atom {

namespace {

void BeforeStartInUI(base::WeakPtr<URLRequestAsyncAsarJob> job,
                     mate::Arguments* args) {
  v8::Local<v8::Value> value;
  int error = net::OK;
  std::unique_ptr<base::Value> request_options = nullptr;

  if (args->GetNext(&value)) {
    V8ValueConverter converter;
    v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
    request_options = converter.FromV8Value(value, context);
  }

  if (request_options) {
    JsAsker::IsErrorOptions(request_options.get(), &error);
  } else {
    error = net::ERR_NOT_IMPLEMENTED;
  }

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&URLRequestAsyncAsarJob::StartAsync, job,
                     std::move(request_options), error));
}

}  // namespace

URLRequestAsyncAsarJob::URLRequestAsyncAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : asar::URLRequestAsarJob(request, network_delegate), weak_factory_(this) {}

URLRequestAsyncAsarJob::~URLRequestAsyncAsarJob() = default;

void URLRequestAsyncAsarJob::Start() {
  auto request_details = std::make_unique<base::DictionaryValue>();
  FillRequestDetails(request_details.get(), request());
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&JsAsker::AskForOptions, base::Unretained(isolate()),
                     handler(), std::move(request_details),
                     base::Bind(&BeforeStartInUI, weak_factory_.GetWeakPtr())));
}

void URLRequestAsyncAsarJob::StartAsync(std::unique_ptr<base::Value> options,
                                        int error) {
  if (error != net::OK) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
    return;
  }

  std::string file_path;
  response_headers_ = new net::HttpResponseHeaders("HTTP/1.1 200 OK");
  if (options->is_dict()) {
    base::DictionaryValue* dict =
        static_cast<base::DictionaryValue*>(options.get());
    base::Value* pathValue =
        dict->FindKeyOfType("path", base::Value::Type::STRING);
    if (pathValue) {
      file_path = pathValue->GetString();
    }
    base::Value* headersValue =
        dict->FindKeyOfType("headers", base::Value::Type::DICTIONARY);
    if (headersValue) {
      for (const auto& iter : headersValue->DictItems()) {
        response_headers_->AddHeader(iter.first + ": " +
                                     iter.second.GetString());
      }
    }
  } else if (options->is_string()) {
    file_path = options->GetString();
  }
  response_headers_->AddHeader(kCORSHeader);

  if (file_path.empty()) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_NOT_IMPLEMENTED));
  } else {
    asar::URLRequestAsarJob::Initialize(
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
#if defined(OS_WIN)
        base::FilePath(base::UTF8ToWide(file_path)));
#else
        base::FilePath(file_path));
#endif
    asar::URLRequestAsarJob::Start();
  }
}

void URLRequestAsyncAsarJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  URLRequestAsarJob::Kill();
}

void URLRequestAsyncAsarJob::GetResponseInfo(net::HttpResponseInfo* info) {
  info->headers = response_headers_;
}

}  // namespace atom
