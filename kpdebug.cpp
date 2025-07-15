#include "kpdebug.h"
#include "kphandset.h"

void kpdebug::Assert(const char* format, const char* string, int a3)
{
	const char* assertString = "<null>";
	if (string)
		assertString = string;
	kpdebug::nullfunc2((kphandset*)GETAPPINSTANCE(), format, string, a3);
}
void kpdebug::AssertLine(const char* format, int params, ...)
{
	kpdebug::nullfunc3((kphandset*)GETAPPINSTANCE(), format, params);
}
void kpdebug::nullfunc2(kphandset* instance, const char* format, const char* string, int num)
{
#if defined(AEE_SIMULATOR)
	DBGPRINTF(format, string, num);
#endif
}
void kpdebug::nullfunc3(kphandset* instance, const char* format, int num, ...)
{
#if defined(AEE_SIMULATOR)
	va_list vargs;
	va_start(vargs, format);
	DBGPRINTF(format, vargs);
	va_end(vargs);
#endif
}
void kpdebug::nullfunc4(kphandset* instance, const char* format, ...)
{
#if defined(AEE_SIMULATOR)
	va_list vargs;
	va_start(vargs, format);
	DBGPRINTF(format, vargs);
	va_end(vargs);
#endif
}
void kpdebug::DBGPrintF(IApplet* ptr, char* print)
{
	if (print)
	{
		DBGPRINTF(print); //Line 27 - //DBGPRINTF("*dbgprintf-%d* %s:%d", 4, "kpdebug.cpp", 27); //DBGPRINTF automatically adds this
	}
}

void kpdebug::Print(char* print)
{
	kpdebug::DBGPrintF(GETAPPINSTANCE(), print);
}

extern "C" void __aeabi_unwind_cpp_pr0() //Temp
{}

extern "C" void __aeabi_unwind_cpp_pr1() //Temp
{}