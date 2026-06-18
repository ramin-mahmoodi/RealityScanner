#include "xui_api.h"
#include "utils.h"
#include <iostream>

using json = nlohmann::json;

XuiApi::XuiApi(const std::string& baseUrl, const std::string& token)
    : m_baseUrl(baseUrl), m_token(token) {
    if (!m_baseUrl.empty() && m_baseUrl.back() == '/') {
        m_baseUrl.pop_back();
    }
}

std::string XuiApi::BuildHeaders() {
    std::string headers = "Accept: application/json\r\n";
    if (!m_token.empty()) {
        headers += "Authorization: Bearer " + m_token + "\r\n";
    }
    return headers;
}

bool XuiApi::FetchInbounds(std::string& outError) {
    std::string url = m_baseUrl + "/panel/api/inbounds/list";
    std::string response = Utils::HttpRequest(url, "GET", BuildHeaders(), "", "");
    
    if (response.empty()) {
        outError = "Failed to connect to panel or empty response.";
        return false;
    }

    try {
        auto j = json::parse(response);
        if (j.contains("success") && j["success"] == true) {
            m_lastInbounds = j["obj"];
            return true;
        } else {
            outError = "API returned error: " + (j.contains("msg") ? j["msg"].get<std::string>() : "Unknown error");
            return false;
        }
    } catch (const std::exception& e) {
        outError = std::string("JSON Parse error: ") + e.what() + "\nResponse: " + response;
        return false;
    }
}

nlohmann::json XuiApi::GetInboundById(int id) {
    if (m_lastInbounds.is_array()) {
        for (const auto& item : m_lastInbounds) {
            if (item["id"] == id) return item;
        }
    }
    return nlohmann::json();
}

RealityConfigInfo XuiApi::GetOrCreateRealityTestConfig(int port, std::string& outError) {
    RealityConfigInfo info;
    
    // Search existing
    if (m_lastInbounds.is_array()) {
        for (const auto& item : m_lastInbounds) {
            if (item.contains("remark") && item["remark"] == "Reality-Test") {
                // Check if it has reality
                try {
                    std::string streamStr = item["streamSettings"].is_string() ? item["streamSettings"].get<std::string>() : item["streamSettings"].dump();
                    if (streamStr.empty() || streamStr == "\"\"") streamStr = "{}";
                    json streamSettings = json::parse(streamStr);
                    
                    if (streamSettings.contains("security") && streamSettings["security"] == "reality") {
                        info.id = item["id"].is_number() ? item["id"].get<int>() : std::stoi(item["id"].get<std::string>());
                        info.port = item["port"].is_number() ? item["port"].get<int>() : std::stoi(item["port"].get<std::string>());
                        
                        std::string settingsStr = item["settings"].is_string() ? item["settings"].get<std::string>() : item["settings"].dump();
                        if (settingsStr.empty() || settingsStr == "\"\"") settingsStr = "{}";
                        json settings = json::parse(settingsStr);
                        
                        if (settings.contains("clients") && settings["clients"].is_array() && !settings["clients"].empty()) {
                            if (settings["clients"][0].contains("id")) {
                                info.uuid = settings["clients"][0]["id"].get<std::string>();
                            }
                        }
                        
                        if (streamSettings.contains("realitySettings")) {
                            auto& rs = streamSettings["realitySettings"];
                            if (rs.contains("publicKey") && rs["publicKey"].is_string()) {
                                info.publicKey = rs["publicKey"].get<std::string>();
                            } else if (rs.contains("privateKey") && rs["privateKey"].is_string()) {
                                std::string priv = rs["privateKey"].get<std::string>();
                                if (!priv.empty()) {
                                    std::string out = Utils::RunProcessAndReadOutput(".\\xray.exe x25519 -i " + priv, "");
                                    size_t pubPos = out.find("PublicKey):");
                                    if (pubPos != std::string::npos) {
                                        size_t pubEnd = out.find('\n', pubPos);
                                        if (pubEnd != std::string::npos) {
                                            info.publicKey = out.substr(pubPos + 11, pubEnd - pubPos - 11);
                                            info.publicKey.erase(0, info.publicKey.find_first_not_of(" \t\r\n"));
                                            info.publicKey.erase(info.publicKey.find_last_not_of(" \t\r\n") + 1);
                                        }
                                    }
                                }
                            }
                            if (rs.contains("shortIds") && rs["shortIds"].is_array() && !rs["shortIds"].empty()) {
                                info.shortId = rs["shortIds"][0].is_string() ? rs["shortIds"][0].get<std::string>() : "";
                            }
                        }
                        info.isValid = true;
                        return info;
                    } else {
                        outError = "Config 'Reality-Test' exists but security is NOT 'reality'!";
                        return info;
                    }
                } catch (const std::exception& e) {
                    outError = std::string("Failed to parse existing Reality-Test config: ") + e.what();
                    return info;
                }
            }
        }
    }

    // Create new
    json newInbound = {
        {"enable", true},
        {"remark", "Reality-Test"},
        {"listen", ""},
        {"port", port},
        {"protocol", "vless"},
        {"expiryTime", 0},
        {"total", 0},
        {"sniffing", {
            {"enabled", false},
            {"destOverride", {"http", "tls"}}
        }}
    };

    std::string privateKey = "";
    std::string publicKey = "";
    std::string shortId = Utils::GenerateShortId();
    
    // Generate X25519 keys using xray.exe
    std::string xrayOutput = Utils::RunProcessAndReadOutput(".\\xray.exe x25519", "");
    if (!xrayOutput.empty()) {
        size_t privPos = xrayOutput.find("PrivateKey:");
        size_t pubPos = xrayOutput.find("PublicKey):");
        if (privPos != std::string::npos && pubPos != std::string::npos) {
            size_t privEnd = xrayOutput.find('\n', privPos);
            size_t pubEnd = xrayOutput.find('\n', pubPos);
            if (privEnd != std::string::npos && pubEnd != std::string::npos) {
                privateKey = xrayOutput.substr(privPos + 11, privEnd - privPos - 11);
                publicKey = xrayOutput.substr(pubPos + 11, pubEnd - pubPos - 11);
                
                // Trim spaces and returns
                auto trim = [](std::string& s) {
                    s.erase(0, s.find_first_not_of(" \t\r\n"));
                    s.erase(s.find_last_not_of(" \t\r\n") + 1);
                };
                trim(privateKey);
                trim(publicKey);
            }
        }
    }

    std::string uuid = Utils::GenerateUUID();
    json settings = {
        {"clients", {
            {{"id", uuid}, {"email", "Reality-Test"}, {"enable", true}}
        }},
        {"decryption", "none"},
        {"fallbacks", json::array()}
    };
    newInbound["settings"] = settings.dump();

    json realitySettings = {
        {"show", false},
        {"target", "google.com:443"},
        {"dest", "google.com:443"},
        {"xver", 0},
        {"serverNames", {"google.com"}},
        {"privateKey", privateKey},
        {"shortIds", {shortId}},
        {"settings", {
            {"publicKey", publicKey},
            {"fingerprint", "chrome"},
            {"serverName", ""},
            {"spiderX", "/"}
        }}
    };

    json streamSettings = {
        {"network", "tcp"},
        {"security", "reality"},
        {"realitySettings", realitySettings}
    };
    newInbound["streamSettings"] = streamSettings.dump();

    std::string url = m_baseUrl + "/panel/api/inbounds/add";
    std::string reqBody = newInbound.dump();
    std::string headers = BuildHeaders() + "Content-Type: application/json\r\n";
    
    std::string response = Utils::HttpRequest(url, "POST", headers, reqBody, "");
    if (response.empty()) {
        outError = "Failed to send Add Inbound request.";
        return info;
    }

    try {
        auto j = json::parse(response);
        if (j.contains("success") && j["success"] == true) {
            // Created! We must fetch inbounds again to get the generated keys and ID.
            if (!FetchInbounds(outError)) {
                outError = "Created inbound, but failed to fetch updated list: " + outError;
                return info;
            }
            // Recurse once to find it
            return GetOrCreateRealityTestConfig(port, outError);
        } else {
            outError = "Failed to create inbound: " + (j.contains("msg") ? j["msg"].get<std::string>() : "Unknown error");
            return info;
        }
    } catch (const std::exception& e) {
        outError = std::string("JSON parse error on add inbound: ") + e.what() + "\nResp: " + response;
        return info;
    }
}

