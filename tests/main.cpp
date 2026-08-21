#include "harness.hpp"

//Usage: ./run_tests [filter]   (filter matches a suite or test name substring)
int main(int argc, char** argv){
    const char* filter = (argc > 1) ? argv[1] : "";
    printf("Re.Sell test suite%s%s", *filter ? " - filter: " : "", filter);
    printf("\n");
    return testing::run_all(filter);
}
