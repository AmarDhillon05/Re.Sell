#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <unordered_map>
#include <iostream>

#include "kicksdb_client.hpp"

using namespace httplib;
using namespace std;
using json = nlohmann::json;


//Get env + instantiate
KicksDB_Client::KicksDB_Client(){

    root_uri = "api.kicks.dev";

    const unordered_map<string, string> env = getenv();
    api_key = env.at("KICKSDB_API_KEY");

    cout << "KicksDB Client found API key " << api_key << endl;

}


//Retreives json search results
pair<bool, json> KicksDB_Client::search(string query) const{

    SSLClient cli(root_uri, 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);

    Headers headers = {
        {"Accept", "application/json"}, 
        {"Authorization", "Bearer " + api_key}
    };

    auto res = cli.Get("/v3/stockx/products?query=" + encode_uri_component(query), headers);

    if(res){
        if(res->status != 200){
            cout << "KicksDB search failed with status " << res->status << endl;
            return make_pair(false, json::object());
        }

        json body = json::parse(res->body);
        return make_pair(true, body);

    } else {
        cout << "Error in retriving KicksDB search" << res.error() << endl;
        return make_pair(false, json::object());
    }
}


//Retreives price data per id
pair<bool, json> KicksDB_Client::get_history(string id) const{

    SSLClient cli(root_uri, 443);
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);


    Headers headers = {
        {"Accept", "application/json"}, 
        {"Authorization", "Bearer " + api_key}
    };

    auto res = cli.Get("/v3/stockx/products/" + id + "/sales", headers);

    if(res){
        if(res->status != 200){
            cout << "KicksDB get_history failed with status " << res->status << endl;
            return make_pair(false, json::object());
        }

        json body = json::parse(res->body);
        return make_pair(true, body);

    } else {
        cout << "Error in retreiving KicksDB price data" << res.error() << endl;
        return make_pair(false, json::object());
    }
}



int main(){

    KicksDB_Client cli;
    pair<bool, json> s = cli.search("Air Jordan 1");
    cout << s.second.dump(2) << endl;

    return 0;
}