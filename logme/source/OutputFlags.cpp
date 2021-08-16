#include <Logme/OutputFlags.h>

using namespace Logme;

OutputFlags::OutputFlags()
  : Value(0)
{
  Timestamp = true;
  Signature = true;
  Method = true;
  ErrorPrefix = true;
  Highlight = true;
  Eol = true;
}
