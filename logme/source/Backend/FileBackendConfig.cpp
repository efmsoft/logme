#include <Logme/Backend/FileBackend.h>
#include <Logme/Logme.h>
#include <Logme/Utils.h>

#ifdef USE_JSONCPP
#include <json/json.h>
#endif

#include "../Config/Helper.h"

using namespace Logme;

namespace
{
#ifdef USE_JSONCPP
  bool ParseRetentionInteger(
    const Json::Value& value
    , const char* name
    , int& result
  )
  {
    if (!value.isInt())
    {
      LogmeE(CHINT, "\"%s\" is not an integer", name);
      return false;
    }

    result = value.asInt();
    if (result < 0)
    {
      LogmeE(CHINT, "\"%s\" must not be negative", name);
      return false;
    }

    return true;
  }

  bool ParseRetentionInterval(
    const Json::Value& value
    , const char* name
    , std::uint64_t& result
  )
  {
    if (value.isInt())
    {
      int v = value.asInt();
      if (v < 0)
      {
        LogmeE(CHINT, "\"%s\" must not be negative", name);
        return false;
      }

      result = static_cast<std::uint64_t>(v);
      return true;
    }

    if (!value.isString())
    {
      LogmeE(CHINT, "\"%s\" is not an integer or a string value", name);
      return false;
    }

    if (!ParseInterval(value.asString(), result))
    {
      LogmeE(CHINT, "unsupported value of \"%s\": %s", name, value.asString().c_str());
      return false;
    }

    return true;
  }

  bool ParseRetentionByteSize(
    const Json::Value& value
    , const char* name
    , std::uint64_t& result
  )
  {
    if (value.isInt())
    {
      int v = value.asInt();
      if (v < 0)
      {
        LogmeE(CHINT, "\"%s\" must not be negative", name);
        return false;
      }

      result = static_cast<std::uint64_t>(v);
      return true;
    }

    if (!value.isString())
    {
      LogmeE(CHINT, "\"%s\" is not an integer or a string value", name);
      return false;
    }

    if (!ParseByteSize(value.asString(), result))
    {
      LogmeE(CHINT, "unsupported value of \"%s\": %s", name, value.asString().c_str());
      return false;
    }

    return true;
  }
#endif
}

FileBackendConfig::FileBackendConfig()
  : BackendConfig(FileBackend::TYPE_ID)
  , Append(true)
  , MaxSize(FileBackend::GetMaxSizeDefault())
  , OnSizeLimit(SIZE_LIMIT_TRUNCATE)
  , DailyRotation(false)
  , MaxParts(2)
  , RetentionMaxAge(0)
  , RetentionMaxTotalSize(0)
  , GzipCompression(false)
{
  Async = true;
}

FileBackendConfig::~FileBackendConfig()
{
}

bool FileBackendConfig::Parse(const Json::Value* po)
{
  (void)po;

#ifdef USE_JSONCPP
  if (!BackendConfig::Parse(po))
    return false;

  const Json::Value& o = *po;

  if (o.isMember("append"))
  {
    if (!o["append"].isBool())
    {
      LogmeE(CHINT, "\"append\" is not a boolean value");
      return false;
    }

    Append = o["append"].asBool();
  }

  if (o.isMember("max-size"))
  {
    if (!o["max-size"].isInt() && !o["max-size"].isString())
    {
      LogmeE(CHINT, "\"max-size\" is not an integer or a string value");
      return false;
    }

    MaxSize = GetByteSize(o, "max-size", MaxSize);
  }

  if (o.isMember("on-size-limit"))
  {
    if (!o["on-size-limit"].isString())
    {
      LogmeE(CHINT, "\"on-size-limit\" is not a string value");
      return false;
    }

    std::string v = TrimSpaces(o["on-size-limit"].asString());
    ToLowerAsciiInplace(v);

    if (v == "" || v == "truncate")
      OnSizeLimit = SIZE_LIMIT_TRUNCATE;
    else if (v == "rotate")
      OnSizeLimit = SIZE_LIMIT_ROTATE;
    else
    {
      LogmeE(CHINT, "unsupported value of \"on-size-limit\": %s", v.c_str());
      return false;
    }
  }

  if (o.isMember("rotation"))
  {
    if (!o["rotation"].isString())
    {
      LogmeE(CHINT, "\"rotation\" is not a string value");
      return false;
    }

    std::string v = TrimSpaces(o["rotation"].asString());
    ToLowerAsciiInplace(v);

    if (v == "daily")
      DailyRotation = true;
    else if (v == "" || v == "none" || v == "off" || v == "disabled")
      DailyRotation = false;
    else
    {
      LogmeE(CHINT, "unsupported value of \"rotation\": %s", v.c_str());
      return false;
    }
  }

  bool hasMaxParts = false;

  if (o.isMember("max-parts"))
  {
    if (!ParseRetentionInteger(o["max-parts"], "max-parts", MaxParts))
      return false;

    hasMaxParts = true;
  }

  if (o.isMember("retention"))
  {
    if (!o["retention"].isObject())
    {
      LogmeE(CHINT, "\"retention\" is not an object");
      return false;
    }

    const Json::Value& retention = o["retention"];

    if (retention.isMember("max-files"))
    {
      int maxFiles = 0;
      if (!ParseRetentionInteger(retention["max-files"], "retention.max-files", maxFiles))
        return false;

      if (hasMaxParts && maxFiles != MaxParts)
      {
        LogmeE(CHINT, "\"max-parts\" and \"retention.max-files\" specify different values");
        return false;
      }

      MaxParts = maxFiles;
    }

    if (retention.isMember("max-age"))
    {
      if (!ParseRetentionInterval(retention["max-age"], "retention.max-age", RetentionMaxAge))
        return false;
    }

    if (retention.isMember("max-total-size"))
    {
      if (!ParseRetentionByteSize(retention["max-total-size"], "retention.max-total-size", RetentionMaxTotalSize))
        return false;
    }
  }

  if (o.isMember("compression"))
  {
    if (!o["compression"].isString())
    {
      LogmeE(CHINT, "\"compression\" is not a string value");
      return false;
    }

    std::string v = TrimSpaces(o["compression"].asString());
    ToLowerAsciiInplace(v);

    if (v == "gz" || v == "gzip")
      GzipCompression = true;
    else if (v == "" || v == "none" || v == "off" || v == "disabled")
      GzipCompression = false;
    else
    {
      LogmeE(CHINT, "unsupported value of \"compression\": %s", v.c_str());
      return false;
    }
  }

  if (o.isMember("archive"))
  {
    if (!o["archive"].isString())
    {
      LogmeE(CHINT, "\"archive\" is not a string");
      return false;
    }

    ArchiveFilename = o["archive"].asString();
  }

  if (OnSizeLimit == SIZE_LIMIT_ROTATE)
  {
    if (ArchiveFilename.empty())
    {
      LogmeE(CHINT, "\"archive\" is required when \"on-size-limit\" is \"rotate\"");
      return false;
    }

    if (ArchiveFilename.find("{index}") == std::string::npos)
    {
      LogmeE(CHINT, "\"archive\" must contain {index} when \"on-size-limit\" is \"rotate\"");
      return false;
    }
  }

  if (!o.isMember("file"))
  {
    LogmeE(CHINT, "\"file\" is not specified");
    return false;
  }

  if (!o["file"].isString())
  {
    LogmeE(CHINT, "\"file\" is not a string");
    return false;
  }

  Filename = o["file"].asString();
#endif

  return true;
}
