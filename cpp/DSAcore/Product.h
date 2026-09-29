#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>
using namespace std;

struct Product {
    string id;
    string product_name;
    string made_date;
    string arrived_time;
    string best_by_date;
    string status;
    int quantity = 1;
};

#endif