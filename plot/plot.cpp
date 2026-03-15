#include <iostream>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "plot.hpp"
#include "utils.hpp"


using namespace std;


Plotbot::Plotbot(int xszv, int yszv){
    xsz = xszv; 
    ysz = yszv;

    //Changes graph scales when lower entries are found
    ymin = 0;
    ymax = ysz;

    //Auto scaling array of points (inits to 0)
    //Auto rounded as well, float is moreso for expanded method use
    points = (float*)malloc(sizeof(int) * xsz);
    for(int i = 0; i < xsz; i ++){ points[i] = 0; }

    realPtAmt = 1;
    spaceTaken = 0;
}

Plotbot::~Plotbot(){
    free(points);
}



//Plot rows
void Plotbot::plot(){

    //Indent (includes little dash)
    int yminSpace = (int)log10(ymin) + 1;
    if(ymin == 0){ yminSpace = 1; }
    int ymaxSpace = (int)log10(ymax) + 1;
    int indent = max(yminSpace, ymaxSpace); 

    //Top y
    for(int i = 0; i < indent - ymaxSpace; i ++){ cout << " "; }
    cout << ymax << "-";

    //First data row (no added indent);
    for(int j = 0; j < xsz; j ++){ 
        char toPrint = ' ';
        if(points[j] == ymax){
            toPrint = '*'; //TODO: Fix this
        }
        cout << toPrint;
    }
    cout << endl;

    //2nd to 2nd to last
    for(int i = 1; i < ysz - 1; i ++){
        for(int j = 0; j < indent; j ++){ cout << " "; }
        for(int j = 0; j < xsz; j ++){ 
            char toPrint = ' ';
            if(points[j] == i){
                toPrint = '*';
            }
            cout << toPrint;
        }
        cout << endl;
    }
    
    //Bottom y
    for(int i = 0; i < indent - yminSpace; i ++){ cout << " "; }
    cout << ymin << "-";

    //Last data row (no added indent);
    for(int j = 0; j < xsz; j ++){ 
        char toPrint = ' ';
        if(points[j] == ymin){
            toPrint = '*';
        }
        cout << toPrint;
    }
    cout << endl;

    //Bottom x ticks
    for(int i = 0; i < indent; i ++){ cout << " "; }
    cout << "|";
    for(int i = 0; i < xsz - 1; i ++){ cout << " "; }
    cout << "|" << endl;

    //Xticks
    int xindent = (int)log10(xsz);
    for(int i = 0; i < indent; i ++){ cout << " "; }
    cout << '0';
    for(int i = 0; i < xsz - xindent; i ++){ cout << " "; }
    cout << xsz;
    
    cout << endl;


}


//Edit rows 
void Plotbot::append(float* data, int dsize){
    //Scaling values to fit range
    float data_min = minof(data, dsize); 
    float data_max = maxof(data, dsize);
    int oymax = ymax; int oymin = ymin;
    ymax = max(data_max, (float)ymax);
    ymin = min(data_min, (float)ymin);

    for(int i = 0; i < spaceTaken; i ++){
        points[i] = (int)(ymin + (points[i] - oymin) * (ymax - ymin) / (oymax - oymin));
    }

    for(int i = 0; i < dsize; i ++){
        data[i] = (int)(ymin + (data[i] - data_min) * (ymax - ymin) / (data_max - data_min));
    }

    
    //If we're not full
    if(spaceTaken < xsz){
        memcpy(points, data, sizeof(int) * dsize);
        spaceTaken = max(xsz, spaceTaken + dsize);
        if(dsize > xsz){
            float *toins = (float*)malloc(sizeof(int) * (dsize - xsz));
            memcpy(toins, data + (sizeof(int) * (dsize - xsz)), sizeof(int) * (dsize - xsz));
            Plotbot::append(toins, (dsize - xsz));
            free(toins);
        }
    }
    


}




int main(){

    Plotbot pb(50, 10);


    pb.plot();

    return 0;
}