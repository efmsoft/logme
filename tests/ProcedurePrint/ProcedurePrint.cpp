#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>
#include <Logme/Procedure.h>

std::shared_ptr<TestBackend> Be;

enum class MyEnum
{
  Zero,
  One,
  Two,
};

template<> std::string FormatValue<MyEnum>(const MyEnum& value)
{
  switch (value)
  {
  case MyEnum::Zero: return "Zero";
  case MyEnum::One: return "One";
  case MyEnum::Two: return "Two";
  default:
    break;
  }
  return "";
}

struct CustomType
{
  int Value;
};

template<> std::string FormatValue<CustomType>(const CustomType& v)
{
  return std::string("Custom(") + std::to_string(v.Value) + ")";
}

static void ExpectLast2(const char* enter, const char* leave)
{
  ASSERT_GE(Be->History.size(), 2u);
  EXPECT_EQ(Be->History[Be->History.size() - 2], enter);
  EXPECT_EQ(Be->History[Be->History.size() - 1], leave);
  printf("\n");
}

static int8_t ProcInt8()
{
  int8_t r = (int8_t)-1;
  LogmeP(r, CH);
  return r;
}

static uint8_t ProcUint8()
{
  uint8_t r = (uint8_t)200;
  LogmeP(r, CH);
  return r;
}

static Logme::xint8_t ProcXInt8()
{
  Logme::xint8_t r((int8_t)-1);
  LogmeP(r, CH);
  return r;
}

static Logme::xuint8_t ProcXUint8()
{
  Logme::xuint8_t r((uint8_t)0xAB);
  LogmeP(r, CH);
  return r;
}

static int16_t ProcInt16()
{
  int16_t r = (int16_t)-1234;
  LogmeP(r, CH);
  return r;
}

static uint16_t ProcUint16()
{
  uint16_t r = (uint16_t)60000;
  LogmeP(r, CH);
  return r;
}

static Logme::xuint16_t ProcXUint16()
{
  Logme::xuint16_t r((uint16_t)0xBEEF);
  LogmeP(r, CH);
  return r;
}

static int32_t ProcInt32()
{
  int32_t r = -123456;
  LogmeP(r, CH);
  return r;
}

static uint32_t ProcUint32()
{
  uint32_t r = 4000000000u;
  LogmeP(r, CH);
  return r;
}

static Logme::xuint32_t ProcXUint32()
{
  Logme::xuint32_t r(0xDEADBEEFu);
  LogmeP(r, CH);
  return r;
}

static int64_t ProcInt64()
{
  int64_t r = (int64_t)-1234567890123ll;
  LogmeP(r, CH);
  return r;
}

static uint64_t ProcUint64()
{
  uint64_t r = 1234567890123456789ull;
  LogmeP(r, CH);
  return r;
}

static Logme::xuint64_t ProcXUint64()
{
  Logme::xuint64_t r(0x1122334455667788ull);
  LogmeP(r, CH);
  return r;
}

static bool ProcBool()
{
  bool r = true;
  LogmeP(r, CH);
  return r;
}

static const char* ProcCStr()
{
  const char* r = "abc";
  LogmeP(r, CH);
  return r;
}

static std::string ProcString()
{
  std::string r = "str";
  LogmeP(r, CH);
  return r;
}

static MyEnum ProcEnum()
{
  MyEnum r = MyEnum::Two;
  LogmeP(r, CH);
  return r;
}

static CustomType ProcCustom()
{
  CustomType r{ 7 };
  LogmeP(r, CH);
  return r;
}

static void ProcVoid()
{
  LogmePV(CH);
}

static int ProcDebugLevel(bool& called)
{
  int r = 1;
  called = true;
  LogmePD(r, CH);
  return r;
}

TEST(ProcedurePrint, BuiltinAndHexAndCustom)
{
  Be->Clear();
  ProcInt8();
  ExpectLast2(">> ProcInt8()", "<< ProcInt8(): -1");

  Be->Clear();
  ProcUint8();
  ExpectLast2(">> ProcUint8()", "<< ProcUint8(): 200");

  Be->Clear();
  ProcXInt8();
  ExpectLast2(">> ProcXInt8()", "<< ProcXInt8(): 0xff");

  Be->Clear();
  ProcXUint8();
  ExpectLast2(">> ProcXUint8()", "<< ProcXUint8(): 0xab");

  Be->Clear();
  ProcInt16();
  ExpectLast2(">> ProcInt16()", "<< ProcInt16(): -1234");

  Be->Clear();
  ProcUint16();
  ExpectLast2(">> ProcUint16()", "<< ProcUint16(): 60000");

  Be->Clear();
  ProcXUint16();
  ExpectLast2(">> ProcXUint16()", "<< ProcXUint16(): 0xbeef");

  Be->Clear();
  ProcInt32();
  ExpectLast2(">> ProcInt32()", "<< ProcInt32(): -123456");

  Be->Clear();
  ProcUint32();
  ExpectLast2(">> ProcUint32()", "<< ProcUint32(): 4000000000");

  Be->Clear();
  ProcXUint32();
  ExpectLast2(">> ProcXUint32()", "<< ProcXUint32(): 0xdeadbeef");

  Be->Clear();
  ProcInt64();
  ExpectLast2(">> ProcInt64()", "<< ProcInt64(): -1234567890123");

  Be->Clear();
  ProcUint64();
  ExpectLast2(">> ProcUint64()", "<< ProcUint64(): 1234567890123456789");

  Be->Clear();
  ProcXUint64();
  ExpectLast2(">> ProcXUint64()", "<< ProcXUint64(): 0x1122334455667788");

  Be->Clear();
  ProcBool();
  ExpectLast2(">> ProcBool()", "<< ProcBool(): true");

  Be->Clear();
  ProcCStr();
  ExpectLast2(">> ProcCStr()", "<< ProcCStr(): abc");

  Be->Clear();
  ProcString();
  ExpectLast2(">> ProcString()", "<< ProcString(): str");

  Be->Clear();
  ProcEnum();
  ExpectLast2(">> ProcEnum()", "<< ProcEnum(): Two");

  Be->Clear();
  ProcCustom();
  ExpectLast2(">> ProcCustom()", "<< ProcCustom(): Custom(7)");

  Be->Clear();
  ProcVoid();
  ExpectLast2(">> ProcVoid()", "<< ProcVoid()");
}

TEST(ProcedurePrint, LevelFiltering)
{
  bool called = false;

  Be->Owner->SetFilterLevel(Logme::LEVEL_INFO);
  Be->Clear();
  ProcDebugLevel(called);
  EXPECT_TRUE(called);
  EXPECT_TRUE(Be->History.empty());

  Be->Owner->SetFilterLevel(Logme::LEVEL_DEBUG);
  called = false;
  Be->Clear();
  ProcDebugLevel(called);
  EXPECT_TRUE(called);
  ExpectLast2(">> ProcDebugLevel()", "<< ProcDebugLevel(): 1");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Logme::Instance->GetExistingChannel(CH);
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);

  Logme::OutputFlags flags;
  flags.Value = 0;
  ch->SetFlags(flags);

  ch->SetFilterLevel(Logme::LEVEL_DEBUG);

  return RUN_ALL_TESTS();
}
