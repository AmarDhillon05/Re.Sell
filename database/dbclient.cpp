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


//B-tree item insert
void insert(Item i){

    //Locate item + its parent if needed
    int16_t currPg = rootPg;

    fstream file("shoe.db", ios::binary | ios::in | ios::out);

    while(true){
        Page page;
        file.seekp(currPg * pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&page), pgSize);
        
        bool atLeaf = true;
        for(int i = 0; i < page.nentries; i++){
            if(page.child_pages[i] != -1){
                atLeaf = false;
                if(strcmp(page.entries[i].title, i.title) > 0){
                    currPg = page.child_pages[i]; 
                    break;
                }
            }
        }

        if(atLeaf){ break; } //All should break at leaf
        currPg = page.child_pages[page.nentries];
    }

    //Recursively split, create new right node, pass up middle node
    int16_t new_page = -1;
    Item to_insert = i;
    Page page;

    while(true){

        //if currPg DNE, make a new root
        if(currPg == -1){
            Page p;
            p.nentries = 0;
            npages += 1;
            currPg = npages;
            rootPg = npages;

            file.seekp(currPg * pgSize, ios::beg);
            file.write(reinterpret_cast<char*>(&curr_page), pgSize);
        }

        file.seekg(currPg * pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&curr_page), pgSize);
        
        //Find loc to insert 
        int idx = 0
        while(idx < page.nentries){
            if(strcmp(to_insert.title, page.entries[idx].title) < 0){ break; }
            idx ++;
        }

        //PLain insert if there's room
        if(page.nentries < pageMaxOcc){

            //TODO: write this method
            insert(*page.entries, to_insert, idx);
            insert(*page.child_pages, new_page, idx);

            file.seekg(currPg * pgSize, ios::beg);
            file.write(reinterpret_cast<char*>(&page), pgSize);
            file.close();

            return;
        }

        //Else, divide and move up
        else{
            //Create new left 
            Page new_left;
            new_left.nentries = (int)(pageMaxOcc / 2);
            for(int i = 0; i < p1.nentries; i ++){
                new_left.entries[i] = page.entries[i];
            }
            for(int i = 0; i < p1.nentries + 1; i ++){
                new_left.child_pages[i] = page.child_pages[i];
            }
            new_left.parent_page = page.parent_page;

            //Insert into left if it fits
            if(idx < new_left.nentries){
                insert(*new_left.entries, to_insert, idx);
                insert(*new_left.child_pages, new_page, idx);
            }

            //Write left
            npages += 1;
            file.seekg(npages * pgSize, ios::beg);
            file.write(reinterpret_cast<char*>(&page), pgSize);

            //Move elements in the right
            for(int i = new_left.nentries; i < page.nentries; i ++){
                page.entries[i - new_left.nentries] = page.nentries[i];
            }
            for(int i = new_left.nentries + 1; i < page.nentries + 1; i ++){
                page.child_pages[i - new_left.nentries - 1] = page.child_pages[i];
            }
            page.nentries = page.nentries - new_left.nentries;

            //Insert into the right if needed
            if(idx > new_left.nentries){
                idx -= new_left.nentries;
                insert(*page.entries, to_insert, idx);
                insert(*new_left.child_pages, new_page, idx);
            }

            //Write right and update loop vars
            file.seekg(currPg * pgSize, ios::beg);
            file.write(reinterpret_cast<char*>(&page), pgSize);

            currPg = page.parent_page;
            new_page = npages;
        }

    }

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