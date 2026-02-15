#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>

using namespace httplib;
using namespace std;
using json = nlohmann::json;


//Refreshes + retreives access token 
pair<bool, json> get_access_token(string client_id, string client_secret, string refresh_token){

    SSLClient cli("accounts.stockx.com", 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);

    Params params;
    params.emplace("grant_type", "refresh_token");
    params.emplace("client_id", client_id);
    params.emplace("client_secret", client_secret);
    params.emplace("audience", "gateway.stockx.com");
    params.emplace("refresh_token", refresh_token);

    Headers headers = {
        {"Accept", "application/json"}
    };

    auto res = cli.Post("/oauth/token", headers, params);

    if(res){
        if(res->status != 200){
            cout << "Retreiving access token failed with status " << res->status << endl;
            return make_pair(false, json::object());
        }

        json body = json::parse(res->body);
        return make_pair(true, body);

    } else {
        cout << "Error in retreiving access token" << res.error() << endl;
        return make_pair(false, json::object());
    }

}


