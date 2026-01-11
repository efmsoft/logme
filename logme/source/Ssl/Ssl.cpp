#include <stdio.h>

#include <mutex>
#include <string>
#include <fstream>
#include <vector>

#include <Logme/Ssl.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Logme
{
  namespace
  {
#if defined(_WIN32)
    using LibHandle = HMODULE;

    static LibHandle LoadLibraryAny(const char* const* names)
    {
      for (const char* const* p = names; *p != nullptr; ++p)
      {
        LibHandle h = ::LoadLibraryA(*p);
        if (h != nullptr)
          return h;
      }
      return nullptr;
    }

    static void* LoadSymbol(LibHandle lib, const char* name)
    {
      return reinterpret_cast<void*>(::GetProcAddress(lib, name));
    }

    static std::string LastDlError()
    {
      DWORD err = ::GetLastError();
      if (err == 0)
        return std::string();

      LPSTR msg = nullptr;
      DWORD len = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
        , nullptr
        , err
        , MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
        , reinterpret_cast<LPSTR>(&msg)
        , 0
        , nullptr
      );

      std::string out;
      if (len != 0 && msg != nullptr)
        out.assign(msg, msg + len);

      if (msg != nullptr)
        ::LocalFree(msg);

      return out;
    }
#else
    using LibHandle = void*;

    static LibHandle LoadLibraryAny(const char* const* names)
    {
      for (const char* const* p = names; *p != nullptr; ++p)
      {
        LibHandle h = ::dlopen(*p, RTLD_LAZY);
        if (h != nullptr)
          return h;
      }
      return nullptr;
    }

    static void* LoadSymbol(LibHandle lib, const char* name)
    {
      return ::dlsym(lib, name);
    }

    static std::string LastDlError()
    {
      const char* e = ::dlerror();
      if (e == nullptr)
        return std::string();
      return std::string(e);
    }
#endif

    struct CryptoApi
    {
      LibHandle LibCrypto = nullptr;

      void* (*PEM_read_X509_ptr)(FILE*, void*, void*, void*) = nullptr;
      void* (*PEM_read_PrivateKey_ptr)(FILE*, void*, void*, void*) = nullptr;


void* (*BIO_new_mem_buf_ptr)(const void*, int) = nullptr;
int (*BIO_free_ptr)(void*) = nullptr;
void* (*PEM_read_bio_X509_ptr)(void*, void*, void*, void*) = nullptr;
void* (*PEM_read_bio_PrivateKey_ptr)(void*, void*, void*, void*) = nullptr;

void* (*BIO_new_file_ptr)(const char*, const char*) = nullptr;
int (*PEM_write_bio_X509_ptr)(void*, void*) = nullptr;
int (*PEM_write_bio_PrivateKey_ptr)(
  void*
  , void*
  , const void*
  , unsigned char*
  , int
  , void*
  , void*
) = nullptr;

void* (*EVP_PKEY_new_ptr)() = nullptr;

void* (*EVP_PKEY_CTX_new_id_ptr)(int, void*) = nullptr;
void (*EVP_PKEY_CTX_free_ptr)(void*) = nullptr;
int (*EVP_PKEY_keygen_init_ptr)(void*) = nullptr;
int (*EVP_PKEY_CTX_set_rsa_keygen_bits_ptr)(void*, int) = nullptr;
int (*EVP_PKEY_keygen_ptr)(void*, void**) = nullptr;
void* (*RSA_new_ptr)() = nullptr;
int (*RSA_generate_key_ex_ptr)(void*, int, void*, void*) = nullptr;
void (*RSA_free_ptr)(void*) = nullptr;
void* (*BN_new_ptr)() = nullptr;
int (*BN_set_word_ptr)(void*, unsigned long) = nullptr;
void (*BN_free_ptr)(void*) = nullptr;
int (*EVP_PKEY_assign_ptr)(void*, int, void*) = nullptr;

void* (*X509_new_ptr)() = nullptr;
int (*X509_set_version_ptr)(void*, long) = nullptr;
void* (*X509_get_serialNumber_ptr)(void*) = nullptr;
int (*ASN1_INTEGER_set_ptr)(void*, long) = nullptr;
void* (*X509_gmtime_adj_ptr)(void*, long) = nullptr;
void* (*X509_get_notBefore_ptr)(void*) = nullptr;
void* (*X509_get_notAfter_ptr)(void*) = nullptr;
int (*X509_set_pubkey_ptr)(void*, void*) = nullptr;
void* (*X509_get_subject_name_ptr)(void*) = nullptr;
int (*X509_NAME_add_entry_by_txt_ptr)(
  void*
  , const char*
  , int
  , const unsigned char*
  , int
  , int
  , int
) = nullptr;
int (*X509_set_issuer_name_ptr)(void*, void*) = nullptr;
void* (*EVP_sha256_ptr)() = nullptr;
int (*X509_sign_ptr)(void*, void*, void*) = nullptr;
      void (*X509_free_ptr)(void*) = nullptr;
      void (*EVP_PKEY_free_ptr)(void*) = nullptr;

      unsigned long (*ERR_get_error_ptr)() = nullptr;
      void (*ERR_error_string_n_ptr)(unsigned long, char*, size_t) = nullptr;
      void (*ERR_clear_error_ptr)() = nullptr;

      bool Loaded() const
      {
        return LibCrypto != nullptr;
      }
    };

    static std::mutex CryptoLock;
    static CryptoApi Crypto;

    static std::string JoinTried(const char* const* names)
    {
      std::string out;
      for (const char* const* p = names; *p != nullptr; ++p)
      {
        if (!out.empty())
          out += ", ";
        out += *p;
      }
      return out;
    }

    static void AppendOpenSslError(std::string& error)
    {
      if (!Crypto.ERR_get_error_ptr || !Crypto.ERR_error_string_n_ptr)
        return;

      unsigned long code = Crypto.ERR_get_error_ptr();
      if (code == 0)
        return;

      char buf[256] = {0};
      Crypto.ERR_error_string_n_ptr(code, buf, sizeof(buf));
      if (buf[0] == 0)
        return;

      error += " (OpenSSL: ";
      error += buf;
      error += ")";
    }

    static bool EnsureCrypto(std::string& error)
    {
      std::lock_guard guard(CryptoLock);

      if (Crypto.Loaded())
        return true;

#if defined(_WIN32) && defined(_WIN64)
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto-3-x64.dll",
        "libcrypto-1_1-x64.dll",
        "libcrypto-1_1.dll",
        "libcrypto.dll",
        nullptr
      };
#elif defined(_WIN32)
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto-3.dll",
        "libcrypto-1_1.dll",
        "libcrypto.dll",
        nullptr
      };
