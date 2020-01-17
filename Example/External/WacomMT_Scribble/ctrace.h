// CTrace.h - header file for trace output
//
// COPYRIGHT
//		Copyright WACOM Technologies, Inc. 1996 - 2011
//		All rights reserved.

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <fstream>
#include <iomanip>

extern std::ofstream gDebugFileStream;

//////////////////////////////////////////////////////////////////////////////
inline int WacTraceD(const char *fmt_I, ...)
{
	va_list args;
	va_start(args, fmt_I);
	int len = _vscprintf(fmt_I, args) + 1;
	if (len)
	{
		char *buffer = (char*)malloc(len * sizeof(char));
		int res = vsprintf_s(buffer, len, fmt_I, args);
		va_end(args);
		OutputDebugStringA(buffer);
#if defined(GLOBAL_WACOM_DEBUG_FILE) // {
		if (!GLOBAL_WACOM_DEBUG_FILE.empty())
		{
#if defined(GLOBAL_WACOM_DEBUG_FILE_SEMAPHORE) // {
			STSemaphoreLock inUse(GLOBAL_WACOM_DEBUG_FILE_SEMAPHORE);
#endif // }
			if (!gDebugFileStream.is_open())
			{
				gDebugFileStream.open(GLOBAL_WACOM_DEBUG_FILE.c_str(), std::ios::app);
			}
			if (gDebugFileStream.is_open())
			{
				gDebugFileStream	<< std::setw(5) << ::GetTickCount()
										<< " -- "
										<< std::setw(5) << ::GetCurrentProcessId()
										<< "//"
										<< std::setw(5) << ::GetCurrentThreadId()
										<< " -- "
										<< buffer << std::flush;
			}
		}
#endif // }
		free(buffer);
		return res;
	}
	return -1;
}

#define WacTrace(...) WacTraceD(__VA_ARGS__)
