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

  bool is_user_db_available() const { return is_user_db_available_; }

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

  // Import user certificate from DER encoded |data|.
  // Returns a net error code on failure.
  int ImportUserCert(const std::string& data);

  // Import CA certificates.
  // Tries to import all the certificates given.  The root will be trusted
  // according to |trust_bits|.  Any certificates that could not be imported
  // will be listed in |not_imported|.
  // |trust_bits| should be a bit field of TRUST* values from NSSCertDatabase.
  // Returns false if there is an internal error, otherwise true is returned and
  // |not_imported| should be checked for any certificates that were not
  // imported.
  bool ImportCACerts(const net::ScopedCERTCertificateList& certificates,
                     net::NSSCertDatabase::TrustBits trust_bits,
                     net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // Import server certificate.  The first cert should be the server cert.  Any
  // additional certs should be intermediate/CA certs and will be imported but
  // not given any trust.
  // Any certificates that could not be imported will be listed in
  // |not_imported|.
  // |trust_bits| can be set to explicitly trust or distrust the certificate, or
  // use TRUST_DEFAULT to inherit trust as normal.
  // Returns false if there is an internal error, otherwise true is returned and
  // |not_imported| should be checked for any certificates that were not
  // imported.
  bool ImportServerCert(
      const net::ScopedCERTCertificateList& certificates,
      net::NSSCertDatabase::TrustBits trust_bits,
      net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // Set trust values for certificate.
  // |trust_bits| should be a bit field of TRUST* values from NSSCertDatabase.
  // Returns true on success or false on failure.
  bool SetCertTrust(CERTCertificate* cert,
                    net::CertType type,
                    net::NSSCertDatabase::TrustBits trust_bits);

  // Delete the cert.  Returns true on success.  |cert| is still valid when this
  // function returns.
  bool Delete(CERTCertificate* cert);

 private:
  CertificateManagerModel(net::NSSCertDatabase* nss_cert_database,
                          bool is_user_db_available);

  // Methods used during initialization, see the comment at the top of the .cc
  // file for details.
  static void DidGetCertDBOnUIThread(net::NSSCertDatabase* cert_db,
                                     bool is_user_db_available,
                                     CreationCallback callback);
  static void DidGetCertDBOnIOThread(CreationCallback callback,
                                     net::NSSCertDatabase* cert_db);
  static void GetCertDBOnIOThread(CreationCallback callback);

  raw_ptr<net::NSSCertDatabase> cert_db_;
  // Whether the certificate database has a public slot associated with the
  // profile. If not set, importing certificates is not allowed with this model.
  bool is_user_db_available_;
};

#endif  // ELECTRON_SHELL_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
