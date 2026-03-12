#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>

#pragma once;


using namespace std;


//Transpose
float* transpose(float* mat, int x, int y);

//Simple MMul
float* mmul(float* mat1, float* mat2, int x1, int y1, int x2, int y2);

//Simple Madd
float* madd(float* mat1, float* mat2, int x, int y);

//Mprint 
void matprint(float* mat, int x, int y);


//Mean + Std
float mean(float* mat, int len);

float stdev(float* mat, int len);
