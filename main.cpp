#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

#include "database/dbclient.hpp"
#include "clients/kicksdb_client.hpp"
#include "auth/auth.hpp"
#include "plot/plot.hpp"
#include "models/hmc.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <ctime>
#include <random>

using namespace std;


static const char* DB_PATH = "database/shoe.db";
static const char* CONFIG_PATH = "database/config.json";

static const int PLOT_W = 90;
static const int PLOT_H = 18;


static void usage(){
    cout << "Re.Sell - sneaker resale price tracker\n\n"
         << "Usage: ./main <command> [args]\n\n"
         << "  search <query>     Search the API and cache the results locally\n"
         << "  track  <query>     Search, pull sale history, fit the model, save it\n"
         << "  history <query>    Plot the sale history of the best match\n"
         << "  fit <query>        Fit the price model for a tracked or searched shoe\n"
         << "  show <query>       Show what is stored locally for a shoe\n"
         << "  list               List everything tracked locally\n"
         << "  pages              Dump raw database pages (debug)\n"
         << "  clear              Reset the local database\n"
         << "  token              Refresh a StockX access token from .env\n"
         << "  demo               Run the plotter and sampler on synthetic data\n"
         << endl;
}


//Shared: print a fitted posterior
static void print_fit(const string& title, const HMCResult& fit, int n_obs){
    cout << GREEN << "Fitted " << title << RESET << endl;
    cout << fixed << setprecision(2);
    cout << "  observations   " << n_obs << endl;
    cout << "  mean price     $" << fit.mu_mean << "  (+/- " << fit.mu_sd << ")" << endl;
    cout << "  volatility     $" << fit.sigma_mean << "  (+/- " << fit.sigma_sd << ")" << endl;

    //Roughly the middle 95% of sales under the fitted Normal
    cout << "  typical range  $" << (fit.mu_mean - 1.96f * fit.sigma_mean)
         << " - $" << (fit.mu_mean + 1.96f * fit.sigma_mean) << endl;
    cout << defaultfloat;
    cout << "  (" << fit.n_samples << " draws, acceptance " << fit.accept_rate << ")" << endl;

    //With a handful of sales the posterior spread is dominated by the flat prior,
    //so the range above is far wider than the shoe's real variation.
    if(n_obs < 10){
        cout << YELLOW << "  warning: only " << n_obs
             << " sale(s) - treat this fit as indicative only" << RESET << endl;
    }
}


//Shared: resolve a query to a product, preferring the local cache
static bool resolve(const KicksDB_Client& api, DBClient& db, const string& query,
                    string& out_id, string& out_title){

    optional<Item> cached = db.search_best(query);
    if(cached.has_value() && strlen(cached.value().id) > 0){
        out_id = cached.value().id;
        out_title = cached.value().title;
        cout << GREEN << "[cache] hit" << RESET << " \"" << query << "\" -> "
             << out_title << " (" << out_id << ")" << endl;
        return true;
    }

    //A local title with no id is a partial record: it cannot answer an API call
    if(cached.has_value()){
        cout << YELLOW << "[cache] partial" << RESET << " \"" << query
             << "\" matched " << cached.value().title
             << " but it has no id, refetching" << endl;
    } else {
        cout << "[cache] miss \"" << query << "\"" << endl;
    }

    if(!api.ready()){
        cout << RED << "No local match, and the API is unavailable." << RESET << endl;
        return false;
    }

    vector<Product> hits = api.search_products(query, 5);
    if(hits.empty()){
        cout << RED << "No results for \"" << query << "\"" << RESET << endl;
        return false;
    }

    out_id = hits[0].id;
    out_title = hits[0].title;
    cout << "Best match: " << out_title << " (" << out_id << ")" << endl;

    //Cache what we just learned so the next lookup can skip the network.
    //A size change is what distinguishes a fresh row from an overwrite.
    int before = db.size();
    if(db.insert(make_item(out_title, out_id, hits[0].price))){
        cout << "[cache] " << (db.size() > before ? "stored" : "refreshed")
             << " " << out_title << endl;
    } else {
        cout << YELLOW << "[cache] not stored: database is full" << RESET << endl;
    }
    return true;
}


static int cmd_search(KicksDB_Client& api, DBClient& db, const string& query){
    if(!api.ready()){
        cout << RED << "Searching needs a KICKSDB_API_KEY in .env." << RESET << endl;
        return 1;
    }

    vector<Product> hits = api.search_products(query, 20);
    if(hits.empty()){
        cout << RED << "No results for \"" << query << "\"" << RESET << endl;
        return 1;
    }

    cout << GREEN << hits.size() << " result(s)" << RESET << endl;

    int stored = 0;
    int refreshed = 0;
    bool full = false;

    for(size_t i = 0; i < hits.size(); i ++){
        cout << "  " << setw(2) << (i + 1) << ". " << hits[i].title;
        if(!hits[i].brand.empty()){ cout << "  [" << hits[i].brand << "]"; }
        if(hits[i].price > 0){ cout << "  $" << fixed << setprecision(2) << hits[i].price << defaultfloat; }
        cout << endl;

        //Cache every hit so `show` / `list` work without the network
        int before = db.size();
        if(!db.insert(make_item(hits[i].title, hits[i].id, hits[i].price))){
            cout << YELLOW << "  (not cached: database is full)" << RESET << endl;
            full = true;
            break;
        }
        if(db.size() > before){ stored ++; } else { refreshed ++; }
    }

    cout << "[cache] " << stored << " stored, " << refreshed << " refreshed";
    if(full){ cout << YELLOW << " (database full)" << RESET; }
    cout << " - " << db.size() << " item(s) tracked" << endl;

    return 0;
}


