#pragma once
#include "Common.h"

#ifdef __linux__
#include <pthread.h>
typedef pthread_t Thread_Handler;
typedef void* Thread_Proc_Return_Type;
#define _stdcall
#elif defined(_WIN32)
typedef void* Thread_Handler;
typedef s32 Thread_Proc_Return_Type;
#endif

typedef Thread_Proc_Return_Type (_stdcall *THREAD_FUNCTION)(void*);

extern void print(const char *fmt, ...);
void* memory_set(void* _Dst, s32 _Val, u32 _Size);
extern s32 str_length(const s8* str);
extern r32 str_to_r32(const s8* str);
extern s32 str_to_s32(const s8* str);
extern void str_copy(s8* dst, s8* src);
extern void* alloc_memory(const s64 size);
extern void dealloc_memory(void* ptr);
extern void* alloc_arena_memory(const s32 size);
extern void dealloc_arena_memory();
extern Thread_Handler create_thread(THREAD_FUNCTION function, void* parameters);
extern s32 join_threads(s32 number_of_threads, Thread_Handler* threads_array, s32 wait_all);
extern r32 absolute(r32 x);
extern r32 r32_pow(r32 b, r32 e);
extern r32 r32_sqrt(r32 v);
extern r32 r32_exp(r32 v);
extern s32 r32_round(r32 value);
extern r32 r32_clamp(r32 value, r32 min, r32 max);
extern void* copy_memory(void* destination, const void* source, s32 num);
extern void start_clock();
extern r32 end_clock();
extern void store_image(const s8* result_path,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	const r32* image_bytes);
extern r32* load_image(const s8* image_path,
	s32* image_width,
	s32* image_height,
	s32* image_channels,
	s32 desired_channels);
extern u8* read_file(u8* filename, s64* out_size);
extern void free_file(u8* file);