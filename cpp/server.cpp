#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cctype>

#include "src/httplib.h"
#include "src/json.hpp"
#include "DSAcore/LRU_Cache/LRU_Cache.h"

using namespace std;
using namespace httplib;
using json = nlohmann::json;

// ============================================================
// GLOBAL STATE
// ============================================================

LRU_Cache recentCache;

// Current text inside the user's search box.
string currentSearchQuery;

// The query that was used to generate currentSearchResult.
string lastProcessedQuery;

// Current search result.
json currentSearchResult = json::array();

// Protect shared search state because HTTP handlers
// may be executed concurrently.
mutex searchMutex;

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
// HELPER FUNCTIONS
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
// MOCK LRU OPERATIONS
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
// SEARCH LOGIC
// ============================================================
//
// This function represents the CORE search layer.
//
// Right now it uses simple mock searching so that the HTTP
// architecture can be tested.
//
// Later this is where we can connect:
//
//     Trie
//     HashTable
//     conflict resolution
//     sorting
//     other algorithms
//
// The server should eventually only call this function.
// It should NOT know how Trie or HashTable work.
// ============================================================

json performSearch(const string &query)
{
    json results = json::array();

    if (query.empty())
        return results;

    // --------------------------------------------------------
    // ID SEARCH
    // --------------------------------------------------------
    //
    // "#" is a user-facing convention.
    //
    // Example:
    //      #P00005
    //
    // The Core interprets this as an ID query.
    // Later this branch can call HashTable.
    // --------------------------------------------------------

    if (query[0] == '#')
    {
        string id = query.substr(1);

        for (const auto &product : products)
        {
            if (product.id == id)
            {
                results.push_back(productToJson(product));
                break;
            }
        }

        return results;
    }

    // --------------------------------------------------------
    // NAME SEARCH
    // --------------------------------------------------------
    //
    // Temporary implementation.
    //
    // Later this is where the Core can use Trie / HashTable
    // and resolve whatever interaction our project requires.
    // --------------------------------------------------------

    for (const auto &product : products)
    {
        if (product.product_name == query)
        {
            results.push_back(productToJson(product));
        }
    }

    return results;
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
//
// Frontend polls this endpoint periodically.
//
// Example:
//
//     GET /product/recent
//
// The server reads the current LRU cache and returns it.
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
// POST /search/input
// ============================================================
//
// This endpoint receives ONLY the current search-bar string.
//
// Example:
//
//     POST /search/input
//
//     "lap"
//
// There is intentionally NO response body.
//
// The server simply updates:
//
//     currentSearchQuery
//
// The actual search result is NOT calculated here.
// ============================================================

void setupSearchInputEndpoint(Server &server)
{
    server.Post(
        "/search/input",
        [](const Request &req, Response &res)
        {
            lock_guard<mutex> lock(searchMutex);

            currentSearchQuery = req.body;

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
// This endpoint immediately returns suggestions.
//
// For now we perform a simple prefix search.
//
// Later this will call Trie.
// ============================================================

void setupAutocompleteEndpoint(Server &server)
{
    server.Get(
        "/search/autocomplete",
        [](const Request &req, Response &res)
        {
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

            vector<string> suggestions;

            // ------------------------------------------------
            // Temporary prefix search.
            //
            // Later:
            //
            //     Trie -> suggestions
            // ------------------------------------------------

            for (const auto &product : products)
            {
                if (product.product_name.size() < prefix.size())
                    continue;

                bool matches = true;

                for (size_t i = 0; i < prefix.size(); ++i)
                {
                    char productChar =
                        static_cast<char>(
                            tolower(
                                static_cast<unsigned char>(
                                    product.product_name[i])));

                    char prefixChar =
                        static_cast<char>(
                            tolower(
                                static_cast<unsigned char>(
                                    prefix[i])));

                    if (productChar != prefixChar)
                    {
                        matches = false;
                        break;
                    }
                }

                if (!matches)
                    continue;

                // Avoid duplicate product names.
                bool alreadyExists = false;

                for (const auto &suggestion : suggestions)
                {
                    if (suggestion == product.product_name)
                    {
                        alreadyExists = true;
                        break;
                    }
                }

                if (!alreadyExists)
                    suggestions.push_back(product.product_name);
            }

            res.set_content(
                json(suggestions).dump(),
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
//             vs
//     lastProcessedQuery
//
// If they are different, a new search is performed.
//
// If they are the same, the existing result is returned.
//
// This prevents the search algorithm from running repeatedly
// when nothing has changed.
// ============================================================

void setupSearchResultEndpoint(Server &server)
{
    server.Get(
        "/search/result",
        [](const Request &, Response &res)
        {
            lock_guard<mutex> lock(searchMutex);

            // ------------------------------------------------
            // Has the user entered something new?
            // ------------------------------------------------

            if (currentSearchQuery != lastProcessedQuery)
            {
                cout
                    << "New search query: "
                    << currentSearchQuery
                    << "\n";

                // Run the actual search.
                currentSearchResult =
                    performSearch(currentSearchQuery);

                // Mark this query as processed.
                lastProcessedQuery =
                    currentSearchQuery;
            }

            // ------------------------------------------------
            // Return the current result.
            // --------------------------------------- ---------

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
    Server server;

    // --------------------------------------------------------
    // Initialization
    // --------------------------------------------------------

    setupMockOperations();

    // --------------------------------------------------------
    // Middleware
    // --------------------------------------------------------

    setupCORS(server);

    // --------------------------------------------------------
    // Endpoints
    // --------------------------------------------------------

    setupRecentProductEndpoint(server);

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
        << "Server: http://localhost:8080\n"
        << "\n"
        << "Endpoints:\n"
        << "GET  /product/recent\n"
        << "POST /search/input\n"
        << "GET  /search/autocomplete?prefix=<prefix>\n"
        << "GET  /search/result\n"
        << "========================================\n";

    // --------------------------------------------------------
    // Start server
    // --------------------------------------------------------

    if (!server.listen("localhost", 8080))
    {
        cerr << "Failed to start server.\n";
        return 1;
    }

    return 0;
}
