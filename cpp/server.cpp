#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "src/httplib.h"
#include "src/json.hpp"
#include "DSAcore/LRU_Cache/LRU_Cache.h"

using namespace std;
using namespace httplib;
using json = nlohmann::json;

// ============================================================
// GLOBAL LRU CACHE
// ============================================================

LRU_Cache recentCache;

// ============================================================
// MOCK PRODUCT DATA
// ============================================================

vector<Product> products = {
    {"P00001", "Power Bank", "2023-02-21", "2023-03-17 08:15:14", "2024-01-02", "EXPIRED"},
    {"P00002", "Power Bank", "2026-01-21", "2026-01-24 18:27:02", "2026-04-22", "EXPIRED"},
    {"P00003", "USB Cable", "2024-03-23", "2024-03-31 16:38:01", "2025-06-03", "EXPIRED"},
    {"P00004", "Laundry Detergent", "2025-05-09", "2025-05-17 14:37:17", "2025-06-21", "EXPIRED"},
    {"P00005", "Laptop Stand", "2025-05-15", "2025-05-26 08:09:13", "2027-05-04", "AVAILABLE"},
    {"P00006", "USB Cable", "2025-02-17", "2025-02-21 11:54:22", "2026-09-11", "EXPIRED"},
    {"P00007", "Mechanical Keyboard", "2025-07-29", "2025-08-16 03:59:24", "2026-02-05", "EXPIRED"},
    {"P00008", "Laundry Detergent", "2024-08-23", "2024-09-19 20:39:56", "2026-10-02", "AVAILABLE"},
    {"P00009", "Water Bottle", "2023-05-23", "2023-05-25 21:14:49", "2025-02-03", "EXPIRED"},
    {"P00010", "USB Cable", "2024-04-21", "2024-05-19 03:24:17", "2026-12-05", "RESERVED"}};

// ============================================================
// HELPER: PRODUCT -> JSON
// ============================================================

json productToJson(const Product &product)
{
    return {
        {"id", product.id},
        {"product_name", product.product_name},
        {"made_date", product.made_date},
        {"arrived_time", product.arrived_time},
        {"best_by_date", product.best_by_date},
        {"status", product.status}};
}

// ============================================================
// MOCK LRU DATA
// ============================================================

void setupMockOperations()
{
    vector<string> operations = {
        "Exact Search",
        "Prefix Search",
        "Exact Search",
        "Prefix Search",
        "Exact Search",
        "Exact Search",
        "Prefix Search",
        "Exact Search",
        "Prefix Search",
        "Exact Search"};

    for (size_t i = 0; i < products.size(); ++i)
    {
        recentCache.Put(products[i], operations[i]);
    }

    cout << "Mock Recent Workspace initialized.\n";
    cout << "Cache size: " << recentCache.Size() << "\n";
}

// ============================================================
// CORS
// ============================================================

void setupCORS(Server &server)
{
    server.set_pre_routing_handler(
        [](const Request &req, Response &res)
        {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header(
                "Access-Control-Allow-Methods",
                "GET, OPTIONS");
            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type");

            if (req.method == "OPTIONS")
            {
                res.status = 200;
                return Server::HandlerResponse::Handled;
            }

            return Server::HandlerResponse::Unhandled;
        });
}

// ============================================================
// GET /product/recent
//
// Returns recent operations stored in the LRU cache.
// ============================================================

void setupRecentProductEndpoint(Server &server)
{
    server.Get(
        "/product/recent",
        [](const Request &, Response &res)
        {
            vector<CacheItem> items = recentCache.GetAll();

            json response = json::array();

            int order = 1;

            for (const auto &item : items)
            {
                response.push_back({{"order", order++},
                                    {"product", productToJson(item.product)},
                                    {"operation", item.operation},
                                    {"time", item.time}});
            }

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// GET /search/autocomplete?prefix={prefix}
//
// Called while the user is typing.
//
// Later:
//     prefix -> Trie -> suggestions
//
// Currently:
//     mock prefix search
// ============================================================

void setupAutocompleteEndpoint(Server &server)
{
    server.Get(
        "/search/autocomplete",
        [](const Request &req, Response &res)
        {
            // Check whether prefix was provided
            if (!req.has_param("prefix"))
            {
                json response = {
                    {"success", false},
                    {"message", "Missing prefix"}};

                res.status = 400;

                res.set_content(
                    response.dump(),
                    "application/json");

                return;
            }

            string prefix = req.get_param_value("prefix");

            vector<json> suggestions;

            // ------------------------------------------------
            // MOCK TRIE BEHAVIOR
            // ------------------------------------------------

            for (const auto &product : products)
            {
                if (
                    product.product_name.size() >= prefix.size() &&
                    equal(
                        prefix.begin(),
                        prefix.end(),
                        product.product_name.begin(),
                        [](char a, char b)
                        {
                            return tolower(a) == tolower(b);
                        }))
                {
                    bool alreadyExists = false;

                    for (const auto &suggestion : suggestions)
                    {
                        if (
                            suggestion["product_name"] ==
                            product.product_name)
                        {
                            alreadyExists = true;
                            break;
                        }
                    }

                    if (!alreadyExists)
                    {
                        suggestions.push_back({{"id", product.id},
                                               {"product_name", product.product_name}});
                    }
                }
            }

            res.set_content(
                json(suggestions).dump(),
                "application/json");
        });
}

// ============================================================
// GET /search/name/{name}
//
// Exact product-name search.
//
// Later:
//     name -> Core -> name lookup
//
// Currently:
//     mock exact search
// ============================================================

void setupNameSearchEndpoint(Server &server)
{
    server.Get(
        R"(/search/name/(.+))",
        [](const Request &req, Response &res)
        {
            string name = req.matches[1];

            json response = json::array();

            for (const auto &product : products)
            {
                if (product.product_name == name)
                {
                    response.push_back(
                        productToJson(product));
                }
            }

            if (response.empty())
            {
                json error = {
                    {"success", false},
                    {"message", "Product not found"}};

                res.status = 404;

                res.set_content(
                    error.dump(),
                    "application/json");

                return;
            }

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// GET /search/id/{id}
//
// Exact ID search.
//
// Later:
//     id -> HashTable -> Product
//
// Currently:
//     mock exact search
// ============================================================

void setupIdSearchEndpoint(Server &server)
{
    server.Get(
        R"(/search/id/(.+))",
        [](const Request &req, Response &res)
        {
            string id = req.matches[1];

            for (const auto &product : products)
            {
                if (product.id == id)
                {
                    res.set_content(
                        productToJson(product).dump(),
                        "application/json");

                    return;
                }
            }

            json response = {
                {"success", false},
                {"message", "Product not found"}};

            res.status = 404;

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    Server server;

    // Initialize mock data
    setupMockOperations();

    // Configure server
    setupCORS(server);

    // Register endpoints
    setupRecentProductEndpoint(server);
    setupAutocompleteEndpoint(server);
    setupNameSearchEndpoint(server);
    setupIdSearchEndpoint(server);

    cout
        << "========================================\n"
        << "DASA Server\n"
        << "========================================\n"
        << "Server: http://localhost:8080\n"
        << "\n"
        << "Endpoints:\n"
        << "GET  /product/recent\n"
        << "GET  /search/autocomplete?prefix=<prefix>\n"
        << "GET  /search/name/<name>\n"
        << "GET  /search/id/<id>\n"
        << "========================================\n";

    if (!server.listen("localhost", 8080))
    {
        cerr << "Failed to start server.\n";
        return 1;
    }

    return 0;
}