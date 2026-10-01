#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#endif

#include <iostream>
#include <string>
#include <vector>
#include <mutex>

#include "src/httplib.h"
#include "src/json.hpp"
#include "DSAcore/Product.h"
#include "DSAcore/SearchCore.h"

SearchCore searchCore;
using namespace std;
using namespace httplib;

using json = nlohmann::json;

// ============================================================
// GLOBAL STATE
// ============================================================

// Current text inside the user's search box.
string currentSearchQuery;

// The query that was used to generate currentSearchResult.
string lastProcessedQuery;

// Current search result.
json currentSearchResult = json::array();

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

// Protect shared search state because HTTP handlers
// may be executed concurrently.
mutex searchMutex;

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
                "GET, POST, DELETE, OPTIONS");

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
// POST /product/add
// Request JSON: product_name, made_date, arrived_time, best_by_date, quantity.
// quantity controls how many products are created; it is not stored in Product.
// ============================================================

void setupAddProductEndpoint(Server &server)
{
    server.Post(
        "/product/add",
        [](const Request &req, Response &res)
        {
            cout << "[API] POST /product/add called\n"
                 << "[API] Request body: " << req.body << '\n';

            json payload;
            try
            {
                payload = json::parse(req.body);
            }
            catch (const json::parse_error &)
            {
                res.status = 400;
                res.set_content(json{{"success", false}, {"message", "Invalid JSON body."}}.dump(), "application/json");
                return;
            }

            if (!payload.is_object() ||
                !payload.contains("product_name") || !payload["product_name"].is_string() ||
                !payload.contains("made_date") || !payload["made_date"].is_string() ||
                !payload.contains("arrived_time") || !payload["arrived_time"].is_string() ||
                !payload.contains("best_by_date") || !payload["best_by_date"].is_string() ||
                !payload.contains("quantity") || !payload["quantity"].is_number_integer() ||
                payload["quantity"].get<int>() < 1)
            {
                res.status = 400;
                res.set_content(json{{"success", false}, {"message", "Product fields are missing or invalid."}}.dump(), "application/json");
                return;
            }

            const int quantity = payload["quantity"].get<int>();
            json createdProducts = json::array();

            lock_guard<mutex> lock(searchMutex);
            for (int i = 0; i < quantity; i++)
            {
                Product product;
                product.product_name = payload["product_name"].get<string>();
                product.made_date = payload["made_date"].get<string>();
                product.arrived_time = payload["arrived_time"].get<string>();
                product.best_by_date = payload["best_by_date"].get<string>();
                product.status = "AVAILABLE";

                if (!searchCore.addProduct(product))
                {
                    res.status = 500;
                    res.set_content(json{{"success", false}, {"message", "Could not generate a unique product ID."}}.dump(), "application/json");
                    return;
                }

                createdProducts.push_back(productToJson(product));
            }

            res.status = 201;
            res.set_content(
                json{{"success", true}, {"products", createdProducts}}.dump(),
                "application/json");

            cout << "[API] Added " << quantity << " products\n";
        });
}

// ============================================================
// DELETE /product/delete
// Request JSON: { "id": "<product-id>" }
// ============================================================

void setupDeleteProductEndpoint(Server &server)
{
    server.Delete(
        "/product/delete",
        [](const Request &req, Response &res)
        {
            cout << "[API] DELETE /product/delete called\n"
                 << "[API] Request body: " << req.body << '\n';

            json payload;
            try
            {
                payload = json::parse(req.body);
            }
            catch (const json::parse_error &)
            {
                res.status = 400;
                res.set_content(json{{"success", false}, {"message", "Invalid JSON body."}}.dump(), "application/json");
                return;
            }

            if (!payload.is_object() || !payload.contains("id") || !payload["id"].is_string())
            {
                res.status = 400;
                res.set_content(json{{"success", false}, {"message", "A product id is required."}}.dump(), "application/json");
                return;
            }

            const string productId = payload["id"].get<string>();
            lock_guard<mutex> lock(searchMutex);

            if (!searchCore.deleteProduct(productId))
            {
                res.status = 404;
                res.set_content(json{{"success", false}, {"message", "Product not found."}}.dump(), "application/json");
                return;
            }

            if (currentSearchQuery == productId)
            {
                lastProcessedQuery.clear();
                currentSearchResult = json::array();
            }

            res.set_content(
                json{{"success", true}, {"id", productId}}.dump(),
                "application/json");

            cout << "[API] Deleted product " << productId << '\n';
        });
}

