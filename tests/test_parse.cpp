#include "harness.hpp"
#include "clients/kicksdb_types.hpp"

#include <fstream>
#include <string>

using namespace std;


TEST(parse, real_api_search_response){
    //A trimmed but otherwise untouched capture from api.kicks.dev, so a schema
    //change on their side fails here rather than silently returning nothing.
    ifstream f("tests/fixtures/stockx_search.json");
    CHECK(f.good());
    if(!f.good()){ return; }

    json body = json::parse(f, nullptr, false);
    CHECK(!body.is_discarded());
    if(body.is_discarded()){ return; }

    vector<Product> got = parse_products(body);
    CHECK(got.size() == 5);

    for(const Product& p : got){
        CHECK(!p.id.empty());
        CHECK(!p.title.empty());
        CHECK(p.price >= 0.0f);
    }

    if(!got.empty()){
        CHECK_EQ(got[0].brand, string("Jordan"));
        //The id is what every follow up request is keyed on
        CHECK(got[0].id.size() > 8);
    }
}


TEST(parse, products_from_a_data_envelope){
    json body = json::parse(R"({"status":200,"data":[
        {"id":"abc","title":"Air Jordan 1 Retro","brand":"Jordan","avg_price":320.5}
    ]})");

    vector<Product> got = parse_products(body);
    CHECK_EQ((int)got.size(), 1);
    if(!got.empty()){
        CHECK_EQ(got[0].id, string("abc"));
        CHECK_EQ(got[0].title, string("Air Jordan 1 Retro"));
        CHECK_NEAR(got[0].price, 320.5, 1e-3);
    }
}


TEST(parse, products_from_a_bare_array){
    json body = json::parse(R"([{"id":"1","title":"Dunk Low","price":110}])");
    vector<Product> got = parse_products(body);
    CHECK_EQ((int)got.size(), 1);
    if(!got.empty()){ CHECK_NEAR(got[0].price, 110.0, 1e-3); }
}


TEST(parse, accepts_alternate_field_spellings){
    json body = json::parse(R"({"data":[
        {"uuid":"xyz","name":"Yeezy 350","brand_name":"adidas","last_sale":"215.75"}
    ]})");

    vector<Product> got = parse_products(body);
    CHECK_EQ((int)got.size(), 1);
    if(!got.empty()){
        CHECK_EQ(got[0].id, string("xyz"));
        CHECK_EQ(got[0].title, string("Yeezy 350"));
        CHECK_EQ(got[0].brand, string("adidas"));
        //Numbers arriving as strings still parse
        CHECK_NEAR(got[0].price, 215.75, 1e-3);
    }
}


TEST(parse, drops_unaddressable_records){
    json body = json::parse(R"({"data":[
        {"nothing":"useful"},
        {"id":"only-id"},
        {"title":"only-title"}
    ]})");

    vector<Product> got = parse_products(body);

    //The record with neither id nor title is dropped; the other two survive
    CHECK_EQ((int)got.size(), 2);
    for(const Product& p : got){ CHECK(!p.title.empty()); }
}


TEST(parse, malformed_input_yields_nothing){
    CHECK_EQ((int)parse_products(json::object()).size(), 0);
    CHECK_EQ((int)parse_products(json::array()).size(), 0);
    CHECK_EQ((int)parse_products(json("a string")).size(), 0);
    CHECK_EQ((int)parse_sales(json::object()).size(), 0);
    CHECK_EQ((int)parse_sales(json(42)).size(), 0);

    //A data key holding the wrong type must not throw
    CHECK_EQ((int)parse_products(json::parse(R"({"data": "not an array"})")).size(), 0);
}


TEST(parse, sales_sort_oldest_first){
    json body = json::parse(R"({"data":[
        {"amount":300,"date":"2024-03-01"},
        {"amount":280,"date":"2024-01-15"},
        {"salePrice":"295.50","createdAt":"2024-02-20"}
    ]})");

    vector<Sale> got = parse_sales(body);
    CHECK_EQ((int)got.size(), 3);
    if(got.size() == 3){
        CHECK_NEAR(got[0].price, 280.0, 1e-3);
        CHECK_NEAR(got[1].price, 295.5, 1e-3);
        CHECK_NEAR(got[2].price, 300.0, 1e-3);
    }
}


TEST(parse, sales_drop_non_positive_prices){
    json body = json::parse(R"({"data":[
        {"amount":300,"date":"2024-03-01"},
        {"amount":0,"date":"2024-02-01"},
        {"amount":-5,"date":"2024-02-02"}
    ]})");

    vector<Sale> got = parse_sales(body);
    CHECK_EQ((int)got.size(), 1);
    if(!got.empty()){ CHECK_NEAR(got[0].price, 300.0, 1e-3); }
}
