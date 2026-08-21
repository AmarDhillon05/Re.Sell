#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "plot.hpp"


using namespace std;

namespace {

    //Short, fixed width axis label (e.g. "142.5")
    string axis_label(float v){
        char buf[32];
        if(fabs(v) >= 1000.0f || (v != 0.0f && fabs(v) < 0.01f)){
            snprintf(buf, sizeof(buf), "%.4g", v);
        } else {
            snprintf(buf, sizeof(buf), "%.2f", v);
        }
        return string(buf);
    }

}


Plotbot::Plotbot(int xszv, int yszv){
    xsz = max(2, xszv);
    ysz = max(2, yszv);

    ymin = 0.0f;
    ymax = 1.0f;
    yscl = 1.0f;
    xscl = 1;

    //One accumulator per column. calloc so a fresh column never starts on garbage.
    sums = (float*)calloc((size_t)xsz, sizeof(float));
    counts = (int*)calloc((size_t)xsz, sizeof(int));

    spaceTaken = 0;
    totalPts = 0;
}

Plotbot::~Plotbot(){
    free(sums);
    free(counts);
}


void Plotbot::clear(){
    memset(sums, 0, (size_t)xsz * sizeof(float));
    memset(counts, 0, (size_t)xsz * sizeof(int));
    spaceTaken = 0;
    totalPts = 0;
    xscl = 1;
    ymin = 0.0f; ymax = 1.0f; yscl = 1.0f;
}


float Plotbot::value(int i) const {
    if(i < 0 || i >= spaceTaken || counts[i] == 0){ return 0.0f; }
    return sums[i] / (float)counts[i];
}


//Halves the number of occupied columns by merging neighbours pairwise,
//which doubles how many raw samples one column stands for.
void Plotbot::compress(){
    for(int i = 0; i < xsz; i ++){
        int src = 2 * i;
        if(src >= spaceTaken){
            sums[i] = 0.0f;
            counts[i] = 0;
            continue;
        }

        float s = sums[src];
        int c = counts[src];
        if(src + 1 < spaceTaken){
            s += sums[src + 1];
            c += counts[src + 1];
        }
        sums[i] = s;
        counts[i] = c;
    }

    spaceTaken = (spaceTaken + 1) / 2;
    xscl *= 2;
}


//Y bounds always come from the data actually held, never from stale state
void Plotbot::rescale_y(){
    if(spaceTaken == 0){
        ymin = 0.0f; ymax = 1.0f; yscl = 1.0f;
        return;
    }

    float lo = value(0);
    float hi = value(0);
    for(int i = 1; i < spaceTaken; i ++){
        lo = min(lo, value(i));
        hi = max(hi, value(i));
    }

    //A flat series would give a zero height axis, so pad it out
    if(hi - lo < 1e-6f){
        float pad = max(1.0f, fabs(hi) * 0.05f);
        lo -= pad;
        hi += pad;
    }

    ymin = lo;
    ymax = hi;

    //Rows are 0..ysz-1, so the span is divided into ysz-1 gaps
    yscl = (ymax - ymin) / (float)(ysz - 1);
    if(!(yscl > 0.0f)){ yscl = 1.0f; }
}


//Edit rows
void Plotbot::append(const float* data, int dsize){

    if(data == nullptr || dsize <= 0){ return; }

    for(int i = 0; i < dsize; i ++){

        //Out of columns and the last one is already full -> merge to make room
        if(spaceTaken == xsz && counts[spaceTaken - 1] >= xscl){
            compress();
        }

        //Open a new column when there is no current one, or the current one is full
        if(spaceTaken == 0 || counts[spaceTaken - 1] >= xscl){
            sums[spaceTaken] = 0.0f;
            counts[spaceTaken] = 0;
            spaceTaken += 1;
        }

        //Store the real value; mapping to rows is the plotter's job, not the store's
        sums[spaceTaken - 1] += data[i];
        counts[spaceTaken - 1] += 1;
        totalPts += 1;
    }

    rescale_y();
}


//Plot rows
void Plotbot::plot(const string& title) const {

    if(!title.empty()){ cout << title << endl; }

    if(spaceTaken == 0){
        cout << "(no data)" << endl;
        return;
    }

    //Label gutter is as wide as the widest label we actually print
    string top = axis_label(ymax);
    string bot = axis_label(ymin);
    int indent = (int)max(top.size(), bot.size());

    //Rasterise into a grid first: one row per y step, marking the nearest row
    //for each column instead of smearing a point across every row within yscl.
    vector<string> grid((size_t)ysz, string((size_t)spaceTaken, ' '));
    for(int j = 0; j < spaceTaken; j ++){
        int row = (int)lround((ymax - value(j)) / yscl);
        row = max(0, min(ysz - 1, row));
        grid[(size_t)row][(size_t)j] = '*';
    }

    for(int i = 0; i < ysz; i ++){

        //Only the first and last rows carry a number; the rest are blank gutter
        string label;
        if(i == 0){ label = top; }
        else if(i == ysz - 1){ label = bot; }

        for(int p = 0; p < indent - (int)label.size(); p ++){ cout << ' '; }
        cout << label << (label.empty() ? ' ' : '-') << grid[(size_t)i] << endl;
    }

    //Bottom x axis
    for(int i = 0; i < indent; i ++){ cout << ' '; }
    cout << '|';
    for(int i = 0; i < spaceTaken - 2; i ++){ cout << '-'; }
    if(spaceTaken >= 2){ cout << '|'; }
    cout << endl;

    //Xticks: 0 on the left, the number of samples plotted on the right
    string right = to_string(totalPts);
    for(int i = 0; i < indent; i ++){ cout << ' '; }
    cout << '0';
    int gap = spaceTaken - 1 - (int)right.size();
    for(int i = 0; i < gap; i ++){ cout << ' '; }
    if(gap >= 0){ cout << right; }
    cout << endl;

    cout << "  " << spaceTaken << " cols @ " << xscl << " sample(s) each, "
         << totalPts << " total" << endl;
}
