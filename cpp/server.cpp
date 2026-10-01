#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER _WIN32_WINNT
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "src/httplib.h"
#include "src/json.hpp"
#include "DSAcore/SearchCore.h"
#include "Persistence/CsvProductRepository.h"

using namespace std;
using namespace httplib;
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

SearchCore searchCore;
CsvProductRepository repository;
mutex coreMutex;
string dataPath = "runtime/inventory.csv";

json productToJson(const Product& product) {
    return {{"id", product.id},
            {"product_name", product.product_name},
            {"made_date", product.made_date},
            {"arrived_time", product.arrived_time},
            {"best_by_date", product.best_by_date},
            {"status", product.status}};
}

void sendJson(Response& response, int status, const json& body) {
    response.status = status;
    response.set_content(body.dump(), "application/json; charset=utf-8");
}

void sendError(Response& response, int status, const string& code,
               const string& message) {
    sendJson(response, status, {{"error", {{"code", code},
                                            {"message", message}}}});
}

bool persist(Response& response) {
    string error;
    if (repository.save(dataPath, searchCore.getAllProducts(), error)) {
        return true;
    }
    sendError(response, 500, "STORAGE_WRITE_FAILED", error);
    return false;
}

bool parseBody(const Request& request, json& body, Response& response) {
    try {
        body = json::parse(request.body);
        if (!body.is_object()) {
            sendError(response, 400, "INVALID_JSON", "Body phai la JSON object.");
            return false;
        }
        return true;
    } catch (const exception&) {
        sendError(response, 400, "INVALID_JSON", "Body JSON khong hop le.");
        return false;
    }
}

bool looksLikeProductId(string value) {
    value.erase(remove_if(value.begin(), value.end(), [](unsigned char ch) {
        return isspace(ch);
    }), value.end());
    if (!value.empty() && value.front() == '#') {
        value.erase(value.begin());
    }
    if (value.size() < 2 || tolower(static_cast<unsigned char>(value[0])) != 'p') {
        return false;
    }
    return all_of(value.begin() + 1, value.end(), [](unsigned char ch) {
        return isdigit(ch);
    });
}

size_t readLimit(const Request& request, size_t fallback, size_t maximum) {
    if (!request.has_param("limit")) {
        return fallback;
    }
    try {
        const unsigned long value = stoul(request.get_param_value("limit"));
        return max<size_t>(1, min<size_t>(value, maximum));
    } catch (const exception&) {
        return fallback;
    }
}

