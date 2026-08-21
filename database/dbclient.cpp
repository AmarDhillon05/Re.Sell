#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <optional>
#include <strings.h>
#include <cctype>

#include "dbclient.hpp"
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;


namespace {

    //One comparator for the whole tree. Insert and search MUST agree on ordering,
    //otherwise a key gets filed somewhere search will never look.
    int key_cmp(const char* a, const char* b){
        return strcasecmp(a, b);
    }

    //A node is a leaf when it has no first child
    bool is_leaf(const Page& p){
        return p.child_pages[0] == -1;
    }

    //Lowercased, with runs of whitespace collapsed, for loose matching
    string fold(const string& in){
        string out;
        bool sp = false;
        for(unsigned char ch : in){
            if(isspace(ch)){ sp = !out.empty(); continue; }
            if(sp){ out += ' '; sp = false; }
            out += (char)tolower(ch);
        }
        return out;
    }

    void copy_field(char* dst, size_t cap, const string& src){
        size_t n = min(src.size(), cap - 1);
        memcpy(dst, src.data(), n);
        dst[n] = '\0';
    }

}


Item make_item(const string& title, const string& id, float last_price){
    Item it{};
    copy_field(it.title, sizeof(it.title), title);
    copy_field(it.id, sizeof(it.id), id);
    it.last_price = last_price;
    it.fit_mu = 0.0f;
    it.fit_sigma = 0.0f;
    it.n_prices = 0;
    it.updated_at = (int64_t)time(nullptr);
    return it;
}


//Helper method to insert (guarenteed valid indices by dbhandler)
void insert(void* arr, const void* item, size_t item_size, int idx, int arrsize) {
    char* base = static_cast<char*>(arr);

    //Move if we have to
    if(arrsize > idx){
        memmove(
            base + (idx + 1) * item_size,
            base + idx * item_size,
            (arrsize - idx) * item_size
        );
    }
    memcpy(
        base + idx * item_size,
        item,
        item_size
    );
}


//Implementd DBC methods
DBClient::DBClient(string path, string config_path){
    fpath = path;
    npages = 0; //This minus 1 = pointer to the most recent page
    nentries = 0;
    pageMinOcc = 1;
    pageMaxOcc = 4;
    rootPg = -1; //DNE
    pgSize = sizeof(Page);
    maxPages = 64;
    healthy = false;

    load_config(config_path);

    //An existing, compatible file is reopened; anything else is formatted fresh
    if(!load_header()){
        format();
    }
}


void DBClient::load_config(const string& config_path){
    ifstream cf(config_path);
    if(!cf){
        cerr << "[db] no config at " << config_path
             << ", using defaults (pages=" << maxPages
             << ", entries_per_page=" << pageMaxOcc << ")" << endl;
        return;
    }

    try {
        json cfg = json::parse(cf);
        if(cfg.contains("pages")){ maxPages = cfg["pages"].get<int>(); }
        if(cfg.contains("entries_per_page")){ pageMaxOcc = cfg["entries_per_page"].get<int>(); }
    } catch(const exception& e){
        cerr << "[db] could not parse " << config_path << ": " << e.what() << endl;
        return;
    }

    //A B-tree needs room for at least 2 children per split, and the page layout caps us
    if(pageMaxOcc < 2){ pageMaxOcc = 2; }
    if(pageMaxOcc > DB_MAX_ENTRIES){ pageMaxOcc = DB_MAX_ENTRIES; }
    if(maxPages < 1){ maxPages = 1; }

    pageMinOcc = pageMaxOcc / 2;
}


streamoff DBClient::page_offset(int idx) const {
    return (streamoff)sizeof(DBHeader) + (streamoff)idx * (streamoff)pgSize;
}


bool DBClient::read_page(fstream& f, int idx, Page& p) const {
    if(idx < 0 || idx >= npages){ return false; }
    f.clear();
    f.seekg(page_offset(idx), ios::beg);
    f.read(reinterpret_cast<char*>(&p), pgSize);
    return (bool)f;
}


bool DBClient::write_page(fstream& f, int idx, const Page& p) const {
    if(idx < 0){ return false; }
    f.clear();
    f.seekp(page_offset(idx), ios::beg);
    f.write(reinterpret_cast<const char*>(&p), pgSize);
    f.flush();
    return (bool)f;
}


