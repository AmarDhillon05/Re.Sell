#pragma once

#include <algorithm>


//Raw sequential utils
//inline: these live in a header, so without it a second includer breaks the link


inline float maxof(const float* arr, int dsize){
    if(dsize <= 0){ return 0.0f; }
    float m = arr[0];
    for(int i = 1; i < dsize; i ++){ m = std::max(arr[i], m); }
    return m;
}

inline float minof(const float* arr, int dsize){
    if(dsize <= 0){ return 0.0f; }
    float m = arr[0];
    for(int i = 1; i < dsize; i ++){ m = std::min(arr[i], m); }
    return m;
}
