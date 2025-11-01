#pragma once

#include "balance/Types.h"
#include <string>
#include <map>

namespace Balance {

/**
 * Simple HTTP client using libcurl
 * For Supabase REST API communication
 */
class HttpClient {
public:
    struct Response {
        int statusCode;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    HttpClient();
    ~HttpClient();

    /**
     * Perform GET request
     */
    Result<Response> get(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {});

    /**
     * Perform POST request
     */
    Result<Response> post(
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers = {});

    /**
     * Perform PUT request
     */
    Result<Response> put(
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers = {});

    /**
     * Perform DELETE request
     */
    Result<Response> del(
        const std::string& url,
        const std::map<std::string, std::string>& headers = {});

    /**
     * Set timeout (milliseconds)
     */
    void setTimeout(long timeoutMs);

private:
    void* curl;  // CURL* handle
    long timeout;

    Result<Response> perform(
        const std::string& method,
        const std::string& url,
        const std::string& body,
        const std::map<std::string, std::string>& headers);

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
};

} // namespace Balance
