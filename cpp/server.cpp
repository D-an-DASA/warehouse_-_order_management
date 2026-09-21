#include <iostream>
#include <string>
#include <vector>

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
// MOCK LRU DATA
// ============================================================

void setupMockOperations()
{
    vector<Product> products =
        {
            {"P00001",
             "Power Bank",
             "2023-02-21",
             "2023-03-17 08:15:14",
             "2024-01-02",
             "EXPIRED"},

            {"P00002",
             "Power Bank",
             "2026-01-21",
             "2026-01-24 18:27:02",
             "2026-04-22",
             "EXPIRED"},

            {"P00003",
             "USB Cable",
             "2024-03-23",
             "2024-03-31 16:38:01",
             "2025-06-03",
             "EXPIRED"},

            {"P00004",
             "Laundry Detergent",
             "2025-05-09",
             "2025-05-17 14:37:17",
             "2025-06-21",
             "EXPIRED"},

            {"P00005",
             "Laptop Stand",
             "2025-05-15",
             "2025-05-26 08:09:13",
             "2027-05-04",
             "AVAILABLE"},

            {"P00006",
             "USB Cable",
             "2025-02-17",
             "2025-02-21 11:54:22",
             "2026-09-11",
             "EXPIRED"},

            {"P00007",
             "Mechanical Keyboard",
             "2025-07-29",
             "2025-08-16 03:59:24",
             "2026-02-05",
             "EXPIRED"},

            {"P00008",
             "Laundry Detergent",
             "2024-08-23",
             "2024-09-19 20:39:56",
             "2026-10-02",
             "AVAILABLE"},

            {"P00009",
             "Water Bottle",
             "2023-05-23",
             "2023-05-25 21:14:49",
             "2025-02-03",
             "EXPIRED"},

            {"P00010",
             "USB Cable",
             "2024-04-21",
             "2024-05-19 03:24:17",
             "2026-12-05",
             "RESERVED"}};

    vector<string> operations =
        {
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
        recentCache.Put(
            products[i],
            operations[i]);
    }

    cout << "Mock Recent Workspace initialized.\n";
    cout << "Cache size: "
         << recentCache.Size()
         << "\n";
}

// ============================================================
// CORS
// ============================================================

