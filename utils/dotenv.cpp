#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

using namespace std;

//Very basic .env reader for non-problematic env formats
unordered_map<string, string> getenv(){
    cout << "Reading env..." << endl;
    string ln;
    unordered_map<string, string> env;
    
    ifstream readEnvF(".env");

    while (getline(readEnvF, ln)) {
        int idx = ln.find("=");
        string k = ln.substr(0, idx);
        cout << "Found " << k << " in .env" << endl;
        string v = ln.substr(idx + 1, ln.length());
        env[k] = v;
    }

    readEnvF.close();
    return env;
}