void setupCors(Server& server) {
    server.set_default_headers({
        {"Access-Control-Allow-Origin", "*"},
        {"Access-Control-Allow-Headers", "Content-Type"},
        {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"}
    });
    server.Options(R"(.*)", [](const Request&, Response& response) {
        response.status = 204;
    });
}

void setupEndpoints(Server& server) {
    server.Get("/health", [](const Request&, Response& response) {
        lock_guard<mutex> lock(coreMutex);
        sendJson(response, 200,
                 {{"status", "ok"}, {"products", searchCore.size()}});
    });

    server.Get("/product/recent", [](const Request&, Response& response) {
        lock_guard<mutex> lock(coreMutex);
        json items = json::array();
        for (const CacheItem& item : searchCore.getRecent()) {
            items.push_back({{"product", productToJson(item.product)},
                             {"operation", item.operation},
                             {"time", item.time}});
        }
        sendJson(response, 200, items);
    });

    server.Post("/product/add", [](const Request& request, Response& response) {
        json body;
        if (!parseBody(request, body, response)) {
            return;
        }

        try {
            const int quantity = body.value("quantity", 1);
            if (quantity < 1 || quantity > 100) {
                sendError(response, 400, "INVALID_QUANTITY",
                          "So luong phai tu 1 den 100.");
                return;
            }

            Product base{"", body.at("product_name").get<string>(),
                         body.at("made_date").get<string>(),
                         body.at("arrived_time").get<string>(),
                         body.at("best_by_date").get<string>(), "AVAILABLE"};

            lock_guard<mutex> lock(coreMutex);
            json created = json::array();
            for (int index = 0; index < quantity; ++index) {
                Product product = base;
                string error;
                if (!searchCore.addProduct(product, error)) {
                    sendError(response, 400, "INVALID_PRODUCT", error);
                    return;
                }
                created.push_back(productToJson(product));
            }
            if (!persist(response)) {
                return;
            }
            sendJson(response, 201, {{"products", created}});
        } catch (const exception&) {
            sendError(response, 400, "MISSING_FIELD",
                      "Thieu truong san pham hoac sai kieu du lieu.");
        }
    });

    server.Post("/product/reserve", [](const Request& request,
                                        Response& response) {
        json body;
        if (!parseBody(request, body, response)) {
            return;
        }
        try {
            const string id = body.at("id").get<string>();
            lock_guard<mutex> lock(coreMutex);
            Product reserved;
            string error;
            if (!searchCore.reserveProduct(id, reserved, error)) {
                const int status = error == "Khong tim thay san pham." ? 404 : 409;
                sendError(response, status, "RESERVE_REJECTED", error);
                return;
            }
            if (!persist(response)) {
                return;
            }
            sendJson(response, 200, {{"product", productToJson(reserved)}});
        } catch (const exception&) {
            sendError(response, 400, "MISSING_ID", "Can cung cap ID san pham.");
        }
    });

    server.Delete("/product/delete", [](const Request& request,
                                         Response& response) {
        json body;
        if (!parseBody(request, body, response)) {
            return;
        }
        try {
            const string id = body.at("id").get<string>();
            lock_guard<mutex> lock(coreMutex);
            string error;
            if (!searchCore.deleteProduct(id, error)) {
                sendError(response, 404, "DELETE_REJECTED", error);
                return;
            }
            if (!persist(response)) {
                return;
            }
            sendJson(response, 200, {{"deleted_id", id}});
        } catch (const exception&) {
            sendError(response, 400, "MISSING_ID", "Can cung cap ID san pham.");
        }
    });

    server.Get("/search/autocomplete", [](const Request& request,
                                           Response& response) {
        if (!request.has_param("prefix")) {
            sendError(response, 400, "MISSING_PREFIX", "Can cung cap prefix.");
            return;
        }
        lock_guard<mutex> lock(coreMutex);
        sendJson(response, 200,
                 searchCore.autocomplete(request.get_param_value("prefix"),
                                         readLimit(request, 20, 20)));
    });

    server.Get("/search/result", [](const Request& request, Response& response) {
        if (!request.has_param("query")) {
            sendError(response, 400, "MISSING_QUERY", "Can cung cap query.");
            return;
        }
        const string query = request.get_param_value("query");
        if (query.find_first_not_of(" \t\r\n") == string::npos) {
            sendError(response, 400, "EMPTY_QUERY", "Query khong duoc rong.");
            return;
        }

        lock_guard<mutex> lock(coreMutex);
        const vector<Product> products =
            searchCore.search(query, readLimit(request, 50, 100));
        if (products.empty() && looksLikeProductId(query)) {
            sendError(response, 404, "PRODUCT_NOT_FOUND",
                      "Khong tim thay san pham.");
            return;
        }

        json results = json::array();
        for (const Product& product : products) {
            results.push_back(productToJson(product));
        }
        sendJson(response, 200,
                 {{"query", query},
                  {"mode", looksLikeProductId(query) ? "exact" : "priority"},
                  {"results", results}});
    });
}

bool loadInitialData(const string& seedPath) {
    vector<Product> products;
    string error;
    const bool dataExists = fs::exists(dataPath);
    const string source = dataExists ? dataPath : seedPath;
    if (!repository.load(source, products, error)) {
        cerr << error << '\n';
        return false;
    }
    if (!searchCore.loadProducts(products, error)) {
        cerr << error << '\n';
        return false;
    }
    if (!dataExists && !repository.save(dataPath, products, error)) {
        cerr << error << '\n';
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    string seedPath = "cpp/product_inventory_10 000.csv";
    int port = 8081;
    for (int index = 1; index < argc; ++index) {
        const string argument = argv[index];
        if (argument == "--data" && index + 1 < argc) {
            dataPath = argv[++index];
        } else if (argument == "--seed" && index + 1 < argc) {
            seedPath = argv[++index];
        } else if (argument == "--port" && index + 1 < argc) {
            try {
                port = stoi(argv[++index]);
            } catch (const exception&) {
                cerr << "Port khong hop le.\n";
                return 1;
            }
        } else {
            cerr << "Tham so khong hop le: " << argument << '\n';
            return 1;
        }
    }
    if (port < 1 || port > 65535 || !loadInitialData(seedPath)) {
        return 1;
    }

    Server server;
    setupCors(server);
    setupEndpoints(server);

    cout << "Warehouse server: http://localhost:" << port << '\n'
         << "Data: " << dataPath << '\n'
         << "Products: " << searchCore.size() << '\n';
    if (!server.listen("localhost", port)) {
        cerr << "Khong the khoi dong server.\n";
        return 1;
    }
    return 0;
}
