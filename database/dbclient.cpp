#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "dbclient.hpp"

using namespace std;


//Implementd DBC methods
DBClient::DBClient(){
    npages = 0;
    pageMinOcc = 1;
    pageMaxOcc = 3;
    rootPg = 0;
    pgSize = sizeof(Page); 
}



//Straight file read methods for debugging
void DBClient::raw_insert(Page page){
    fstream file("shoe.db", ios::binary | ios::out);
    file.seekg(npages*pgSize, ios::beg);

    file.write(reinterpret_cast<char*>(&page), pgSize);
    npages += 1;

    file.close();
}


void DBClient::raw_page_print(){
    fstream file("shoe.db", ios::binary | ios::in);
    Page page;
    for(int i = 0; i < npages; i ++){
        file.seekp(i*pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&page), pgSize);

        cout << "Page " << i << endl;
        for(int n = 0; n < page.nentries; n ++){
            cout << "Entry " << n << ": " << page.entries[n].title << endl;
        }
        cout << "Parent page: " << page.parent_page << endl;
    }
}

void DBClient::raw_clear(){
    fstream file("shoe.db", ios::out | ios::trunc);
    file.close();
}




int main(){
    DBClient cli;
    cli.raw_clear();

    Page p1 = {};
    p1.nentries = 2;
    for(int i = 0; i < 4; i ++){
        p1.child_pages[i] = -1;
    }
    p1.entries[0] = {"a"};
    p1.entries[1] = {"b"};
    p1.parent_page = -1;
    

    cli.raw_insert(p1);
    cli.raw_page_print();


    return 0;
}