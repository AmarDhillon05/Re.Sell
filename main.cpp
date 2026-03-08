#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

#include "database/dbclient.hpp"
#include "clients/kicksdb_client.hpp"
#include <iostream>
#include <unordered_map>

using namespace std;


int main(){
    
    //Client init
    KicksDB_Client kcli;
    cout << kcli.search("Air Jordan 1") << endl;
    
    return 0;
}