#include <Logme/Backend/ConsoleBackend.h>
#include <Logme/Logme.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

ConsoleBackendConfig::ConsoleBackendConfig()
  : BackendConfig(ConsoleBackend::TYPE_ID)
  , Async(false)
  , HasQueueLimits(false)
  , QueueRecordLimit(ConsoleBackend::QUEUE_RECORD_LIMIT)
  , QueueByteLimit(ConsoleBackend::QUEUE_BYTE_LIMIT)
  , HasOverflowPolicy(false)
  , OverflowPolicy(ConsoleOverflowPolicy::BLOCK)
{
}

#ifdef USE_JSONCPP

static bool ParseConsoleOverflowPolicy(
  const std::string& text
  , ConsoleOverflowPolicy& policy
)
{
  std::string v = TrimSpaces(text);
  ToLowerAsciiInplace(v);

  if (v == "block")
  {
    policy = ConsoleOverflowPolicy::BLOCK;
    return true;
  }

  if (v == "drop-new" || v == "drop_new")
  {
    policy = ConsoleOverflowPolicy::DROP_NEW;
    return true;
  }

  if (v == "drop-oldest" || v == "drop_oldest")
  {
    policy = ConsoleOverflowPolicy::DROP_OLDEST;
    return true;
  }

  return false;
}

#endif

bool ConsoleBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  const Json::Value& o = *po;

  if (o.isMember("async"))
  {
    if (!o["async"].isBool())
    {
      LogmeE(CHINT, "\"async\" is not a boolean value");
      return false;
    }

    Async = o["async"].asBool();
  }

  if (o.isMember("queue-record-limit"))
  {
    if (!o["queue-record-limit"].isUInt64() && !o["queue-record-limit"].isInt())
    {
      LogmeE(CHINT, "\"queue-record-limit\" is not an integer");
      return false;
    }

    HasQueueLimits = true;
    QueueRecordLimit = static_cast<size_t>(
      o["queue-record-limit"].asUInt64()
    );
  }

  if (o.isMember("queue-byte-limit"))
  {
    if (!o["queue-byte-limit"].isUInt64()
      && !o["queue-byte-limit"].isInt()
      && !o["queue-byte-limit"].isString())
    {
      LogmeE(CHINT, "\"queue-byte-limit\" is not an integer or a string value");
      return false;
    }

    HasQueueLimits = true;
    QueueByteLimit = static_cast<size_t>(
      GetByteSize(o, "queue-byte-limit", QueueByteLimit)
    );
  }

  if (o.isMember("overflow-policy"))
  {
    if (!o["overflow-policy"].isString())
    {
      LogmeE(CHINT, "\"overflow-policy\" is not a string");
      return false;
    }

    HasOverflowPolicy = true;
    if (!ParseConsoleOverflowPolicy(
      o["overflow-policy"].asString()
      , OverflowPolicy
    ))
    {
      LogmeE(CHINT, "\"overflow-policy\" has unsupported value");
      return false;
    }
  }
#endif

  return true;
}