// ============================================================
// GET /product/recent
// ============================================================
//
// Frontend polls this endpoint periodically.
//
// The actual LRU cache will be connected here later.
//
// ============================================================

void setupRecentProductEndpoint(Server &server)
{
    server.Get(
        "/product/recent",
        [](const Request &, Response &res)
        {
            cout << "[API] GET /product/recent\n";
            lock_guard<mutex> lock(searchMutex);

            json response = json::array();
            for (const CacheItem &item : searchCore.getRecent())
            {
                json recent = productToJson(item.product);
                recent["time"] = item.time;
                recent["operation"] = item.operation;
                response.push_back(move(recent));
            }
            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// POST /search/input
// ============================================================
//
// This endpoint receives the search query after the user
// commits a search.
//
// ============================================================

void setupSearchInputEndpoint(Server &server)
{
    server.Post(
        "/search/input",
        [](const Request &req, Response &res)
        {
            lock_guard<mutex> lock(searchMutex);

            currentSearchQuery = req.body;

            cout
                << "[API] POST /search/input"
                << " | query: " << currentSearchQuery
                << "\n";

            // No response body.
            res.status = 204;
        });
}

// ============================================================
// GET /search/autocomplete
// ============================================================
//
// Example:
//
//     GET /search/autocomplete?prefix=lap
//
// This endpoint returns autocomplete suggestions.
//
// ============================================================

void setupAutocompleteEndpoint(Server &server)
{
    server.Get(
        "/search/autocomplete",
        [](const Request &req, Response &res)
        {
            if (!req.has_param("prefix"))
            {
                cout
                    << "[API] GET /search/autocomplete"
                    << " | ERROR: missing prefix\n";

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

            cout
                << "[API] GET /search/autocomplete"
                << " | prefix: " << prefix
                << "\n";

            lock_guard<mutex> lock(searchMutex);
            json response = searchCore.autocomplete(prefix);

            res.set_content(
                response.dump(),
                "application/json");
        });
}

// ============================================================
// GET /search/result
// ============================================================
//
// Frontend polls this endpoint periodically.
//
// The server checks:
//
//     currentSearchQuery
//
//         vs
//
//     lastProcessedQuery
//
// ============================================================

void setupSearchResultEndpoint(Server &server)
{
    server.Get(
        "/search/result",
        [](const Request &, Response &res)
        {
            lock_guard<mutex> lock(searchMutex);

            cout << "[API] GET /search/result\n";

            // ------------------------------------------------
            // Has the user entered something new?
            // ------------------------------------------------

            if (currentSearchQuery != lastProcessedQuery)
            {
                cout
                    << "[SEARCH] New search query: "
                    << currentSearchQuery
                    << "\n";

                currentSearchResult = json::array();
                for (const Product &product : searchCore.search(currentSearchQuery))
                {
                    currentSearchResult.push_back(productToJson(product));
                }

                lastProcessedQuery =
                    currentSearchQuery;
            }

            // ------------------------------------------------
            // Return the current result.
            // ------------------------------------------------

            json response = {
                {"query", currentSearchQuery},
                {"results", currentSearchResult}};

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
    if (!searchCore.loadCSV("cpp/product_inventory_100 000.csv"))
    {
        return 1;
    }

    Server server;

    // --------------------------------------------------------
    // Middleware
    // --------------------------------------------------------

    setupCORS(server);

    // --------------------------------------------------------
    // Endpoints
    // --------------------------------------------------------

    setupRecentProductEndpoint(server);
    setupAddProductEndpoint(server);
    setupDeleteProductEndpoint(server);
    setupSearchInputEndpoint(server);
    setupAutocompleteEndpoint(server);
    setupSearchResultEndpoint(server);

    // --------------------------------------------------------
    // Server information
    // --------------------------------------------------------

    cout
        << "========================================\n"
        << "DASA Server\n"
        << "========================================\n"
        << "Server: http://localhost:8081\n"
        << "\n"
        << "Endpoints:\n"
        << "GET  /product/recent\n"
        << "POST /product/add\n"
        << "DELETE /product/delete\n"
        << "POST /search/input\n"
        << "GET  /search/autocomplete?prefix=<prefix>\n"
        << "GET  /search/result\n"
        << "========================================\n";

    // --------------------------------------------------------
    // Start server
    // --------------------------------------------------------

    if (!server.listen("localhost", 8081))
    {
        cerr << "Failed to start server.\n";
        return 1;
    }

    return 0;
}
