#include <Logme/Channel.h>
#include <Logme/ThreadName.h>
#include <Logme/Utils.h>

using namespace Logme;

ThreadName::ThreadName(ChannelPtr pch, const char* name, bool log)
  : PCH(pch)
  , Log(log)
{
  Initialize(name);
}

ThreadName::ThreadName(ChannelPtr pch, const std::string& name, bool log)
  : PCH(pch)
  , Log(log)
{
  Initialize(name.c_str());
}

ThreadName::~ThreadName()
{
  uint64_t tid = GetCurrentThreadId();
  PCH->SetThreadName(
    tid
    , PreviousName.has_value() 
    ? PreviousName.value().c_str() 
    : nullptr
    , Log
  );

  if (Log)
  {
    Override ovr;
    ovr.Remove.Method = true;

    Logme::ID ch = PCH->GetID();
    Context c = LOGME_CONTEXT(Logme::Level::LEVEL_INFO, &ch);
    c.Ovr = &ovr;

    PCH->Display(c, "@");
  }
}

void ThreadName::Initialize(const char* name)
{
  uint64_t tid = GetCurrentThreadId();

  Channel::ThreadNameInfo info;
  auto p = PCH->GetThreadName(tid, info, nullptr, false);
  if (p != nullptr)
    PreviousName = p;

  PCH->SetThreadName(tid, name, Log);
}