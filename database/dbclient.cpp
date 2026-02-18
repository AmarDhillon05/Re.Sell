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



int DBClient::insert(Item item){
    fstream file("shoe.db", ios::binary | ios::out | ios::in);

    //Locate proper location to add into
    //Items within should already be presorted
    int currPg = rootPg;
    Page page;

    while(true){
        file.seekp(currPg*pgSize, ios::beg);
        file.write(reinterpret_cast<char*>(&page), pgSize);
        if(page.nentries == pageMaxOcc){
            //Go right if applicable
            Item maxItem = page.entries[pageMaxOcc - 1];
            if(strcmp(item.title, maxItem.title) > 0){
                if(page.rpointer != -1){
                    currPg = page.rpointer;
                }
                else{
                    break
                }
            }
            //Find proper entry (if it exists)
            else{
                for(Item entryItem : page.entries){
                    if(strcmp(item.title, entryItem.title) < 0){
                        if(entryItem.lpointer != -1){
                            currPg = page.lpointer;
                        }
                        else{
                            break;
                        }
                    }
                }
            }
           
        }
        else{
            break;
        }
    }

    //Either add or split + loop on precursor
    //make a complete sorted list -> make splits if needed, -> sift up

    //Find insertion
    int insertIdx = 0;
    for(int i = 0; i < page.nentries; i ++){
        if(strcmp(item.title, page.nentries[i].title) < 0){
            insertIdx = i; 
            break;
        }
    }

    //If we have space, insert and done 
    if(page.nentries < pageMaxOcc){
        page.nentries += 1;
        for(int i = page.nentries - 1; i >= insertIdx; i++){
            page.nentries[i + 1] = page.nentries[i];
        }
        memcpy(&page[insertIdx], &item, sizeof(Item));
        return 0; 
        //TODO -> look at readjusting pointers
    }


    //Case where theres an overflow -> BIG TODO
    

    
    file.close();

    return 0;
}


int main(){
    DBClient cli;

    /*
    Page p1 = {};
    p1.nentries = 2;
    p1.rpointer = -1; //Signifies DNE
    p1.ppointer = -1; //Signifies this is root
    p1.entries[0] = {"a", -1};
    p1.entries[1] = {"b", -1};
    */

    int _ = cli.insert();


    return 0;
}