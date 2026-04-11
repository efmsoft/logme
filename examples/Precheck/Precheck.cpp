#include <Logme/Logme.h>

static Logme::ChannelPtr EnsureVisibleChannel(const Logme::ID& id)
{
  auto ch = Logme::Instance->CreateChannel(id);
  ch->AddLink(::CH);
  return ch;
}

static int ExpensiveTextCalls;
static int PrepareValueCalls;

static std::string ExpensiveText()
{
  ExpensiveTextCalls++;
  return "ExpensiveText() was evaluated";
}

static int PrepareValue()
{
  PrepareValueCalls++;
  return 42;
}

int main()
{
  auto active = EnsureVisibleChannel(Logme::ID{"precheck_active_example"});
  auto inactive = Logme::Instance->CreateChannel(Logme::ID{"precheck_inactive_example"});
  inactive->SetEnabled(false);

  LogmeI(active) << "Precheck example";

  // Passing ChannelPtr as the first argument lets Logme check the channel state
  // before evaluating the rest of the logging arguments.
  LogmeI(inactive, "%s", ExpensiveText().c_str());
  LogmeI(active, "%s", ExpensiveText().c_str());

  LogmeI("ExpensiveText() calls after both messages: %d", ExpensiveTextCalls);

  int prepared = 0;

  // The *_Do macro is meant for cases where some preparation code would be
  // wasteful unless the log record is really going to be emitted.
  LogmeI_Do(inactive->GetID(), prepared = PrepareValue(), "inactive prepared=%d", prepared);
  LogmeI("PrepareValue() calls after inactive LogmeI_Do: %d", PrepareValueCalls);

  LogmeI_Do(active->GetID(), prepared = PrepareValue(), "active prepared=%d", prepared);
  LogmeI("PrepareValue() calls after active LogmeI_Do: %d", PrepareValueCalls);

#ifndef LOGME_DISABLE_STD_FORMAT
  prepared = 0;
  fLogmeI_Do(active->GetID(), prepared = PrepareValue(), "std::format prepared={}", prepared);
#endif

  return 0;
}
