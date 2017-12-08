#define HO_ARENA_IMPLEMENT
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define DEFAULT_IMAGE_CHANNELS 4

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
#include <stb_image.h>
#include <stb_image_write.h>

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

extern void* alloc_memory(const s64 size)
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

extern r32 r32_clamp(r32 value, r32 min, r32 max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
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

	return (r32)diff.count();
}

extern r32* load_image(const s8* image_path,
	s32* image_width,
	s32* image_height,
	s32* image_channels,
	s32 desired_channels)
{
	u8* auxiliary_data = stbi_load(image_path, image_width, image_height, image_channels, DEFAULT_IMAGE_CHANNELS);

	if (auxiliary_data == 0)
		return 0;

	// @TODO: check WHY this is happening
	*image_channels = 4;

	r32* image_data = (r32*)alloc_memory(sizeof(r32) * *image_height * *image_width * *image_channels);

	for (s32 i = 0; i < *image_height; ++i)
	{
		for (s32 j = 0; j < *image_width; ++j)
		{
			image_data[i * *image_width * *image_channels + j * *image_channels] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 1] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels + 1] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 2] =
				auxiliary_data[i * *image_width * *image_channels + j * *image_channels + 2] / 255.0f;
			image_data[i * *image_width * *image_channels + j * *image_channels + 3] = 1.0f;
		}
	}

	stbi_image_free(auxiliary_data);

	return image_data;
}

extern void store_image(const s8* result_path,
	s32 image_width,
	s32 image_height,
	s32 image_channels,
	const r32* image_bytes)
{
	u8* auxiliary_data = (u8*)alloc_memory(image_height * image_width * sizeof(u8) * image_channels);

	for (s32 i = 0; i < image_height; ++i)
	{
		for (s32 j = 0; j < image_width; ++j)
		{
			auxiliary_data[i * image_width * image_channels + j * image_channels] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 1] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels + 1]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 2] = (u8)r32_round(255.0f * image_bytes[i * image_width * image_channels + j * image_channels + 2]);
			auxiliary_data[i * image_width * image_channels + j * image_channels + 3] = 255;
		}
	}

	stbi_write_png(result_path, image_width, image_height, image_channels, auxiliary_data, image_width * image_channels);
	dealloc_memory(auxiliary_data);
}

#ifdef _WIN32
extern u8* read_file(u8* filename, s64* out_size)
{
	/* get file size */
	WIN32_FIND_DATA info;
	HANDLE search_handle = FindFirstFileEx((LPCSTR)filename, FindExInfoStandard, &info, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
	if (search_handle == INVALID_HANDLE_VALUE) return 0;
	FindClose(search_handle);
	u64 file_size = (u64)info.nFileSizeLow | ((u64)info.nFileSizeHigh << 32);
	if (out_size) *out_size = file_size;
	/* open file */
	HANDLE fhandle = CreateFile((LPCSTR)filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	s32 error = GetLastError();

	u64 maxs32 = 0x7fffffff;
	DWORD bytes_read = 0;
	if (file_size == 0) return 0;    // error file is empty
	void* memory = alloc_memory(file_size + sizeof(u8));
	if (INVALID_HANDLE_VALUE == fhandle) return 0;

	if (file_size > maxs32) {
		void* mem_aux = memory;
		s64 num_reads = file_size / maxs32;
		s64 rest = file_size % maxs32;
		DWORD bytes_read;
		for (s64 i = 0; i < num_reads; ++i) {
			ReadFile(fhandle, mem_aux, (DWORD)maxs32, &bytes_read, 0);
			mem_aux = (s8*)mem_aux + maxs32;
		}
		ReadFile(fhandle, mem_aux, (DWORD)rest, &bytes_read, 0);
	}
	else {
		ReadFile(fhandle, memory, (DWORD)file_size, &bytes_read, 0);
	}
	CloseHandle(fhandle);
	((u8*)memory)[file_size] = 0;
	return (u8*)memory;
}
#elif defined(__linux__)
#error "read_file not implemented for linux"
#endif

#ifdef _WIN32
extern void free_file(u8* file)
{
	dealloc_memory(file);
}
#elif define(__linux__)
#error "free_file not implemented for linux"
#endif