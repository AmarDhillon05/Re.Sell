#include "harness.hpp"
#include "database/dbclient.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

using namespace std;
namespace fs = std::filesystem;

namespace {

    //Each test gets its own file so they cannot interfere
    string tmp_db(const string& name){
        fs::path dir = fs::temp_directory_path() / "resell_tests";
        fs::create_directories(dir);
        fs::path p = dir / (name + ".db");
        error_code ec;
        fs::remove(p, ec);
        return p.string();
    }

    //Writes a throwaway config so page geometry can be varied per test
    string tmp_config(const string& name, int pages, int entries_per_page){
        fs::path dir = fs::temp_directory_path() / "resell_tests";
        fs::create_directories(dir);
        fs::path p = dir / (name + ".json");
        ofstream f(p);
        f << "{\"pages\": " << pages << ", \"entries_per_page\": " << entries_per_page << "}";
        return p.string();
    }

    const char* CFG = "database/config.json";

}


TEST(database, opens_and_formats_a_missing_file){
    string path = tmp_db("format");
    DBClient db(path, CFG);

    CHECK(db.ok());
    CHECK_EQ(db.size(), 0);
    CHECK_EQ(db.page_count(), 0);
    CHECK(!db.search("anything").has_value());
    CHECK(db.list_all().empty());
}


TEST(database, reads_geometry_from_config){
    //The shipped config is what the app runs with
    DBClient db(tmp_db("config"), CFG);
    CHECK_EQ(db.max_entries_per_page(), 8);

    //And an override is honoured
    DBClient small(tmp_db("config_small"), tmp_config("cfg_small", 32, 4));
    CHECK_EQ(small.max_entries_per_page(), 4);
}


TEST(database, insert_then_exact_query){
    string path = tmp_db("exact");
    DBClient db(path, CFG);

    CHECK(db.insert(make_item("Air Jordan 1 Chicago", "aj1-chi", 412.5f)));
    CHECK_EQ(db.size(), 1);

    auto hit = db.search("Air Jordan 1 Chicago");
    CHECK(hit.has_value());
    if(hit.has_value()){
        CHECK_EQ(string(hit->title), string("Air Jordan 1 Chicago"));
        CHECK_EQ(string(hit->id), string("aj1-chi"));
        CHECK_NEAR(hit->last_price, 412.5f, 1e-3);
    }
}


TEST(database, absent_key_is_not_found){
    string path = tmp_db("absent");
    DBClient db(path, CFG);
    db.insert(make_item("Yeezy 350", "yz350", 210.0f));

    CHECK(!db.search("Air Jordan 1").has_value());
    CHECK(!db.search("").has_value());
}


TEST(database, query_is_case_insensitive){
    string path = tmp_db("case");
    DBClient db(path, CFG);
    db.insert(make_item("Air Jordan 1 Chicago", "aj1-chi", 412.5f));

    CHECK(db.search("AIR JORDAN 1 CHICAGO").has_value());
    CHECK(db.search("air jordan 1 chicago").has_value());
}


TEST(database, best_query_resolves_partial_titles){
    string path = tmp_db("prefix");
    DBClient db(path, CFG);
    db.insert(make_item("Air Jordan 1 Chicago", "aj1-chi", 412.0f));
    db.insert(make_item("Nike Dunk Low Panda", "dunk-panda", 118.0f));
    db.insert(make_item("Yeezy 350 Zebra", "yz-zebra", 265.0f));

    auto hit = db.search_best("Air Jordan");
    CHECK(hit.has_value());
    if(hit.has_value()){ CHECK_EQ(string(hit->id), string("aj1-chi")); }

    //An exact title still wins over any prefix logic
    auto exact = db.search_best("Nike Dunk Low Panda");
    CHECK(exact.has_value());
    if(exact.has_value()){ CHECK_EQ(string(exact->id), string("dunk-panda")); }
}


