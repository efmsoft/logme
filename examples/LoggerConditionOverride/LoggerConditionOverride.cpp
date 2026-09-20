#include <Logme/Logme.h>

// Every LogmeD/LogmeI/LogmeW/LogmeE/... macro checks the unqualified,
// global LoggerCondition() before doing any argument preparation or
// formatting. By default this is just Logme::Instance->Condition() -- the
// process-wide gate set through Logger::SetCondition(). A class can replace
// that check for its own methods, at zero extra runtime cost, by declaring
// a member with the exact same name: ordinary C++ member lookup finds it
// before the macro's call ever considers the global default (a qualified
// Logme::LoggerCondition() call could not be shadowed this way). This is
// the same mechanism
// already used to let CH/SUBSID resolve to a class member instead of the
// file-scope default (see Logme/ID.h, Logme/SID.h).

static Logme::ChannelPtr EnsureVisibleChannel(const Logme::ID& id)
{
  auto ch = Logme::Instance->CreateChannel(id);
  ch->AddLink(::CH);
  return ch;
}

// Imagine a per-request/per-connection object: decide once, when the object
// is created, whether logging is even worth attempting for it (e.g. based on
// a Sip/Dip/Host filter, or a "this request/connection will never be
// inspected" decision) and cache the answer in a plain member. Every log
// macro called from this class's own methods then costs a single bool read
// instead of the thread-local lookup and filter comparisons the default
// Logme::Instance->Condition() may need to do.
struct Connection
{
  bool LoggingDecidedOnce;

  explicit Connection(bool loggingEnabled)
    : LoggingDecidedOnce(loggingEnabled)
  {
  }

  bool LoggerCondition() const
  {
    return LoggingDecidedOnce;
  }

  void HandleRequest(const Logme::ChannelPtr& ch)
  {
    LogmeI(ch, "handling request");
  }
};

// Unrelated code in the same program keeps using the default -- the
// override above is only ever considered from inside Connection's own
// methods, via ordinary member lookup.
static void LogFromUnrelatedCode(const Logme::ChannelPtr& ch)
{
  LogmeI(ch, "unrelated code, not affected by Connection's override");
}

int main()
{
  auto ch = EnsureVisibleChannel(Logme::ID{"logger_condition_override_example"});

  Connection loggingConnection(true);
  loggingConnection.HandleRequest(ch);

  Connection silentConnection(false);
  silentConnection.HandleRequest(ch); // suppressed: LoggerCondition() returns false

  LogFromUnrelatedCode(ch); // always uses the process-wide default

  return 0;
}
