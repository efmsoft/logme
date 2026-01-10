#include <stdio.h>

#include <mutex>
#include <string>
#include <fstream>
#include <vector>
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

    static bool ReadWholeFile(
  const char* filename
  , std::vector<unsigned char>& buffer
  , std::string& error
  , const char* what
)
{
  buffer.clear();

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

  if (Crypto.ERR_clear_error_ptr)
    Crypto.ERR_clear_error_ptr();

  if (!reader)
  {
    error = std::string("OpenSSL reader is not available for ") + what;
    return false;
  }

  std::vector<unsigned char> buffer;
  if (!ReadWholeFile(filename, buffer, error, what))
    return false;

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
    
    if (!EnsureCrypto(error))
      return false;

    *cert = nullptr;

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

    if (!EnsureCrypto(error))
      return false;

    *key = nullptr;

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
}
