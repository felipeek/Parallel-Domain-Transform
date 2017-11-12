#include "Util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

clock_t aux;

extern void print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

extern s32 str_length(const s8* str)
{
	s32 len = 0;

	while (str[len]) ++len;

	return len;
}

extern r32 str_to_r32(const s8* str)
{
	return (r32)atof(str);
}

extern s32 str_to_s32(const s8* str)
{
	return atoi(str);
}

extern void* alloc_memory(const s32 size)
{
	return malloc(size);
}

extern void dealloc_memory(void* ptr)
{
	return free(ptr);
}

extern void* create_thread(THREAD_FUNCTION function, void* parameters)
{
#ifdef _WIN32
	return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)function, parameters, 0, 0);
#else
	return 0;
#endif
}

extern s32 join_threads(s32 number_of_threads, void** threads_array, s32 wait_all)
{
#ifdef _WIN32
	return WaitForMultipleObjects(number_of_threads, threads_array, wait_all, INFINITE);
#else
	return -1;
#endif
}

extern r32 absolute(r32 x)
{
	return (x < 0) ? -x : x;
}

extern r32 r32_pow(r32 b, r32 e)
{
	return powf(b, e);
}

extern r32 r32_sqrt(r32 v)
{
	return sqrtf(v);
}

extern r32 r32_exp(r32 v)
{
	return expf(v);
}

extern s32 r32_round(r32 value)
{
	return (s32)round(value);
}

extern void* copy_memory(void* destination, const void* source, s32 num)
{
	return memcpy(destination, source, num);
}

extern void start_clock()
{
	aux = clock();
}

extern r32 end_clock()
{
	clock_t end = clock();
	return (r32)(end - aux) / CLOCKS_PER_SEC;
}