TEST(database, reinsert_updates_in_place){
    string path = tmp_db("update");
    DBClient db(path, CFG);
    db.insert(make_item("Air Jordan 1 Chicago", "aj1-chi", 412.0f));

    Item updated = make_item("Air Jordan 1 Chicago", "aj1-chi-v2", 999.0f);
    CHECK(db.insert(updated));

    //An update must not grow the tree
    CHECK_EQ(db.size(), 1);

    auto hit = db.search("Air Jordan 1 Chicago");
    CHECK(hit.has_value());
    if(hit.has_value()){
        CHECK_EQ(string(hit->id), string("aj1-chi-v2"));
        CHECK_NEAR(hit->last_price, 999.0f, 1e-3);
    }
}


TEST(database, model_fit_survives_a_roundtrip){
    string path = tmp_db("fit");
    DBClient db(path, CFG);

    Item it = make_item("Air Jordan 1 Chicago", "aj1-chi", 400.0f);
    it.fit_mu = 412.75f;
    it.fit_sigma = 31.5f;
    it.n_prices = 120;
    db.insert(it);

    auto hit = db.search("Air Jordan 1 Chicago");
    CHECK(hit.has_value());
    if(hit.has_value()){
        CHECK_NEAR(hit->fit_mu, 412.75f, 1e-3);
        CHECK_NEAR(hit->fit_sigma, 31.5f, 1e-3);
        CHECK_EQ(hit->n_prices, 120);
        CHECK(hit->updated_at > 0);
    }
}


TEST(database, state_persists_across_reopen){
    string path = tmp_db("persist");

    {
        DBClient db(path, CFG);
        for(int i = 0; i < 40; i ++){
            db.insert(make_item("Shoe " + to_string(i), "id" + to_string(i), 100.0f + i));
        }
        CHECK_EQ(db.size(), 40);
    }

    //Reopening must recover the root and page count from the file header
    DBClient reopened(path, CFG);
    CHECK(reopened.ok());
    CHECK_EQ(reopened.size(), 40);
    CHECK(reopened.page_count() > 1);

    for(int i = 0; i < 40; i ++){
        auto hit = reopened.search("Shoe " + to_string(i));
        CHECK(hit.has_value());
        if(hit.has_value()){ CHECK_EQ(string(hit->id), "id" + to_string(i)); }
    }
}


TEST(database, incompatible_file_is_reformatted){
    string path = tmp_db("legacy");

    //Garbage that is not a v1 database
    {
        ofstream f(path, ios::binary);
        for(int i = 0; i < 4096; i ++){ f.put((char)(i % 251)); }
    }

    DBClient db(path, CFG);
    CHECK(db.ok());
    CHECK_EQ(db.size(), 0);

    //And it is usable afterwards
    CHECK(db.insert(make_item("Air Jordan 1", "aj1", 400.0f)));
    CHECK(db.search("Air Jordan 1").has_value());
}


TEST(database, bulk_insert_splits_and_stays_queryable){
    string path = tmp_db("bulk");
    DBClient db(path, CFG);

    //Shuffled keys so the tree splits in a non trivial order
    vector<string> titles;
    for(int i = 0; i < 200; i ++){ titles.push_back("Shoe " + to_string(1000 + i)); }
    shuffle(titles.begin(), titles.end(), mt19937(99));

    for(size_t i = 0; i < titles.size(); i ++){
        CHECK(db.insert(make_item(titles[i], "id" + to_string(i), 100.0f + (float)i)));
    }

    CHECK_EQ(db.size(), 200);
    CHECK(db.page_count() > 1);   //Must actually have split

    int misses = 0;
    for(const string& t : titles){
        if(!db.search(t).has_value()){ misses ++; }
    }
    CHECK_EQ(misses, 0);
}


TEST(database, list_all_returns_sorted_items){
    string path = tmp_db("sorted");
    DBClient db(path, CFG);

    vector<string> titles;
    for(int i = 0; i < 60; i ++){ titles.push_back("Shoe " + to_string(1000 + i)); }
    shuffle(titles.begin(), titles.end(), mt19937(7));
    for(const string& t : titles){ db.insert(make_item(t, "x", 1.0f)); }

    vector<Item> all = db.list_all();
    CHECK_EQ((int)all.size(), 60);

    bool sorted = true;
    for(size_t i = 1; i < all.size(); i ++){
        if(strcasecmp(all[i-1].title, all[i].title) >= 0){ sorted = false; }
    }
    CHECK(sorted);
}


