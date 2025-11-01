#include "HttpClient.h"
#include <curl/curl.h>
#include <sstream>

namespace Balance {

HttpClient::HttpClient() : timeout(30000) {
    curl = curl_easy_init();
}

HttpClient::~HttpClient() {
    if (curl) {
        curl_easy_cleanup(static_cast<CURL*>(curl));
    }
}

void HttpClient::setTimeout(long timeoutMs) {
    timeout = timeoutMs;
}

Result<HttpClient::Response> HttpClient::get(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {
    return perform("GET", url, "", headers);
}

Result<HttpClient::Response> HttpClient::post(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
    return perform("POST", url, body, headers);
}

Result<HttpClient::Response> HttpClient::put(
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
    return perform("PUT", url, body, headers);
}

Result<HttpClient::Response> HttpClient::del(
    const std::string& url,
    const std::map<std::string, std::string>& headers) {
    return perform("DELETE", url, "", headers);
}

Result<HttpClient::Response> HttpClient::perform(
    const std::string& method,
    const std::string& url,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {

    if (!curl) {
        return Error("CURL not initialized", -1);
    }

    CURL* curlHandle = static_cast<CURL*>(curl);

    // Reset curl handle
    curl_easy_reset(curlHandle);

    // Set URL
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());

    // Set method
    if (method == "POST") {
        curl_easy_setopt(curlHandle, CURLOPT_POST, 1L);
    } else if (method == "PUT") {
        curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (method == "DELETE") {
        curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
    }

    // Set headers
    struct curl_slist* headerList = nullptr;
    for (const auto& [key, value] : headers) {
        std::string header = key + ": " + value;
        headerList = curl_slist_append(headerList, header.c_str());
    }

    if (headerList) {
        curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);
    }

    // Set body for POST/PUT
    if (!body.empty() && (method == "POST" || method == "PUT")) {
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
    }

    // Set timeout
    curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT_MS, timeout);

    // Set write callback
    std::string responseBody;
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &responseBody);

    // Set header callback
    std::map<std::string, std::string> responseHeaders;
    curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, &responseHeaders);

    // Perform request
    CURLcode res = curl_easy_perform(curlHandle);

    // Clean up header list
    if (headerList) {
        curl_slist_free_all(headerList);
    }

    if (res != CURLE_OK) {
        return Error("HTTP request failed: " + std::string(curl_easy_strerror(res)), res);
    }

    // Get status code
    long statusCode;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &statusCode);

    Response response;
    response.statusCode = static_cast<int>(statusCode);
    response.body = responseBody;
    response.headers = responseHeaders;

    return response;
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t HttpClient::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalSize = size * nitems;
    std::string header(buffer, totalSize);

    // Parse header
    size_t colonPos = header.find(':');
    if (colonPos != std::string::npos) {
        std::string key = header.substr(0, colonPos);
        std::string value = header.substr(colonPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);
        value.erase(0, value.find_first_not_of(" \t\r\n"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);

        std::map<std::string, std::string>* headers =
            static_cast<std::map<std::string, std::string>*>(userdata);
        (*headers)[key] = value;
    }

    return totalSize;
}

} // namespace Balance
