#include "scanner.h"
#include "utils.h"
#include <iostream>
#include <chrono>
#include <cstdio>

using json = nlohmann::json;

Scanner::Scanner() : m_running(false), m_stopRequested(false) {}

Scanner::~Scanner() {
    StopScan();
    if (m_downloadThread.joinable()) {
        m_downloadThread.join();
    }
}

bool Scanner::IsXrayReady() const {
    return Utils::FileExists("xray.exe");
}

void Scanner::DownloadXrayAsync() {
    if (m_isDownloading) return;
    
    m_isDownloading = true;
    m_downloadProgress = 0.0f;
    SetStatus("Starting download...");
    
    if (m_downloadThread.joinable()) {
        m_downloadThread.join();
    }
    
    m_downloadThread = std::thread([this]() {
        std::string zipUrl = "https://github.com/XTLS/Xray-core/releases/latest/download/Xray-windows-64.zip";
        std::string zipPath = "xray.zip";
        std::string error;
        
        auto cb = [this](size_t downloaded, size_t total) {
            if (total > 0) {
                m_downloadProgress = static_cast<float>(downloaded) / total;
            }
        };
        
        if (!Utils::DownloadFile(zipUrl, zipPath, cb, error)) {
            SetStatus("Download failed: " + error);
            m_isDownloading = false;
            return;
        }
        
        SetStatus("Extracting Xray...");
        if (!Utils::ExtractZip(zipPath, ".")) {
            SetStatus("Extraction failed.");
            m_isDownloading = false;
            return;
        }
        
        std::remove("xray.zip");
        SetStatus("Xray installed successfully.");
        m_isDownloading = false;
    });
}

void Scanner::StartScan(const std::string& baseUrl, const std::string& token, const std::string& serverIp, int port, const std::vector<std::string>& domains) {
    if (m_running) return;
    
    m_results.clear();
    m_stopRequested = false;
    m_scanProgress = 0.0f;
    m_running = true;
    
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
    
    m_scanThread = std::thread(&Scanner::ScanWorker, this, baseUrl, token, serverIp, port, domains);
}

void Scanner::StopScan() {
    m_stopRequested = true;
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
}

std::string Scanner::GetCurrentStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentStatus;
}

std::vector<ScanResult> Scanner::GetResults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_results;
}

void Scanner::SetStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentStatus = status;
}

