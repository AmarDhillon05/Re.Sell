#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

//Very basic .env reader for non-problematic env formats
unordered_map<string, string> getenv(){
    string ln;
    unordered_map<string, string> env;
    
    ifstream readEnvF("filename.txt");

    while (getline(readEnvF, ln)) {
        int idx = ln.find("=");
        string k = ln.substr(0, idx);
        string v = ln.substr(idx, ln.length());
        env[k] = v;
    }

    readEnvF.close();
    return env;
}