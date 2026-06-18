#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct RealityConfigInfo {
    int id = -1;
    int port = 0;
    std::string uuid;
    std::string publicKey;
    std::string shortId;
    bool isValid = false;
};

class XuiApi {
public:
    XuiApi(const std::string& baseUrl, const std::string& token);

    // Fetch inbounds, return true if successful
    bool FetchInbounds(std::string& outError);

    // Find Reality-Test inbound, or create if not exists
    // Returns populated RealityConfigInfo or invalid if failed
    RealityConfigInfo GetOrCreateRealityTestConfig(int port, std::string& outError);

    // Update the destination and serverNames for the Reality-Test config
    bool UpdateRealityTestDomain(int id, const nlohmann::json& fullConfig, const std::string& domain, std::string& outError);

    // Delete inbound and client
    bool DeleteInbound(int id, std::string& outError);
    bool DeleteClient(const std::string& email, std::string& outError);

    // Get the full JSON object of an inbound by ID
    nlohmann::json GetInboundById(int id);

private:
    std::string m_baseUrl;
    std::string m_token;
    nlohmann::json m_lastInbounds;

    std::string BuildHeaders();
};
