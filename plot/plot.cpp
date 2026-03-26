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
    yscl = 1;
    xscl = 1;

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
    for(int i = 0; i < indent - ymaxSpace; i ++){ cout << "#"; }
    cout << ymax << "-";

    //First data row (no added indent);
    for(int j = 0; j < xsz; j ++){ 
        char toPrint = ' ';
        //if((int)((ymax - points[j]) / yscl) == ymax){
        if(abs( points[j] - ymax ) < yscl){
            toPrint = '*'; 
        }
        cout << toPrint;
    }
    cout << endl;

    //2nd to 2nd to last
    for(int i = 1; i < ysz - 1; i ++){
        for(int j = 0; j < indent; j ++){ cout << "#"; }
        for(int j = 0; j < xsz; j ++){ 
            char toPrint = ' ';
            //if((int)((ymax - points[j]) / yscl) == i){
            if(abs( points[j] - (ymax - i*yscl) ) < yscl){
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
        if(abs( points[j] - ymin ) < yscl){
            toPrint = '*';
        }
        cout << toPrint;
    }
    cout << endl;

    //Bottom x ticks
    for(int i = 0; i < indent; i ++){ cout << "#"; }
    cout << "|";
    for(int i = 0; i < xsz - 1; i ++){ cout << "#"; }
    cout << "|" << endl;

    //Xticks
    int xindent = (int)log10(xsz * xscl);
    for(int i = 0; i < indent; i ++){ cout << " "; }
    cout << '0';
    for(int i = 0; i < xsz - xindent; i ++){ cout << " "; }
    cout << xsz * xscl;
    
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

    //Fit y scale
    yscl = (int)( (ymax - ymin) / ysz );


    if(spaceTaken < xsz){
    
        //Scaling data if we're not full to fit current graph
        int nCopyable = (int)(dsize / xscl);
        int didx = 0;
        while(spaceTaken < xsz){
            if(didx >= dsize){ break; }
            for(int i = 0; i < xscl; i ++){
                points[spaceTaken - 1] += data[didx];
                didx ++;
            }
            spaceTaken += 1;
        }


        if(didx < dsize){
            float *toins = (float*) malloc(sizeof(float) * (dsize - didx));
            memcpy(toins, data + didx, sizeof(float) * (dsize - didx));
            Plotbot::append(toins, (dsize - didx));
            free(toins);
        }

    }

    //If we have too much space/on recursive calls
    else{
        //Calculating size to compress to (undershot) (TODO - fix these)
        float totalAmt = (spaceTaken * xscl) + dsize;
        int xscl_new = ceil(totalAmt / (float) xsz);
        int xsclDiff = ceil((float) xscl_new / (float) xscl);
    
        //Scaling existing data + new data 
        float* tmp = (float*)malloc(sizeof(float) * xsz);

        int tidx = -1;


        for(int i = 0; i < xsz; i ++){
            if(i % xsclDiff == 0){ tmp[tidx] /= xsclDiff; tidx ++; }
            tmp[tidx] += points[i];
        }

        tmp[tidx] /= xsclDiff;

        for(int i = 0; i < dsize; i ++){
            if(i % xscl_new == 0){ tmp[tidx] /= xscl_new; tidx ++; }
            tmp[tidx] += data[i];
        }

        tmp[tidx] /= xscl_new;

        //Final copy + memmanage
        mempcpy(points, tmp, sizeof(float) * xsz);
        free(tmp);
        xscl = xscl_new;
        spaceTaken = tidx + 1;

        
    }


}

/*


int main(){

    Plotbot pb(150, 30);

    int ssize = 60;
    float start[ssize];
    for(int i = 0; i < ssize; i ++){ start[i] = (i * 10) + 1; }

    pb.append(start, ssize);

    for(int i = 0; i < ssize; i ++){ start[i] = (i * 10) + 1; }
    pb.append(start, ssize);

    pb.plot();

    ssize = 100;
    float starttoo[ssize];
    for(int i = 0; i < ssize; i ++){ starttoo[i] = i*i + 10; }
    pb.append(starttoo, ssize);

    pb.plot();

    ssize = 100;
    float start3[ssize];
    for(int i = 0; i < ssize; i ++){ start3[i] = -150 + i*i*i; };
    pb.append(start3, ssize);

    pb.plot();

    return 0;
}


*/