bool DBClient::load_header(){
    fstream f(fpath, ios::binary | ios::in);
    if(!f){ return false; }

    DBHeader h{};
    f.read(reinterpret_cast<char*>(&h), sizeof(h));
    if(!f || f.gcount() != (streamsize)sizeof(h)){ return false; }

    //Reject anything written by a different layout instead of trusting the bytes
    if(h.magic != DB_MAGIC || h.version != DB_VERSION){
        cerr << "[db] " << fpath << " is not a v" << DB_VERSION
             << " database, reformatting" << endl;
        return false;
    }
    if(h.page_size != (uint32_t)pgSize || h.item_size != (uint32_t)sizeof(Item)){
        cerr << "[db] " << fpath << " has an incompatible page layout, reformatting" << endl;
        return false;
    }

    npages = h.npages;
    rootPg = h.rootPg;
    nentries = h.nentries;
    pageMaxOcc = (int)h.max_occ;
    pageMinOcc = pageMaxOcc / 2;
    if(npages > maxPages){ maxPages = npages; }

    healthy = true;
    return true;
}


bool DBClient::store_header(){
    fstream f(fpath, ios::binary | ios::in | ios::out);
    if(!f){ return false; }

    DBHeader h{};
    h.magic = DB_MAGIC;
    h.version = DB_VERSION;
    h.npages = npages;
    h.rootPg = rootPg;
    h.nentries = nentries;
    h.page_size = (uint32_t)pgSize;
    h.item_size = (uint32_t)sizeof(Item);
    h.max_occ = (uint32_t)pageMaxOcc;

    f.seekp(0, ios::beg);
    f.write(reinterpret_cast<const char*>(&h), sizeof(h));
    f.flush();
    return (bool)f;
}


//Truncates to a bare, valid, empty database
void DBClient::format(){
    npages = 0;
    nentries = 0;
    rootPg = -1;

    ofstream f(fpath, ios::binary | ios::out | ios::trunc);
    if(!f){
        cerr << "[db] cannot open " << fpath << " for writing" << endl;
        healthy = false;
        return;
    }
    f.close();

    healthy = store_header();
    if(!healthy){
        cerr << "[db] cannot write header to " << fpath << endl;
    }
}


//Straight file read methods for debugging
void DBClient::raw_insert(Page page){
    //in|out (never plain out, which truncates the whole file)
    fstream file(fpath, ios::binary | ios::in | ios::out);
    if(!file){ return; }

    if(npages >= maxPages){
        cerr << "[db] page limit of " << maxPages << " reached" << endl;
        return;
    }

    int idx = npages;
    npages += 1;
    write_page(file, idx, page);
    file.close();

    store_header();
}


void DBClient::raw_page_print() const {
    fstream file(fpath, ios::binary | ios::in);
    if(!file){ cerr << "[db] cannot open " << fpath << endl; return; }

    Page page;

    cout << "\n\nRoot = " << rootPg << endl;
    cout << "NPages = " << npages << "  NEntries = " << nentries
         << "  MaxOcc = " << pageMaxOcc << endl;

    for(int i = 0; i < npages; i ++){
        if(!read_page(file, i, page)){ break; }

        for(int d = 0; d < 50; d ++){ cout << "-"; }
        cout << endl;

        cout << "Page " << i << endl;
        cout << "Nentries: " << page.nentries << endl;
        for(int n = 0; n < page.nentries; n ++){
            cout << "Entry " << n << ": " << page.entries[n].title
                 << " ($" << page.entries[n].last_price << ")" << endl;
        }
        cout << "Parent page: " << page.parent_page << endl;
        cout << "Child pages: ";
        //nentries entries means nentries + 1 children
        for(int n = 0; n < page.nentries + 1; n ++){
            cout << page.child_pages[n] << ", ";
        }
        cout << endl;
    }

    for(int d = 0; d < 50; d ++){ cout << "-"; }
    cout << endl;
}


void DBClient::raw_clear(){
    format();
}


