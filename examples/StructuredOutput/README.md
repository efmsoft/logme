# StructuredOutput example

## StructuredOutput

This example shows how to switch logme output between plain text, JSON and XML at runtime.
It also demonstrates structured field name customization.

## What it demonstrates

- Keeping the default text formatter unchanged.
- Switching `OutputFlags::Format` to JSON.
- Switching `OutputFlags::Format` to XML.
- Renaming structured fields with `SetOutputFieldName()`.
- Restoring default field names with `ResetOutputFieldNames()`.

## Notes

JSON output is emitted as one JSON object per log record.
XML output is emitted as one `<event>` element per log record.

Use `logmefmt --finalize` when the stream must be converted into a complete JSON or XML document.
