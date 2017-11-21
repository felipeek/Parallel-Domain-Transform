#define HO_ARENA_IMPLEMENT
#include "Util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <memory_arena.h>
#include <iostream>
#include <chrono>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

#define ARENA_SIZE 1073741824
std::chrono::time_point<std::chrono::system_clock> aux;
Memory_Arena arena;
s32 arena_valid = 0;

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

extern void str_copy(s8* dst, s8* src)
{
	strcpy(dst, src);
}

extern void* alloc_memory(const s32 size)
{
	return malloc(size);
}

extern void dealloc_memory(void* ptr)
{
	return free(ptr);
}

extern void* alloc_arena_memory(const s32 size)
{
	if (!arena_valid)
	{
		arena_create(&arena, ARENA_SIZE);
		arena_valid = 1;
	}

	return arena_alloc(&arena, size);
}

extern void dealloc_arena_memory()
{
	arena_release(&arena);
}

extern Thread_Handler create_thread(THREAD_FUNCTION function, void* parameters)
{
#ifdef _WIN32
	return CreateThread(0, 0, (LPTHREAD_START_ROUTINE)function, parameters, 0, 0);
#elif defined(__linux__)
	pthread_t thread;
	pthread_create(&thread, 0, function, parameters);
	return thread;
#endif
}

extern s32 join_threads(s32 number_of_threads, Thread_Handler* threads_array, s32 wait_all)
{
#ifdef _WIN32
	return WaitForMultipleObjects(number_of_threads, threads_array, wait_all, INFINITE);
#elif defined(__linux__)
	for (s32 i = 0; i < number_of_threads; ++i)
		pthread_join(threads_array[i], 0);
#endif
	return 0;
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
	aux = std::chrono::system_clock::now();
}

extern r32 end_clock()
{
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end - aux;

	return diff.count();
}
