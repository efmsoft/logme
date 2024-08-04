# logme library
Compact cross-platform logging framework for C &amp; C++. Implements the concept of Channels. Supports dynamic creation of Channels, modifying channel configuration, messages redirection. Supports logging to console, files on disk, buffer, debugger window, VT100 message highlighting.

## Introduction
It would seem that what could be simpler than logging? And indeed, for small projects it is enough to have code that, for example, uses **printf** to output messages to the console, or writes messages to a file using **fprintf**. But this is not enough in large projects. These projects are usually cross-platform, have many subsystems, and require high performance from the logging system.

Generally speaking, when creating a logging system in a project, there is always a struggle between two extremes: 1) nothing is logged and if the system fails, it is not clear what happened 2) logging is too detailed and it is very difficult to find the necessary information in hundreds and thousands of log lines.

One of the ideas for creating this library was to solve the problem mentioned above. This was done by introducing the concept of **Channel**. A **Channel** is a named object to which **Backends** are attached. **Backend** is a transport for writing data (for example, to the console or to a file on disk). Thus, using a macro for writing to a log and a channel specified, you can ensure that a message is output to several places at once. But the most important thing when using channels is that each of your project's subsystems can use output to its own channel. And then at the configuration level on disk or dynamically, channels can be turned on/off, redirected output.

For example, there are two subsystems **A** and **B**. Modules **A** write detailed work about their work to channel **A**. Modules **B** to channel **B**. At the application initialization stage, only channel **A** was created and the filtering level allowed writing to the log only error messages

**A.cpp**:
```cpp
Logme::ID A{"A"};

void initialize()
{
  LogmeI(A, "starting initialization");
  int r = DoSomething();
  if (r <= 0)
  {
    LogmeE(A, "DoSomething() failed. Error: %i", r);
  }

  LogmeI(A) << "you can also use c++ symantec: " << r;
}
```

**B.cpp**:
```cpp
Logme::ID B{"any_name"};

int DoSomething()
{
  LogmeE(B, "not impemented");
  return -1;
}
```
Then the log will only contain the error message "DoSomething() failed...". Having seen it, you can change the filtering level and get all messages written to channel **A**. Or create channel **B** in search of the reason why **DoSomething** returned an error.

Channels can be created, deleted, enabled, the filtering level can be changed, and they can be redirected to other channels. The latter can be very useful if, for example, your project has a central log that collects all messages (or only errors from all subsystems) and separate logs with messages about the operation of subsystems. Then, by linking the subsystem channel to the central log channel, you can write to the subsystem channel and at the same time duplicate everything in the central channel.

## Logging macros

To start using the library, simply include the Logme/Logme.h file and call one of the following macros: LogmeD(), LogmeI(), LogmeW(), LogmeE(), LogmeC(). The last letter in the macro name defines the level/type of the message: D - debug, I - information (usually the most frequently used type), W - warning, E - error, C - critical error. Each of these macros takes a variable number of arguments. They can define the name of the channel to which the record should be made, as well as Override parameters, format string, substitution parameters.

Examples of use:
```cpp
#include <Logme/Logme.h>

...
Logme::ID ch{"test"};
...

// Use default channel
LogmeW("Hello %s", "World!");
LogmeI() << "C++ style of " << "output";

// Specify channel explicitly LogmeI(ch, "output to ch");
LogmeD(ch) << "something" << 123;

// Specify both channels and override Logme::Override ovr;
ovr.Remove.Method = false;
LogmeC(ch, ovr, "something went wrong");
```