TEST(database, oversized_strings_are_truncated_safely){
    string path = tmp_db("longstr");
    DBClient db(path, CFG);

    string huge_title(500, 'A');
    string huge_id(300, 'B');
    CHECK(db.insert(make_item(huge_title, huge_id, 1.0f)));

    vector<Item> all = db.list_all();
    CHECK_EQ((int)all.size(), 1);
    if(!all.empty()){
        //Must be null terminated inside the fixed buffers
        CHECK(strlen(all[0].title) == sizeof(all[0].title) - 1);
        CHECK(strlen(all[0].id) == sizeof(all[0].id) - 1);
    }
}


TEST(database, refuses_to_grow_past_the_page_limit){
    //A deliberately tiny database: inserts must fail cleanly, not corrupt
    string path = tmp_db("full");
    DBClient db(path, tmp_config("cfg_full", 3, 2));

    int accepted = 0;
    for(int i = 0; i < 200; i ++){
        if(!db.insert(make_item("Shoe " + to_string(1000 + i), "id", 1.0f))){ break; }
        accepted ++;
    }

    CHECK(accepted > 0);
    CHECK(accepted < 200);          //It did hit the limit
    CHECK(db.page_count() <= 3);

    //Whatever was accepted is still retrievable
    CHECK(db.ok());
    CHECK_EQ((int)db.list_all().size(), db.size());
}


TEST(database, clear_empties_the_tree){
    string path = tmp_db("clear");
    DBClient db(path, CFG);
    db.insert(make_item("Air Jordan 1", "aj1", 400.0f));
    CHECK_EQ(db.size(), 1);

    db.raw_clear();
    CHECK(db.ok());
    CHECK_EQ(db.size(), 0);
    CHECK(!db.search("Air Jordan 1").has_value());

    //Still writable after a reset
    CHECK(db.insert(make_item("Yeezy 350", "yz", 200.0f)));
    CHECK_EQ(db.size(), 1);
}


TEST(database, unrelated_query_is_a_miss){
    //Returning the "nearest" title in sort order made the CLI report a cache
    //hit for a shoe that was never tracked.
    string path = tmp_db("unrelated");
    DBClient db(path, CFG);
    db.insert(make_item("adidas Yeezy Boost 350 V2 Zebra", "yz-zebra", 295.0f));
    db.insert(make_item("adidas Yeezy 350 V2 CMPCT Slate Onyx", "yz-onyx", 156.0f));

    CHECK(!db.search_best("New Balance 550").has_value());
    CHECK(!db.search_best("Air Jordan 1 Chicago").has_value());
    CHECK(!db.search_best("").has_value());
}


TEST(database, matches_past_a_brand_prefix){
    //API titles lead with the brand, so a user query is rarely a true prefix
    string path = tmp_db("brandprefix");
    DBClient db(path, CFG);
    db.insert(make_item("adidas Yeezy Boost 350 V2 Zebra", "yz-zebra", 295.0f));
    db.insert(make_item("adidas Yeezy 350 V2 CMPCT Slate Onyx", "yz-onyx", 156.0f));
    db.insert(make_item("Nike Dunk Low Panda", "dunk", 118.0f));

    auto onyx = db.search_best("Yeezy 350 V2 CMPCT Slate Onyx");
    CHECK(onyx.has_value());
    if(onyx.has_value()){ CHECK_EQ(string(onyx->id), string("yz-onyx")); }

    auto panda = db.search_best("dunk low panda");
    CHECK(panda.has_value());
    if(panda.has_value()){ CHECK_EQ(string(panda->id), string("dunk")); }
}


TEST(database, prefers_the_least_padded_match){
    //Both titles contain every query word; the tighter one should win
    string path = tmp_db("tightest");
    DBClient db(path, CFG);
    db.insert(make_item("adidas Yeezy Boost 350 V2 Zebra The Design Museum Charity Campaign", "yz-museum", 0.0f));
    db.insert(make_item("adidas Yeezy Boost 350 V2 Zebra", "yz-zebra", 295.0f));

    auto hit = db.search_best("Yeezy 350 V2 Zebra");
    CHECK(hit.has_value());
    if(hit.has_value()){ CHECK_EQ(string(hit->id), string("yz-zebra")); }
}