//B-tree item insert
bool DBClient::insert(Item item){

    if(!healthy){ return false; }

    fstream file(fpath, ios::binary | ios::in | ios::out);
    if(!file){ cerr << "[db] cannot open " << fpath << endl; return false; }

    //Empty tree: the item becomes a one entry root
    if(rootPg == -1){
        Page p{};
        p.nentries = 1;
        p.entries[0] = item;
        for(int c = 0; c <= pageMaxOcc; c ++){ p.child_pages[c] = -1; }
        p.parent_page = -1;

        if(npages >= maxPages){ cerr << "[db] page limit reached" << endl; return false; }
        rootPg = npages;
        npages += 1;
        nentries += 1;

        write_page(file, rootPg, p);
        file.close();
        return store_header();
    }

    //Search for correct node
    int currPg = rootPg;
    Page page;

    while(true){
        if(!read_page(file, currPg, page)){ return false; }

        //An existing key is updated in place, wherever it sits in the tree
        for(int x = 0; x < page.nentries; x ++){
            if(key_cmp(page.entries[x].title, item.title) == 0){
                page.entries[x] = item;
                write_page(file, currPg, page);
                file.close();
                return true;
            }
        }

        if(is_leaf(page)){ break; }

        //First entry greater than the key decides the child; otherwise go rightmost.
        //The original overwrote this choice with the rightmost child unconditionally.
        int c = 0;
        while(c < page.nentries && key_cmp(item.title, page.entries[c].title) > 0){ c ++; }
        currPg = page.child_pages[c];

        if(currPg == -1){ break; }
    }


    //These vars should guarentee order (left stays the same, right is new)
    int16_t newleft = -1;
    int16_t newright = -1;
    int page_toadd = currPg;
    Item toinsert = item;


    while(true){

        //New root case: add the new root, then repoint both children at it
        if(page_toadd == -1){

            Page p{};
            p.nentries = 1;
            p.entries[0] = toinsert;
            for(int c = 0; c <= pageMaxOcc; c ++){ p.child_pages[c] = -1; }
            p.child_pages[0] = newleft;
            p.child_pages[1] = newright;
            p.parent_page = -1;

            if(npages >= maxPages){ cerr << "[db] page limit reached" << endl; return false; }
            int newRoot = npages;
            npages += 1;
            rootPg = newRoot;

            write_page(file, newRoot, p);

            int16_t children[2] = {newleft, newright};
            for(int16_t idx : children){
                if(idx != -1){
                    Page child;
                    if(read_page(file, idx, child)){
                        child.parent_page = (int16_t)newRoot;
                        write_page(file, idx, child);
                    }
                }
            }

            nentries += 1;
            file.close();
            return store_header();
        }

        //Insertion Loop
        Page p;
        if(!read_page(file, page_toadd, p)){ return false; }

        //Locating location to write to
        int idx = 0;
        while(idx < p.nentries && key_cmp(toinsert.title, p.entries[idx].title) > 0){ idx ++; }

        //Case in which there's room to insert here
        if(p.nentries < pageMaxOcc){
            ::insert(p.entries, &toinsert, sizeof(Item), idx, p.nentries);
            ::insert(p.child_pages, &newright, sizeof(int16_t), idx + 1, p.nentries + 1);
            p.nentries += 1;

            write_page(file, page_toadd, p);

            nentries += 1;
            file.close();
            return store_header();
        }

        //Full page: overflow into scratch arrays, then split down the middle.
        //Splitting in place got the counts wrong whenever the new key landed left
        //of the median, which silently dropped an entry.
        Item tmp_entries[DB_MAX_ENTRIES + 1];
        int16_t tmp_children[DB_MAX_ENTRIES + 2];

        memcpy(tmp_entries, p.entries, sizeof(Item) * p.nentries);
        memcpy(tmp_children, p.child_pages, sizeof(int16_t) * (p.nentries + 1));

        ::insert(tmp_entries, &toinsert, sizeof(Item), idx, p.nentries);
        ::insert(tmp_children, &newright, sizeof(int16_t), idx + 1, p.nentries + 1);

        int total = p.nentries + 1;      //Entries held across the scratch arrays
        int mid = total / 2;             //Promoted to the parent
        int left_size = mid;
        int right_size = total - mid - 1;

        //Left keeps the low half and is written back over the original page
        Page left{};
        left.nentries = (uint16_t)left_size;
        left.parent_page = p.parent_page;
        memcpy(left.entries, tmp_entries, sizeof(Item) * left_size);
        memcpy(left.child_pages, tmp_children, sizeof(int16_t) * (left_size + 1));
        for(int c = left_size + 1; c <= pageMaxOcc; c ++){ left.child_pages[c] = -1; }

        //Right is a brand new page holding the high half
        Page right{};
        right.nentries = (uint16_t)right_size;
        right.parent_page = p.parent_page;
        memcpy(right.entries, tmp_entries + mid + 1, sizeof(Item) * right_size);
        memcpy(right.child_pages, tmp_children + mid + 1, sizeof(int16_t) * (right_size + 1));
        for(int c = right_size + 1; c <= pageMaxOcc; c ++){ right.child_pages[c] = -1; }

        Item median = tmp_entries[mid];

        if(npages >= maxPages){ cerr << "[db] page limit reached" << endl; return false; }
        int rightPg = npages;
        npages += 1;

        write_page(file, page_toadd, left);
        write_page(file, rightPg, right);

        //Everything that moved to the right page needs its parent pointer rewritten,
        //or later inserts walk back up into the wrong node.
        if(!is_leaf(right)){
            for(int c = 0; c <= right_size; c ++){
                int16_t ch = right.child_pages[c];
                if(ch == -1){ continue; }
                Page cp;
                if(read_page(file, ch, cp)){
                    cp.parent_page = (int16_t)rightPg;
                    write_page(file, ch, cp);
                }
            }
        }

        //Recurse: the median moves up, with left/right as its two children
        newleft = (int16_t)page_toadd;
        newright = (int16_t)rightPg;
        toinsert = median;
        page_toadd = p.parent_page;
    }
}


