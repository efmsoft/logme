#include <Common/TestBackend.h>

#include <gtest/gtest.h>

#include <Logme/Logme.h>
#include <Logme/Procedure.h>
#include <Logme/ArgumentList.h>

using namespace Logme; 

ID CHT{ "procedure_print" };
std::shared_ptr<TestBackend> Be;
std::shared_ptr<TestBackend> Be2;

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

static void ExpectLast2Backend(const std::shared_ptr<TestBackend>& be, const char* enter, const char* leave)
{
  ASSERT_GE(be->History.size(), 2u);
  EXPECT_EQ(be->History[be->History.size() - 2], enter);
  EXPECT_EQ(be->History[be->History.size() - 1], leave);
}

static void ExpectLast2(const char* enter, const char* leave)
{
  ExpectLast2Backend(Be, enter, leave);
}

static int8_t ProcInt8()
{
  int8_t r = (int8_t)-1;
  LogmeP(r, CHT);
  return r;
}

static int8_t ProcInt8Default()
{
  int8_t r = (int8_t)-1;
  LogmeP(r);
  return r;
}

static uint8_t ProcUint8()
{
  uint8_t r = (uint8_t)200;
  LogmeP(r, CHT);
  return r;
}

static uint8_t ProcUint8Default()
{
  uint8_t r = (uint8_t)200;
  LogmeP(r);
  return r;
}

static xint8_t ProcXInt8()
{
  xint8_t r((int8_t)-1);
  LogmeP(r, CHT);
  return r;
}

static xint8_t ProcXInt8Default()
{
  xint8_t r((int8_t)-1);
  LogmeP(r);
  return r;
}

static xuint8_t ProcXUint8()
{
  xuint8_t r((uint8_t)0xAB);
  LogmeP(r, CHT);
  return r;
}

static xuint8_t ProcXUint8Default()
{
  xuint8_t r((uint8_t)0xAB);
  LogmeP(r);
  return r;
}

static int16_t ProcInt16()
{
  int16_t r = (int16_t)-1234;
  LogmeP(r, CHT);
  return r;
}

static int16_t ProcInt16Default()
{
  int16_t r = (int16_t)-1234;
  LogmeP(r);
  return r;
}

static uint16_t ProcUint16()
{
  uint16_t r = (uint16_t)60000;
  LogmeP(r, CHT);
  return r;
}

static uint16_t ProcUint16Default()
{
  uint16_t r = (uint16_t)60000;
  LogmeP(r);
  return r;
}

static xuint16_t ProcXUint16()
{
  xuint16_t r((uint16_t)0xBEEF);
  LogmeP(r, CHT);
  return r;
}

static xuint16_t ProcXUint16Default()
{
  xuint16_t r((uint16_t)0xBEEF);
  LogmeP(r);
  return r;
}

static int32_t ProcInt32()
{
  int32_t r = -123456;
  LogmeP(r, CHT);
  return r;
}

static int32_t ProcInt32Default()
{
  int32_t r = -123456;
  LogmeP(r);
  return r;
}

static uint32_t ProcUint32()
{
  uint32_t r = 4000000000u;
  LogmeP(r, CHT);
  return r;
}

static uint32_t ProcUint32Default()
{
  uint32_t r = 4000000000u;
  LogmeP(r);
  return r;
}

static xuint32_t ProcXUint32()
{
  xuint32_t r(0xDEADBEEFu);
  LogmeP(r, CHT);
  return r;
}

static xuint32_t ProcXUint32Default()
{
  xuint32_t r(0xDEADBEEFu);
  LogmeP(r);
  return r;
}

static int64_t ProcInt64()
{
  int64_t r = (int64_t)-1234567890123ll;
  LogmeP(r, CHT);
  return r;
}

static int64_t ProcInt64Default()
{
  int64_t r = (int64_t)-1234567890123ll;
  LogmeP(r);
  return r;
}

static uint64_t ProcUint64()
{
  uint64_t r = 1234567890123456789ull;
  LogmeP(r, CHT);
  return r;
}

static uint64_t ProcUint64Default()
{
  uint64_t r = 1234567890123456789ull;
  LogmeP(r);
  return r;
}

static xuint64_t ProcXUint64()
{
  xuint64_t r(0x1122334455667788ull);
  LogmeP(r, CHT);
  return r;
}

static xuint64_t ProcXUint64Default()
{
  xuint64_t r(0x1122334455667788ull);
  LogmeP(r);
  return r;
}

static bool ProcBool()
{
  bool r = true;
  LogmeP(r, CHT);
  return r;
}

static bool ProcBoolDefault()
{
  bool r = true;
  LogmeP(r);
  return r;
}

