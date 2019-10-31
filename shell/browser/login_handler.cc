// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/login_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/task/post_task.h"
#include "base/values.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/net_converter.h"

using content::BrowserThread;

namespace electron {

namespace {

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request) {
  const net::UploadDataStream* upload_data = request->get_upload_for_testing();
  if (!upload_data)
    return;
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      upload_data->GetElementReaders();
  for (const auto& reader : *readers) {
    auto upload_data_dict = std::make_unique<base::DictionaryValue>();
    if (reader->AsBytesReader()) {
      const net::UploadBytesElementReader* bytes_reader =
          reader->AsBytesReader();
      auto bytes = std::make_unique<base::Value>(
          std::vector<char>(bytes_reader->bytes(),
                            bytes_reader->bytes() + bytes_reader->length()));
      upload_data_dict->Set("bytes", std::move(bytes));
    } else if (reader->AsFileReader()) {
      const net::UploadFileElementReader* file_reader = reader->AsFileReader();
      auto file_path = file_reader->path().AsUTF8Unsafe();
      upload_data_dict->SetKey("file", base::Value(file_path));
    }
    // else {
    //   const storage::UploadBlobElementReader* blob_reader =
    //       static_cast<storage::UploadBlobElementReader*>(reader.get());
    //   upload_data_dict->SetString("blobUUID", blob_reader->uuid());
    // }
    upload_data_list->Append(std::move(upload_data_dict));
  }
}

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request) {
  details->SetString("method", request->method());
  std::string url;
  if (!request->url_chain().empty())
    url = request->url().spec();
  details->SetKey("url", base::Value(url));
  details->SetString("referrer", request->referrer());
  auto list = std::make_unique<base::ListValue>();
  GetUploadData(list.get(), request);
  if (!list->empty())
    details->Set("uploadData", std::move(list));
  auto headers_value = std::make_unique<base::DictionaryValue>();
  for (net::HttpRequestHeaders::Iterator it(request->extra_request_headers());
       it.GetNext();) {
    headers_value->SetString(it.name(), it.value());
  }
  details->Set("headers", std::move(headers_value));
}

}  // namespace

LoginHandler::LoginHandler(net::URLRequest* request,
                           const net::AuthChallengeInfo& auth_info,
                           // net::NetworkDelegate::AuthCallback callback,
                           net::AuthCredentials* credentials)
    : credentials_(credentials),
      auth_info_(std::make_unique<net::AuthChallengeInfo>(auth_info)),
      // auth_callback_(std::move(callback)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::unique_ptr<base::DictionaryValue> request_details(
      new base::DictionaryValue);
  // TODO(zcbenz): Use the converters from net_converter.
  FillRequestDetails(request_details.get(), request);

  // TODO(deepak1556): fix with network service
  // tracking issue: #19602
  CHECK(false) << "fix with network service";
  // web_contents_getter_ =
  //     resource_request_info->GetWebContentsGetterForRequest();

  base::PostTask(
      FROM_HERE, {BrowserThread::UI},
      base::BindOnce(&Browser::RequestLogin, base::Unretained(Browser::Get()),
                     base::RetainedRef(this), std::move(request_details)));
}

LoginHandler::~LoginHandler() = default;

void LoginHandler::Login(const base::string16& username,
                         const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&LoginHandler::DoLogin, weak_factory_.GetWeakPtr(),
                     username, password));
}

void LoginHandler::CancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTask(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&LoginHandler::DoCancelAuth, weak_factory_.GetWeakPtr()));
}

void LoginHandler::NotifyRequestDestroyed() {
  // auth_callback_.Reset();
  credentials_ = nullptr;
  weak_factory_.InvalidateWeakPtrs();
}

content::WebContents* LoginHandler::GetWebContents() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO(deepak1556): fix with network service
  // tracking issue: #19602
  CHECK(false) << "fix with network service";
  return web_contents_getter_.Run();
}

void LoginHandler::DoCancelAuth() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  /*
  if (!auth_callback_.is_null())
    std::move(auth_callback_)
        .Run(net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_CANCEL_AUTH);
        */
}

void LoginHandler::DoLogin(const base::string16& username,
                           const base::string16& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  /*
  if (!auth_callback_.is_null()) {
    credentials_->Set(username, password);
    std::move(auth_callback_)
        .Run(net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_SET_AUTH);
  }
  */
}

}  // namespace electron
