#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#endif

#include <iostream>
#include <algorithm>
#include <string>
#include <ctime>
#include <vector>
#include <mutex>
#include <memory>
#include <cstdlib>
#include <filesystem>

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

// The query that was used to generate currentProductResult.
string lastProcessedQuery;

// Current search result.
vector<Product> currentProductResult;
// ============================================================
// WEBSOCKET CLIENTS
// ============================================================

// Store connected WebSocket clients.
//
vector<ws::WebSocket *> websocketClients;

mutex websocketMutex;

// ============================================================
// JSON
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
// SEARCH STATE MUTEX
// ============================================================

// Protect shared search state because HTTP handlers
// may be executed concurrently.
mutex searchMutex;

const filesystem::path persistentCsvPath =
    filesystem::path("cpp") / "DSAcore" / "Persistent" / "Persistent.csv";

void savePersistentDataOnExit()
{
    lock_guard<mutex> lock(searchMutex);
    if (!searchCore.saveCSV(persistentCsvPath.string()))
    {
        cerr << "[Persistence] Failed to save "
             << persistentCsvPath.string() << '\n';
        return;
    }

    cout << "[Persistence] Saved products to "
         << persistentCsvPath.string() << '\n';
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
// WEBSOCKET BROADCAST
// ============================================================

void broadcastSearchResult()
{
    json response;

    {
        lock_guard<mutex> lock(searchMutex);

        response = {
            {"type", "search_result"},
            {"query", currentSearchQuery},
            {"results", json::array()}};

        for (const Product &product : currentProductResult)
        {
            response["results"].push_back(
                productToJson(product));
        }
    }

    const string message = response.dump();

    lock_guard<mutex> lock(websocketMutex);

    cout << "[WebSocket] Broadcasting search result\n";

    for (auto client = websocketClients.begin();
         client != websocketClients.end();)
    {
        if (*client == nullptr || !(*client)->send(message))
        {
            client = websocketClients.erase(client);
        }
        else
        {
            ++client;
        }
    }
}

void broadcastRecentProducts()
{
    json response = {
        {"type", "recent_products"},
        {"items", json::array()}};

    {
        lock_guard<mutex> lock(searchMutex);

        for (const CacheItem &item : searchCore.getRecent())
        {
            response["items"].push_back(
                {{"product", productToJson(item.product)},
                 {"time", item.time},
                 {"operation", item.operation}});
        }
    }

    const string message = response.dump();

    lock_guard<mutex> lock(websocketMutex);

    cout << "[WebSocket] Broadcasting recent products\n";

    for (auto client = websocketClients.begin();
         client != websocketClients.end();)
    {
        if (*client == nullptr || !(*client)->send(message))
        {
            client = websocketClients.erase(client);
        }
        else
        {
            ++client;
        }
    }
}

// ============================================================
// WEBSOCKET ENDPOINT
// ============================================================

void setupWebSocketEndpoint(Server &server)
{
    server.WebSocket(
        "/ws",
        [](const Request &, ws::WebSocket &socket)
        {
            cout << "[WebSocket] Client connected\n";

            {
                lock_guard<mutex> lock(websocketMutex);
                websocketClients.push_back(&socket);
                cout << "[WebSocket] Connected clients: "
                     << websocketClients.size()
                     << '\n';
            }

            broadcastRecentProducts();

            string message;
            while (socket.is_open())
            {
                const ws::ReadResult result = socket.read(message);
                if (result == ws::Fail)
                {
                    break;
                }

                if (result == ws::Text || result == ws::Binary)
                {
                    cout << "[WebSocket] Message received: "
                         << message
                         << '\n';
                }
            }

            cout << "[WebSocket] Client disconnected\n";

            {
                lock_guard<mutex> lock(websocketMutex);
                websocketClients.erase(
                    remove(
                        websocketClients.begin(),
                        websocketClients.end(),
                        &socket),
                    websocketClients.end());
                cout << "[WebSocket] Connected clients: "
                     << websocketClients.size()
                     << '\n';
            }
        });
}

// ============================================================
// POST /product/add
// Request JSON:
// product_name,
// made_date,
// arrived_time,
// best_by_date,
// quantity.
//
// quantity controls how many products are created.
// ============================================================

void setupAddProductEndpoint(Server &server)
{
    server.Post(
        "/product/add",
        [](const Request &req, Response &res)
        {
            cout << "[API] POST /product/add called\n"
                 << "[API] Request body: "
                 << req.body
                 << '\n';

            json payload;

            try
            {
                payload = json::parse(req.body);
            }
            catch (const json::parse_error &)
            {
                res.status = 400;

                res.set_content(
                    json{
                        {"success", false},
                        {"message", "Invalid JSON body."}}
                        .dump(),
                    "application/json");

                return;
            }

            if (!payload.is_object() ||
                !payload.contains("product_name") ||
                !payload["product_name"].is_string() ||
                !payload.contains("made_date") ||
                !payload["made_date"].is_string() ||
                !payload.contains("arrived_time") ||
                !payload["arrived_time"].is_string() ||
                !payload.contains("best_by_date") ||
                !payload["best_by_date"].is_string() ||
                !payload.contains("quantity") ||
                !payload["quantity"].is_number_integer() ||
                payload["quantity"].get<int>() < 1)
            {
                res.status = 400;

                res.set_content(
                    json{
                        {"success", false},
                        {"message",
                         "Product fields are missing or invalid."}}
                        .dump(),
                    "application/json");

                return;
            }

            const int quantity =
                payload["quantity"].get<int>();

            json createdProducts = json::array();

            {
                lock_guard<mutex> lock(searchMutex);

                for (int i = 0; i < quantity; i++)
                {
                    Product product;

                    product.product_name =
                        payload["product_name"].get<string>();

                    product.made_date =
                        payload["made_date"].get<string>();

                    product.arrived_time =
                        payload["arrived_time"].get<string>();

                    product.best_by_date =
                        payload["best_by_date"].get<string>();

                    const time_t now = time(nullptr);
                    const tm *today = localtime(&now);
                    char todayDate[11];
                    strftime(todayDate, sizeof(todayDate), "%Y-%m-%d", today);
                    product.status = product.best_by_date > todayDate
                                         ? "AVAILABLE"
                                         : "EXPIRED";

                    if (!searchCore.addProduct(product))
                    {
                        res.status = 500;

                        res.set_content(
                            json{
                                {"success", false},
                                {"message",
                                 "Could not generate a unique product ID."}}
                                .dump(),
                            "application/json");

                        return;
                    }

                    createdProducts.push_back(
                        productToJson(product));
                }

                currentProductResult =
                    searchCore.search(currentSearchQuery);
            }

            res.status = 201;

            res.set_content(
                json{
                    {"success", true},
                    {"products", createdProducts}}
                    .dump(),
                "application/json");

            cout << "[API] Added "
                 << quantity
                 << " products\n";

            broadcastSearchResult();
            broadcastRecentProducts();
        });
}

// ============================================================
// DELETE /product/delete
// Request JSON:
// { "id": "<product-id>" }
// ============================================================

void setupDeleteProductEndpoint(Server &server)
{
    server.Delete(
        "/product/delete",
        [](const Request &req, Response &res)
        {
            cout << "[API] DELETE /product/delete called\n"
                 << "[API] Request body: "
                 << req.body
                 << '\n';

            json payload;

            try
            {
                payload = json::parse(req.body);
            }
            catch (const json::parse_error &)
            {
                res.status = 400;

                res.set_content(
                    json{
                        {"success", false},
                        {"message", "Invalid JSON body."}}
                        .dump(),
                    "application/json");

                return;
            }

            if (!payload.is_object() ||
                !payload.contains("id") ||
                !payload["id"].is_string())
            {
                res.status = 400;

                res.set_content(
                    json{
                        {"success", false},
                        {"message",
                         "A product id is required."}}
                        .dump(),
                    "application/json");

                return;
            }

            const string productId =
                payload["id"].get<string>();

            {
                lock_guard<mutex> lock(searchMutex);

                if (!searchCore.deleteProduct(productId))
                {
                    res.status = 404;

                    res.set_content(
                        json{
                            {"success", false},
                            {"message",
                             "Product not found."}}
                            .dump(),
                        "application/json");

                    return;
                }

                // Keep current search result updated.
                currentProductResult = searchCore.search(currentSearchQuery);
            }

            res.set_content(
                json{
                    {"success", true},
                    {"id", productId}}
                    .dump(),
                "application/json");

            cout << "[API] Deleted product "
                 << productId
                 << '\n';

            // ------------------------------------------------
            // NEW:
            // Notify connected clients.
            // ------------------------------------------------

            broadcastSearchResult();
            broadcastRecentProducts();
        });
}

// ============================================================
// POST /search/input
//
// This endpoint receives the search query.
//
// BEFORE:
//
// POST /search/input
//       |
//       v
// searchCore.search()
//       |
//       v
// currentProductResult
//       |
//       v
// frontend POLLS /search/result
//
// NOW:
//
// POST /search/input
//       |
//       v
// searchCore.search()
//       |
//       v
// currentProductResult
//       |
//       v
// WebSocket broadcast
// ============================================================

void setupSearchInputEndpoint(Server &server)
{
    Server *serverInstance = &server;
    server.Post(
        "/search/input",
        [serverInstance](const Request &req, Response &res)
        {
            bool resultChanged = false;
            bool shutdownRequested = false;

            {
                lock_guard<mutex> lock(searchMutex);

                currentSearchQuery = req.body;

                cout << "[API] POST /search/input"
                     << " | query: "
                     << currentSearchQuery
                     << "\n";

                shutdownRequested =
                    currentSearchQuery == "/SHUTDOWN";

                // Has the search query changed?
                if (!shutdownRequested &&
                    currentSearchQuery !=
                    lastProcessedQuery)
                {
                    cout << "[SEARCH] New search query: "
                         << currentSearchQuery
                         << "\n";

                    currentProductResult =
                        searchCore.search(
                            currentSearchQuery);

                    lastProcessedQuery =
                        currentSearchQuery;

                    resultChanged = true;
                }
            }

            if (shutdownRequested)
            {
                cout << "[SERVER] Shutdown requested via /search/input\n";
                res.status = 204;
                serverInstance->stop();
                return;
            }

            // ------------------------------------------------
            // NEW:
            // Push the result immediately.
            // ------------------------------------------------

            if (resultChanged)
            {
                broadcastSearchResult();
                broadcastRecentProducts();
            }

            res.status = 204;
        });
}

// ============================================================
// GET /search/autocomplete
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

            string prefix =
                req.get_param_value("prefix");

            cout
                << "[API] GET /search/autocomplete"
                << " | prefix: "
                << prefix
                << "\n";

            lock_guard<mutex> lock(searchMutex);

            json response =
                searchCore.autocomplete(prefix);

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
    error_code filesystemError;
    const bool hasPersistentData =
        filesystem::exists(persistentCsvPath, filesystemError);
    if (filesystemError)
    {
        cerr << "Could not check persistent CSV: "
             << filesystemError.message() << '\n';
        return 1;
    }

    const string initialCsv = hasPersistentData
                                  ? persistentCsvPath.string()
                                  : "cpp/product_inventory_10 000.csv";
    if (!searchCore.loadCSV(initialCsv))
    {
        return 1;
    }

    if (!hasPersistentData &&
        !searchCore.saveCSV(persistentCsvPath.string()))
    {
        return 1;
    }

    if (std::atexit(savePersistentDataOnExit) != 0)
    {
        cerr << "Could not register persistent save on exit.\n";
        return 1;
    }

    Server server;

    // --------------------------------------------------------
    // Middleware
    // --------------------------------------------------------

    setupCORS(server);

    // --------------------------------------------------------
    // WebSocket
    // --------------------------------------------------------

    setupWebSocketEndpoint(server);

    // --------------------------------------------------------
    // REST API
    // --------------------------------------------------------

    setupAddProductEndpoint(server);
    setupDeleteProductEndpoint(server);

    setupSearchInputEndpoint(server);
    setupAutocompleteEndpoint(server);

    // --------------------------------------------------------
    // Server information
    // --------------------------------------------------------

    cout
        << "========================================\n"
        << "DASA Server\n"
        << "========================================\n"
        << "Server: http://localhost:8081\n"
        << "WebSocket: ws://localhost:8081/ws\n"
        << "\n"
        << "Endpoints:\n"
        << "WebSocket: BroadcastRecentProducts & BroadcastSearchResult\n"
        << "POST /product/add\n"
        << "DELETE /product/delete\n"
        << "POST /search/input\n"
        << "GET  /search/autocomplete?prefix=<prefix>\n"
        << "\n"
        << "WebSocket:\n"
        << "ws://localhost:8081/ws\n"
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