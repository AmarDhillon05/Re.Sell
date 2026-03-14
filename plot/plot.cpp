#include <iostream>
#include <cstring>
#include <cmath>
#include "plot.hpp"


Plotbot::Plotbot(int xszv, int yszv){
    xsz = xszv; 
    ysz = yszv;

    //xsz,ysz printed array
    data = (char*)malloc(sizeof(char) * xsz * ysz);
    for(int i = 0; i < xsz*ysz; i ++){ data[i] = ' '; }

    //Changes graph scales when lower entries are found
    ymin = 0;
    ymax = ysz;

    sizeused = 0;

}

Plotbot::~Plotbot(){
    free(data);
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
    cout << ymaxSpace << "-";

    //First data row (no added indent);
    for(int j = 0; j < xsz; j ++){ cout << data[j]; }
    cout << endl;

    //2nd to 2nd to last
    for(int i = 1; i < ysz - 1; i ++){
        for(int j = 0; j < indent; j ++){ cout << " "; }
        for(int j = 0; j < xsz; j ++){ cout << data[ysz * i + j]; }
        cout << endl;
    }
    
    //Bottom y
    for(int i = 0; i < indent - yminSpace; i ++){ cout << " "; }
    cout << yminSpace << "-";

    //Last data row (no added indent);
    for(int j = 0; j < xsz; j ++){ cout << data[(ysz-1) * ysz + j]; }
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





int main(){

    Plotbot pb(50, 10);
    pb.plot();

    return 0;
}