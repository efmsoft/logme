#pragma once

#include <string>

#include <Logme/Types.h>

namespace Logme
{
  // Loads OpenSSL dynamically (libcrypto) and reads an X509 certificate from a PEM file.
  // On success, *cert is set and must be freed with FreeSslCertificate().
  LOGMELNK bool LoadSslCertificateFromFile(
    const char* filename
    , X509** cert
    , std::string& error
  );

  // Loads OpenSSL dynamically (libcrypto) and reads a private key from a PEM file.
  // On success, *key is set and must be freed with FreeSslPrivateKey().
  LOGMELNK bool LoadSslPrivateKeyFromFile(
    const char* filename
    , EVP_PKEY** key
    , std::string& error
  );

  // Frees objects returned from LoadSslCertificateFromFile / LoadSslPrivateKeyFromFile.
  // It is safe to call these functions with nullptr.
  LOGMELNK void FreeSslCertificate(X509* cert);
  LOGMELNK void FreeSslPrivateKey(EVP_PKEY* key);
}
