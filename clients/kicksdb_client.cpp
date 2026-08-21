#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <unordered_map>
#include <iostream>
#include <algorithm>

#include "kicksdb_client.hpp"

using namespace httplib;
using namespace std;
using json = nlohmann::json;


namespace {

    constexpr int REQUEST_TIMEOUT_SEC = 15;

    //Percent encodes a value for use in a path segment or query string.
    //httplib only exposes its encoder under detail::, which is not a stable API.
    string url_encode(const string& value){
        static const char* hex = "0123456789ABCDEF";
        string out;
        out.reserve(value.size());

        for(unsigned char c : value){
            if(isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'){
                out += (char)c;
            } else {
                out += '%';
                out += hex[(c >> 4) & 0xF];
                out += hex[c & 0xF];
            }
        }

        return out;
    }

    //Every response goes through here so a truncated or HTML body cannot throw
    //out of the client and kill the process.
    pair<bool, json> finish(const Result& res, const char* what){
        if(!res){
            cout << "[api] " << what << " failed: " << to_string(res.error()) << endl;
            return make_pair(false, json::object());
        }

        if(res->status != 200){
            cout << "[api] " << what << " failed with status " << res->status << endl;
            if(res->status == 401 || res->status == 403){
                cout << "[api] check that KICKSDB_API_KEY in .env is valid" << endl;
            }
            return make_pair(false, json::object());
        }

        json body = json::parse(res->body, nullptr, false);
        if(body.is_discarded()){
            cout << "[api] " << what << " returned a malformed JSON body" << endl;
            return make_pair(false, json::object());
        }

        return make_pair(true, body);
    }

}


//Get env + instantiate
KicksDB_Client::KicksDB_Client(){

    root_uri = "api.kicks.dev";

    const unordered_map<string, string> env = read_env();

    //.at() threw whenever the key was absent; degrade instead
    api_key = env_or(env, "KICKSDB_API_KEY", "");
    has_key = !api_key.empty();

    if(has_key){
        //Never echo the raw secret
        cout << "KicksDB client using API key " << mask_secret(api_key) << endl;
    } else {
        cout << "[api] KICKSDB_API_KEY not found in .env, network commands are disabled" << endl;
    }
}


//Retreives json search results
pair<bool, json> KicksDB_Client::search(string query, int limit) const{

    if(!has_key){ return make_pair(false, json::object()); }
    if(limit < 1){ limit = 1; }

    SSLClient cli(root_uri, 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);
    cli.set_connection_timeout(REQUEST_TIMEOUT_SEC, 0);
    cli.set_read_timeout(REQUEST_TIMEOUT_SEC, 0);

    Headers headers = {
        {"Accept", "application/json"},
        {"Authorization", "Bearer " + api_key}
    };

    string path = "/v3/stockx/products?query=" + url_encode(query)
                + "&limit=" + to_string(limit);

    return finish(cli.Get(path, headers), "search");
}


//Retreives price data per id
pair<bool, json> KicksDB_Client::get_history(string id) const{

    if(!has_key){ return make_pair(false, json::object()); }

    SSLClient cli(root_uri, 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);
    cli.set_connection_timeout(REQUEST_TIMEOUT_SEC, 0);
    cli.set_read_timeout(REQUEST_TIMEOUT_SEC, 0);

    Headers headers = {
        {"Accept", "application/json"},
        {"Authorization", "Bearer " + api_key}
    };

    //The id is user supplied, so it has to be escaped before it lands in the path
    string path = "/v3/stockx/products/" + url_encode(id) + "/sales";

    return finish(cli.Get(path, headers), "get_history");
}


//Single product record
pair<bool, json> KicksDB_Client::get_product(string id) const{

    if(!has_key){ return make_pair(false, json::object()); }

    SSLClient cli(root_uri, 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);
    cli.set_connection_timeout(REQUEST_TIMEOUT_SEC, 0);
    cli.set_read_timeout(REQUEST_TIMEOUT_SEC, 0);

    Headers headers = {
        {"Accept", "application/json"},
        {"Authorization", "Bearer " + api_key}
    };

    return finish(cli.Get("/v3/stockx/products/" + url_encode(id), headers),
                  "get_product");
}

vector<Product> KicksDB_Client::search_products(string query, int limit) const{
    auto res = search(query, limit);
    if(!res.first){ return {}; }
    return parse_products(res.second);
}


vector<float> KicksDB_Client::price_series(string id) const{
    auto res = get_history(id);
    if(!res.first){ return {}; }

    vector<float> prices;
    for(const Sale& s : parse_sales(res.second)){ prices.push_back(s.price); }
    return prices;
}
