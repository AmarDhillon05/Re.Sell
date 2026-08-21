#include "harness.hpp"
#include "plot/plot.hpp"
#include "plot/utils.hpp"

#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>

using namespace std;

namespace {

    //plot() writes to stdout, so capture it to assert on the rendering
    string capture_plot(const Plotbot& pb, const string& title){
        fflush(stdout);
        string path = "/tmp/resell_plot_capture.txt";

        int saved = dup(fileno(stdout));
        FILE* f = freopen(path.c_str(), "w", stdout);
        (void)f;
        pb.plot(title);
        fflush(stdout);
        dup2(saved, fileno(stdout));
        close(saved);

        FILE* r = fopen(path.c_str(), "r");
        string out;
        if(r){
            char buf[4096];
            size_t n;
            while((n = fread(buf, 1, sizeof(buf), r)) > 0){ out.append(buf, n); }
            fclose(r);
        }
        return out;
    }

}


TEST(plot, utils_min_and_max){
    float d[] = {3.0f, -1.5f, 9.25f, 0.0f};
    CHECK_NEAR(maxof(d, 4), 9.25, 1e-5);
    CHECK_NEAR(minof(d, 4), -1.5, 1e-5);

    //Empty input must not read past the buffer
    CHECK_NEAR(maxof(d, 0), 0.0, 1e-5);
    CHECK_NEAR(minof(d, 0), 0.0, 1e-5);
}


TEST(plot, starts_empty){
    Plotbot pb(40, 10);
    CHECK_EQ(pb.columns(), 0);
    CHECK_EQ((int)pb.points_seen(), 0);

    string out = capture_plot(pb, "empty");
    CHECK(out.find("(no data)") != string::npos);
}


TEST(plot, one_column_per_sample_until_full){
    Plotbot pb(50, 10);
    vector<float> d(20);
    for(int i = 0; i < 20; i ++){ d[i] = (float)i; }

    pb.append(d.data(), 20);
    CHECK_EQ(pb.columns(), 20);
    CHECK_EQ((int)pb.points_seen(), 20);

    //Below capacity each column is one raw sample, stored unchanged
    for(int i = 0; i < 20; i ++){ CHECK_NEAR(pb.value(i), (double)i, 1e-4); }
}


TEST(plot, values_are_not_rescaled_by_later_appends){
    //Appending a wider range must not rewrite earlier values, which is what
    //the old auto-scaling did.
    Plotbot pb(50, 10);

    float first[] = {10.0f, 20.0f, 30.0f};
    pb.append(first, 3);
    CHECK_NEAR(pb.value(0), 10.0, 1e-4);

    float second[] = {1000.0f, 2000.0f};
    pb.append(second, 2);

    CHECK_NEAR(pb.value(0), 10.0, 1e-4);
    CHECK_NEAR(pb.value(1), 20.0, 1e-4);
    CHECK_NEAR(pb.value(2), 30.0, 1e-4);
    CHECK_NEAR(pb.value(3), 1000.0, 1e-4);
    CHECK_NEAR(pb.value(4), 2000.0, 1e-4);
}


TEST(plot, compresses_past_capacity_and_keeps_averages){
    const int W = 20;
    Plotbot pb(W, 10);

    //Every sample is 5.0, so any correct averaging keeps every column at 5.0
    vector<float> d(500, 5.0f);
    pb.append(d.data(), 500);

    CHECK(pb.columns() <= W);
    CHECK_EQ((int)pb.points_seen(), 500);

    for(int i = 0; i < pb.columns(); i ++){ CHECK_NEAR(pb.value(i), 5.0, 1e-4); }
}


TEST(plot, compression_preserves_the_overall_mean){
    const int W = 16;
    Plotbot pb(W, 10);

    vector<float> d(640);
    for(int i = 0; i < 640; i ++){ d[i] = 100.0f + (float)i; }
    pb.append(d.data(), 640);

    CHECK(pb.columns() <= W);

    //Mean across columns must still match the mean of the raw series
    double sum = 0;
    for(int i = 0; i < pb.columns(); i ++){ sum += pb.value(i); }
    double col_mean = sum / pb.columns();

    double raw_mean = 0;
    for(float v : d){ raw_mean += v; }
    raw_mean /= d.size();

    CHECK_REL(col_mean, raw_mean, 0.02);
}


TEST(plot, renders_axes_and_marks){
    Plotbot pb(40, 8);
    vector<float> d(40);
    for(int i = 0; i < 40; i ++){ d[i] = 100.0f + 50.0f * sinf(i * 0.3f); }
    pb.append(d.data(), 40);

    string out = capture_plot(pb, "test series");

    CHECK(out.find("test series") != string::npos);
    CHECK(out.find('*') != string::npos);      //Data was drawn
    CHECK(out.find('|') != string::npos);      //X axis
    CHECK(out.find("40 total") != string::npos);

    //One row per y step, plus title, axis and two tick lines
    int lines = 0;
    for(char c : out){ if(c == '\n'){ lines ++; } }
    CHECK(lines >= 8);
}


TEST(plot, flat_series_does_not_collapse_the_axis){
    //A zero height range would divide by zero when mapping values to rows
    Plotbot pb(20, 6);
    vector<float> d(10, 42.0f);
    pb.append(d.data(), 10);

    string out = capture_plot(pb, "flat");
    CHECK(out.find('*') != string::npos);
    CHECK(out.find("nan") == string::npos);
    CHECK(out.find("inf") == string::npos);
}


TEST(plot, handles_negative_values){
    //log10 of a negative bound used to produce NaN labels
    Plotbot pb(30, 8);
    vector<float> d(30);
    for(int i = 0; i < 30; i ++){ d[i] = -100.0f + (float)i * 3.0f; }
    pb.append(d.data(), 30);

    string out = capture_plot(pb, "negatives");
    CHECK(out.find("nan") == string::npos);
    CHECK(out.find('*') != string::npos);
}


TEST(plot, ignores_empty_and_null_appends){
    Plotbot pb(20, 6);
    float d[] = {1.0f};

    pb.append(nullptr, 5);
    pb.append(d, 0);
    pb.append(d, -3);

    CHECK_EQ(pb.columns(), 0);
    CHECK_EQ((int)pb.points_seen(), 0);
}


TEST(plot, clear_resets_state){
    Plotbot pb(20, 6);
    vector<float> d(50, 3.0f);
    pb.append(d.data(), 50);
    CHECK(pb.columns() > 0);

    pb.clear();
    CHECK_EQ(pb.columns(), 0);
    CHECK_EQ((int)pb.points_seen(), 0);

    //Reusable afterwards
    pb.append(d.data(), 5);
    CHECK_EQ(pb.columns(), 5);
}