//B-tree search
optional<Item> DBClient::search(string query) const {

    if(!healthy || rootPg == -1){ return nullopt; }

    fstream file(fpath, ios::in | ios::binary);
    if(!file){ return nullopt; }

    Item key = make_item(query);
    int currPg = rootPg;

    while(currPg != -1){
        Page p;
        if(!read_page(file, currPg, p)){ return nullopt; }

        int i = 0;
        while(i < p.nentries){
            int cmp_res = key_cmp(key.title, p.entries[i].title);
            if(cmp_res == 0){ return p.entries[i]; }
            if(cmp_res < 0){ break; }
            i ++;
        }

        if(is_leaf(p)){ return nullopt; }
        currPg = p.child_pages[i];
    }

    return nullopt;
}


//Best match, used by the CLI so a partial title still resolves.
//
//The tree is ordered by full title, so anything looser than an exact hit needs a
//scan. That is fine at this size (capped by config.json, a few hundred items).
optional<Item> DBClient::search_best(string query) const {

    if(!healthy || rootPg == -1){ return nullopt; }

    //Exact hits still go through the tree
    optional<Item> exact = search(query);
    if(exact.has_value()){ return exact; }

    string q = fold(query);
    if(q.empty()){ return nullopt; }

    vector<Item> all = list_all();

    //Query words must all appear somewhere in the title. This is what lets
    //"Yeezy 350 V2 Zebra" find "adidas Yeezy Boost 350 V2 Zebra", where the
    //brand prefix means the query is not a prefix at all.
    vector<string> words;
    {
        string cur;
        for(char ch : q){
            if(ch == ' '){ if(!cur.empty()){ words.push_back(cur); cur.clear(); } }
            else { cur += ch; }
        }
        if(!cur.empty()){ words.push_back(cur); }
    }

    const Item* best = nullptr;
    int best_rank = 99;
    size_t best_len = string::npos;

    for(const Item& it : all){
        string t = fold(it.title);

        int rank;
        if(t.rfind(q, 0) == 0){ rank = 0; }             //Prefix
        else if(t.find(q) != string::npos){ rank = 1; } //Substring
        else {
            bool all_present = !words.empty();
            for(const string& w : words){
                if(t.find(w) == string::npos){ all_present = false; break; }
            }
            if(!all_present){ continue; }               //No relation: skip it
            rank = 2;
        }

        //Better rank wins; among equals prefer the title with the least padding
        size_t len = t.size();
        if(rank < best_rank || (rank == best_rank && len < best_len)){
            best = &it;
            best_rank = rank;
            best_len = len;
        }
    }

    //Deliberately no "closest in sort order" fallback: returning an unrelated
    //shoe reads as a cache hit and is worse than admitting the miss.
    if(best == nullptr){ return nullopt; }
    return *best;
}


void DBClient::collect(fstream& f, int idx, vector<Item>& out) const {
    if(idx == -1){ return; }

    Page p;
    if(!read_page(f, idx, p)){ return; }

    //Copy the page before recursing: the shared Page buffer would be clobbered
    Page local = p;
    bool leaf = is_leaf(local);

    for(int i = 0; i < local.nentries; i ++){
        if(!leaf){ collect(f, local.child_pages[i], out); }
        out.push_back(local.entries[i]);
    }
    if(!leaf){ collect(f, local.child_pages[local.nentries], out); }
}


vector<Item> DBClient::list_all() const {
    vector<Item> out;
    if(!healthy || rootPg == -1){ return out; }

    fstream file(fpath, ios::in | ios::binary);
    if(!file){ return out; }

    collect(file, rootPg, out);
    return out;
}
