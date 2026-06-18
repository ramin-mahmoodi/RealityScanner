#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "xui_api.h"

struct ScanResult {
    std::string domain;
    bool success = false;
    long long delayMs = -1;
    std::string error;
};

class Scanner {
public:
    Scanner();
    ~Scanner();

    // Setup Xray (download if needed)
    bool PrepareXray(std::string& outError);

    // Start scanning domains
    void StartScan(const std::string& baseUrl, const std::string& token, const std::string& serverIp, int port,
                   const std::vector<std::string>& domains);

    // Stop scanning
    void StopScan();

    // Get current status
    bool IsRunning() const { return m_running; }
    bool IsDownloading() const { return m_isDownloading; }
    float GetDownloadProgress() const { return m_downloadProgress; }
    float GetScanProgress() const { return m_scanProgress; }
    std::string GetCurrentStatus() const;
    std::vector<ScanResult> GetResults();
    
    bool IsXrayReady() const;
    void DownloadXrayAsync();

private:
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isDownloading{false};
    std::atomic<float> m_downloadProgress{0.0f};
    std::atomic<float> m_scanProgress{0.0f};
    std::atomic<bool> m_stopRequested{false};
    std::thread m_scanThread;
    std::thread m_downloadThread;
    
    mutable std::mutex m_mutex;
    std::string m_currentStatus;
    std::vector<ScanResult> m_results;

    void SetStatus(const std::string& status);

    void ScanWorker(std::string baseUrl, std::string token, std::string serverIp, int port, std::vector<std::string> domains);

    bool WriteClientConfig(const RealityConfigInfo& info, const std::string& serverIp, const std::string& domain, const std::string& configPath);
    
    long long TestDelay(int timeoutMs);
};
