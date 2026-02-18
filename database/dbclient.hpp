#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>

using namespace std;

#pragma once



//Struct for storing data
struct Item {
    char title[128]; //No data as of data
    int32_t lpointer; //Less than page redir
};

struct Page {
    uint16_t nentries;
    Item entries[3];
    int32_t rpointer; //Greater than page redir
    int32_t ppointer; //Parent
};




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

        int search(string query) const;
        int insert(Page query);

};