bool XuiApi::UpdateRealityTestDomain(int id, const json& fullConfig, const std::string& domain, std::string& outError) {
    json updateData = fullConfig;
    
    try {
        json streamSettings = fullConfig["streamSettings"].is_string() ? json::parse(fullConfig["streamSettings"].get<std::string>()) : fullConfig["streamSettings"];
        streamSettings["realitySettings"]["target"] = domain + ":443";
        streamSettings["realitySettings"]["dest"] = domain + ":443";
        streamSettings["realitySettings"]["serverNames"] = json::array({domain});
        
        updateData["streamSettings"] = streamSettings.dump();
        
        // Settings might be string as well
        if (updateData["settings"].is_object()) {
            updateData["settings"] = updateData["settings"].dump();
        }
        if (updateData["sniffing"].is_object()) {
            updateData["sniffing"] = updateData["sniffing"].dump();
        }

    } catch (const std::exception& e) {
        outError = std::string("Failed to build update payload: ") + e.what();
        return false;
    }

    std::string url = m_baseUrl + "/panel/api/inbounds/update/" + std::to_string(id);
    std::string reqBody = updateData.dump();
    std::string headers = BuildHeaders() + "Content-Type: application/json\r\n";

    std::string response = Utils::HttpRequest(url, "POST", headers, reqBody, "");
    if (response.empty()) {
        outError = "Failed to send update request.";
        return false;
    }

    try {
        auto j = json::parse(response);
        if (j.contains("success") && j["success"] == true) {
            return true;
        } else {
            outError = "Failed to update inbound: " + (j.contains("msg") ? j["msg"].get<std::string>() : "Unknown error");
            return false;
        }
    } catch (const std::exception& e) {
        outError = std::string("JSON parse error on update inbound: ") + e.what() + "\nResp: " + response;
        return false;
    }
}

bool XuiApi::DeleteClient(const std::string& email, std::string& outError) {
    std::string url = m_baseUrl + "/panel/api/clients/del/" + email;
    std::string res = Utils::HttpRequest(url, "POST", BuildHeaders(), "");
    if (res.empty()) {
        outError = "Failed to send delete client request";
        return false;
    }
    return true;
}

bool XuiApi::DeleteInbound(int id, std::string& outError) {
    std::string url = m_baseUrl + "/panel/api/inbounds/del/" + std::to_string(id);
    std::string res = Utils::HttpRequest(url, "POST", BuildHeaders(), "");
    if (res.empty()) {
        outError = "Failed to send delete inbound request";
        return false;
    }
    return true;
}
