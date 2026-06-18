#include "app.h"
#include <imgui.h>
#include <sstream>
#include "utils.h"
#include <algorithm>
#include <cstring>

App::App() {
}

App::~App() {
}

void App::Init() {
    // Initialization is now async via UI
}

void App::RenderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    
    ImGui::Begin("Reality Scanner", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    
    if (!m_appError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", m_appError.c_str());
        ImGui::Separator();
    }
    
    ImGui::Text("Panel Address");
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##BaseURL", m_baseUrl, IM_ARRAYSIZE(m_baseUrl));
    ImGui::PopItemWidth();
    
    ImGui::Text("Panel token");
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##Token", m_token, IM_ARRAYSIZE(m_token));
    ImGui::PopItemWidth();
    
    ImGui::Text("Config port");
    ImGui::PushItemWidth(-1);
    ImGui::InputInt("##Port", &m_configPort);
    ImGui::PopItemWidth();
    
    ImGui::Spacing();
    
    // Scale button sizes based on font size and window width
    float availWidth = ImGui::GetContentRegionAvail().x;
    float btnWidthHalf = (availWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    float btnWidthFull = availWidth;
    float btnHeight = ImGui::GetFontSize() * 2.0f;
    
    if (ImGui::Button("Test API Connection", ImVec2(btnWidthHalf, btnHeight))) {
        m_apiTestResult = "Testing...";
        XuiApi testApi(m_baseUrl, m_token);
        std::string err;
        if (testApi.FetchInbounds(err)) {
            m_apiTestResult = "Success! Connected to panel.";
        } else {
            m_apiTestResult = "Error: " + err;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Check Xray Core", ImVec2(btnWidthHalf, btnHeight))) {
        if (!m_scanner.IsXrayReady()) {
            m_scanner.DownloadXrayAsync();
        } else {
            m_apiTestResult = "Xray Core is already installed and ready.";
        }
    }
    
    if (!m_apiTestResult.empty()) {
        ImGui::TextWrapped("%s", m_apiTestResult.c_str());
    }
    
    ImGui::Separator();
    ImGui::Text("Domains to Test (One per line)");
    ImGui::InputTextMultiline("##domains", m_domainsInput, IM_ARRAYSIZE(m_domainsInput), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() * 8));
    
    ImGui::Separator();
    ImGui::Spacing();
    

    
    if (m_scanner.IsRunning()) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("Stop Scan", ImVec2(btnWidthFull, btnHeight))) {
            m_scanner.StopScan();
        }
        ImGui::PopStyleColor(3);
        ImGui::Text("Status: %s", m_scanner.GetCurrentStatus().c_str());
    } else {
        bool canScan = m_scanner.IsXrayReady() && !m_scanner.IsDownloading();
        if (!canScan) {
            ImGui::BeginDisabled();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.5f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.2f, 1.0f));
        if (ImGui::Button("Start Scan", ImVec2(btnWidthFull, btnHeight))) {
            std::vector<std::string> domains;
            std::stringstream ss(m_domainsInput);
            std::string item;
            while (std::getline(ss, item, '\n')) {
                item.erase(0, item.find_first_not_of(" \t\r\n"));
                item.erase(item.find_last_not_of(" \t\r\n") + 1);
                if (!item.empty()) domains.push_back(item);
            }
            if (!domains.empty()) {
                std::string extractedIp = Utils::ExtractHostFromUrl(m_baseUrl);
                m_scanner.StartScan(m_baseUrl, m_token, extractedIp, m_configPort, domains);
            }
        }
        ImGui::PopStyleColor(3);
        if (!canScan) {
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Install Xray to start");
        }
    }
    
    ImGui::Separator();
    
    // Persistent Progress Bar
    float progress = 0.0f;
    std::string progressStatus = "Idle";
    
    if (m_scanner.IsDownloading()) {
        progress = m_scanner.GetDownloadProgress();
        progressStatus = "Downloading Xray: " + m_scanner.GetCurrentStatus();
    } else if (m_scanner.IsRunning()) {
        progress = m_scanner.GetScanProgress();
        progressStatus = "Scanning: " + m_scanner.GetCurrentStatus();
    } else if (!m_scanner.IsXrayReady()) {
        progressStatus = "Xray Core not found. Please click 'Check Xray Core' to install.";
    }
    
    ImGui::Text("%s", progressStatus.c_str());
    
    float originalScale = ImGui::GetFont()->Scale;
    ImGui::SetWindowFontScale(0.7f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x, 2.0f));
    ImGui::ProgressBar(progress, ImVec2(-1.0f, ImGui::GetFontSize() * 1.2f));
    ImGui::PopStyleVar();
    ImGui::SetWindowFontScale(originalScale);
    
    ImGui::Separator();
    ImGui::Text("Results");
    
    // Calculate table height, ensuring it doesn't collapse entirely if the window is too small
    float tableHeight = ImGui::GetContentRegionAvail().y;
    if (tableHeight < 200.0f) {
        tableHeight = 200.0f;
    }
    
    // Use dynamic height for the table
    if (ImGui::BeginTable("ResultsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable, ImVec2(0.0f, tableHeight))) {
        ImGui::TableSetupScrollFreeze(0, 1); // Freeze the header row
        ImGui::TableSetupColumn("Domain", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Delay (ms)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableHeadersRow();
        
        auto results = m_scanner.GetResults();
        
        if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs()) {
            if (sorts_specs->SpecsCount > 0) {
                const ImGuiTableColumnSortSpecs* sort_spec = &sorts_specs->Specs[0];
                std::sort(results.begin(), results.end(), [sort_spec](const ScanResult& a, const ScanResult& b) {
                    int delta = 0;
                    if (sort_spec->ColumnIndex == 0) {
                        delta = a.domain.compare(b.domain);
                    } else if (sort_spec->ColumnIndex == 1) {
                        auto getStatusVal = [](const ScanResult& r) {
                            if (r.success) return 1;
                            if (r.error == "Checking...") return 0;
                            return 2;
                        };
                        delta = getStatusVal(a) - getStatusVal(b);
                    } else if (sort_spec->ColumnIndex == 2) {
                        long long delayA = a.success ? a.delayMs : 999999;
                        long long delayB = b.success ? b.delayMs : 999999;
                        if (delayA < delayB) delta = -1;
                        else if (delayA > delayB) delta = 1;
                        else delta = 0;
                    }
                    return sort_spec->SortDirection == ImGuiSortDirection_Ascending ? (delta < 0) : (delta > 0);
                });
            }
        }
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& res = results[i];
            ImGui::PushID((int)i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            
            // Make domain selectable and copyable
            ImGui::SetNextItemWidth(-FLT_MIN);
            char buf[256];
            strncpy(buf, res.domain.c_str(), sizeof(buf));
            buf[sizeof(buf) - 1] = '\0';
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::InputText("##domain", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleColor(2);
            
            ImGui::TableNextColumn();
            if (res.success) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Success");
            } else if (res.error == "Checking...") {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Checking...");
            } else if (!res.error.empty()) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed");
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res.error.c_str());
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed");
            }
            
            ImGui::TableNextColumn();
            if (res.success) {
                ImGui::Text("%lld ms", res.delayMs);
            } else {
                ImGui::Text("-");
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    
    ImGui::End();
}
