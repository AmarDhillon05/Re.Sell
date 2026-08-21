#pragma once

//Minimal self-registering test harness. Header only (C++17 inline variables),
//so the suite needs no external framework.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>

namespace testing {

    struct TestCase {
        std::string suite;
        std::string name;
        void (*fn)();
    };

    inline std::vector<TestCase>& registry(){
        static std::vector<TestCase> r;
        return r;
    }

    inline int checks_run = 0;
    inline int current_failures = 0;

    inline void fail(const char* file, int line, const std::string& msg){
        current_failures ++;
        //Trim to the basename so output stays readable
        const char* base = strrchr(file, '/');
        printf("      FAIL %s:%d  %s\n", base ? base + 1 : file, line, msg.c_str());
    }

    template <typename A, typename B>
    std::string describe(const char* expr, const A& a, const B& b){
        std::ostringstream os;
        os << expr << "  (left=" << a << ", right=" << b << ")";
        return os.str();
    }

    struct Registrar {
        Registrar(const char* s, const char* n, void (*f)()){
            registry().push_back({s, n, f});
        }
    };

    //Runs everything, or only the suites/names containing `filter`
    inline int run_all(const char* filter){
        int passed = 0;
        int failed = 0;
        std::string last_suite;

        for(const TestCase& tc : registry()){
            if(filter && *filter &&
               tc.suite.find(filter) == std::string::npos &&
               tc.name.find(filter) == std::string::npos){ continue; }

            if(tc.suite != last_suite){
                printf("\n  [%s]\n", tc.suite.c_str());
                last_suite = tc.suite;
            }

            current_failures = 0;
            tc.fn();

            if(current_failures == 0){
                printf("    ok   %s\n", tc.name.c_str());
                passed ++;
            } else {
                printf("    FAIL %s (%d check(s))\n", tc.name.c_str(), current_failures);
                failed ++;
            }
        }

        printf("\n%s  %d passed, %d failed, %d checks\n",
               failed ? "FAILED" : "PASSED", passed, failed, checks_run);
        return failed == 0 ? 0 : 1;
    }

}

#define TEST(suite_name, test_name)                                              \
    static void test_##suite_name##_##test_name();                               \
    static testing::Registrar reg_##suite_name##_##test_name(                    \
        #suite_name, #test_name, test_##suite_name##_##test_name);               \
    static void test_##suite_name##_##test_name()

#define CHECK(cond)                                                              \
    do { testing::checks_run ++;                                                 \
         if(!(cond)){ testing::fail(__FILE__, __LINE__, "CHECK(" #cond ")"); }   \
    } while(0)

#define CHECK_EQ(a, b)                                                           \
    do { testing::checks_run ++;                                                 \
         if(!((a) == (b))){                                                      \
             testing::fail(__FILE__, __LINE__,                                   \
                 testing::describe(#a " == " #b, (a), (b))); }                   \
    } while(0)

//Absolute tolerance
#define CHECK_NEAR(a, b, tol)                                                    \
    do { testing::checks_run ++;                                                 \
         if(!(std::fabs((double)(a) - (double)(b)) <= (double)(tol))){           \
             testing::fail(__FILE__, __LINE__,                                   \
                 testing::describe(#a " ~= " #b, (a), (b))); }                   \
    } while(0)

//Relative tolerance, for values whose scale varies across tests
#define CHECK_REL(a, b, frac)                                                    \
    do { testing::checks_run ++;                                                 \
         double _d = std::fabs((double)(a) - (double)(b));                       \
         double _s = std::fabs((double)(b));                                     \
         if(!(_d <= (double)(frac) * (_s > 0 ? _s : 1.0))){                       \
             testing::fail(__FILE__, __LINE__,                                   \
                 testing::describe(#a " ~= " #b, (a), (b))); }                   \
    } while(0)
