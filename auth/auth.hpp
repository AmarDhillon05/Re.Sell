#pragma once 

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>

using namespace httplib;
using namespace std;
using json = nlohmann::json;


//Refreshes + retreives access token 
pair<bool, json> get_stockx_access_token(string client_id, string client_secret, string refresh_token)