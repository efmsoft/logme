#include <Logme/Logme.h>

#include <chrono>
#include <thread>

int main()
{
  Logme::ID id{"invisible"};

  // This is allowed, but you may not see any output if the channel is not created
  // and does not have any backends or links.
  LogmeI(id, "This may be invisible (channel has no backends/links yet)");

  // Create the channel explicitly.
  auto ch = Logme::Instance->CreateChannel(id);

  // Option A: link to the default channel (::CH) which already has backends.
  // This is the simplest way to make a custom channel visible.
  ch->AddLink(::CH);

  LogmeI(id, "Now visible via AddLink(::CH)");

  // Option B: add a backend directly (example: console backend).
  // If you prefer this approach, keep AddLink() disabled to avoid duplicate output.
  //
  // ch->AddBackend(std::make_shared<ConsoleBackend>(ch));

  // Small delay so the user can see output in quick runs.
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  return 0;
}
