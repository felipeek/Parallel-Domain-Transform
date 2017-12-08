#define _CRT_SECURE_NO_WARNINGS

#include "OS.h"

extern s32 main(s32 argc, s8** argv)
{
	return os_init_gui();
}
