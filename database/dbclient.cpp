#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <optional>
#include "dbclient.hpp"

using namespace std;

//Helper method to insert (guarenteed valid indices by dbhandler)
void insert(void* arr, void* item, size_t item_size, int idx, int arrsize) {
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
DBClient::DBClient(){
    npages = 0; //This minus 1 = pointer to the most recent page 
    pageMinOcc = 1;
    pageMaxOcc = 4;
    rootPg = -1; //DNE
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

    cout << "\n\nRoot = " << rootPg << endl;
    cout << "NPages = " << npages << endl;
    for(int i = 0; i < npages; i ++){
        file.seekp(i*pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&page), pgSize);

        for(int i = 0; i < 50; i ++){ cout << "-"; }
        cout << endl;

        cout << "Page " << i << endl;
        cout << "Nentries: " << page.nentries << endl;
        for(int n = 0; n < page.nentries; n ++){
            cout << "Entry " << n << ": " << page.entries[n].title << endl;
        }
        cout << "Parent page: " << page.parent_page << endl;
        cout << "Child pages: ";
        for(int i = 0; i < page.nentries + 1; i ++){
            cout << page.child_pages[i] << ", ";
        }
        cout << endl;
        
    }

    for(int i = 0; i < 50; i ++){ cout << "-"; }
    cout << endl;

}

void DBClient::raw_clear(){
    fstream file("shoe.db", ios::out | ios::trunc);
    file.close();
}


//B-tree item insert
void DBClient::insert(Item i){


    fstream file("shoe.db", ios::binary | ios::in | ios::out);

    int16_t currPg = rootPg;

    //Search for correct node
    while(currPg != -1){
        Page page;
        file.seekg(currPg * pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&page), pgSize);
        
        bool atLeaf = true;
        for(int x = 0; x < page.nentries; x++){
            if(page.child_pages[x] != -1){
                atLeaf = false;
                if(strcmp(page.entries[x].title, i.title) > 0){
                    currPg = page.child_pages[x]; 
                    break;
                }
            }
        }
        
        if(atLeaf){ break; } //All should break at leaf
        currPg = page.child_pages[page.nentries];
    }
    

    //These vars should guarentee order (left stays the same, right is new)
    int16_t newleft = -1;
    int16_t newright = -1;
    int16_t page_toadd = currPg;
    Item toinsert = i;


    //New root case: Add the new root + new pages raw and rewrite parents of previous, then return
    while(1){
        if(page_toadd == -1){
        
            Page p;
            p.nentries = 1;
            p.entries[0] = toinsert;
            for(int i = 0; i <= pageMaxOcc; i ++){ p.child_pages[i] = -1; }
            p.child_pages[0] = newleft;
            p.child_pages[1] = newright;
            p.parent_page = -1;

            npages += 1;
            rootPg = npages - 1;

            int16_t children[2] = {newleft, newright};
            for(int16_t idx : children){
                if(idx != -1){
                    Page child;
                    file.seekg(idx * pgSize, ios::beg);
                    file.read(reinterpret_cast<char*>(&child), pgSize);
                    child.parent_page = rootPg;
                    file.seekp(idx * pgSize, ios::beg);
                    file.write(reinterpret_cast<char*>(&child), pgSize);
                }
            }
            
            file.seekp(rootPg * pgSize, ios::beg);
            file.write(reinterpret_cast<char*>(&p), pgSize);


            return;
        }

        //Insertion Loop
        else{

            Page p;
            file.seekg(page_toadd * pgSize, ios::beg);
            file.read(reinterpret_cast<char*>(&p), pgSize);

            //Locating location to write to
            int idx = 0;
            while(idx < p.nentries){
                if(strcmp(
                    toinsert.title, p.entries[idx].title
                ) < 0){
                    break;
                }
                idx ++;
            }

            //Case in which there's room to insert here
            if(p.nentries < pageMaxOcc){


                //TDOO -> Go back and fix this indexing (or just check if it works)
                ::insert(p.entries, &toinsert, sizeof(toinsert), idx, p.nentries);
                ::insert(p.child_pages, &newright, sizeof(newright), idx+1, p.nentries+1); 
                p.nentries += 1;
                
                file.seekg(page_toadd * pgSize, ios::beg);
                file.write(reinterpret_cast<char*>(&p), pgSize);
                return;
            }

            //If there's not enough room to insert
            else{

                //Index left_size is what is passed up
                int left_size = (int)(p.nentries / 2);

                //Create new page for right + write
                Page new_right;
                new_right.parent_page = p.parent_page;
                new_right.nentries = (uint16_t)(p.nentries - left_size - 1);

                for(int idx_left = left_size+1; idx_left < p.nentries; idx_left ++){ 
                    new_right.entries[idx_left-left_size-1] = p.entries[idx_left]; 
                }

                for(int idx_left = left_size+1; idx_left <= p.nentries; idx_left ++){ 
                    new_right.child_pages[idx_left-left_size-1] = p.child_pages[idx_left]; 
                }


                if(idx > left_size){
                    int nidx = idx-left_size-1;
                    ::insert(new_right.entries, &toinsert, sizeof(toinsert), nidx, new_right.nentries);
                    ::insert(new_right.child_pages, &newright, sizeof(newright), nidx+1, new_right.nentries+1);
                    new_right.nentries += 1;
                }

                npages += 1;
                file.seekp((npages-1) * pgSize, ios::beg);
                file.write(reinterpret_cast<char*>(&new_right), pgSize);
                newright = npages-1;

                //Edit left (doesn't change raw entries, but shouldn't need to)
                p.nentries = left_size + 1;
                if(idx <= left_size){
                    ::insert(p.entries, &toinsert, sizeof(toinsert), idx, left_size);
                    ::insert(p.child_pages, &newright, sizeof(newright), idx+1, left_size+1); 
                    p.nentries += 1;
                }
                p.nentries -= 1;

                file.seekp(page_toadd * pgSize, ios::beg);
                file.write(reinterpret_cast<char*>(&p), pgSize);
                newleft = page_toadd;

                //Recurse
                page_toadd = p.parent_page;
                toinsert = p.entries[left_size];

            }
        }

    }

 
}

