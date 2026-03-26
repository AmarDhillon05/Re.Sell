#include <algorithm>


//Raw sequential utils



float maxof(float* arr, int dsize){
    float m = arr[0];
    for(int i = 0; i < dsize; i ++){ m = std::max(arr[i], m); }
    return m;
}

float minof(float* arr, int dsize){
    float m = arr[0];
    for(int i = 0; i < dsize; i ++){ m = std::min(arr[i], m); }
    return m;
}