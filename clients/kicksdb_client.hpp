

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>

using namespace httplib;
using namespace std;
using json = nlohmann::json;


#ifndef KICKSDB_CLIENT
#define KICKSDB_CLIENT

class KicksDB_Client{

    private:
        string root_uri;
        string api_key;

    public:

        KicksDB_Client();

        pair<bool, json> search(string query) const;
        pair<bool, json> get_history(string id) const;

};

#endif