#include <string.h>
#include <cstring>
#include <time.h>

#include <Logme/Obfuscate.h>
#include <Logme/Logger.h>
#include <Logme/Logme.h>


static const char* ErrnoText(int e)
{
  const char* s = std::strerror(e);
  return (s != NULL) ? s : "unknown";
}

static std::string PathToUtf8String(const std::filesystem::path& path)
{
  std::u8string u8 = path.u8string();
  return std::string(
    reinterpret_cast<const char*>(u8.data())
    , u8.size()
  );
}


static void StoreLe16(uint8_t* p, uint16_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static uint16_t LoadLe16(const uint8_t* p)
{
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void StoreLe32(uint8_t* p, uint32_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
}

static uint32_t LoadLe32(const uint8_t* p)
{
  return
    ((uint32_t)p[0])
    | ((uint32_t)p[1] << 8)
    | ((uint32_t)p[2] << 16)
    | ((uint32_t)p[3] << 24);
}

static void StoreLe64(uint8_t* p, uint64_t v)
{
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
  p[2] = (uint8_t)(v >> 16);
  p[3] = (uint8_t)(v >> 24);
  p[4] = (uint8_t)(v >> 32);
  p[5] = (uint8_t)(v >> 40);
  p[6] = (uint8_t)(v >> 48);
  p[7] = (uint8_t)(v >> 56);
}

void NonceGenInit(NonceGen* gen)
{
  if (gen == NULL)
    return;

  gen->Counter = 0;
  gen->Salt =
    (uint32_t)time(NULL)
    ^ (uint32_t)(uintptr_t)gen;
}

void NonceGenNext(
  NonceGen* gen
  , uint8_t nonce[LOGOBF_NONCE_BYTES]
)
{
  uint64_t c;

  if (gen == NULL || nonce == NULL)
    return;

  c = gen->Counter++;
  StoreLe32(nonce + 0, gen->Salt);
  StoreLe64(nonce + 4, c);
}

static uint32_t Rotl32(uint32_t v, int r)
{
  return (v << r) | (v >> (32 - r));
}

static void QuarterRound(
  uint32_t* a
  , uint32_t* b
  , uint32_t* c
  , uint32_t* d
)
{
  *a += *b;
  *d ^= *a;
  *d = Rotl32(*d, 16);

  *c += *d;
  *b ^= *c;
  *b = Rotl32(*b, 12);

  *a += *b;
  *d ^= *a;
  *d = Rotl32(*d, 8);

  *c += *d;
  *b ^= *c;
  *b = Rotl32(*b, 7);
}

static void ChaCha20Block(
  const uint8_t key[32]
  , uint32_t counter
  , const uint8_t nonce[12]
  , uint8_t out[64]
)
{
  uint32_t state[16]{};
  uint32_t work[16]{};

  state[0]  = 0x61707865u;
  state[1]  = 0x3320646eu;
  state[2]  = 0x79622d32u;
  state[3]  = 0x6b206574u;

  state[4]  = LoadLe32(key + 0);
  state[5]  = LoadLe32(key + 4);
  state[6]  = LoadLe32(key + 8);
  state[7]  = LoadLe32(key + 12);
  state[8]  = LoadLe32(key + 16);
  state[9]  = LoadLe32(key + 20);
  state[10] = LoadLe32(key + 24);
  state[11] = LoadLe32(key + 28);

  state[12] = counter;
  state[13] = LoadLe32(nonce + 0);
  state[14] = LoadLe32(nonce + 4);
  state[15] = LoadLe32(nonce + 8);

  memcpy(work, state, sizeof(state));

  for (int i = 0; i < 10; i++)
  {
    QuarterRound(&work[0], &work[4], &work[8],  &work[12]);
    QuarterRound(&work[1], &work[5], &work[9],  &work[13]);
    QuarterRound(&work[2], &work[6], &work[10], &work[14]);
    QuarterRound(&work[3], &work[7], &work[11], &work[15]);

    QuarterRound(&work[0], &work[5], &work[10], &work[15]);
    QuarterRound(&work[1], &work[6], &work[11], &work[12]);
    QuarterRound(&work[2], &work[7], &work[8],  &work[13]);
    QuarterRound(&work[3], &work[4], &work[9],  &work[14]);
  }

  for (int i = 0; i < 16; i++)
  {
    StoreLe32(out + (size_t)i * 4, work[i] + state[i]);
  }
}

static void ChaCha20Xor(
  const uint8_t key[32]
  , const uint8_t nonce[12]
  , uint32_t counter
  , const uint8_t* in
  , uint8_t* out
  , size_t len
)
{
  uint8_t block[64];

  while (len != 0)
  {
    ChaCha20Block(key, counter++, nonce, block);

    size_t n = (len > 64) ? 64 : len;
    for (size_t i = 0; i < n; i++)
    {
      out[i] = (uint8_t)(in[i] ^ block[i]);
    }

    in += n;
    out += n;
    len -= n;
  }

  memset(block, 0, sizeof(block));
}

int ObfEncryptRecord(
  const ObfKey* key
  , NonceGen* nonceGen
  , const uint8_t* plaintext
  , size_t plaintextLen
  , uint8_t* outRecord
  , size_t outRecordCap
  , size_t* outRecordLen
)
{
  if (key == NULL || nonceGen == NULL || outRecord == NULL || outRecordLen == NULL)
    return 0;

  if (plaintextLen > 0xffffu)
    return 0;

  size_t need = ObfCalcRecordSize(plaintextLen);
  if (outRecordCap < need)
    return 0;

  uint8_t* header = outRecord;
  uint8_t* nonce  = header + 4;
  uint8_t* ct     = header + LOGOBF_HEADER_BYTES;

  StoreLe16(header + 0, LOGOBF_SIGNATURE);
  StoreLe16(header + 2, (uint16_t)plaintextLen);

  NonceGenNext(nonceGen, nonce);

  ChaCha20Xor(
    key->Bytes
    , nonce
    , 1
    , plaintext
    , ct
    , plaintextLen
  );

  *outRecordLen = need;
  return 1;
}

int ObfDecryptRecord(
  const ObfKey* key
  , const uint8_t* record
  , size_t recordLen
  , uint8_t* outPlaintext
  , size_t outPlaintextCap
  , size_t* outPlaintextLen
  , uint8_t outNonce[LOGOBF_NONCE_BYTES]
)
{
  if (key == NULL || record == NULL || outPlaintext == NULL || outPlaintextLen == NULL)
    return 0;

  if (recordLen < LOGOBF_HEADER_BYTES)
    return 0;

  uint16_t sig = LoadLe16(record + 0);
  uint16_t len = LoadLe16(record + 2);

  if (sig != LOGOBF_SIGNATURE)
    return 0;

  size_t need = LOGOBF_HEADER_BYTES + (size_t)len;
  if (recordLen < need)
    return 0;

  if (outPlaintextCap < (size_t)len)
    return 0;

  const uint8_t* nonce = record + 4;
  const uint8_t* ct    = record + LOGOBF_HEADER_BYTES;

  if (outNonce != NULL)
    memcpy(outNonce, nonce, LOGOBF_NONCE_BYTES);

  ChaCha20Xor(
    key->Bytes
    , nonce
    , 1
    , ct
    , outPlaintext
    , (size_t)len
  );

  *outPlaintextLen = (size_t)len;
  return 1;
}

size_t ObfCalcRecordSize(size_t plaintextLen)
{
  if (plaintextLen > 0xffffu)
    return 0;

  return LOGOBF_HEADER_BYTES + plaintextLen;
}

bool Logme::Logger::DeobfuscateRecord(const uint8_t* r, size_t len, std::string& line) const
{
  if (len < LOGOBF_HEADER_BYTES)
    return false;

  size_t size;
  uint8_t nonce[LOGOBF_NONCE_BYTES];

  if (len < 4ULL * 1024)
  {
    uint8_t buffer[4ULL * 1024];
    if (!ObfDecryptRecord(&Key, r, len, buffer, sizeof(buffer), &size, nonce))
      return false;

    line = (const char*)buffer;
    return true;
  }

  std::vector<uint8_t> buffer(len);
  if (!ObfDecryptRecord(&Key, r, len, buffer.data(), buffer.size(), &size, nonce))
    return false;

  line = (const char*)buffer.data();
  return true;
}

static bool ReadExact(FILE* f, void* dst, size_t len)
{
  uint8_t* p = (uint8_t*)dst;
  size_t got = 0;

  while (got < len)
  {
    size_t n = fread(p + got, 1, len - got, f);
    if (n == 0)
      return false;

    got += n;
  }

  return true;
}

static bool WriteExact(FILE* f, const void* src, size_t len)
{
  const uint8_t* p = (const uint8_t*)src;
  size_t done = 0;

  while (done < len)
  {
    size_t n = fwrite(p + done, 1, len - done, f);
    if (n == 0)
      return false;

    done += n;
  }

  return true;
}

static bool ReadLe16FromFile(FILE* f, uint16_t& v)
{
  uint8_t b[2];
  size_t n = fread(b, 1, 2, f);
  if (n == 0)
    return false; /* EOF */

  if (n != 2)
    return false;

  v = (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
  return true;
}

bool DeobfuscateLogFile(
  const Logme::ID& ch
  , const ObfKey* key
  , const std::string& inPath
  , const std::string& outPath
)
{
  if (key == NULL)
    return false;

  std::filesystem::path in(inPath);
  std::filesystem::path out;

  if (outPath.empty())
    out = in;
  else
    out = std::filesystem::path(outPath);

  bool overwrite = false;
  {
    std::error_code ec;
    if (std::filesystem::equivalent(in, out, ec) && !ec)
      overwrite = true;
    else if (in == out)
      overwrite = true;
  }

  std::filesystem::path tmp;
  if (overwrite)
  {
    tmp = out;
    tmp += ".deobf.tmp";
  }
  else
  {
    tmp = out;
  }

  FILE* fi = fopen(in.string().c_str(), "rb");
  if (fi == NULL)
    return false;

  // Pre-check: if the file does not start with the record signature, treat it as already decoded.
  unsigned short sig0 = 0;
  if (fread(&sig0, 1, sizeof(sig0), fi) != sizeof(sig0))
  {
    if (ferror(fi))
    {
      fLogmeE(
        ch
        , "DeobfuscateLogFile: failed to read signature: path='{}' errno={} ({})"
        , PathToUtf8String(in)
        , errno
        , ErrnoText(errno)
      );
      fclose(fi);
      return false;
    }

    // Empty file: nothing to do.
    fclose(fi);
    return true;
  }

  if (sig0 != 0xA55Au)
  {
    fclose(fi);

    if (overwrite)
    {
      // In-place: keep the original file unchanged.
      return true;
    }

    std::error_code ec;
    std::filesystem::copy_file(
      in
      , out
      , std::filesystem::copy_options::overwrite_existing
      , ec
    );
    if (ec)
    {
      fLogmeE(
        ch
        , "DeobfuscateLogFile: copy_file failed: in='{}' out='{}' ec={}"
        , PathToUtf8String(in)
        , PathToUtf8String(out)
        , ec.message().c_str()
      );
      return false;
    }

    return true;
  }

  // Rewind to the beginning for the normal decode flow.
  if (fseek(fi, 0, SEEK_SET) != 0)
  {
    fLogmeE(
      ch
      , "DeobfuscateLogFile: fseek(0) failed: path='{}' errno={} ({})"
      , PathToUtf8String(in)
      , errno
      , ErrnoText(errno)
    );
    fclose(fi);
    return false;
  }

  FILE* fo = fopen(tmp.string().c_str(), "wb");
  if (fo == NULL)
  {
    fclose(fi);
    return false;
  }

  for (;;)
  {
    uint16_t sig = 0;
    uint16_t plainLen16 = 0;

    if (!ReadLe16FromFile(fi, sig))
      break; /* EOF */

    if (sig != (uint16_t)LOGOBF_SIGNATURE)
    {
      fclose(fi);
      fclose(fo);
      std::filesystem::remove(tmp);
      return false;
    }

    if (!ReadLe16FromFile(fi, plainLen16))
    {
      fclose(fi);
      fclose(fo);
      std::filesystem::remove(tmp);
      return false;
    }

    size_t plainLen = (size_t)plainLen16;
    size_t recordLen = LOGOBF_HEADER_BYTES + plainLen;

    std::vector<uint8_t> record;
    record.resize(recordLen);

    /* Rebuild the full record buffer for ObfDecryptRecord() */
    StoreLe16(record.data() + 0, sig);
    StoreLe16(record.data() + 2, plainLen16);

    if (!ReadExact(fi, record.data() + 4, recordLen - 4))
    {
      fclose(fi);
      fclose(fo);
      std::filesystem::remove(tmp);
      return false;
    }

    size_t outLen = 0;
    uint8_t nonce[LOGOBF_NONCE_BYTES];

    if (plainLen <= 4ULL * 1024)
    {
      uint8_t buffer[4ULL * 1024];

      if (!ObfDecryptRecord(key, record.data(), record.size(), buffer, sizeof(buffer), &outLen, nonce))
      {
        fclose(fi);
        fclose(fo);
        std::filesystem::remove(tmp);
        return false;
      }

      if (!WriteExact(fo, buffer, outLen))
      {
        fclose(fi);
        fclose(fo);
        std::filesystem::remove(tmp);
        return false;
      }
    }
    else
    {
      std::vector<uint8_t> buffer;
      buffer.resize(plainLen);

      if (!ObfDecryptRecord(key, record.data(), record.size(), buffer.data(), buffer.size(), &outLen, nonce))
      {
        fclose(fi);
        fclose(fo);
        std::filesystem::remove(tmp);
        return false;
      }

      if (!WriteExact(fo, buffer.data(), outLen))
      {
        fclose(fi);
        fclose(fo);
        std::filesystem::remove(tmp);
        return false;
      }
    }

    /* Optional structural check: next bytes should be signature or EOF */
    long pos = ftell(fi);
    if (pos >= 0)
    {
      uint8_t peek[2];
      size_t n = fread(peek, 1, 2, fi);
      if (n == 2)
      {
        uint16_t nextSig = LoadLe16(peek);
        if (nextSig != (uint16_t)LOGOBF_SIGNATURE)
        {
          fclose(fi);
          fclose(fo);
          std::filesystem::remove(tmp);
          return false;
        }

        fseek(fi, pos, SEEK_SET);
      }
      else
      {
        fseek(fi, pos, SEEK_SET);
      }
    }
  }

  fclose(fi);
  fclose(fo);

  if (overwrite)
  {
    std::error_code ec;
    std::filesystem::remove(out, ec);
    
    ec.clear();
    std::filesystem::rename(tmp, out, ec);
    if (ec)
    {
      std::filesystem::remove(tmp);
      fLogmeE(ch, "Unable to remove {}", PathToUtf8String(tmp.string()));
      return false;
    }
  }

  return true;
}