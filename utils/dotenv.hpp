#pragma once

#include <unordered_map>
#include <string>

using namespace std;

//Loads key/value pairs from a .env style file.
//NOTE: named read_env (not getenv) so it does not collide with ::getenv from <cstdlib>,
//which cpp-httplib and friends pull in transitively.
unordered_map<string, string> read_env(const string& path = ".env");

//Lookup that never throws: returns fallback when the key is absent or empty.
string env_or(const unordered_map<string, string>& env, const string& key, const string& fallback = "");

//Masks a secret for logging: "abcd1234efgh" -> "abcd...efgh"
string mask_secret(const string& secret);
