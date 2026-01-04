#include <Logme/Context.h>
#include <Logme/Logger.h>
#include <Logme/Procedure.h>
#include <string.h>

using namespace Logme;

#pragma warning(disable : 6255 6386)

Procedure::Procedure(
  const Context& context
  , Printer* printer
  , const ID& ch
  , const char* format
  , ...
)
  : BeginContext(context)
  , RValPrinter(printer)
  , Channel(ch)
  , Begin(std::chrono::system_clock::now())
{
  va_list args;
  va_start(args, format);
  std::string params = Format(format, args);
  va_end(args);

  Print(true, params.c_str());
}

Procedure::Procedure(
  const Context& context
  , Printer* printer
  , ChannelPtr pch
  , const char* format
  , ...
)
  : BeginContext(context)
  , RValPrinter(printer)
  , Channel(pch->GetID())
  , Begin(std::chrono::system_clock::now())
{
  va_list args;
  va_start(args, format);
  std::string params = Format(format, args);
  va_end(args);

  Print(true, params.c_str());
}

Procedure::Procedure(
  const Context& context
  , Printer* printer
  , const char* format
  , ...
)
  : BeginContext(context)
  , Channel(CH)
  , RValPrinter(printer)
  , Begin(std::chrono::system_clock::now())
{
  va_list args;
  va_start(args, format);
  std::string params = Format(format, args);
  va_end(args);

  Print(true, params.c_str());
}

Procedure::~Procedure()
{
  std::string result;
  
  if (RValPrinter)
    result = RValPrinter->Format();

  Print(false, result.c_str());
}

void Procedure::Print(bool begin, const char* text)
{
  const char* dir = begin ? ">>" : "<<";

  Context context(BeginContext);
  context.Channel = &Channel;
  context.Applied.ProcPrint = true;
  context.Applied.ProcPrintIn = begin;
  context.AppendProc = &AppendDuration;
  context.AppendContext = this;
  
  if (text == nullptr || *text == '\0')
    Instance->Log(context, "%s %s()", dir, context.Method);
  else
    Instance->Log(context, "%s %s(): %s", dir, context.Method, text);
}

std::string Procedure::Format(const char* format, va_list args)
{
  if (format == nullptr)
    return std::string();

  size_t size = std::max(4096U, (unsigned int)(strlen(format) + 512));

  char* buffer = (char*)alloca(size);

  buffer[0] = '\0';
  buffer[size - 1] = '\0';

  int rc = vsnprintf(buffer + strlen(buffer), size - 1, format, args);
  if (rc == -1)
  {
    char error[] = "[format error]";
    memcpy(buffer, error, sizeof(error));
  }

  return buffer;
}

const char* Procedure::AppendDuration(struct Context& context)
{
  Procedure* self = (Procedure*)context.AppendContext;
  if (self->Duration.empty())
  {
    auto end = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - self->Begin);

    char buffer[32];
#ifdef _WIN32
    sprintf_s(buffer, " [%i ms]", (int)ms.count());
#else
    sprintf(buffer, " [%i ms]", (int)ms.count());
#endif
    self->Duration = buffer;
  }
  return self->Duration.c_str();
}
