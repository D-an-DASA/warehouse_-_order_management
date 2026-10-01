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
#include <mutex>
#include <vector>
#include <algorithm>
#include <cctype>
#include <unordered_set>

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

// Protect shared search state because HTTP handlers
// may be executed concurrently.
mutex searchMutex;

mutex mockDataMutex;
unsigned int nextMockProductId = 1001;

json mockProducts = json::array({{{"id", "P00001"}, {"product_name", "Power Bank"}, {"made_date", "2026-09-01"}, {"arrived_time", "2026-09-03"}, {"best_by_date", "2028-09-01"}, {"status", "AVAILABLE"}},
                                 {{"id", "P00002"}, {"product_name", "USB Cable"}, {"made_date", "2026-08-12"}, {"arrived_time", "2026-08-15"}, {"best_by_date", "2027-08-12"}, {"status", "AVAILABLE"}},
                                 {{"id", "P00003"}, {"product_name", "Laptop Stand"}, {"made_date", "2026-07-20"}, {"arrived_time", "2026-07-22"}, {"best_by_date", "2028-07-20"}, {"status", "AVAILABLE"}},
                                 {{"id", "P00004"}, {"product_name", "Coffee Beans"}, {"made_date", "2025-08-01"}, {"arrived_time", "2025-08-03"}, {"best_by_date", "2026-08-01"}, {"status", "EXPIRED"}},
                                 {{"id", "P00005"}, {"product_name", "Mechanical Keyboard"}, {"made_date", "2026-09-10"}, {"arrived_time", "2026-09-12"}, {"best_by_date", "2029-09-10"}, {"status", "AVAILABLE"}},
                                 {{"id", "P00006"}, {"product_name", "Power Bank"}, {"made_date", "2026-06-05"}, {"arrived_time", "2026-06-08"}, {"best_by_date", "2028-06-05"}, {"status", "AVAILABLE"}}});

vector<json> recentOperations = {
    {{"product", mockProducts[2]}, {"operation", "SEARCH"}, {"time", "2026-09-29 09:42:00"}},
    {{"product", mockProducts[1]}, {"operation", "VIEW"}, {"time", "2026-09-29 09:35:00"}},
    {{"product", mockProducts[0]}, {"operation", "ADD"}, {"time", "2026-09-29 09:20:00"}}};

string lowercase(string value)
{
    transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
              { return static_cast<char>(tolower(character)); });
    return value;
}

void recordRecentOperation(const json &product, const string &operation)
{
    recentOperations.insert(
        recentOperations.begin(),
        json{{"product", product}, {"operation", operation}, {"time", "Just now"}});

    if (recentOperations.size() > 8)
    {
        recentOperations.pop_back();
    }
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
// quantity controls how many products are created; it is not stored per product.
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
            {
                lock_guard<mutex> lock(mockDataMutex);
                for (int i = 0; i < quantity; i++)
                {
                    json product = {
                        {"id", "MOCK-" + to_string(nextMockProductId++)},
                        {"product_name", payload["product_name"]},
                        {"made_date", payload["made_date"]},
                        {"arrived_time", payload["arrived_time"]},
                        {"best_by_date", payload["best_by_date"]},
                        {"status", "AVAILABLE"}};
                    mockProducts.push_back(product);
                    createdProducts.push_back(product);
                    recordRecentOperation(product, "ADD");
                }
            }

            res.status = 201;
            res.set_content(
                json{{"success", true}, {"products", createdProducts}}.dump(),
                "application/json");

            cout << "[API] Added " << quantity << " mock products\n";
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
            json deletedProduct;
            {
                lock_guard<mutex> lock(mockDataMutex);
                const auto product = find_if(
                    mockProducts.begin(),
                    mockProducts.end(),
                    [&productId](const json &item)
                    { return item["id"] == productId; });

                if (product == mockProducts.end())
                {
                    res.status = 404;
                    res.set_content(json{{"success", false}, {"message", "Product not found."}}.dump(), "application/json");
                    return;
                }

                deletedProduct = *product;
                mockProducts.erase(product);
                recordRecentOperation(deletedProduct, "DELETE");
            }

            res.set_content(
                json{{"success", true}, {"id", productId}}.dump(),
                "application/json");

            cout << "[API] Deleted mock product " << productId << '\n';
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

            json response = json::array();
            {
                lock_guard<mutex> lock(mockDataMutex);
                for (const json &operation : recentOperations)
                {
                    response.push_back(operation);
                }
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

            json response = json::array();
            const string normalizedPrefix = lowercase(prefix);
            unordered_set<string> seenSuggestions;
            {
                lock_guard<mutex> lock(mockDataMutex);
                for (const json &product : mockProducts)
                {
                    const string name = product["product_name"].get<string>();
                    const string normalizedName = lowercase(name);
                    if (normalizedName.rfind(normalizedPrefix, 0) == 0 && seenSuggestions.insert(normalizedName).second)
                    {
                        response.push_back(name);
                    }
                }
            }

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
            string query;
            {
                lock_guard<mutex> lock(searchMutex);
                query = currentSearchQuery;
            }

            cout << "[API] GET /search/result | query: " << query << '\n';

            json results = json::array();
            const string normalizedQuery = lowercase(query);
            {
                lock_guard<mutex> lock(mockDataMutex);
                for (const json &product : mockProducts)
                {
                    const string id = lowercase(product["id"].get<string>());
                    const string name = lowercase(product["product_name"].get<string>());
                    if (normalizedQuery.empty() ||
                        id.find(normalizedQuery) != string::npos ||
                        name.find(normalizedQuery) != string::npos)
                    {
                        results.push_back(product);
                    }
                }
            }

            json response = {
                {"query", query},
                {"results", results}};

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
        << "DASA Mock Server\n"
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
