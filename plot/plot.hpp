#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <unordered_map>

using namespace std;

//Pair hash for points 
struct pair_hash {
    size_t operator()(const std::pair<int,int>& p) const {
        return p.first * 31 + p.second;
    }
};

//Plotbot blueprint
class Plotbot{

    private:
        int xsz;
        int ysz;
        int ymin; 
        int ymax;

        int realPtAmt;
        int spaceTaken;

        float* points;

    public:

        Plotbot(int xsz, int ysz);
        ~Plotbot();
        void append(float* data, int dsize);
        void plot();
};
