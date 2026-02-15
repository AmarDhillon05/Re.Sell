#include "utils/dotenv.hpp"
#include "auth/auth.hpp"
#include <iostream>
#include <unordered_map>

using namespace std;


int main(){
    
    unordered_map<string, string> env = getenv();
    
    return 0;
}