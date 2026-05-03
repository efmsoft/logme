# logmeobf

`logmeobf` generates log obfuscation keys and converts logs between readable text and logme binary obfuscated records.

## Usage

```bash
logmeobf --generate-key --base64
logmeobf --generate-key --key-out logme.key
logmeobf --generate-key --header logme_obf_key.h --name LOGME_OBF_KEY

logmeobf --obfuscate --key-file logme.key --in app.log --out app.logobf
logmeobf --obfuscate --key-base64 BASE64KEY --in app.log --out app.logobf
logmeobf --deobfuscate --key-file logme.key --in app.logobf --out app.log
```

## Options

```text
--generate-key              Generate a new 32-byte obfuscation key.
--obfuscate                 Convert readable log records to binary obfuscated records.
--deobfuscate               Convert binary obfuscated records to readable log records.
--key-file FILE             Read a binary 32-byte key from FILE.
--key-base64 TEXT           Read a base64-encoded 32-byte key from TEXT.
--key-out FILE              Write generated key as a binary file.
--base64                    Print generated key as base64.
--header FILE               Write generated key as a C/C++ header file.
--name IDENTIFIER           Header variable name. Default: LOGME_OBFUSCATION_KEY.
--in FILE                   Input file. Default: stdin.
--out FILE                  Output file. Default: stdout.
--format auto|text|json|xml Input text format used during --obfuscate. Default: auto.
--nonce-salt VALUE          Use fixed nonce salt for reproducible obfuscation output.
--help                      Show help.
```

When obfuscating a readable text log, `--format auto` examines the first records and selects JSON, XML or text mode. JSON and XML logs are treated as one record per line. Text logs are grouped by the usual logme record prefix when it is visible, so message text containing embedded `\n` lines remains part of the same obfuscated record.

`--nonce-salt` is intended for reproducible tests and golden files. Normal production use should let the tool generate the nonce salt automatically.
