#pragma once

#include "AEEStdLib.h"

class kpdebug {
public:
/*#define ASSERT_EQUAL(value, control) \
if ( value == control) \
{ \
kpdebug_assert("ASSERT: %s", "##value != control##", 0); \
kpdebug_assert("FILE %s", FILENAME (or FILE_NAME or FILE I dun remember), 0); \
kpdebug_assert("LINE %llu", LINE); \
}*/
#define ASSERT(condition, assert) if (condition) { DBGPRINTF("ASSERT: %s", assert, 0); DBGPRINTF("FILE %s", __FILE__, 0); DBGPRINTF("LINE %llu", __LINE__); }

	static void Assert(const char* format, const char* string, int a3);
	static void AssertLine(const char* format, int params, ...);
	static void nullfunc2(class kphandset* instance, const char* format, const char* string, int num);
	static void nullfunc3(class kphandset* instance, const char* format, int num, ...);
	static void nullfunc4(class kphandset* instance, const char* format, ...);

	static void Print(char* print);
	static void DBGPrintF(IApplet* ptr, char* print);
};