static const char* ProcCStr()
{
  const char* r = "abc";
  LogmeP(r, CHT);
  return r;
}

static const char* ProcCStrDefault()
{
  const char* r = "abc";
  LogmeP(r);
  return r;
}

static std::string ProcString()
{
  std::string r = "str";
  LogmeP(r, CHT);
  return r;
}

static std::string ProcStringDefault()
{
  std::string r = "str";
  LogmeP(r);
  return r;
}

static MyEnum ProcEnum()
{
  MyEnum r = MyEnum::Two;
  LogmeP(r, CHT);
  return r;
}

static MyEnum ProcEnumDefault()
{
  MyEnum r = MyEnum::Two;
  LogmeP(r);
  return r;
}

static CustomType ProcCustom()
{
  CustomType r{ 7 };
  LogmeP(r, CHT);
  return r;
}

static CustomType ProcCustomDefault()
{
  CustomType r{ 7 };
  LogmeP(r);
  return r;
}

static void ProcVoid()
{
  LogmePV(CHT);
}

static void ProcVoidDefault()
{
  LogmePV();
}

static int ProcDebugLevel(bool& called)
{
  int r = 1;
  called = true;
  LogmePD(r, CHT);
  return r;
}

static int ProcDebugLevelDefault(bool& called)
{
  int r = 1;
  called = true;
  LogmePD(r);
  return r;
}


static int ProcArgs2(xint64_t p1, const char* p2)
{
  int r = 7;
  LogmeP(r, CHT, ARGS2(p1, p2));
  return r;
}

static int ProcArgs2Default(xint64_t p1, const char* p2)
{
  int r = 7;
  LogmeP(r, ARGS2(p1, p2));
  return r;
}

static int ProcArgs3(int32_t a, uint16_t b, MyEnum c)
{
  int r = 123;
  LogmeP(r, CHT, ARGS3(a, b, c));
  return r;
}

static int ProcArgs3Default(int32_t a, uint16_t b, MyEnum c)
{
  int r = 123;
  LogmeP(r, ARGS3(a, b, c));
  return r;
}

static void ProcArgs4(bool a, const char* b, const std::string& c, const CustomType& d)
{
  LogmePV(CHT, ARGS4(a, b, c, d));
}

static void ProcArgs4Default(bool a, const char* b, const std::string& c, const CustomType& d)
{
  LogmePV(ARGS4(a, b, c, d));
}

static xuint32_t ProcArgs5(uint8_t a, xuint16_t b, int64_t c, const char* d, MyEnum e)
{
  xuint32_t r(0xAABBCCDDu);
  LogmeP(r, CHT, ARGS5(a, b, c, d, e));
  return r;
}

static xuint32_t ProcArgs5Default(uint8_t a, xuint16_t b, int64_t c, const char* d, MyEnum e)
{
  xuint32_t r(0xAABBCCDDu);
  LogmeP(r, ARGS5(a, b, c, d, e));
  return r;
}

static int ProcArgsUnnamed(xint64_t p1, const char* p2)
{
  int r = 5;
  LogmeP(r, CHT, ARGS(p1, p2));
  return r;
}

static int ProcArgsUnnamedDefault(xint64_t p1, const char* p2)
{
  int r = 5;
  LogmeP(r, ARGS(p1, p2));
  return r;
}