//B-tree search
optional<Item> DBClient::search(string query) const {
    
    fstream file("shoe.db", ios::in | ios::out | ios::binary);
    int16_t currPg = rootPg;

    optional<Item> last_valid = nullopt;

    while(currPg != -1){
        Page p;
        file.seekg(currPg * pgSize, ios::beg);
        file.read(reinterpret_cast<char*>(&p), pgSize);

        int currPgBef = currPg;
        
        //Loop through children
        for(int i = 0; i < p.nentries; i ++){
            Item child_contender = p.entries[i];
            int cmp_res = strcmp(query.c_str(), child_contender.title);

            //If this is equal, return
            if(cmp_res == 0){
                return child_contender;
            }

            //Else if smaller, either traverse or stop here
            else if(cmp_res < 0){
                if(p.child_pages[i] != -1){ 
                    currPg = p.child_pages[i]; 
                }
                else{
                    //Super soft middle comparison
                    if( last_valid != nullopt &&  ( 
                        strcmp(query.c_str(), last_valid.value().title) < -1*cmp_res |
                        query[0] == last_valid.value().title[0]
                    )){
                        return last_valid;
                    }
                    return child_contender;
                }
                break;
            }

            //If still here (moving on), set last_valid
            else{ last_valid = p.entries[i]; }

            
        }


        //If no change, go to rightmost
        if(currPg == currPgBef){
            last_valid = p.entries[p.nentries - 1];
            currPg = p.child_pages[p.nentries];
        }

    }

    return last_valid;

}



int main(){
    DBClient cli;
    cli.raw_clear();

    cli.insert({ "b" });
    cli.insert({ "d" });
    cli.insert({ "a" });
    cli.insert({ "c" });
    cli.insert({ "f" });
    cli.insert({ "g" });
    cli.raw_page_print();

    string query = "fetus";
    optional<Item> x = cli.search(query);
    if(x == nullopt){
        cout << "Search failed" << endl;
    }
    else{
        cout << "Found " << x.value().title << " when searching for " << query << endl;
    }


    return 0;
}