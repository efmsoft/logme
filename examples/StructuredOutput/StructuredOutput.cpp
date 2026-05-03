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
  PrintMessage("json format");

  Logme::SetOutputFieldName(Logme::OUTPUT_FIELD_MESSAGE, "msg");
  Logme::SetOutputFieldName(Logme::OUTPUT_FIELD_LEVEL, "lvl");
  PrintMessage("json with custom field names");

  Logme::ResetOutputFieldNames();
  flags.Format = Logme::OUTPUT_XML;
  Logme::Instance->GetDefaultChannelPtr()->SetFlags(flags);
  PrintMessage("xml format");

  flags.Format = Logme::OUTPUT_TEXT;
  Logme::Instance->GetDefaultChannelPtr()->SetFlags(flags);
  PrintMessage("text format again");

  return 0;
}
