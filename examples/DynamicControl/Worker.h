#pragma once

#include <atomic>
#include <string>
#include <thread>

#include <Logme/ID.h>
#include <Logme/SID.h>

class Worker
{
  std::string Name;
  Logme::ID ChannelId;
  Logme::SID SubsystemA;
  Logme::SID SubsystemB;

  std::string SubsystemAName;
  std::string SubsystemBName;

  std::thread Thread;
  std::atomic_bool StopFlag;

public:
  Worker(
    const char* name
    , const char* channel
    , const char* subsystemA
    , const char* subsystemB
  );

  ~Worker();

  void Start();
  void Stop();

private:
  void Run();
  void WriteA();
  void WriteB();
};
