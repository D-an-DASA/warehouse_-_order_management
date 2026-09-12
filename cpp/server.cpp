#include <iostream>
#include <vector>
#include <string>
#include "httplib.h"
#include "json.hpp"

using namespace std;
using namespace httplib;
using namespace nlohmann;

// One data record
struct DataItem {
    string category;
    int value;
};


// Server's current data
vector<DataItem> dataItems = {
    {"A", 120},
    {"B", 180},
    {"C", 300},
    {"D", 220}
};


int main() {

    Server server;


    // =========================
    // GET /data
    // =========================
    server.Get("/data", [](const Request& req,
                           Response& res) {

        string json = "[";

        for (size_t i = 0; i < dataItems.size(); i++) {

            json += "{";
            json += "\"category\":\"" + dataItems[i].category + "\",";
            json += "\"value\":" + to_string(dataItems[i].value);
            json += "}";

            if (i < dataItems.size() - 1) {
                json += ",";
            }
        }

        json += "]";


        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(json, "application/json");
    });


    // =========================
    // POST /data
    // =========================
    server.Post("/data", [](const httplib::Request& req,
                            httplib::Response& res) {

        try {

            // =========================
            // 1. Parse JSON
            // =========================

            json requestData = json::parse(req.body);

            string category = requestData["category"];
            int amount = requestData["amount"];
            string operation = requestData["operation"];


            cout << "Category: " << category << "\n";
            cout << "Amount: " << amount << "\n";
            cout << "Operation: " << operation << "\n";


            // =========================
            // 2. Validate amount
            // =========================

            if (amount <= 0) {

                res.status = 400;

                res.set_content(
                    R"({"success":false,"message":"Amount must be greater than 0"})",
                    "application/json"
                );

                return;
            }


            // =========================
            // 3. Find category
            // =========================

            for (auto& item : dataItems) {

                if (item.category == category) {


                    // =========================
                    // 4. ADD
                    // =========================

                    if (operation == "add") {

                        item.value += amount;
                    }


                    // =========================
                    // 5. REMOVE
                    // =========================

                    else if (operation == "remove") {

                        if (item.value - amount < 0) {

                            res.status = 400;

                            res.set_content(
                                R"({"success":false,"message":"Value cannot be negative"})",
                                "application/json"
                            );

                            return;
                        }

                        item.value -= amount;
                    }


                    // =========================
                    // 6. Invalid operation
                    // =========================

                    else {

                        res.status = 400;

                        res.set_content(
                            R"({"success":false,"message":"Invalid operation"})",
                            "application/json"
                        );

                        return;
                    }


                    // =========================
                    // 7. Success response
                    // =========================

                    res.set_header(
                        "Access-Control-Allow-Origin",
                        "*"
                    );

                    res.set_content(
                        R"({"success":true,"message":"Updated successfully"})",
                        "application/json"
                    );

                    return;
                }
            }


            // =========================
            // 8. Category not found
            // =========================

            res.status = 404;

            res.set_content(
                R"({"success":false,"message":"Category not found"})",
                "application/json"
            );

        }
        catch (const json::exception&) {

            res.status = 400;

            res.set_content(
                R"({"success":false,"message":"Invalid JSON request"})",
                "application/json"
            );
        }
    });



    cout << "Server running at http://localhost:8080\n";

    server.listen("localhost", 8080);

    return 0;
}
