#include "Worker.h"

#include <chrono>

#include <Logme/Logme.h>

Worker::Worker(
  const char* name
  , const char* channel
  , const char* subsystemA
  , const char* subsystemB
)
  : Name(name ? name : "")
  , ChannelId{channel ? channel : ""}
  , SubsystemA(Logme::SID::Build(subsystemA ? subsystemA : ""))
  , SubsystemB(Logme::SID::Build(subsystemB ? subsystemB : ""))
  , SubsystemAName(subsystemA ? subsystemA : "")
  , SubsystemBName(subsystemB ? subsystemB : "")
  , StopFlag(false)
{
}

Worker::~Worker()
{
  Stop();
}

void Worker::Start()
{
  StopFlag = false;
  Thread = std::thread(&Worker::Run, this);
}

void Worker::Stop()
{
  StopFlag = true;
  if (Thread.joinable())
    Thread.join();
}

void Worker::WriteA()
{
  LogmeI(
    ChannelId
    , SubsystemA
    , "%s [%s/%s]: A: channel + subsystem"
    , Name.c_str()
    , (ChannelId.Name ? ChannelId.Name : "")
    , SubsystemAName.c_str()
  );
}

void Worker::WriteB()
{
  LogmeI(
    ChannelId
    , SubsystemB
    , "%s [%s/%s]: B: channel + subsystem"
    , Name.c_str()
    , (ChannelId.Name ? ChannelId.Name : "")
    , SubsystemBName.c_str()
  );
}

void Worker::Run()
{
  LogmeI("%s started", Name.c_str());

  while (!StopFlag)
  {    LogmeI(
      ChannelId
      , "%s [%s]: periodic message"
      , Name.c_str()
      , (ChannelId.Name ? ChannelId.Name : "")
    );

    WriteA();
    WriteB();

    std::this_thread::sleep_for(std::chrono::seconds(3));
  }

  LogmeI("%s stopped", Name.c_str());
}
