#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>

using namespace std;

//Plotbot blueprint
class Plotbot{

    private:
        char* data;

        int xsz;
        int ysz;
        int ymin; 
        int ymax;

        int sizeused;


    public:

        Plotbot(int xsz, int ysz);
        ~Plotbot();

        void insert(float* data, int dsize);
        void plot();
};