TEST(ProcedurePrint, BuiltinAndHexAndCustom)
{
  Be->Clear();
  ProcInt8();
  ExpectLast2(">> ProcInt8()", "<< ProcInt8(): -1");

  Be2->Clear();
  ProcInt8Default();
  ExpectLast2Backend(Be2, ">> ProcInt8Default()\n", "<< ProcInt8Default(): -1\n");

  Be->Clear();
  ProcUint8();
  ExpectLast2(">> ProcUint8()", "<< ProcUint8(): 200");

  Be2->Clear();
  ProcUint8Default();
  ExpectLast2Backend(Be2, ">> ProcUint8Default()\n", "<< ProcUint8Default(): 200\n");

  Be->Clear();
  ProcXInt8();
  ExpectLast2(">> ProcXInt8()", "<< ProcXInt8(): 0xff");

  Be2->Clear();
  ProcXInt8Default();
  ExpectLast2Backend(Be2, ">> ProcXInt8Default()\n", "<< ProcXInt8Default(): 0xff\n");

  Be->Clear();
  ProcXUint8();
  ExpectLast2(">> ProcXUint8()", "<< ProcXUint8(): 0xab");

  Be2->Clear();
  ProcXUint8Default();
  ExpectLast2Backend(Be2, ">> ProcXUint8Default()\n", "<< ProcXUint8Default(): 0xab\n");

  Be->Clear();
  ProcInt16();
  ExpectLast2(">> ProcInt16()", "<< ProcInt16(): -1234");

  Be2->Clear();
  ProcInt16Default();
  ExpectLast2Backend(Be2, ">> ProcInt16Default()\n", "<< ProcInt16Default(): -1234\n");

  Be->Clear();
  ProcUint16();
  ExpectLast2(">> ProcUint16()", "<< ProcUint16(): 60000");

  Be2->Clear();
  ProcUint16Default();
  ExpectLast2Backend(Be2, ">> ProcUint16Default()\n", "<< ProcUint16Default(): 60000\n");

  Be->Clear();
  ProcXUint16();
  ExpectLast2(">> ProcXUint16()", "<< ProcXUint16(): 0xbeef");

  Be2->Clear();
  ProcXUint16Default();
  ExpectLast2Backend(Be2, ">> ProcXUint16Default()\n", "<< ProcXUint16Default(): 0xbeef\n");

  Be->Clear();
  ProcInt32();
  ExpectLast2(">> ProcInt32()", "<< ProcInt32(): -123456");

  Be2->Clear();
  ProcInt32Default();
  ExpectLast2Backend(Be2, ">> ProcInt32Default()\n", "<< ProcInt32Default(): -123456\n");

  Be->Clear();
  ProcUint32();
  ExpectLast2(">> ProcUint32()", "<< ProcUint32(): 4000000000");

  Be2->Clear();
  ProcUint32Default();
  ExpectLast2Backend(Be2, ">> ProcUint32Default()\n", "<< ProcUint32Default(): 4000000000\n");

  Be->Clear();
  ProcXUint32();
  ExpectLast2(">> ProcXUint32()", "<< ProcXUint32(): 0xdeadbeef");

  Be2->Clear();
  ProcXUint32Default();
  ExpectLast2Backend(Be2, ">> ProcXUint32Default()\n", "<< ProcXUint32Default(): 0xdeadbeef\n");

  Be->Clear();
  ProcInt64();
  ExpectLast2(">> ProcInt64()", "<< ProcInt64(): -1234567890123");

  Be2->Clear();
  ProcInt64Default();
  ExpectLast2Backend(Be2, ">> ProcInt64Default()\n", "<< ProcInt64Default(): -1234567890123\n");

  Be->Clear();
  ProcUint64();
  ExpectLast2(">> ProcUint64()", "<< ProcUint64(): 1234567890123456789");

  Be2->Clear();
  ProcUint64Default();
  ExpectLast2Backend(Be2, ">> ProcUint64Default()\n", "<< ProcUint64Default(): 1234567890123456789\n");

  Be->Clear();
  ProcXUint64();
  ExpectLast2(">> ProcXUint64()", "<< ProcXUint64(): 0x1122334455667788");

  Be2->Clear();
  ProcXUint64Default();
  ExpectLast2Backend(Be2, ">> ProcXUint64Default()\n", "<< ProcXUint64Default(): 0x1122334455667788\n");

  Be->Clear();
  ProcBool();
  ExpectLast2(">> ProcBool()", "<< ProcBool(): true");

  Be2->Clear();
  ProcBoolDefault();
  ExpectLast2Backend(Be2, ">> ProcBoolDefault()\n", "<< ProcBoolDefault(): true\n");

  Be->Clear();
  ProcCStr();
  ExpectLast2(">> ProcCStr()", "<< ProcCStr(): abc");

  Be2->Clear();
  ProcCStrDefault();
  ExpectLast2Backend(Be2, ">> ProcCStrDefault()\n", "<< ProcCStrDefault(): abc\n");

  Be->Clear();
  ProcString();
  ExpectLast2(">> ProcString()", "<< ProcString(): str");

  Be2->Clear();
  ProcStringDefault();
  ExpectLast2Backend(Be2, ">> ProcStringDefault()\n", "<< ProcStringDefault(): str\n");

  Be->Clear();
  ProcEnum();
  ExpectLast2(">> ProcEnum()", "<< ProcEnum(): Two");

  Be2->Clear();
  ProcEnumDefault();
  ExpectLast2Backend(Be2, ">> ProcEnumDefault()\n", "<< ProcEnumDefault(): Two\n");

  Be->Clear();
  ProcCustom();
  ExpectLast2(">> ProcCustom()", "<< ProcCustom(): Custom(7)");

  Be2->Clear();
  ProcCustomDefault();
  ExpectLast2Backend(Be2, ">> ProcCustomDefault()\n", "<< ProcCustomDefault(): Custom(7)\n");

  Be->Clear();
  ProcVoid();
  ExpectLast2(">> ProcVoid()", "<< ProcVoid()");

  Be2->Clear();
  ProcVoidDefault();
  ExpectLast2Backend(Be2, ">> ProcVoidDefault()\n", "<< ProcVoidDefault()\n");

  Be->Clear();
  ProcArgs2((xint64_t)0x2A, "qq");
  ExpectLast2(">> ProcArgs2(): p1=0x2a, p2=qq", "<< ProcArgs2(): 7");

  Be2->Clear();
  ProcArgs2Default((xint64_t)0x2A, "qq");
  ExpectLast2Backend(Be2, ">> ProcArgs2Default(): p1=0x2a, p2=qq\n", "<< ProcArgs2Default(): 7\n");

  Be->Clear();
  ProcArgs3(-10, (uint16_t)60000, MyEnum::Two);
  ExpectLast2(">> ProcArgs3(): a=-10, b=60000, c=Two", "<< ProcArgs3(): 123");

  Be2->Clear();
  ProcArgs3Default(-10, (uint16_t)60000, MyEnum::Two);
  ExpectLast2Backend(Be2, ">> ProcArgs3Default(): a=-10, b=60000, c=Two\n", "<< ProcArgs3Default(): 123\n");

  Be->Clear();
  ProcArgs4(true, "xx", std::string("yy"), CustomType{7});
  ExpectLast2(">> ProcArgs4(): a=true, b=xx, c=yy, d=Custom(7)", "<< ProcArgs4()");

  Be2->Clear();
  ProcArgs4Default(true, "xx", std::string("yy"), CustomType{7});
  ExpectLast2Backend(Be2, ">> ProcArgs4Default(): a=true, b=xx, c=yy, d=Custom(7)\n", "<< ProcArgs4Default()\n");

  Be->Clear();
  ProcArgs5((uint8_t)200, (xuint16_t)0xBEEF, -5, "zz", MyEnum::One);
  ExpectLast2(">> ProcArgs5(): a=200, b=0xbeef, c=-5, d=zz, e=One", "<< ProcArgs5(): 0xaabbccdd");

  Be2->Clear();
  ProcArgs5Default((uint8_t)200, (xuint16_t)0xBEEF, -5, "zz", MyEnum::One);
  ExpectLast2Backend(Be2, ">> ProcArgs5Default(): a=200, b=0xbeef, c=-5, d=zz, e=One\n", "<< ProcArgs5Default(): 0xaabbccdd\n");

  Be->Clear();
  ProcArgsUnnamed((xint64_t)0x2A, "qq");
  ExpectLast2(">> ProcArgsUnnamed(): 0x2a, qq", "<< ProcArgsUnnamed(): 5");

  Be2->Clear();
  ProcArgsUnnamedDefault((xint64_t)0x2A, "qq");
  ExpectLast2Backend(Be2, ">> ProcArgsUnnamedDefault(): 0x2a, qq\n", "<< ProcArgsUnnamedDefault(): 5\n");
}