void setupCORS(Server &server)
{
    server.set_pre_routing_handler(
        [](const Request &req, Response &res)
        {
            res.set_header(
                "Access-Control-Allow-Origin",
                "*");

            res.set_header(
                "Access-Control-Allow-Methods",
                "GET, POST, OPTIONS");

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
// ============================================================

void setupRecentProductEndpoint(Server &server)
{
    server.Get(
        "/product/recent",
        [](const Request &, Response &res)
        {
            vector<CacheItem> items =
                recentCache.GetAll();

            json response = json::array();

            int order = 1;

            for (const auto &item : items)
            {
                response.push_back(
                    {{"order", order++},

                     {"product",
                      {{"id", item.product.id},
                       {"product_name", item.product.product_name},
                       {"made_date", item.product.made_date},
                       {"arrived_time", item.product.arrived_time},
                       {"best_by_date", item.product.best_by_date},
                       {"status", item.product.status}}},

                     {"operation", item.operation},
                     {"time", item.time}});
            }

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// GET /product/result
// ============================================================

void setupProductResultEndpoint(Server &server)
{
    server.Get(
        "/product/result",
        [](const Request &, Response &res)
        {
            json response = json::array(
                {{{"id", "P00001"},
                  {"product_name", "Power Bank"},
                  {"made_date", "2023-02-21"},
                  {"arrived_time", "2023-03-17 08:15:14"},
                  {"best_by_date", "2024-01-02"},
                  {"status", "EXPIRED"}},

                 {{"id", "P00002"},
                  {"product_name", "Power Bank"},
                  {"made_date", "2026-01-21"},
                  {"arrived_time", "2026-01-24 18:27:02"},
                  {"best_by_date", "2026-04-22"},
                  {"status", "EXPIRED"}},

                 {{"id", "P00003"},
                  {"product_name", "USB Cable"},
                  {"made_date", "2024-03-23"},
                  {"arrived_time", "2024-03-31 16:38:01"},
                  {"best_by_date", "2025-06-03"},
                  {"status", "EXPIRED"}},

                 {{"id", "P00004"},
                  {"product_name", "Laundry Detergent"},
                  {"made_date", "2025-05-09"},
                  {"arrived_time", "2025-05-17 14:37:17"},
                  {"best_by_date", "2025-06-21"},
                  {"status", "EXPIRED"}},

                 {{"id", "P00005"},
                  {"product_name", "Laptop Stand"},
                  {"made_date", "2025-05-15"},
                  {"arrived_time", "2025-05-26 08:09:13"},
                  {"best_by_date", "2027-05-04"},
                  {"status", "AVAILABLE"}}});

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// GET /search/autocomplete?prefix=Lap
// ============================================================
//
// Temporary mock.
//
// Later:
// server -> Trie -> suggestions
// ============================================================

void setupAutocompleteEndpoint(Server &server)
{
    // --------------------------------------------------------
    // GET /search/autocomplete?prefix=Lap
    // --------------------------------------------------------

    server.Get(
        "/search/autocomplete",
        [](const Request &req, Response &res)
        {
            if (!req.has_param("prefix"))
            {
                json response =
                    {
                        {"success", false},
                        {"message", "Missing prefix"}};

                res.status = 400;

                res.set_content(
                    response.dump(),
                    "application/json");

                return;
            }

            string prefix =
                req.get_param_value("prefix");

            vector<string> suggestions;

            if (prefix == "Lap" || prefix == "lap")
            {
                suggestions =
                    {
                        "Laptop Stand"};
            }
            else if (prefix == "Pow" || prefix == "pow")
            {
                suggestions =
                    {
                        "Power Bank"};
            }
            else if (prefix == "USB" || prefix == "usb")
            {
                suggestions =
                    {
                        "USB Cable"};
            }
            else if (prefix == "La" || prefix == "la")
            {
                suggestions =
                    {
                        "Laptop Stand",
                        "Laundry Detergent"};
            }

            json response = json::array();

            for (const auto &suggestion : suggestions)
            {
                response.push_back(suggestion);
            }

            res.set_content(
                response.dump(),
                "application/json");
        });

    // --------------------------------------------------------
    // POST /search/autocomplete
    //
    // Request body:
    // {
    //     "prefix": "Lap"
    // }
    //
    // Response:
    // [
    //     "Laptop Stand"
    // ]
    // --------------------------------------------------------

    server.Post(
        "/search/autocomplete",
        [](const Request &req, Response &res)
        {
            json body;

            try
            {
                body = json::parse(req.body);
            }
            catch (...)
            {
                json response =
                    {
                        {"success", false},
                        {"message", "Invalid JSON request"}};

                res.status = 400;

                res.set_content(
                    response.dump(),
                    "application/json");

                return;
            }

            if (!body.contains("prefix") ||
                !body["prefix"].is_string())
            {
                json response =
                    {
                        {"success", false},
                        {"message", "Missing or invalid prefix"}};

                res.status = 400;

                res.set_content(
                    response.dump(),
                    "application/json");

                return;
            }

            string prefix =
                body["prefix"].get<string>();

            vector<string> suggestions;

            // ------------------------------------------------
            // Temporary mock Trie behavior
            // ------------------------------------------------

            if (prefix == "Lap" || prefix == "lap")
            {
                suggestions =
                    {
                        "Laptop Stand"};
            }
            else if (prefix == "Pow" || prefix == "pow")
            {
                suggestions =
                    {
                        "Power Bank"};
            }
            else if (prefix == "USB" || prefix == "usb")
            {
                suggestions =
                    {
                        "USB Cable"};
            }
            else if (prefix == "La" || prefix == "la")
            {
                suggestions =
                    {
                        "Laptop Stand",
                        "Laundry Detergent"};
            }

            json response = json::array();

            for (const auto &suggestion : suggestions)
            {
                response.push_back(suggestion);
            }

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

    // --------------------------------------------------------
    // Initialize mock LRU data
    // --------------------------------------------------------

    setupMockOperations();

    // --------------------------------------------------------
    // Setup CORS
    // --------------------------------------------------------

    setupCORS(server);

    // --------------------------------------------------------
    // Setup 3 main data endpoints
    // --------------------------------------------------------

    setupRecentProductEndpoint(server);
    setupProductResultEndpoint(server);
    setupAutocompleteEndpoint(server);

    // --------------------------------------------------------
    // Server information
    // --------------------------------------------------------

    cout
        << "========================================\n"
        << "DASA Server\n"
        << "========================================\n"
        << "Server: http://localhost:8080\n"
        << "\n"
        << "Endpoints:\n"
        << "GET  /product/recent\n"
        << "GET  /product/result\n"
        << "GET  /search/autocomplete?prefix=<prefix>\n"
        << "POST /search/autocomplete\n"
        << "========================================\n";

    // --------------------------------------------------------
    // Start server
    // --------------------------------------------------------

    if (!server.listen("localhost", 8080))
    {
        cerr
            << "Failed to start server.\n";

        return 1;
    }

    return 0;
}