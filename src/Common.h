#pragma once
#define intern static

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float r32;
typedef double r64;

typedef struct Struct_Block Block;
typedef struct Struct_R32_Color R32_Color;
typedef struct Struct_Image_Information Image_Information;

struct Struct_Block
{
	s32 x;
	s32 y;
	s32 width;
	s32 height;
};

struct Struct_R32_Color
{
	r32 r;
	r32 g;
	r32 b;
};

struct Struct_Image_Information
{
	s32 width;
	s32 height;
	s32 channels;
};