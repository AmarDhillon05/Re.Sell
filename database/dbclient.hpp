#include <iostream>
#include <fstream>
#include <cstdint>

using namespace std;

#pragma once



//Struct for storing data
struct Items {
    char title[128]; //No data as of data
    uint32_t lpointer; //Less than page redir
};

struct Page {
    uint16_t nentries;
    Items entries[3];
    int32_t rpointer; //Greater than page redir
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