#elif defined(__APPLE__)
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto.dylib",
        nullptr
      };
#else
      static const char* CRYPTO_NAMES[] =
      {
        "libcrypto.so.3",
        "libcrypto.so.1.1",
        "libcrypto.so",
        nullptr
      };
#endif

      Crypto.LibCrypto = LoadLibraryAny(CRYPTO_NAMES);
      if (!Crypto.LibCrypto)
      {
        error = "Unable to load libcrypto shared library. Tried: ";
        error += JoinTried(CRYPTO_NAMES);

        std::string dl = LastDlError();
        if (!dl.empty())
        {
          error += ". ";
          error += dl;
        }
        return false;
      }

      Crypto.PEM_read_X509_ptr = reinterpret_cast<void* (*)(FILE*, void*, void*, void*)>(
        LoadSymbol(Crypto.LibCrypto, "PEM_read_X509")
      );
      Crypto.PEM_read_PrivateKey_ptr = reinterpret_cast<void* (*)(FILE*, void*, void*, void*)>(
        LoadSymbol(Crypto.LibCrypto, "PEM_read_PrivateKey")
      );

      Crypto.BIO_new_mem_buf_ptr = reinterpret_cast<void* (*)(const void*, int)>(
        LoadSymbol(Crypto.LibCrypto, "BIO_new_mem_buf")
      );
      Crypto.BIO_free_ptr = reinterpret_cast<int (*)(void*)>(
        LoadSymbol(Crypto.LibCrypto, "BIO_free")
      );
      Crypto.PEM_read_bio_X509_ptr = reinterpret_cast<void* (*)(void*, void*, void*, void*)>(
        LoadSymbol(Crypto.LibCrypto, "PEM_read_bio_X509")
      );
      Crypto.PEM_read_bio_PrivateKey_ptr = reinterpret_cast<void* (*)(void*, void*, void*, void*)>(
        LoadSymbol(Crypto.LibCrypto, "PEM_read_bio_PrivateKey")
      );
      Crypto.X509_free_ptr = reinterpret_cast<void (*)(void*)>(
        LoadSymbol(Crypto.LibCrypto, "X509_free")
      );
      Crypto.EVP_PKEY_free_ptr = reinterpret_cast<void (*)(void*)>(
        LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_free")
      );

      Crypto.ERR_get_error_ptr = reinterpret_cast<unsigned long (*)()>(
        LoadSymbol(Crypto.LibCrypto, "ERR_get_error")
      );
      Crypto.ERR_error_string_n_ptr = reinterpret_cast<void (*)(unsigned long, char*, size_t)>(
        LoadSymbol(Crypto.LibCrypto, "ERR_error_string_n")
      );
      Crypto.ERR_clear_error_ptr = reinterpret_cast<void (*)()>(
        LoadSymbol(Crypto.LibCrypto, "ERR_clear_error")
      );

      if (!Crypto.BIO_new_mem_buf_ptr || !Crypto.BIO_free_ptr
        || !Crypto.PEM_read_bio_X509_ptr || !Crypto.PEM_read_bio_PrivateKey_ptr
        || !Crypto.X509_free_ptr || !Crypto.EVP_PKEY_free_ptr)
      {
        error = "Unable to resolve required OpenSSL symbols from libcrypto";
        return false;
      }

      return true;
    }

