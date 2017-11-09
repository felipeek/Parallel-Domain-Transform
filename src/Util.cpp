#include "Util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

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