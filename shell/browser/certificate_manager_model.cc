// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/certificate_manager_model.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/nss_util.h"
#include "crypto/nss_util_internal.h"
#include "net/base/net_errors.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace {

net::NSSCertDatabase* g_nss_cert_database = nullptr;

net::NSSCertDatabase* GetNSSCertDatabase(
    base::OnceCallback<void(net::NSSCertDatabase*)> callback) {
  // This initialization is not thread safe. This CHECK ensures that this code
  // is only run on a single thread.
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  if (!g_nss_cert_database) {
    // Linux has only a single persistent slot compared to ChromeOS's separate
    // public and private slot.
    // Redirect any slot usage to this persistent slot on Linux.
    crypto::EnsureNSSInit();
    g_nss_cert_database = new net::NSSCertDatabase(
        crypto::ScopedPK11Slot(PK11_GetInternalKeySlot()) /* public slot */,
        crypto::ScopedPK11Slot(PK11_GetInternalKeySlot()) /* private slot */);
  }
  return g_nss_cert_database;
}

}  // namespace

// CertificateManagerModel is created on the UI thread. It needs a
// NSSCertDatabase handle (and on ChromeOS it needs to get the TPM status) which
// needs to be done on the IO thread.
//
// The initialization flow is roughly:
//
//               UI thread                              IO Thread
//
//   CertificateManagerModel::Create
//                  \--------------------------------------v
//                                CertificateManagerModel::GetCertDBOnIOThread
//                                                         |
//                                                 GetNSSCertDatabase
//                                                         |
//                               CertificateManagerModel::DidGetCertDBOnIOThread
//                  v--------------------------------------/
// CertificateManagerModel::DidGetCertDBOnUIThread
//                  |
//     new CertificateManagerModel
//                  |
//               callback

// static
void CertificateManagerModel::Create(CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&CertificateManagerModel::GetCertDBOnIOThread,
                                std::move(callback)));
}

CertificateManagerModel::CertificateManagerModel(
    net::NSSCertDatabase* nss_cert_database)
    : cert_db_(nss_cert_database) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

CertificateManagerModel::~CertificateManagerModel() = default;

int CertificateManagerModel::ImportFromPKCS12(
    PK11SlotInfo* slot_info,
    const std::string& data,
    const std::u16string& password,
    bool is_extractable,
    net::ScopedCERTCertificateList* imported_certs) {
  return cert_db_->ImportFromPKCS12(slot_info, data, password, is_extractable,
                                    imported_certs);
}

bool CertificateManagerModel::SetCertTrust(
    CERTCertificate* cert,
    net::CertType type,
    net::NSSCertDatabase::TrustBits trust_bits) {
  return cert_db_->SetCertTrust(cert, type, trust_bits);
}

// static
void CertificateManagerModel::DidGetCertDBOnUIThread(
    net::NSSCertDatabase* cert_db,
    CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto model = base::WrapUnique(new CertificateManagerModel(cert_db));
  std::move(callback).Run(std::move(model));
}

// static
void CertificateManagerModel::DidGetCertDBOnIOThread(
    CreationCallback callback,
    net::NSSCertDatabase* cert_db) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&CertificateManagerModel::DidGetCertDBOnUIThread, cert_db,
                     std::move(callback)));
}

// static
void CertificateManagerModel::GetCertDBOnIOThread(CreationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto split_callback = base::SplitOnceCallback(base::BindOnce(
      &CertificateManagerModel::DidGetCertDBOnIOThread, std::move(callback)));

  net::NSSCertDatabase* cert_db =
      GetNSSCertDatabase(std::move(split_callback.first));

  // If the NSS database was already available, |cert_db| is non-null and
  // |did_get_cert_db_callback| has not been called. Call it explicitly.
  if (cert_db)
    std::move(split_callback.second).Run(cert_db);
}
