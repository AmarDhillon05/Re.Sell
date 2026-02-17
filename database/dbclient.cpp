#include <iostream>
#include <fstream>
#include <cstdint>
#include "dbclient.hpp"

using namespace std;


//Implementd DBC methods
DBClient::DBClient(){
    npages = 0;
    pageMinOcc = 1;
    pageMaxOcc = 3;
    rootPg = 0;
    pgSize = sizeof(Page); //Should be (3 * (128 + 32)) + 16 + 32 = 528b
}


int DBClient::insert(Page page){
    fstream file("shoe.db", ios::binary | ios::out | ios::in);
    file.seekp(1 * pgSize, ios::beg);

    file.write(reinterpret_cast<char*>(&page), pgSize);

    file.close();

    return 0;
}

int DBClient::search(string query) const{
    fstream file("shoe.db", ios::binary | ios::in);
    file.seekg(1 * pgSize, ios::beg);

    Page page;
    file.read(reinterpret_cast<char*>(&page), pgSize);

    cout << "Read page w nentries = " << page.nentries  << " and rp = " << page.rpointer << endl;

    file.close();

    return 0;
}


int main(){
    DBClient cli;

    Page p1 = {};
    p1.nentries = 3;
    p1.rpointer = -1; //Signifies DNE

    int a = cli.insert(p1);
    int b = cli.search("query");

    return 0;
}