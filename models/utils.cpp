#include "utils.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>

using namespace std;

//Transpose
float* transpose(float* mat, int x, int y){
    float* tf = (float*)malloc(x * y * sizeof(float));

    for(int i = 0; i < x; i ++){
        for(int j = 0; j < y; j ++){
            //(i, j) of a x-by-y matrix becomes (j, i) of a y-by-x matrix
            tf[j * x + i] = mat[i * y + j];
        }
    }

    return tf;
}

//Simple MMul
float* mmul(float* mat1, float* mat2, int x1, int y1, int x2, int y2){
    if(y1 != x2){
        throw invalid_argument("Matrix dimensions " + to_string(y1) + " and " + to_string(x2) + " do not match");
    }

    float* tf = (float*)malloc(x1 * y2 * sizeof(float));

    for(int i = 0; i < x1; i++){
        for(int j = 0; j < y2; j++){
            float sum = 0;
            for(int offset = 0; offset < y1; offset++){
                sum += mat1[i * y1 + offset] * mat2[offset * y2 + j];
            }
            tf[i * y2 + j] = sum;
        }
    }

    return tf;
}

//Simple Madd
float* madd(float* mat1, float* mat2, int x, int y){

    float *tf = (float*)malloc(x * y * sizeof(float));

    for(int i = 0; i < x; i ++){
        for(int j = 0; j < y; j ++){
            tf[i * y + j] = mat1[i * y + j] + mat2[i * y + j];
        }
    }

    return tf;
}

//Print matrix
void matprint(float* mat, int x, int y){
    cout << "[" << endl;
    for(int i = 0; i < x; i ++){
        cout << "   [";
        for(int j = 0; j < y; j ++){
            cout << mat[i * y + j];
            if(j + 1 < y){ cout << ", "; }
        }
        cout << "]," << endl;
    }
    cout << "]" << endl;
}

//Element wise mul
void emul(float* mat, int tsize, float el){
    for(int i = 0; i < tsize; i ++){
        mat[i] *= el;
    }
}


//2d inv
float* twodinv(float* mat){
    float *inv = (float*)malloc(4 * sizeof(float));

    float a = mat[0]; float b = mat[1]; float c = mat[2]; float d = mat[3];
    float det = a*d - b*c;
    if(det == 0.0f){
        free(inv);
        throw invalid_argument("2x2 matrix is singular and cannot be inverted");
    }

    float invdet = 1.0f / det;
    inv[0] = invdet * d;
    inv[1] = invdet * b * -1;
    inv[2] = invdet * c * -1;
    inv[3] = invdet * a;

    return inv;
}



//Mean + Std
float mean(float* mat, int len){
    if(len <= 0){ return 0.0f; }

    float sum = 0;
    for(int i = 0; i < len; i ++){ sum += mat[i]; }
    return sum / (float)len;
}

float stdev(float* mat, int len){
    if(len <= 1){ return 0.0f; }

    float mu = mean(mat, len);
    float theta = 0;
    for(int i = 0; i < len; i ++){
        float diff = mat[i] - mu;
        theta += diff * diff;
    }

    //Sample standard deviation (Bessel corrected), not the raw sum of squares
    return sqrt(theta / (float)(len - 1));
}
