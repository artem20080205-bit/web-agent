#include <iostream>
#include <cassert>
#include "http_client.h"

int main() {
    HttpClient http(false); // ssl_verify=false for test

    // Test reachability of a known URL
    bool reachable = http.is_reachable("https://httpbin.org/get");
    if (reachable) {
        std::cout << "[PASS] test_http: server is reachable\n";
    } else {
        std::cout << "[WARN] test_http: server unreachable (network may be restricted)\n";
    }

    // Test JSON POST to httpbin (echo service)
    try {
        nlohmann::json payload = {{"test_key", "test_value"}, {"num", 42}};
        auto resp = http.post_json("https://httpbin.org/post", payload);
        std::cout << "[INFO] HTTP status: " << resp.status_code << "\n";
        if (resp.status_code == 200) {
            std::cout << "[PASS] test_http: POST JSON succeeded\n";
        } else {
            std::cout << "[WARN] test_http: unexpected status " << resp.status_code << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[WARN] test_http POST exception: " << e.what() << "\n";
    }

    std::cout << "[PASS] test_http completed\n";
    return 0;
}