TEST(ProcedurePrint, LevelFiltering)
{
  bool called = false;

  Be->Owner->SetFilterLevel(LEVEL_INFO);
  Be->Clear();
  ProcDebugLevel(called);
  EXPECT_TRUE(called);
  EXPECT_TRUE(Be->History.empty());

  Be->Owner->SetFilterLevel(LEVEL_DEBUG);
  called = false;
  Be->Clear();
  ProcDebugLevel(called);
  EXPECT_TRUE(called);
  ExpectLast2(">> ProcDebugLevel()", "<< ProcDebugLevel(): 1");

  Be2->Owner->SetFilterLevel(LEVEL_INFO);
  called = false;
  Be2->Clear();
  ProcDebugLevelDefault(called);
  EXPECT_TRUE(called);
  EXPECT_TRUE(Be2->History.empty());

  Be2->Owner->SetFilterLevel(LEVEL_DEBUG);
  called = false;
  Be2->Clear();
  ProcDebugLevelDefault(called);
  EXPECT_TRUE(called);
  ExpectLast2Backend(Be2, ">> ProcDebugLevelDefault()\n", "<< ProcDebugLevelDefault(): 1\n");
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  auto ch = Instance->CreateChannel(CHT);
  Be = std::make_shared<TestBackend>(ch);
  ch->AddBackend(Be);
  ch->AddLink(::CH);

  auto ch2 = Instance->GetExistingChannel(::CH);
  Be2 = std::make_shared<TestBackend>(ch2);
  ch2->AddBackend(Be2);

  OutputFlags flags;
  flags.Value = 0;

  ch->SetFlags(flags);
  ch->SetFilterLevel(LEVEL_DEBUG);

  flags.Eol = true; 
  ch2->SetFlags(flags);
  ch2->SetFilterLevel(LEVEL_DEBUG);

  return RUN_ALL_TESTS();
}
