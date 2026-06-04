#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

struct HttpResponse {
    long        status_code{0};
    std::string body;
};

struct UploadFile {
    std::string field_name;
    std::string file_path;
};

class HttpClient {
public:
    explicit HttpClient(bool ssl_verify = true) : ssl_verify_(ssl_verify) {
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~HttpClient() {
        curl_global_cleanup();
    }

    // POST JSON, returns response
    HttpResponse post_json(const std::string& url, const nlohmann::json& payload) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        HttpResponse resp;
        std::string body_str = payload.dump();
        std::string response_body;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_str.size());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        if (!ssl_verify_) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
        resp.body = response_body;

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return resp;
    }

    // POST multipart/form-data with files
    HttpResponse post_multipart(const std::string& url,
                                const std::map<std::string, std::string>& fields,
                                const std::vector<UploadFile>& files) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        HttpResponse resp;
        std::string response_body;

        curl_mime* form = curl_mime_init(curl);

        for (auto& [key, val] : fields) {
            curl_mimepart* part = curl_mime_addpart(form);
            curl_mime_name(part, key.c_str());
            curl_mime_data(part, val.c_str(), CURL_ZERO_TERMINATED);
        }

        for (auto& uf : files) {
            curl_mimepart* part = curl_mime_addpart(form);
            curl_mime_name(part, uf.field_name.c_str());
            curl_mime_filedata(part, uf.file_path.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        if (!ssl_verify_) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_mime_free(form);
            curl_easy_cleanup(curl);
            throw std::runtime_error(std::string("curl multipart error: ") + curl_easy_strerror(res));
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
        resp.body = response_body;

        curl_mime_free(form);
        curl_easy_cleanup(curl);
        return resp;
    }

    // Simple connectivity check (HEAD request)
    bool is_reachable(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        if (!ssl_verify_) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return (res == CURLE_OK);
    }

private:
    bool ssl_verify_;

    static size_t write_cb(char* ptr, size_t size, size_t nmemb, std::string* out) {
        out->append(ptr, size * nmemb);
        return size * nmemb;
    }
};
