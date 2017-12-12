#pragma once
#include "Input.h"

typedef struct Input_Data_Struct Input_Data;

struct Input_Data_Struct
{
	r32 mx;					// mouse x position
	r32 my;					// mouse y position
	s32 mclicked;			// mouse clicked
};

extern Input_Data input_data;