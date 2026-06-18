#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace Utils {

    // String conversions
    std::wstring Utf8ToWide(const std::string& str);
    std::string WideToUtf8(const std::wstring& wstr);
    
    // Extract host/IP from a URL
    std::string ExtractHostFromUrl(const std::string& url);

    // Generate random UUID
    std::string GenerateUUID();

    // Generate random shortId (hex string of length 16)
    std::string GenerateShortId();

    // HTTP Request using WinHttp
    // returns empty string on failure. Not ideal for 204, so we add HttpGetStatusCode
    std::string HttpRequest(const std::string& url, 
                            const std::string& method, 
                            const std::string& headers = "", 
                            const std::string& body = "",
                            const std::string& proxy = ""); // proxy format: "127.0.0.1:10809"

    // Ping a URL through proxy and return delay in ms, -1 if failed
    long long TestProxyDelay(const std::string& url, const std::string& proxy, int timeoutMs = 5000);

    // Download file from URL to local path
    bool DownloadFile(const std::string& url, const std::string& destPath, std::function<void(size_t, size_t)> progressCb, std::string& errorMsg);

    // Extract ZIP file using PowerShell
    bool ExtractZip(const std::string& zipPath, const std::string& extractDir);

    // Check if file exists
    bool FileExists(const std::string& path);

    // Read file contents
    std::string ReadFile(const std::string& path);

    // Write file contents
    bool WriteFile(const std::string& path, const std::string& content);

    // Run process and return handle, or nullptr if failed
    void* RunProcess(const std::string& commandLine, const std::string& currentDir);
    std::string RunProcessAndReadOutput(const std::string& commandLine, const std::string& currentDir);

    // Terminate process by handle
    void TerminateProcessHandle(void* handle);

} // namespace Utils
