#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>
#include <unordered_map>

#include "auth.hpp"

using namespace httplib;
using namespace std;
using json = nlohmann::json;


//Refreshes + retreives access token (stockx)
pair<bool, json> get_stockx_access_token(string client_id, string client_secret, string refresh_token){

    if(client_id.empty() || client_secret.empty() || refresh_token.empty()){
        cout << "[auth] missing StockX credentials" << endl;
        return make_pair(false, json::object());
    }

    SSLClient cli("accounts.stockx.com", 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);
    cli.set_connection_timeout(15, 0);
    cli.set_read_timeout(15, 0);

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
            cout << "[auth] retrieving access token failed with status " << res->status << endl;
            return make_pair(false, json::object());
        }

        //A non JSON body would otherwise throw straight out of the call
        json body = json::parse(res->body, nullptr, false);
        if(body.is_discarded()){
            cout << "[auth] token endpoint returned a malformed JSON body" << endl;
            return make_pair(false, json::object());
        }

        return make_pair(true, body);

    } else {
        cout << "[auth] error retrieving access token: " << to_string(res.error()) << endl;
        return make_pair(false, json::object());
    }

}


string stockx_access_token(){

    const unordered_map<string, string> env = read_env();

    auto res = get_stockx_access_token(
        env_or(env, "STOCKX_CLIENT_ID"),
        env_or(env, "STOCKX_CLIENT_SECRET"),
        env_or(env, "STOCKX_REFRESH_TOKEN")
    );

    if(!res.first){ return ""; }

    const json& body = res.second;
    if(!body.contains("access_token") || !body["access_token"].is_string()){
        cout << "[auth] response contained no access_token" << endl;
        return "";
    }

    return body["access_token"].get<string>();
}
