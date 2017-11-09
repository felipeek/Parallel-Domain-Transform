#pragma once
#include "Common.h"

extern void print(const char *fmt, ...);
extern s32 str_length(const s8* str);
extern r32 str_to_r32(const s8* str);
extern s32 str_to_s32(const s8* str);
extern void* alloc_memory(const s32 size);
extern void dealloc_memory(void* ptr);