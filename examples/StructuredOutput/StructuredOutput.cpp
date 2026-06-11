#include <Logme/Logme.h>

static void PrintMessage(const char* text)
{
  LogmeI("%s", text);
}

int main()
{
  Logme::OutputFlags flags = Logme::Instance->GetDefaultChannelPtr()->GetFlags();

  PrintMessage("text format");

  flags.Format = Logme::OUTPUT_JSON;
  flags.Channel = true;
  flags.Method = true;
  Logme::Instance->GetDefaultChannelPtr()->SetFlags(flags);

  Logme::ThreadFields fields;
  fields.Set("request_id", "request-42");
  fields.Set("tenant", "demo");

  {
    LogmeThreadFields(fields);
    PrintMessage("json with thread fields");
  }

  Logme::SetOutputFieldName(Logme::OUTPUT_FIELD_MESSAGE, "msg");
  Logme::SetOutputFieldName(Logme::OUTPUT_FIELD_LEVEL, "lvl");
  PrintMessage("json with custom field names");

  Logme::ResetOutputFieldNames();
  flags.Format = Logme::OUTPUT_XML;
  Logme::Instance->GetDefaultChannelPtr()->SetFlags(flags);

  Logme::Instance->SetThreadField("request_id", "request-43");
  Logme::Instance->SetThreadField("tenant", "demo");
  PrintMessage("xml with thread fields");

  flags.Format = Logme::OUTPUT_TEXT;
  Logme::Instance->GetDefaultChannelPtr()->SetFlags(flags);
  PrintMessage("text format ignores thread fields");

  Logme::Instance->ClearThreadFields();

  return 0;
}
