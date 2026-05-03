# logmefmt

`logmefmt` converts logme records between text, JSON and XML formats.
By default the tool keeps the log stream shape: one input record produces one output record.
Use `--finalize` when the output must be a complete JSON or XML document.

## Usage

```bash
logmefmt --input json --output json --finalize < log.jsonl > log.json
logmefmt --input xml --output xml --finalize --root log < log.xmlfrag > log.xml
logmefmt --input json --output xml --finalize --field message=msg < log.jsonl > log.xml
```

## Options

```text
--input text|json|xml       Input format.
--output text|json|xml      Output format.
--finalize                  Complete the output document.
--root NAME                 XML root element used with --finalize. Default: log.
--field OLD=NEW             Rename a field on both input and output sides.
--input-field OLD=NEW       Rename an input field before conversion.
--output-field OLD=NEW      Rename an output field after conversion.
--help                      Show help.
```

For JSON output, `--finalize` writes a JSON array. Without `--finalize`, it writes one JSON object per record.
For XML output, `--finalize` wraps records into the XML root element. Without `--finalize`, it writes one `<event>` element per record.