static bool EnsureCryptoGenerate(std::string& error)
{
  if (!EnsureCrypto(error))
    return false;

  if (!Crypto.BIO_new_file_ptr)
  {
    Crypto.BIO_new_file_ptr = reinterpret_cast<void* (*)(const char*, const char*)>(
      LoadSymbol(Crypto.LibCrypto, "BIO_new_file")
    );
    Crypto.PEM_write_bio_X509_ptr = reinterpret_cast<int (*)(void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "PEM_write_bio_X509")
    );
    Crypto.PEM_write_bio_PrivateKey_ptr = reinterpret_cast<int (*)(void*, void*, const void*, unsigned char*, int, void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "PEM_write_bio_PrivateKey")
    );

    Crypto.EVP_PKEY_new_ptr = reinterpret_cast<void* (*)()>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_new")
    );

    Crypto.EVP_PKEY_CTX_new_id_ptr = reinterpret_cast<void* (*)(int, void*)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_CTX_new_id")
    );
    Crypto.EVP_PKEY_CTX_free_ptr = reinterpret_cast<void (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_CTX_free")
    );
    Crypto.EVP_PKEY_keygen_init_ptr = reinterpret_cast<int (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_keygen_init")
    );
    Crypto.EVP_PKEY_CTX_set_rsa_keygen_bits_ptr = reinterpret_cast<int (*)(void*, int)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_CTX_set_rsa_keygen_bits")
    );
    Crypto.EVP_PKEY_keygen_ptr = reinterpret_cast<int (*)(void*, void**)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_keygen")
    );
    Crypto.RSA_new_ptr = reinterpret_cast<void* (*)()>(
      LoadSymbol(Crypto.LibCrypto, "RSA_new")
    );
    Crypto.RSA_generate_key_ex_ptr = reinterpret_cast<int (*)(void*, int, void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "RSA_generate_key_ex")
    );
    Crypto.RSA_free_ptr = reinterpret_cast<void (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "RSA_free")
    );
    Crypto.BN_new_ptr = reinterpret_cast<void* (*)()>(
      LoadSymbol(Crypto.LibCrypto, "BN_new")
    );
    Crypto.BN_set_word_ptr = reinterpret_cast<int (*)(void*, unsigned long)>(
      LoadSymbol(Crypto.LibCrypto, "BN_set_word")
    );
    Crypto.BN_free_ptr = reinterpret_cast<void (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "BN_free")
    );
    Crypto.EVP_PKEY_assign_ptr = reinterpret_cast<int (*)(void*, int, void*)>(
      LoadSymbol(Crypto.LibCrypto, "EVP_PKEY_assign")
    );

    Crypto.X509_new_ptr = reinterpret_cast<void* (*)()>(
      LoadSymbol(Crypto.LibCrypto, "X509_new")
    );
    Crypto.X509_set_version_ptr = reinterpret_cast<int (*)(void*, long)>(
      LoadSymbol(Crypto.LibCrypto, "X509_set_version")
    );
    Crypto.X509_get_serialNumber_ptr = reinterpret_cast<void* (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_get_serialNumber")
    );
    Crypto.ASN1_INTEGER_set_ptr = reinterpret_cast<int (*)(void*, long)>(
      LoadSymbol(Crypto.LibCrypto, "ASN1_INTEGER_set")
    );
    Crypto.X509_gmtime_adj_ptr = reinterpret_cast<void* (*)(void*, long)>(
      LoadSymbol(Crypto.LibCrypto, "X509_gmtime_adj")
    );
    Crypto.X509_get_notBefore_ptr = reinterpret_cast<void* (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_getm_notBefore")
    );
    if (!Crypto.X509_get_notBefore_ptr)
    {
      Crypto.X509_get_notBefore_ptr = reinterpret_cast<void* (*)(void*)>(
        LoadSymbol(Crypto.LibCrypto, "X509_get_notBefore")
      );
    }

    Crypto.X509_get_notAfter_ptr = reinterpret_cast<void* (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_getm_notAfter")
    );
    if (!Crypto.X509_get_notAfter_ptr)
    {
      Crypto.X509_get_notAfter_ptr = reinterpret_cast<void* (*)(void*)>(
        LoadSymbol(Crypto.LibCrypto, "X509_get_notAfter")
      );
    }
    Crypto.X509_set_pubkey_ptr = reinterpret_cast<int (*)(void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_set_pubkey")
    );
    Crypto.X509_get_subject_name_ptr = reinterpret_cast<void* (*)(void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_get_subject_name")
    );
    Crypto.X509_NAME_add_entry_by_txt_ptr = reinterpret_cast<int (*)(void*, const char*, int, const unsigned char*, int, int, int)>(
      LoadSymbol(Crypto.LibCrypto, "X509_NAME_add_entry_by_txt")
    );
    Crypto.X509_set_issuer_name_ptr = reinterpret_cast<int (*)(void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_set_issuer_name")
    );
    Crypto.EVP_sha256_ptr = reinterpret_cast<void* (*)()>(
      LoadSymbol(Crypto.LibCrypto, "EVP_sha256")
    );
    Crypto.X509_sign_ptr = reinterpret_cast<int (*)(void*, void*, void*)>(
      LoadSymbol(Crypto.LibCrypto, "X509_sign")
    );
  }

  if (!Crypto.BIO_new_file_ptr || !Crypto.PEM_write_bio_X509_ptr || !Crypto.PEM_write_bio_PrivateKey_ptr
        || !Crypto.X509_new_ptr || !Crypto.X509_set_version_ptr || !Crypto.X509_get_serialNumber_ptr
        || !Crypto.ASN1_INTEGER_set_ptr || !Crypto.X509_gmtime_adj_ptr
        || !Crypto.X509_get_notBefore_ptr || !Crypto.X509_get_notAfter_ptr
        || !Crypto.X509_set_pubkey_ptr || !Crypto.X509_get_subject_name_ptr
        || !Crypto.X509_NAME_add_entry_by_txt_ptr || !Crypto.X509_set_issuer_name_ptr
        || !Crypto.EVP_sha256_ptr || !Crypto.X509_sign_ptr)
  {
    error = "Unable to resolve required OpenSSL symbols for certificate generation";
    return false;
  }

  bool haveCtxKeygen = Crypto.EVP_PKEY_CTX_new_id_ptr && Crypto.EVP_PKEY_CTX_free_ptr
    && Crypto.EVP_PKEY_keygen_init_ptr && Crypto.EVP_PKEY_CTX_set_rsa_keygen_bits_ptr
    && Crypto.EVP_PKEY_keygen_ptr;

  bool haveRsaKeygen = Crypto.EVP_PKEY_new_ptr && Crypto.RSA_new_ptr && Crypto.RSA_generate_key_ex_ptr
    && Crypto.RSA_free_ptr && Crypto.BN_new_ptr && Crypto.BN_set_word_ptr && Crypto.BN_free_ptr
    && Crypto.EVP_PKEY_assign_ptr;

  if (!haveCtxKeygen && !haveRsaKeygen)
  {
    error = "Unable to resolve required OpenSSL symbols for certificate generation";
    return false;
  }

  return true;
}


    static bool ReadFileObject(
  const char* filename
  , void* (*reader)(void*, void*, void*, void*)
  , void** out
  , std::string& error
  , const char* what
)
{
  if (!filename || !*filename)
  {
    error = std::string(what) + " filename is not specified";
    return false;
  }

  if (!reader)
  {
    error = std::string("OpenSSL reader is not available for ") + what;
    return false;
  }

  std::vector<unsigned char> buffer;

  std::ifstream f(filename, std::ios::binary);
  if (!f)
  {
    error = std::string("Unable to open ") + what + " file: " + filename;
    return false;
  }

  f.seekg(0, std::ios::end);
  std::streamoff size = f.tellg();
  if (size <= 0)
  {
    error = std::string("Empty ") + what + " file: " + filename;
    return false;
  }

  f.seekg(0, std::ios::beg);
  buffer.resize(static_cast<size_t>(size));

  if (!f.read(reinterpret_cast<char*>(buffer.data()), size))
  {
    error = std::string("Unable to read ") + what + " file: " + filename;
    buffer.clear();
    return false;
  }

  void* bio = Crypto.BIO_new_mem_buf_ptr(buffer.data(), static_cast<int>(buffer.size()));
  if (!bio)
  {
    error = std::string("BIO_new_mem_buf failed for ") + what + ": " + filename;
    AppendOpenSslError(error);
    return false;
  }

  void* obj = reader(bio, nullptr, nullptr, nullptr);

  Crypto.BIO_free_ptr(bio);

  if (!obj)
  {
    error = std::string("Unable to read ") + what + " from file: " + filename;
    AppendOpenSslError(error);
    return false;
  }

  *out = obj;
  return true;
}

  }

  bool LoadSslCertificateFromFile(
    const char* filename
    , X509** cert
    , std::string& error
  )
  {
    if (!cert)
    {
      error = "Invalid argument: cert is null";
      return false;
    }

    *cert = nullptr;

    if (!EnsureCrypto(error))
      return false;

    void* obj = nullptr;
    if (!ReadFileObject(filename, Crypto.PEM_read_bio_X509_ptr, &obj, error, "certificate"))
      return false;

    *cert = reinterpret_cast<X509*>(obj);
    return true;
  }

  bool LoadSslPrivateKeyFromFile(
    const char* filename
    , EVP_PKEY** key
    , std::string& error
  )
  {
    if (!key)
    {
      error = "Invalid argument: key is null";
      return false;
    }

    *key = nullptr;

    if (!EnsureCrypto(error))
      return false;

    void* obj = nullptr;
    if (!ReadFileObject(filename, Crypto.PEM_read_bio_PrivateKey_ptr, &obj, error, "private key"))
      return false;

    *key = reinterpret_cast<EVP_PKEY*>(obj);
    return true;
  }

  void FreeSslCertificate(X509* cert)
  {
    if (!cert)
      return;

    std::string error;
    if (!EnsureCrypto(error))
      return;

    Crypto.X509_free_ptr(reinterpret_cast<void*>(cert));
  }

  void FreeSslPrivateKey(EVP_PKEY* key)
  {
    if (!key)
      return;

    std::string error;
    if (!EnsureCrypto(error))
      return;

    Crypto.EVP_PKEY_free_ptr(reinterpret_cast<void*>(key));
  }

