#pragma once

#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>

using namespace std;

//Convention: every matrix is row-major with `x` rows and `y` columns,
//so element (i, j) lives at mat[i * y + j].

//Transpose (x by y -> y by x)
float* transpose(float* mat, int x, int y);

//Simple MMul
float* mmul(float* mat1, float* mat2, int x1, int y1, int x2, int y2);

//Simple Madd
float* madd(float* mat1, float* mat2, int x, int y);

//Mprint
void matprint(float* mat, int x, int y);

//Ewise mult
void emul(float* mat, int tsize, float el);

//2D inverse (for simplicity)
float* twodinv(float* mat);


//Mean + Std
float mean(float* mat, int len);

float stdev(float* mat, int len);
