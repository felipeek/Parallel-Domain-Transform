#include "Mathematics.h"

extern void mat4_multiply(Mat4* m1, Mat4* m2, Mat4* result)
{
	result->m[0][0] = m1->m[0][0] * m2->m[0][0] + m1->m[0][1] * m2->m[1][0] + m1->m[0][2] * m2->m[2][0] + m1->m[0][3] * m2->m[3][0];
	result->m[0][1] = m1->m[0][0] * m2->m[0][1] + m1->m[0][1] * m2->m[1][1] + m1->m[0][2] * m2->m[2][1] + m1->m[0][3] * m2->m[3][1];
	result->m[0][2] = m1->m[0][0] * m2->m[0][2] + m1->m[0][2] * m2->m[1][2] + m1->m[0][2] * m2->m[2][2] + m1->m[0][3] * m2->m[3][2];
	result->m[0][3] = m1->m[0][0] * m2->m[0][3] + m1->m[0][3] * m2->m[1][3] + m1->m[0][2] * m2->m[2][3] + m1->m[0][3] * m2->m[3][3];

	result->m[1][0] = m1->m[1][0] * m2->m[0][0] + m1->m[1][1] * m2->m[1][0] + m1->m[1][2] * m2->m[2][0] + m1->m[1][3] * m2->m[3][0];
	result->m[1][1] = m1->m[1][0] * m2->m[0][1] + m1->m[1][1] * m2->m[1][1] + m1->m[1][2] * m2->m[2][1] + m1->m[1][3] * m2->m[3][1];
	result->m[1][2] = m1->m[1][0] * m2->m[0][2] + m1->m[1][2] * m2->m[1][2] + m1->m[1][2] * m2->m[2][2] + m1->m[1][3] * m2->m[3][2];
	result->m[1][3] = m1->m[1][0] * m2->m[0][3] + m1->m[1][3] * m2->m[1][3] + m1->m[1][2] * m2->m[2][3] + m1->m[1][3] * m2->m[3][3];

	result->m[2][0] = m1->m[2][0] * m2->m[0][0] + m1->m[2][1] * m2->m[1][0] + m1->m[2][2] * m2->m[2][0] + m1->m[2][3] * m2->m[3][0];
	result->m[2][1] = m1->m[2][0] * m2->m[0][1] + m1->m[2][1] * m2->m[1][1] + m1->m[2][2] * m2->m[2][1] + m1->m[2][3] * m2->m[3][1];
	result->m[2][2] = m1->m[2][0] * m2->m[0][2] + m1->m[2][2] * m2->m[1][2] + m1->m[2][2] * m2->m[2][2] + m1->m[2][3] * m2->m[3][2];
	result->m[2][3] = m1->m[2][0] * m2->m[0][3] + m1->m[2][3] * m2->m[1][3] + m1->m[2][2] * m2->m[2][3] + m1->m[2][3] * m2->m[3][3];

	result->m[3][0] = m1->m[3][0] * m2->m[0][0] + m1->m[3][1] * m2->m[1][0] + m1->m[3][2] * m2->m[2][0] + m1->m[3][3] * m2->m[3][0];
	result->m[3][1] = m1->m[3][0] * m2->m[0][1] + m1->m[3][1] * m2->m[1][1] + m1->m[3][2] * m2->m[2][1] + m1->m[3][3] * m2->m[3][1];
	result->m[3][2] = m1->m[3][0] * m2->m[0][2] + m1->m[3][2] * m2->m[1][2] + m1->m[3][2] * m2->m[2][2] + m1->m[3][3] * m2->m[3][2];
	result->m[3][3] = m1->m[3][0] * m2->m[0][3] + m1->m[3][3] * m2->m[1][3] + m1->m[3][2] * m2->m[2][3] + m1->m[3][3] * m2->m[3][3];
}

extern void mat4_transpose(Mat4* matrix, Mat4* result)
{
	result->m[0][0] = matrix->m[0][0];
	result->m[0][1] = matrix->m[1][0];
	result->m[0][2] = matrix->m[2][0];
	result->m[0][3] = matrix->m[3][0];
	result->m[1][0] = matrix->m[0][1];
	result->m[1][1] = matrix->m[1][1];
	result->m[1][2] = matrix->m[2][1];
	result->m[1][3] = matrix->m[3][1];
	result->m[2][0] = matrix->m[0][2];
	result->m[2][1] = matrix->m[1][2];
	result->m[2][2] = matrix->m[2][2];
	result->m[2][3] = matrix->m[3][2];
	result->m[3][0] = matrix->m[0][3];
	result->m[3][1] = matrix->m[1][3];
	result->m[3][2] = matrix->m[2][3];
	result->m[3][3] = matrix->m[3][3];
}