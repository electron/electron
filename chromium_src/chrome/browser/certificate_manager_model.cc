// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/certificate_manager_model.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "crypto/nss_util.h"
#include "crypto/nss_util_internal.h"
#include "net/base/crypto_module.h"
#include "net/base/net_errors.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/x509_certificate.h"

using content::BrowserThread;

namespace {

net::NSSCertDatabase* g_nss_cert_database = nullptr;

net::NSSCertDatabase* GetNSSCertDatabaseForResourceContext(
    content::ResourceContext* context,
    const base::Callback<void(net::NSSCertDatabase*)>& callback) {
  // This initialization is not thread safe. This CHECK ensures that this code
  // is only run on a single thread.
  CHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  if (!g_nss_cert_database) {
    // Linux has only a single persistent slot compared to ChromeOS's separate
    // public and private slot.
    // Redirect any slot usage to this persistent slot on Linux.
    g_nss_cert_database = new net::NSSCertDatabase(
        crypto::ScopedPK11Slot(
            crypto::GetPersistentNSSKeySlot()) /* public slot */,
        crypto::ScopedPK11Slot(
            crypto::GetPersistentNSSKeySlot()) /* private slot */);
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
//                                     GetNSSCertDatabaseForResourceContext
//                                                         |
//                               CertificateManagerModel::DidGetCertDBOnIOThread
//                  v--------------------------------------/
// CertificateManagerModel::DidGetCertDBOnUIThread
//                  |
//     new CertificateManagerModel
//                  |
//               callback

// static
void CertificateManagerModel::Create(
    content::BrowserContext* browser_context,
    const CreationCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&CertificateManagerModel::GetCertDBOnIOThread,
                 browser_context->GetResourceContext(),
                 callback));
}

CertificateManagerModel::CertificateManagerModel(
    net::NSSCertDatabase* nss_cert_database,
    bool is_user_db_available)
    : cert_db_(nss_cert_database),
      is_user_db_available_(is_user_db_available) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

CertificateManagerModel::~CertificateManagerModel() {
}

int CertificateManagerModel::ImportFromPKCS12(net::CryptoModule* module,
                                              const std::string& data,
                                              const base::string16& password,
                                              bool is_extractable,
                                              net::CertificateList* imported_certs) {
  return cert_db_->ImportFromPKCS12(module, data, password,
                                    is_extractable, imported_certs);
}

int CertificateManagerModel::ImportUserCert(const std::string& data) {
  return cert_db_->ImportUserCert(data);
}

bool CertificateManagerModel::ImportCACerts(
    const net::CertificateList& certificates,
    net::NSSCertDatabase::TrustBits trust_bits,
    net::NSSCertDatabase::ImportCertFailureList* not_imported) {
  return cert_db_->ImportCACerts(certificates, trust_bits, not_imported);
}

bool CertificateManagerModel::ImportServerCert(
    const net::CertificateList& certificates,
    net::NSSCertDatabase::TrustBits trust_bits,
    net::NSSCertDatabase::ImportCertFailureList* not_imported) {
  return cert_db_->ImportServerCert(certificates, trust_bits,
                                    not_imported);
}

bool CertificateManagerModel::SetCertTrust(
    const net::X509Certificate* cert,
    net::CertType type,
    net::NSSCertDatabase::TrustBits trust_bits) {
  return cert_db_->SetCertTrust(cert, type, trust_bits);
}

bool CertificateManagerModel::Delete(net::X509Certificate* cert) {
  return cert_db_->DeleteCertAndKey(cert);
}

// static
void CertificateManagerModel::DidGetCertDBOnUIThread(
    net::NSSCertDatabase* cert_db,
    bool is_user_db_available,
    const CreationCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  scoped_ptr<CertificateManagerModel> model(new CertificateManagerModel(
      cert_db, is_user_db_available));
  callback.Run(std::move(model));
}

// static
void CertificateManagerModel::DidGetCertDBOnIOThread(
    const CreationCallback& callback,
    net::NSSCertDatabase* cert_db) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  bool is_user_db_available = !!cert_db->GetPublicSlot();
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&CertificateManagerModel::DidGetCertDBOnUIThread,
                 cert_db,
                 is_user_db_available,
                 callback));
}

// static
void CertificateManagerModel::GetCertDBOnIOThread(
    content::ResourceContext* context,
    const CreationCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  net::NSSCertDatabase* cert_db = GetNSSCertDatabaseForResourceContext(
      context,
      base::Bind(&CertificateManagerModel::DidGetCertDBOnIOThread,
                 callback));
  if (cert_db)
    DidGetCertDBOnIOThread(callback, cert_db);
}
