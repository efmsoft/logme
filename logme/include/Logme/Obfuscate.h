#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

#include <Logme/ID.h>
#include <Logme/Types.h>

#define LOGOBF_KEY_BYTES   32
#define LOGOBF_NONCE_BYTES 12

#define LOGOBF_SIGNATURE   0xA55Au
#define LOGOBF_HEADER_BYTES (2 + 2 + LOGOBF_NONCE_BYTES)

typedef struct ObfKey
{
  uint8_t Bytes[LOGOBF_KEY_BYTES];
} ObfKey;

typedef struct NonceGen
{
  uint64_t Counter;
  uint32_t Salt;
} NonceGen;

/* Initialize nonce generator (variant 1: fast, no RNG) */
LOGMELNK void NonceGenInit(
  NonceGen* gen
);

/* Generate next nonce (12 bytes) */
LOGMELNK void NonceGenNext(
  NonceGen* gen
  , uint8_t nonce[LOGOBF_NONCE_BYTES]
);

/* Encrypt one log record */
LOGMELNK int ObfEncryptRecord(
  const ObfKey* key
  , NonceGen* nonceGen
  , const uint8_t* plaintext
  , size_t plaintextLen
  , uint8_t* outRecord
  , size_t outRecordCap
  , size_t* outRecordLen
);

/* Decrypt one log record */
LOGMELNK int ObfDecryptRecord(
  const ObfKey* key
  , const uint8_t* record
  , size_t recordLen
  , uint8_t* outPlaintext
  , size_t outPlaintextCap
  , size_t* outPlaintextLen
  , uint8_t outNonce[LOGOBF_NONCE_BYTES]
);

LOGMELNK size_t ObfCalcRecordSize(
  size_t plaintextLen
);

LOGMELNK bool DeobfuscateLogFile(
  const Logme::ID& ch
  , const ObfKey* key
  , const std::string& inPath
  , const std::string& outPath
);
