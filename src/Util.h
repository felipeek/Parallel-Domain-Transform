#pragma once
#include "Common.h"

typedef s32 (__stdcall *THREAD_FUNCTION)(void*);

extern void print(const char *fmt, ...);
extern s32 str_length(const s8* str);
extern r32 str_to_r32(const s8* str);
extern s32 str_to_s32(const s8* str);
extern void* alloc_memory(const s32 size);
extern void dealloc_memory(void* ptr);
extern void* create_thread(THREAD_FUNCTION function, void* parameters);
extern s32 join_threads(s32 number_of_threads, void** threads_array, s32 wait_all);
extern r32 absolute(r32 x);
extern r32 r32_pow(r32 b, r32 e);
extern r32 r32_sqrt(r32 v);
extern r32 r32_exp(r32 v);
extern s32 r32_round(r32 value);
extern void* copy_memory(void* destination, const void* source, s32 num);
extern void start_clock();
extern r32 end_clock();