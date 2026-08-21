#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include "kicksdb_types.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace httplib;
using namespace std;
using json = nlohmann::json;


#ifndef KICKSDB_CLIENT
#define KICKSDB_CLIENT

class KicksDB_Client{

    private:
        string root_uri;
        string api_key;
        bool has_key;

    public:

        KicksDB_Client();

        //False when KICKSDB_API_KEY was missing: every call will fail, but
        //construction does not throw, so the CLI can still run offline commands.
        bool ready() const { return has_key; }

        pair<bool, json> search(string query, int limit = 20) const;
        pair<bool, json> get_history(string id) const;
        pair<bool, json> get_product(string id) const;

        //Typed convenience wrappers used by the CLI
        vector<Product> search_products(string query, int limit = 20) const;
        vector<float> price_series(string id) const;
};


#endif
