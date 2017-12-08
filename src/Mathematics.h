#pragma once
#include "Common.h"

typedef struct Vec2_Struct Vec2;
typedef struct Vec3_Struct Vec3;
typedef struct Vec4_Struct Vec4;
typedef struct Mat3_Struct Mat3;
typedef struct Mat4_Struct Mat4;

struct Vec2_Struct
{
	r32 x, y;
};

struct Vec3_Struct
{
	r32 x, y, z;
};

struct Vec4_Struct
{
	r32 x, y, z, w;
};

struct Mat3_Struct
{
	r32 m[3][3];
};

struct Mat4_Struct
{
	r32 m[4][4];
};

extern void mat4_multiply(Mat4* m1, Mat4* m2, Mat4* result);
extern void mat4_transpose(Mat4* matrix, Mat4* result);