void Scanner::ScanWorker(std::string baseUrl, std::string token, std::string serverIp, int port, std::vector<std::string> domains) {
    SetStatus("Initializing API...");
    XuiApi api(baseUrl, token);
    
    std::string err;
    if (!api.FetchInbounds(err)) {
        SetStatus("Error fetching inbounds: " + err);
        m_running = false;
        return;
    }
    
    SetStatus("Getting/Creating Reality-Test config...");
    // Check if xray.exe exists BEFORE creating config because we need it to generate x25519 keys
    if (!Utils::FileExists("xray.exe")) {
        SetStatus("Downloading Xray core...");
        std::string zipUrl = "https://github.com/XTLS/Xray-core/releases/latest/download/Xray-windows-64.zip";
        std::string zipPath = "xray.zip";
        std::string errZip;
        
        auto cb = [this](size_t downloaded, size_t total) {
            if (total > 0) {
                m_scanProgress = static_cast<float>(downloaded) / total;
            }
        };
        
        if (!Utils::DownloadFile(zipUrl, zipPath, cb, errZip)) {
            SetStatus("Failed to download Xray core: " + errZip);
            m_running = false;
            return;
        }
        SetStatus("Extracting Xray...");
        if (!Utils::ExtractZip(zipPath, ".")) {
            SetStatus("Failed to extract Xray core.");
            m_running = false;
            return;
        }
        std::remove("xray.zip");
        m_scanProgress = 0.0f; // reset progress
    }

    RealityConfigInfo info = api.GetOrCreateRealityTestConfig(port, err);
    if (!info.isValid) {
        SetStatus("Failed to get Reality-Test config: " + err);
        m_running = false;
        return;
    }
    
    json fullConfig = api.GetInboundById(info.id);
    
    int totalDomains = domains.size();
    int currentIndex = 0;
    
    for (const auto& domain : domains) {
        if (m_stopRequested) break;
        if (domain.empty()) continue;
        
        ScanResult result;
        result.domain = domain;
        result.error = "Checking...";
        result.success = false;
        result.delayMs = -1;
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_results.push_back(result);
        }
        
        auto updateResult = [this, &domain](bool success, const std::string& error, long long delay) {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& r : m_results) {
                if (r.domain == domain) {
                    r.success = success;
                    r.error = error;
                    r.delayMs = delay;
                    break;
                }
            }
        };

        SetStatus("Updating server for domain: " + domain);
        if (!api.UpdateRealityTestDomain(info.id, fullConfig, domain, err)) {
            updateResult(false, "Update failed: " + err, -1);
            continue;
        }
        
        SetStatus("Generating client config for: " + domain);
        if (!WriteClientConfig(info, serverIp, domain, "config.json")) {
            updateResult(false, "Failed to write config.json", -1);
            continue;
        }
        
        SetStatus("Starting Xray for: " + domain);
        void* hProcess = Utils::RunProcess("xray.exe run -c config.json", "");
        if (!hProcess) {
            updateResult(false, "Failed to start xray.exe", -1);
            continue;
        }
        
        // Wait a bit for xray to start up and connect
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        
        if (m_stopRequested) {
            Utils::TerminateProcessHandle(hProcess);
            updateResult(false, "Stopped by user", -1);
            break;
        }
        
        SetStatus("Testing delay for: " + domain);
        long long delay = TestDelay(5000); // 5 seconds timeout
        
        if (delay >= 0) {
            updateResult(true, "", delay);
        } else {
            updateResult(false, "Connection failed/timeout", -1);
        }
        
        // Cleanup
        Utils::TerminateProcessHandle(hProcess);
        
        currentIndex++;
        if (totalDomains > 0) {
            m_scanProgress = static_cast<float>(currentIndex) / totalDomains;
        }
        
        // Wait a bit before next domain to let panel apply changes
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    // Delete the config and client after tests
    if (info.isValid) {
        SetStatus("Cleaning up test inbound...");
        std::string errDel;
        api.DeleteClient("Reality-Test", errDel);
        api.DeleteInbound(info.id, errDel);
    }
    
    m_scanProgress = 1.0f;
    SetStatus("Scan finished.");
    m_running = false;
}

bool Scanner::WriteClientConfig(const RealityConfigInfo& info, const std::string& serverIp, const std::string& domain, const std::string& configPath) {
    json conf = {
        {"inbounds", {
            {
                {"port", 10809},
                {"listen", "127.0.0.1"},
                {"protocol", "http"},
                {"settings", {{"timeout", 0}}}
            }
        }},
        {"outbounds", {
            {
                {"protocol", "vless"},
                {"settings", {
                    {"vnext", {
                        {
                            {"address", serverIp},
                            {"port", info.port},
                            {"users", {
                                {
                                    {"id", info.uuid},
                                    {"encryption", "none"},
                                    {"flow", ""}
                                }
                            }}
                        }
                    }}
                }},
                {"streamSettings", {
                    {"network", "tcp"},
                    {"security", "reality"},
                    {"realitySettings", {
                        {"fingerprint", "chrome"},
                        {"serverName", domain},
                        {"publicKey", info.publicKey},
                        {"shortId", info.shortId},
                        {"spiderX", ""}
                    }}
                }}
            }
        }}
    };
    
    return Utils::WriteFile(configPath, conf.dump(4));
}

long long Scanner::TestDelay(int timeoutMs) {
    return Utils::TestProxyDelay("https://www.google.com/generate_204", "127.0.0.1:10809", timeoutMs);
}
