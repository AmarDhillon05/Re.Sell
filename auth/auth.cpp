#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "cpp-httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "utils/dotenv.hpp"
#include <iostream>

using namespace httplib;
using namespace std;

//Refreshes + retreives access token 
string get_access_token(string client_id, string client_secret, string refresh_token){

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
        { "Content-Type", "application/x-www-form-urlencoded" }
    };

    auto res = cli.Post("/oauth/token", headers, params);

    if (res) {
        std::cout << "Status: " << res->status << "\n";
        std::cout << "Body: " << res->body << "\n";
    } else {
        std::cerr << "Error: " << res.error() << "\n";
    }
    return "";

}

int main(){

    string CLIENT_ID = "VsDrlW828FSwBH6WOFj5L8eutFdc6c1u";
    string CLIENT_SECRET = "CAGzKx2SO9dWbGgaD1NfBG3xWnfeVYZYffwaAHGcBu-UaF6Vruv4MbMZuylTVTTL";
    string REFRESH_TOKEN = "tXtK9bsy68j9DrCiqn2a-VAp6TaRiTadzOw1SJMw4adIz";
    cout << get_access_token(CLIENT_ID, CLIENT_SECRET, REFRESH_TOKEN);

    return 0;
}

