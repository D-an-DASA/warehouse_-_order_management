#include <iostream>
#include <string>
#include <mutex>

#include "src/httplib.h"
#include "src/json.hpp"
#include "DSAcore/LRU_Cache/LRU_Cache.h"

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
// Response: 501 until product creation logic is connected.
// ============================================================

void setupAddProductEndpoint(Server &server)
{
    server.Post(
        "/product/add",
        [](const Request &req, Response &res)
        {
            cout << "[API] POST /product/add called\n"
                 << "[API] Request body: " << req.body << '\n';

            res.status = 501;
            res.set_content(
                json{{"success", false}, {"message", "Add product is not implemented."}}.dump(),
                "application/json");

            cout << "[API] POST /product/add response: 501 Not Implemented\n";
        });
}

// ============================================================
// DELETE /product/delete
// Request JSON: { "id": "<product-id>" }
// Response: 501 until product deletion logic is connected.
// ============================================================

void setupDeleteProductEndpoint(Server &server)
{
    server.Delete(
        "/product/delete",
        [](const Request &req, Response &res)
        {
            cout << "[API] DELETE /product/delete called\n"
                 << "[API] Request body: " << req.body << '\n';

            res.status = 501;
            res.set_content(
                json{{"success", false}, {"message", "Delete product is not implemented."}}.dump(),
                "application/json");

            cout << "[API] DELETE /product/delete response: 501 Not Implemented\n";
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

            // TODO:
            // Connect this endpoint to the actual LRU Cache.

            json response = json::array();

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

            // TODO:
            // Connect prefix search to the actual Trie.
            //
            // Example:
            //
            // vector<string> suggestions =
            //     trie.getSuggestions(prefix);

            json response = json::array();

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

                // TODO:
                // Connect to the actual search core.
                //
                // Example:
                //
                // currentSearchResult =
                //     searchCore.search(currentSearchQuery);

                currentSearchResult = json::array();

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
        << "Server: http://localhost:8080\n"
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

    if (!server.listen("localhost", 8080))
    {
        cerr << "Failed to start server.\n";
        return 1;
    }

    return 0;
}
