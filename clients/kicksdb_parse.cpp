#include "kicksdb_types.hpp"

#include <algorithm>
#include <string>

using namespace std;

//Field names and envelope shapes drift between kicks.dev endpoints, so every
//lookup tries the plausible spellings and skips whatever is absent.
//Kept free of cpp-httplib so the parsing layer builds and tests without OpenSSL.

namespace {

    //The payload is sometimes a bare array, sometimes wrapped in "data"/"results"
    const json& payload_array(const json& body, json& scratch){
        for(const char* k : {"data", "results", "products", "sales", "hits"}){
            if(body.contains(k) && body[k].is_array()){ return body[k]; }
        }
        if(body.is_array()){ return body; }

        scratch = json::array();
        return scratch;
    }

    //Field names drift between endpoints, so try the plausible ones in order
    string pick_string(const json& o, initializer_list<const char*> keys){
        for(const char* k : keys){
            if(o.contains(k)){
                if(o[k].is_string()){ return o[k].get<string>(); }
                if(o[k].is_number()){ return to_string(o[k].get<double>()); }
            }
        }
        return "";
    }

    float pick_number(const json& o, initializer_list<const char*> keys){
        for(const char* k : keys){
            if(!o.contains(k)){ continue; }
            if(o[k].is_number()){ return (float)o[k].get<double>(); }
            if(o[k].is_string()){
                try { return stof(o[k].get<string>()); } catch(const exception&){ }
            }
        }
        return 0.0f;
    }

}


vector<Product> parse_products(const json& body){
    vector<Product> out;

    json scratch;
    const json& arr = payload_array(body, scratch);

    for(const json& o : arr){
        if(!o.is_object()){ continue; }

        Product p;
        p.id = pick_string(o, {"id", "uuid", "productId", "slug", "sku"});
        p.title = pick_string(o, {"title", "name", "shoeName", "product_title"});
        p.brand = pick_string(o, {"brand", "brand_name", "make"});
        p.price = pick_number(o, {"avg_price", "average_price", "last_sale",
                                  "lastSale", "min_price", "retail_price", "price"});

        //A record with neither a title nor an id is not addressable, so drop it
        if(p.title.empty() && p.id.empty()){ continue; }
        if(p.title.empty()){ p.title = p.id; }

        out.push_back(p);
    }

    return out;
}


vector<Sale> parse_sales(const json& body){
    vector<Sale> out;

    json scratch;
    const json& arr = payload_array(body, scratch);

    for(const json& o : arr){
        if(!o.is_object()){ continue; }

        Sale s;
        s.price = pick_number(o, {"amount", "price", "sale_price", "salePrice",
                                  "local_amount", "value"});
        s.date = pick_string(o, {"date", "created_at", "createdAt", "sale_date", "timestamp"});

        //Zero or negative prices are placeholders, not observations
        if(!(s.price > 0.0f)){ continue; }

        out.push_back(s);
    }

    //Oldest first, so the plot reads left to right in time
    stable_sort(out.begin(), out.end(), [](const Sale& a, const Sale& b){
        return a.date < b.date;
    });

    return out;
}
