#include <Logme/Channel.h>
#include <Logme/ThreadName.h>
#include <Logme/Utils.h>

using namespace Logme;

ThreadName::ThreadName(ChannelPtr pch, const char* name, bool log)
  : PCH(pch)
  , ForwardTransitionPrinted(false)
  , Log(log)
  , Skip(false)
{
  Initialize(name);
}

ThreadName::ThreadName(ChannelPtr pch, const std::string& name, bool log)
  : PCH(pch)
  , ForwardTransitionPrinted(false)
  , Log(log)
  , Skip(false)
{
  Initialize(name.c_str());
}

ThreadName::~ThreadName()
{
  // Skip is set in Initialize() when the channel was inactive at
  // construction time -- ordinary LogmeI/LogmeW/... calls already precheck
  // Channel::GetActive() before doing any work (LOGME_WOULD_LOG_ARGS), but
  // this RAII pair didn't, so it paid the full DataLock+ThreadName-map cost
  // even for connections routed to a permanently-disabled channel (e.g.
  // BufferedLogger's shared "null channel" when debug logging is off --
  // VTune, 2026-09). If GetActive() flips mid-scope this just means a
  // thread label is missing around that transition -- cosmetic, not a
  // correctness issue.
  if (PCH && !Skip)
  {
    uint64_t tid = GetCurrentThreadId();
    bool logReturn = Log && ForwardTransitionPrinted;
    PCH->SetThreadName(
      tid
      , PreviousName.has_value()
      ? PreviousName.value().c_str()
      : nullptr
      , logReturn
    );

    if (logReturn)
    {
      Override ovr;
      ovr.Remove.Method = true;

      Logme::ID ch = PCH->GetID();
      Logme::ContextCache cache;
      Context c = LOGME_CONTEXT(cache, Logme::Level::LEVEL_INFO, &ch, &SUBSID);
      c.Ovr = &ovr;

      // After returning the initial name, a message should be printed. 
      // The text of the message doesn't matter. Otherwise, it might 
      // happen that the next message to the channel will be output much 
      // later, and the channel name change will appear as if it happened 
      // much later than it actually did
      c.SetText(".");
      PCH->Display(c);
    }
  }
}

void ThreadName::Initialize(const char* name)
{
  if (PCH && PCH->GetActive())
  {
    uint64_t tid = GetCurrentThreadId();

    Channel::ThreadNameInfo info;
    auto p = PCH->GetThreadName(tid, info, nullptr, false);
    if (p != nullptr)
      PreviousName = p;

    PCH->SetThreadName(tid, name, Log, &ForwardTransitionPrinted);
  }
  else
  {
    Skip = true;
  }
}