static int cmd_history(KicksDB_Client& api, DBClient& db, const string& query, bool do_fit){

    string id, title;
    if(!resolve(api, db, query, id, title)){ return 1; }

    if(!api.ready()){
        cout << RED << "Sale history needs the API." << RESET << endl;
        return 1;
    }

    vector<float> prices = api.price_series(id);
    if(prices.empty()){
        cout << RED << "No sale history for " << title << RESET << endl;
        return 1;
    }

    Plotbot pb(PLOT_W, PLOT_H);
    pb.append(prices.data(), (int)prices.size());
    pb.plot(title + " - sale price ($)");

    if(!do_fit){ return 0; }

    HMCResult fit = hmc_fit_normal(prices);
    if(!fit.ok){
        cout << RED << "Fit failed: " << fit.error << RESET << endl;
        return 1;
    }

    print_fit(title, fit, (int)prices.size());

    //Persist the fit alongside the item
    Item it = make_item(title, id, prices.back());
    it.fit_mu = fit.mu_mean;
    it.fit_sigma = fit.sigma_mean;
    it.n_prices = (int32_t)prices.size();
    db.insert(it);

    return 0;
}


static int cmd_show(DBClient& db, const string& query){
    optional<Item> hit = db.search_best(query);
    if(!hit.has_value()){
        cout << "[cache] miss \"" << query << "\"" << endl;
        cout << RED << "Nothing tracked matching \"" << query << "\"" << RESET << endl;
        return 1;
    }

    const Item& it = hit.value();
    cout << GREEN << "[cache] hit" << RESET << " \"" << query << "\" -> "
         << it.title << endl;
    cout << GREEN << it.title << RESET << endl;
    cout << "  id            " << (strlen(it.id) ? it.id : "(unknown)") << endl;
    cout << fixed << setprecision(2);
    cout << "  last price    $" << it.last_price << endl;

    if(it.n_prices > 0){
        cout << "  fitted mean   $" << it.fit_mu << endl;
        cout << "  volatility    $" << it.fit_sigma << endl;
        cout << "  observations  " << it.n_prices << endl;
    } else {
        cout << "  " << YELLOW << "not fitted yet - run: ./main fit \"" << it.title << "\"" << RESET << endl;
    }
    cout << defaultfloat;

    time_t t = (time_t)it.updated_at;
    cout << "  updated       " << (t > 0 ? ctime(&t) : "(unknown)\n");

    return 0;
}


static int cmd_list(DBClient& db){
    vector<Item> all = db.list_all();
    if(all.empty()){
        cout << "Nothing tracked yet. Try: ./main search \"Air Jordan 1\"" << endl;
        return 0;
    }

    cout << GREEN << all.size() << " tracked item(s)" << RESET << endl;
    for(const Item& it : all){
        cout << "  " << left << setw(46) << it.title << right << fixed << setprecision(2);
        cout << "  $" << setw(9) << it.last_price;
        if(it.n_prices > 0){ cout << "   fit $" << it.fit_mu << " +/- " << it.fit_sigma; }
        cout << defaultfloat << endl;
    }

    return 0;
}


//Offline path: exercises the plotter and the sampler without any network access
static int cmd_demo(){
    const int N = 240;
    vector<float> prices(N);

    mt19937 rng(7);
    normal_distribution<float> noise(0.0f, 18.0f);
    for(int i = 0; i < N; i ++){
        //A drifting price with noise, roughly what a sale series looks like
        prices[i] = 300.0f + 40.0f * sin(i * 0.05f) + 0.15f * i + noise(rng);
    }

    Plotbot pb(PLOT_W, PLOT_H);
    pb.append(prices.data(), N);
    pb.plot("demo shoe - synthetic sale price ($)");

    HMCResult fit = hmc_fit_normal(prices);
    if(!fit.ok){
        cout << RED << "Fit failed: " << fit.error << RESET << endl;
        return 1;
    }
    print_fit("demo shoe", fit, N);

    return 0;
}


int main(int argc, char** argv){

    if(argc < 2){ usage(); return 1; }

    const string cmd = argv[1];
    const string arg = (argc > 2) ? argv[2] : "";

    //Commands that need neither the database nor the network
    if(cmd == "help" || cmd == "-h" || cmd == "--help"){ usage(); return 0; }
    if(cmd == "demo"){ return cmd_demo(); }
    if(cmd == "token"){
        string tok = stockx_access_token();
        if(tok.empty()){ cout << RED << "Could not refresh a StockX token." << RESET << endl; return 1; }
        cout << GREEN << "Access token: " << RESET << mask_secret(tok) << endl;
        return 0;
    }

    //Database init
    DBClient db(DB_PATH, CONFIG_PATH);
    if(!db.ok()){
        cout << RED << "Could not open the database at " << DB_PATH << RESET << endl;
        return 1;
    }

    if(cmd == "list"){ return cmd_list(db); }
    if(cmd == "pages"){ db.raw_page_print(); return 0; }
    if(cmd == "clear"){ db.raw_clear(); cout << "Database reset." << endl; return 0; }

    //Validate the command before complaining about its arguments
    if(cmd != "show" && cmd != "search" && cmd != "history" && cmd != "track" && cmd != "fit"){
        cout << RED << "Unknown command '" << cmd << "'" << RESET << "\n" << endl;
        usage();
        return 1;
    }

    if(arg.empty()){
        cout << RED << "'" << cmd << "' needs a query." << RESET << "\n" << endl;
        usage();
        return 1;
    }

    if(cmd == "show"){ return cmd_show(db, arg); }

    //Everything below can touch the network
    KicksDB_Client api;

    if(cmd == "search"){ return cmd_search(api, db, arg); }
    if(cmd == "history"){ return cmd_history(api, db, arg, false); }
    return cmd_history(api, db, arg, true);   //track / fit
}
