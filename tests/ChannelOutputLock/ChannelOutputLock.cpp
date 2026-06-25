#include <Common/TestBackend.h>

#include <atomic>
#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <Logme/Logme.h>

namespace
{
  Logme::ID OutputLockChannelID{ "channel-output-lock" };
  Logme::ChannelPtr OutputLockChannel;
  std::shared_ptr<TestBackend> OutputLockBackend;

  bool WaitForAccessCount(uint64_t expected)
  {
    for (int index = 0; index < 1000; ++index)
    {
      if (OutputLockChannel->GetAccessCount() >= expected)
        return true;

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return false;
  }
}

TEST(ChannelOutputLock, BlocksOtherThreadsAndAllowsNestedOutput)
{
  OutputLockBackend->Clear();

  uint64_t accessCount = OutputLockChannel->GetAccessCount();
  std::atomic<bool> completed{ false };

  std::thread worker;

  bool workerEnteredDisplay = false;
  bool workerBlocked = false;

  {
    auto outputLock = OutputLockChannel->LockOutput();

    worker = std::thread([&completed]()
    {
      LogmeI(OutputLockChannelID, "worker");
      completed.store(true, std::memory_order_release);
    });

    workerEnteredDisplay = WaitForAccessCount(accessCount + 1);
    workerBlocked = workerEnteredDisplay &&
      !completed.load(std::memory_order_acquire);

    LogmeI(OutputLockChannelID, "owner");

    EXPECT_EQ(OutputLockBackend->History.size(), 1u);
    EXPECT_EQ(OutputLockBackend->History[0], "owner");
  }

  worker.join();

  EXPECT_TRUE(workerEnteredDisplay);
  EXPECT_TRUE(workerBlocked);
  EXPECT_TRUE(completed.load(std::memory_order_acquire));
  ASSERT_EQ(OutputLockBackend->History.size(), 2u);
  EXPECT_EQ(OutputLockBackend->History[0], "owner");
  EXPECT_EQ(OutputLockBackend->History[1], "worker");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  OutputLockChannel = Logme::Instance->CreateChannel(OutputLockChannelID);
  OutputLockChannel->RemoveBackends();

  Logme::OutputFlags flags;
  flags.Value = 0;
  OutputLockChannel->SetFlags(flags);
  OutputLockChannel->SetFilterLevel(Logme::LEVEL_DEBUG);

  OutputLockBackend = std::make_shared<TestBackend>(OutputLockChannel);
  OutputLockChannel->AddBackend(OutputLockBackend);

  return RUN_ALL_TESTS();
}
