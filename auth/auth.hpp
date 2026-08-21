#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>
#include <string>

using namespace httplib;
using namespace std;
using json = nlohmann::json;


//Refreshes + retreives access token (stockx)
pair<bool, json> get_stockx_access_token(string client_id, string client_secret, string refresh_token);

//Same, but pulls STOCKX_CLIENT_ID / STOCKX_CLIENT_SECRET / STOCKX_REFRESH_TOKEN
//out of .env and hands back just the token. Empty string on failure.
string stockx_access_token();
