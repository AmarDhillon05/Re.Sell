#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>

using namespace std;

#pragma once



//Struct for storing data
struct Item {
    char title[128]; //No data as of now
};

struct Page {
    uint16_t nentries;
    Item entries[3];
    int16_t child_pages[4];
    int16_t parent_page;
};

//Print page
void print_page(Page page){

    for(int i = 0; i < 50; i ++){ cout << "*"; }

    cout << "Nentries: " << page.nentries << endl;
    for(int n = 0; n < page.nentries; n ++){
        cout << "Entry " << n << ": " << page.entries[n].title << endl;
    }
    cout << "Parent page: " << page.parent_page << endl;

    for(int i = 0; i < 50; i ++){ cout << "*"; }

}


//Helper to insert
void insert(void* arr, void* item, size_t item_size, int idx, int arrsize);


//DBCLIENT blueprint
class DBClient{

    private:
        int npages;
        int pageMinOcc;
        int pageMaxOcc;
        int rootPg;
        int pgSize;

    public:

        DBClient();

        Item search(string query) const;
        void insert(Item i);
        void raw_clear();
        void raw_insert(Page page);
        void raw_page_print();

};

