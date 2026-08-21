#pragma once

//Response shapes, split out of kicksdb_client.hpp so the parsing layer can be
//built and tested without OpenSSL / cpp-httplib.

#include "nlohmann/json.hpp"
#include <string>
#include <vector>

using namespace std;
using json = nlohmann::json;


//A search result, flattened out of whatever shape the API hands back
struct Product {
    string id;
    string title;
    string brand;
    float price = 0.0f;
};

//One historical sale
struct Sale {
    string date;
    float price = 0.0f;
};


//Parsing is tolerant and public: it can be pointed at a cached json blob
//without going over the network.
vector<Product> parse_products(const json& body);
vector<Sale> parse_sales(const json& body);
