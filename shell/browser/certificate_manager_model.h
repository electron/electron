// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
#define ELECTRON_SHELL_BROWSER_CERTIFICATE_MANAGER_MODEL_H_

#include <memory>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "net/cert/nss_cert_database.h"

// CertificateManagerModel provides the data to be displayed in the certificate
// manager dialog, and processes changes from the view.
class CertificateManagerModel {
 public:
  using CreationCallback =
      base::OnceCallback<void(std::unique_ptr<CertificateManagerModel>)>;

  // Creates a CertificateManagerModel. The model will be passed to the callback
  // when it is ready.
  static void Create(CreationCallback callback);

  // disable copy
  CertificateManagerModel(const CertificateManagerModel&) = delete;
  CertificateManagerModel& operator=(const CertificateManagerModel&) = delete;

  ~CertificateManagerModel();

  // Accessor for read-only access to the underlying NSSCertDatabase.
  const net::NSSCertDatabase* cert_db() const { return cert_db_; }

  // Import private keys and certificates from PKCS #12 encoded
  // |data|, using the given |password|. If |is_extractable| is false,
  // mark the private key as unextractable from the module.
  // Returns a net error code on failure.
  int ImportFromPKCS12(PK11SlotInfo* slot_info,
                       const std::string& data,
                       const std::u16string& password,
                       bool is_extractable,
                       net::ScopedCERTCertificateList* imported_certs);

  // Set trust values for certificate.
  // |trust_bits| should be a bit field of TRUST* values from NSSCertDatabase.
  // Returns true on success or false on failure.
  bool SetCertTrust(CERTCertificate* cert,
                    net::CertType type,
                    net::NSSCertDatabase::TrustBits trust_bits);

 private:
  explicit CertificateManagerModel(net::NSSCertDatabase* nss_cert_database);

  // Methods used during initialization, see the comment at the top of the .cc
  // file for details.
  static void DidGetCertDBOnUIThread(net::NSSCertDatabase* cert_db,
                                     CreationCallback callback);
  static void DidGetCertDBOnIOThread(CreationCallback callback,
                                     net::NSSCertDatabase* cert_db);
  static void GetCertDBOnIOThread(CreationCallback callback);

  raw_ptr<net::NSSCertDatabase> cert_db_;
};

#endif  // ELECTRON_SHELL_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
