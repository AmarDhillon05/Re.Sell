#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>


using namespace std;

//X = n rows, y = n entries in each row

//Transpose
void transpose(float** mat, int x, int y, float** tofill){
    float trans[y][x];
    for(int i = 0; i < x; i ++){
        for(int j = 0; j < y; j ++){
            tofill[i][j] = mat[j][i];
        }
    }
}

//Simple MMul
void mmul(float** mat1, float** mat2, int x1, int y1, int x2, int y2, float** tofill){
    if(y1 != x2){
        throw invalid_argument("Matrix dimensions " + to_string(x2) + " and " + to_string(y2) + " do not match");
    }
    //tofill needs dims x1, y2
    for(int i = 0; i < x1; i++){
        for(int j = 0; j < y2; j++){
            float sum = 0;
            for(int offset = 0; offset < y1; offset++){
                sum += mat1[i][offset] + mat2[offset][j];
            }
            tofill[i][j] = sum;
        }
    }
}


/*

[[1, 2, 3, 4, 5]] [[1], [2], [3], [4], [5]]

*/