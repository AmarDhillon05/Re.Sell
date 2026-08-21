#include "dotenv.hpp"

#include <fstream>
#include <iostream>

using namespace std;

namespace {

    //Strips surrounding whitespace, CR (CRLF files) and matching quotes
    string trim(const string& s){
        size_t b = 0;
        size_t e = s.size();
        while(b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')){ b ++; }
        while(e > b && (s[e-1] == ' ' || s[e-1] == '\t' || s[e-1] == '\r' || s[e-1] == '\n')){ e --; }

        string out = s.substr(b, e - b);
        if(out.size() >= 2 && (out.front() == '"' || out.front() == '\'') && out.back() == out.front()){
            out = out.substr(1, out.size() - 2);
        }
        return out;
    }

}

//Very basic .env reader for non-problematic env formats
unordered_map<string, string> read_env(const string& path){
    unordered_map<string, string> env;

    ifstream readEnvF(path);
    if(!readEnvF){
        cerr << "[env] could not open " << path << " (continuing with an empty environment)" << endl;
        return env;
    }

    string ln;
    while(getline(readEnvF, ln)){
        string line = trim(ln);

        //Skip blanks and comments
        if(line.empty() || line[0] == '#'){ continue; }

        //Skip malformed lines instead of letting npos wrap around in substr
        size_t idx = line.find('=');
        if(idx == string::npos || idx == 0){
            cerr << "[env] skipping malformed line: " << line << endl;
            continue;
        }

        string k = trim(line.substr(0, idx));
        string v = trim(line.substr(idx + 1));

        //Allow "export KEY=VAL"
        if(k.rfind("export ", 0) == 0){ k = trim(k.substr(7)); }

        if(!k.empty()){ env[k] = v; }
    }

    readEnvF.close();
    return env;
}

string env_or(const unordered_map<string, string>& env, const string& key, const string& fallback){
    auto it = env.find(key);
    if(it == env.end() || it->second.empty()){ return fallback; }
    return it->second;
}

string mask_secret(const string& secret){
    if(secret.empty()){ return "<empty>"; }
    if(secret.size() <= 8){ return string(secret.size(), '*'); }
    return secret.substr(0, 4) + "..." + secret.substr(secret.size() - 4);
}
