#pragma once

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace std;


//Upper bound baked into the on-disk page layout. The *effective* branching factor
//comes from database/config.json ("entries_per_page") and must stay <= this.
constexpr int DB_MAX_ENTRIES = 128;

//"RES1". Bumped whenever the layout below changes, so stale files are detected
//rather than reinterpreted as garbage.
constexpr uint32_t DB_MAGIC = 0x52455331;
constexpr uint32_t DB_VERSION = 1;


//Struct for storing data. Fixed size POD: written to disk verbatim.
struct Item {
    char title[128];   //Sort key
    char id[64];       //KicksDB / StockX product id, used to pull price history
    float last_price;  //Most recent observed sale price
    float fit_mu;      //Posterior mean price from the modeler
    float fit_sigma;   //Posterior spread from the modeler
    int32_t n_prices;  //Observations behind the fit
    int64_t updated_at;//Unix seconds
};

struct Page {
    uint16_t nentries;
    Item entries[DB_MAX_ENTRIES];
    int16_t child_pages[DB_MAX_ENTRIES + 1]; //One more child than entries
    int16_t parent_page;
};

//File header, so a reopened database still knows where its root is
struct DBHeader {
    uint32_t magic;
    uint32_t version;
    int32_t npages;
    int32_t rootPg;
    int32_t nentries;
    uint32_t page_size;
    uint32_t item_size;
    uint32_t max_occ;
};


//Builds a null terminated Item without overrunning the fixed buffers
Item make_item(const string& title, const string& id = "", float last_price = 0.0f);

//Helper to insert into a fixed array (guarenteed valid indices by dbhandler)
void insert(void* arr, const void* item, size_t item_size, int idx, int arrsize);


//DBCLIENT blueprint
class DBClient{

    private:
        string fpath;
        int npages;
        int nentries;
        int pageMinOcc;
        int pageMaxOcc;
        int rootPg;
        int pgSize;
        int maxPages;
        bool healthy;

        //Page 0 starts after the header
        streamoff page_offset(int idx) const;

        bool read_page(fstream& f, int idx, Page& p) const;
        bool write_page(fstream& f, int idx, const Page& p) const;

        bool load_header();
        bool store_header();
        void format();

        void load_config(const string& config_path);
        void collect(fstream& f, int idx, vector<Item>& out) const;

    public:

        DBClient(string path, string config_path = "database/config.json");

        //False when the file could not be opened or formatted
        bool ok() const { return healthy; }
        int size() const { return nentries; }
        int page_count() const { return npages; }
        int max_entries_per_page() const { return pageMaxOcc; }

        //Exact match on title, case insensitive
        optional<Item> search(string query) const;

        //Best match: exact, else prefix, else substring, else all-words-present.
        //Returns nothing rather than an unrelated "nearest" title.
        optional<Item> search_best(string query) const;

        //Insert, or overwrite in place when the title already exists
        bool insert(Item i);

        //Every item in sort order
        vector<Item> list_all() const;

        void raw_clear();
        void raw_insert(Page page);
        void raw_page_print() const;

};
