#pragma once

#include <string>

using namespace std;

//Fixed width ASCII line plot for a stream of samples.
//The plot keeps `xsz` columns; once they are full, adjacent columns are merged
//(and the samples-per-column scale doubles) so older data stays on screen.
class Plotbot{

    private:
        int xsz;
        int ysz;

        //Y axis range, in data units
        float ymin;
        float ymax;
        float yscl;   //Data units per row

        int xscl;        //Raw samples represented by one column
        int spaceTaken;  //Columns currently in use
        long totalPts;   //Raw samples ever appended

        //Columns are kept as (sum, count) so merging preserves true averages
        float* sums;
        int* counts;

        void compress();
        void rescale_y();

    public:

        Plotbot(int xsz, int ysz);
        ~Plotbot();

        //Owns a raw buffer, so no copying
        Plotbot(const Plotbot&) = delete;
        Plotbot& operator=(const Plotbot&) = delete;

        void append(const float* data, int dsize);
        void plot(const string& title = "") const;
        void clear();

        //Value shown by column i (its mean), or 0 for an empty column
        float value(int i) const;
        int columns() const { return spaceTaken; }
        long points_seen() const { return totalPts; }
};