bool GenerateSelfSignedCertificateFiles(
  const char* certFilename
  , const char* keyFilename
  , std::string& error
)
{
  if (!certFilename || !*certFilename || !keyFilename || !*keyFilename)
  {
    error = "Invalid argument: certFilename/keyFilename is empty";
    return false;
  }

  if (!EnsureCryptoGenerate(error))
    return false;

  void* pkey = nullptr;

  bool haveCtxKeygen = Crypto.EVP_PKEY_CTX_new_id_ptr && Crypto.EVP_PKEY_CTX_free_ptr
    && Crypto.EVP_PKEY_keygen_init_ptr && Crypto.EVP_PKEY_CTX_set_rsa_keygen_bits_ptr
    && Crypto.EVP_PKEY_keygen_ptr;

  if (haveCtxKeygen)
  {
    void* ctx = Crypto.EVP_PKEY_CTX_new_id_ptr(6, nullptr);
    if (!ctx)
    {
      error = "EVP_PKEY_CTX_new_id failed";
      AppendOpenSslError(error);
      return false;
    }

    if (Crypto.EVP_PKEY_keygen_init_ptr(ctx) <= 0)
    {
      Crypto.EVP_PKEY_CTX_free_ptr(ctx);
      error = "EVP_PKEY_keygen_init failed";
      AppendOpenSslError(error);
      return false;
    }

    if (Crypto.EVP_PKEY_CTX_set_rsa_keygen_bits_ptr(ctx, 2048) <= 0)
    {
      Crypto.EVP_PKEY_CTX_free_ptr(ctx);
      error = "EVP_PKEY_CTX_set_rsa_keygen_bits failed";
      AppendOpenSslError(error);
      return false;
    }

    if (Crypto.EVP_PKEY_keygen_ptr(ctx, &pkey) <= 0 || !pkey)
    {
      Crypto.EVP_PKEY_CTX_free_ptr(ctx);
      error = "EVP_PKEY_keygen failed";
      AppendOpenSslError(error);
      return false;
    }

    Crypto.EVP_PKEY_CTX_free_ptr(ctx);
  }
  else
  {
    if (!Crypto.EVP_PKEY_new_ptr || !Crypto.RSA_new_ptr || !Crypto.BN_new_ptr
      || !Crypto.BN_set_word_ptr || !Crypto.RSA_generate_key_ex_ptr
      || !Crypto.BN_free_ptr || !Crypto.RSA_free_ptr || !Crypto.EVP_PKEY_assign_ptr)
    {
      error = "Unable to resolve required OpenSSL symbols for certificate generation";
      return false;
    }

    pkey = Crypto.EVP_PKEY_new_ptr();
    if (!pkey)
    {
      error = "EVP_PKEY_new failed";
      AppendOpenSslError(error);
      return false;
    }

    void* rsa = Crypto.RSA_new_ptr();
    if (!rsa)
    {
      Crypto.EVP_PKEY_free_ptr(pkey);
      error = "RSA_new failed";
      AppendOpenSslError(error);
      return false;
    }

    void* e = Crypto.BN_new_ptr();
    if (!e)
    {
      Crypto.RSA_free_ptr(rsa);
      Crypto.EVP_PKEY_free_ptr(pkey);
      error = "BN_new failed";
      AppendOpenSslError(error);
      return false;
    }

    if (Crypto.BN_set_word_ptr(e, 65537UL) <= 0)
    {
      Crypto.BN_free_ptr(e);
      Crypto.RSA_free_ptr(rsa);
      Crypto.EVP_PKEY_free_ptr(pkey);
      error = "BN_set_word failed";
      AppendOpenSslError(error);
      return false;
    }

    if (Crypto.RSA_generate_key_ex_ptr(rsa, 2048, e, nullptr) <= 0)
    {
      Crypto.BN_free_ptr(e);
      Crypto.RSA_free_ptr(rsa);
      Crypto.EVP_PKEY_free_ptr(pkey);
      error = "RSA_generate_key_ex failed";
      AppendOpenSslError(error);
      return false;
    }

    Crypto.BN_free_ptr(e);

    if (Crypto.EVP_PKEY_assign_ptr(pkey, 6, rsa) <= 0)
    {
      Crypto.RSA_free_ptr(rsa);
      Crypto.EVP_PKEY_free_ptr(pkey);
      pkey = nullptr;
      error = "EVP_PKEY_assign failed";
      AppendOpenSslError(error);
      return false;
    }
  }

  void* x509 = Crypto.X509_new_ptr();


  if (!x509)
  {
    Crypto.EVP_PKEY_free_ptr(pkey);
    error = "X509_new failed";
    AppendOpenSslError(error);
    return false;
  }

  Crypto.X509_set_version_ptr(x509, 2);

  void* serial = Crypto.X509_get_serialNumber_ptr(x509);
  Crypto.ASN1_INTEGER_set_ptr(serial, 1);

  Crypto.X509_gmtime_adj_ptr(Crypto.X509_get_notBefore_ptr(x509), 0);
  Crypto.X509_gmtime_adj_ptr(Crypto.X509_get_notAfter_ptr(x509), 60L * 60L * 24L * 365L * 10L);

  Crypto.X509_set_pubkey_ptr(x509, pkey);

  void* name = Crypto.X509_get_subject_name_ptr(x509);
  // V_ASN1_UTF8STRING = 12
  Crypto.X509_NAME_add_entry_by_txt_ptr(
    name
    , "CN"
    , 12
    , reinterpret_cast<const unsigned char*>("logme-demo")
    , -1
    , -1
    , 0
  );

  Crypto.X509_set_issuer_name_ptr(x509, name);

  if (!Crypto.X509_sign_ptr(x509, pkey, Crypto.EVP_sha256_ptr()))
  {
    Crypto.X509_free_ptr(x509);
    Crypto.EVP_PKEY_free_ptr(pkey);
    error = "X509_sign failed";
    AppendOpenSslError(error);
    return false;
  }

  void* keyBio = Crypto.BIO_new_file_ptr(keyFilename, "wb");
  if (!keyBio)
  {
    Crypto.X509_free_ptr(x509);
    Crypto.EVP_PKEY_free_ptr(pkey);
    error = std::string("BIO_new_file failed for key: ") + keyFilename;
    AppendOpenSslError(error);
    return false;
  }

  int keyOk = Crypto.PEM_write_bio_PrivateKey_ptr(
    keyBio
    , pkey
    , nullptr
    , nullptr
    , 0
    , nullptr
    , nullptr
  );

  Crypto.BIO_free_ptr(keyBio);

  if (!keyOk)
  {
    Crypto.X509_free_ptr(x509);
    Crypto.EVP_PKEY_free_ptr(pkey);
    error = std::string("PEM_write_bio_PrivateKey failed: ") + keyFilename;
    AppendOpenSslError(error);
    return false;
  }

  void* certBio = Crypto.BIO_new_file_ptr(certFilename, "wb");
  if (!certBio)
  {
    Crypto.X509_free_ptr(x509);
    Crypto.EVP_PKEY_free_ptr(pkey);
    error = std::string("BIO_new_file failed for cert: ") + certFilename;
    AppendOpenSslError(error);
    return false;
  }

  int certOk = Crypto.PEM_write_bio_X509_ptr(certBio, x509);

  Crypto.BIO_free_ptr(certBio);

  Crypto.X509_free_ptr(x509);
  Crypto.EVP_PKEY_free_ptr(pkey);

  if (!certOk)
  {
    error = std::string("PEM_write_bio_X509 failed: ") + certFilename;
    AppendOpenSslError(error);
    return false;
  }

  return true;
}

}
