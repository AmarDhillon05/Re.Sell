#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>


using namespace std;

//Transpose
float* transpose(float* mat, int x, int y){
    float* tf = (float*)malloc(x * y * sizeof(float));

    for(int i = 0; i < x; i ++){
        for(int j = 0; j < y; j ++){
            tf[j * y + i] = mat[i * x + j];
        }
    }

    return tf;
}

//Simple MMul
float* mmul(float* mat1, float* mat2, int x1, int y1, int x2, int y2){
    if(y1 != x2){
        throw invalid_argument("Matrix dimensions " + to_string(x2) + " and " + to_string(y1) + " do not match");
    }

    float* tf = (float*)malloc(x1 * y2 * sizeof(float));

    for(int i = 0; i < x1; i++){
        for(int j = 0; j < y2; j++){
            float sum = 0;
            for(int offset = 0; offset < y1; offset++){
                sum += mat1[i * x1 + offset] * mat2[offset * x2 + j];

            }
            tf[i * x1 + j] = sum;
        }
    }

    return tf;
}

//Simple Madd
float* madd(float* mat1, float* mat2, int x, int y){

    float *tf = (float*)malloc(x * y * sizeof(float));

    for(int i = 0; i < x; i ++){
        for(int j = 0; j < x; j ++){
            tf[i * x + j] = mat1[i * x + j] + mat2[i * x + j];
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
            cout << mat[i * x + j] << ", ";
        }
        cout << "], " << endl;
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
    float det = 1 / (a*d - b*c);
    inv[0] = det * d;
    inv[1] = det * b * -1;
    inv[2] = det * c * -1;
    inv[3] = det * a;

    return inv;
}



//Mean + Std
float mean(float* mat, int len){
    float sum = 0;
    for(int i = 0; i < len; i ++){ sum += mat[i]; }
    return sum;
}

float stdev(float* mat, int len){
    float mu = mean(mat, len);
    float theta = 0;
    for(int i = 0; i < len; i ++){
        float diff = mat[i] - mu;
        theta += diff * diff;
    }
    return theta;
}

/*

int main(){

    float m1[] = {2, 0, 1, 3, 5, 6};
    float* m1trans = transpose(m1, 2, 3);
    matprint(m1, 2, 3);
    matprint(m1trans, 3, 2);
    
    float* prod = mmul(m1, m1trans, 2, 3, 3, 2);
    matprint(prod, 2, 2);

    float x2[] = {2, -1, 9, -4};
    float* add = madd(prod, x2, 2, 2);
    matprint(add, 2, 2);

    free(prod);
    free(m1trans);

    return 0;
}


*/