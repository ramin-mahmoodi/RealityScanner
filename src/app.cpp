#include "app.h"
#include <imgui.h>
#include <sstream>
#include "utils.h"
#include <algorithm>
#include <cstring>
#include "IconsFontAwesome6.h"

App::App() {
}

App::~App() {
}

void App::Init() {
    // Initialization is now async via UI
}

void App::ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    if (m_isDarkTheme) {
        ImGui::StyleColorsDark(&style);
        // User requested neutral textboxes instead of default blue tint in dark mode
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.30f, 0.32f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.40f, 0.40f, 0.42f, 1.00f);
    } else {
        ImGui::StyleColorsLight(&style);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    }
}

void App::RenderUI() {
    ApplyTheme();

    auto ApplyHandCursor = []() {
        if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    };


    // Animation state update
    float dt = ImGui::GetIO().DeltaTime;
    if (m_drawerOpen) {
        m_drawerAnimProgress += dt * 6.0f; // ~0.16s animation
        if (m_drawerAnimProgress > 1.0f) m_drawerAnimProgress = 1.0f;
    } else {
        m_drawerAnimProgress -= dt * 6.0f;
        if (m_drawerAnimProgress < 0.0f) m_drawerAnimProgress = 0.0f;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize, ImGuiCond_Always);
    
    // Remove global padding to allow edge-to-edge Material layout
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // No main window border
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::Begin("Reality Scanner", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(3);
    
    if (!m_appError.empty()) {
        ImGui::SetCursorPosX(16.0f);
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", m_appError.c_str());
        ImGui::Separator();
    }
    
    float appBarHeight = ImGui::GetFontSize() * 3.0f;
    float btnHeight = ImGui::GetFontSize() * 2.0f;
    float padding = 16.0f;
    float cardRounding = 12.0f;
    float itemSpacingY = ImGui::GetStyle().ItemSpacing.y; // For general use
    
    float progBarHeight = btnHeight / 2.0f; // Half the thickness of the button
    
    float cardPadding = 24.0f; // Matches ContentCard WindowPadding
    float innerGap = cardPadding / 2.0f; // Gap between elements is half the padding
    
    // Exact height needed for footer card: top pad + prog bar + middle gap + button + bottom pad + borders
    float footerCardHeight = cardPadding + progBarHeight + innerGap + btnHeight + cardPadding + 2.0f;
    float contentCardHeight = ImGui::GetWindowHeight() - appBarHeight - footerCardHeight - (padding * 3.0f);

    ImVec4 bgColor = m_isDarkTheme ? ImVec4(0.05f, 0.05f, 0.05f, 1.0f) : ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    ImVec4 surfaceColor = m_isDarkTheme ? ImVec4(0.12f, 0.12f, 0.12f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    // Main window background color
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);

    // --- APP BAR ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceColor); // Distinct surface color for AppBar (White in light theme)
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f); // Force 0 rounding (flat)
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f); // Force 0 border
    if (ImGui::BeginChild("AppBar", ImVec2(0, appBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        float btnSize = appBarHeight * 0.7f;
        float yCenter = (appBarHeight - btnSize) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(16.0f, yCenter));
        
        // Hamburger Button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_isDarkTheme ? ImVec4(1, 1, 1, 0.1f) : ImVec4(0, 0, 0, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_isDarkTheme ? ImVec4(1, 1, 1, 0.2f) : ImVec4(0, 0, 0, 0.2f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btnSize / 2.0f); // Circular ripple effect
        if (ImGui::Button(ICON_FA_BARS, ImVec2(btnSize, btnSize))) {
            m_drawerOpen = !m_drawerOpen;
        }
        ApplyHandCursor();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        
        // Title (Centered and Bold)
        const char* title = m_currentTab == 0 ? "Settings" : (m_currentTab == 1 ? "Domains List" : "Results");
        
        ImFont* boldFont = ImGui::GetIO().Fonts->Fonts.Size > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr;
        if (boldFont) ImGui::PushFont(boldFont);
        
        float titleWidth = ImGui::CalcTextSize(title).x;
        float titleX = (ImGui::GetWindowWidth() - titleWidth) / 2.0f;
        float titleY = (appBarHeight - ImGui::GetFontSize()) / 2.0f;
        
        ImGui::SetCursorPos(ImVec2(titleX, titleY));
        ImGui::Text("%s", title);
        
        if (boldFont) ImGui::PopFont();
        
        // Theme Toggle Button
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - btnSize - 16.0f);
        ImGui::SetCursorPosY(yCenter);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_isDarkTheme ? ImVec4(1, 1, 1, 0.1f) : ImVec4(0, 0, 0, 0.1f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_isDarkTheme ? ImVec4(1, 1, 1, 0.2f) : ImVec4(0, 0, 0, 0.2f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, btnSize / 2.0f);
        if (ImGui::Button(m_isDarkTheme ? ICON_FA_SUN : ICON_FA_MOON, ImVec2(btnSize, btnSize))) {
            m_isDarkTheme = !m_isDarkTheme;
        }
        ApplyHandCursor();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2); // AppBar rounding and border
    ImGui::PopStyleColor();
    
    // --- CONTENT CARD ---
    ImGui::SetCursorPos(ImVec2(padding, appBarHeight + padding));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, cardRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f)); // Generous breathing room inside card
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 16.0f)); // Generous vertical space
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f); // Borders on inputs
    
    if (!m_appError.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", m_appError.c_str());
        ImGui::Separator();
    }
    
    if (ImGui::BeginChild("ContentCard", ImVec2(ImGui::GetWindowWidth() - (padding * 2.0f), contentCardHeight), true, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
        
        if (m_currentTab == 0) { // Settings
            ImGui::Text("Panel Address");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##BaseURL", m_baseUrl, IM_ARRAYSIZE(m_baseUrl));
            ImGui::PopItemWidth();
            
            ImGui::Text("API Token");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##Token", m_token, IM_ARRAYSIZE(m_token));
            ImGui::PopItemWidth();
            
            ImGui::Text("Config Port");
            ImGui::PushItemWidth(-1);
            ImGui::InputInt("##Port", &m_configPort);
            ImGui::PopItemWidth();
            
            ImGui::Spacing();
            
            float availWidth = ImGui::GetContentRegionAvail().x;
            float btnWidthHalf = (availWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
            
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
            ApplyHandCursor();
            
            ImGui::SameLine();
            
            if (ImGui::Button("Check Xray Core", ImVec2(btnWidthHalf, btnHeight))) {
                if (!m_scanner.IsXrayReady()) {
                    m_scanner.DownloadXrayAsync();
                } else {
                    m_apiTestResult = "Xray Core is already installed and ready.";
                }
            }
            ApplyHandCursor();
            
            if (!m_apiTestResult.empty()) {
                ImGui::TextWrapped("%s", m_apiTestResult.c_str());
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            ImGui::Text("Load Domains from URL/File");
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##DomainsUrl", m_domainsUrl, IM_ARRAYSIZE(m_domainsUrl));
            ImGui::PopItemWidth();
            
            availWidth = ImGui::GetContentRegionAvail().x;
            btnWidthHalf = (availWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
            
            if (ImGui::Button("Fetch from URL", ImVec2(btnWidthHalf, btnHeight))) {
                m_domainLoadStatus = "Fetching...";
                std::string url = m_domainsUrl;
                if (!url.empty()) {
                    std::string content = Utils::HttpRequest(url, "GET");
                    if (!content.empty()) {
                        strncpy(m_domainsInput, content.c_str(), sizeof(m_domainsInput) - 1);
                        m_domainsInput[sizeof(m_domainsInput) - 1] = '\0';
                        m_domainLoadStatus = "Successfully loaded domains from URL.";
                    } else {
                        m_domainLoadStatus = "Error: Failed to fetch from URL.";
                    }
                }
            }
            ApplyHandCursor();
            
            ImGui::SameLine();
            
            if (ImGui::Button("Load from File", ImVec2(btnWidthHalf, btnHeight))) {
                std::string filepath = Utils::OpenFileDialog();
                if (!filepath.empty()) {
                    std::string content = Utils::ReadFile(filepath);
                    if (!content.empty()) {
                        strncpy(m_domainsInput, content.c_str(), sizeof(m_domainsInput) - 1);
                        m_domainsInput[sizeof(m_domainsInput) - 1] = '\0';
                        m_domainLoadStatus = "Successfully loaded domains from file.";
                    } else {
                        m_domainLoadStatus = "Error: Failed to read file or file is empty.";
                    }
                }
            }
            ApplyHandCursor();
            
            if (!m_domainLoadStatus.empty()) {
                ImGui::TextWrapped("%s", m_domainLoadStatus.c_str());
            }
            
        } else if (m_currentTab == 1) { // Domains List
            ImGui::Text("Domains to Test (One per line)");
            auto textFilter = [](ImGuiInputTextCallbackData* data) -> int {
                if (data->EventChar == ' ' || data->EventChar == '\t' || data->EventChar == ',') return 1;
                return 0;
            };
            ImGui::InputTextMultiline("##domains", m_domainsInput, IM_ARRAYSIZE(m_domainsInput), ImVec2(-1, -1), ImGuiInputTextFlags_CallbackCharFilter, textFilter);
            
        } else if (m_currentTab == 2) { // Results
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0, 0, 0, 0)); // Transparent header bg
            ImVec4 rowBg1 = m_isDarkTheme ? ImVec4(0.10f, 0.10f, 0.10f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
            ImVec4 rowBg2 = m_isDarkTheme ? ImVec4(0.12f, 0.12f, 0.12f, 1.0f) : ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, rowBg1);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, rowBg2);
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(16.0f, 16.0f)); // Spacious cells
            
            // NO borders at all, just alternating row backgrounds
            ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
            
            if (ImGui::BeginTable("ResultsTable", 3, flags, ImVec2(0.0f, -1.0f))) {
                ImGui::TableSetupScrollFreeze(0, 1);
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
                
                auto CenterText = [](const char* text, const ImVec4& color = ImVec4(1,1,1,1)) {
                    float windowWidth = ImGui::GetColumnWidth();
                    float textWidth = ImGui::CalcTextSize(text).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (windowWidth - textWidth) * 0.5f);
                    if (color.x == 1 && color.y == 1 && color.z == 1 && color.w == 1) ImGui::TextUnformatted(text);
                    else ImGui::TextColored(color, "%s", text);
                };
                
                for (size_t i = 0; i < results.size(); ++i) {
                    const auto& res = results[i];
                    ImGui::PushID((int)i);
                    ImGui::TableNextRow();
                    
                    // Column 0: Domain
                    ImGui::TableNextColumn();
                    CenterText(res.domain.c_str());
                    
                    // Column 1: Status
                    ImGui::TableNextColumn();
                    if (res.success) {
                        CenterText("Success", ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green
                    } else if (res.error == "Checking...") {
                        CenterText("Checking...", ImVec4(0.8f, 0.8f, 0.2f, 1.0f)); // Yellow
                    } else {
                        CenterText("Failed", ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // Red
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", res.error.c_str());
                    }
                    
                    // Column 2: Delay
                    ImGui::TableNextColumn();
                    if (res.success) {
                        char delayStr[64];
                        snprintf(delayStr, sizeof(delayStr), "%lld ms", res.delayMs);
                        CenterText(delayStr);
                    } else {
                        CenterText("-");
                    }
                    
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::PopStyleVar(); // CellPadding
            ImGui::PopStyleColor(3); // TableHeaderBg, TableRowBg, TableRowBgAlt
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(4); // ChildRounding, WindowPadding, ItemSpacing, FrameBorderSize
    ImGui::PopStyleColor(); // ChildBg
    
    // --- FOOTER CARD ---
    ImGui::SetCursorPos(ImVec2(padding, appBarHeight + padding + contentCardHeight + padding));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, cardRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(cardPadding, cardPadding));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(cardPadding, innerGap));
    
    if (ImGui::BeginChild("FooterCard", ImVec2(ImGui::GetWindowWidth() - (padding * 2.0f), footerCardHeight), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysUseWindowPadding)) {
        
        bool isRunning = m_scanner.IsRunning() || m_scanner.IsDownloading();
        float progress = 0.0f;
        if (isRunning) {
            progress = m_scanner.IsDownloading() ? m_scanner.GetDownloadProgress() : m_scanner.GetScanProgress();
        }
        
        ImGui::ProgressBar(progress, ImVec2(-1.0f, progBarHeight), ""); // Takes full width of the card's padded area
        
        bool canScan = m_scanner.IsXrayReady() && !m_scanner.IsDownloading();
        
        if (m_scanner.IsRunning()) {
            // Equivalent Red to ImGui's default Blue
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.98f, 0.26f, 0.26f, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.98f, 0.26f, 0.26f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.98f, 0.06f, 0.06f, 1.00f));
            
            char btnLabel[128];
            snprintf(btnLabel, sizeof(btnLabel), "%s Stop Scan (%.0f%%)", ICON_FA_SQUARE, progress * 100.0f);
            if (ImGui::Button(btnLabel, ImVec2(-1.0f, btnHeight))) {
                m_scanner.StopScan();
            }
            ApplyHandCursor();
            ImGui::PopStyleColor(3);
        } else {
            // Equivalent Green to ImGui's default Blue
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.80f, 0.30f, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.80f, 0.30f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.06f, 0.70f, 0.15f, 1.00f));
            
            if (!canScan) ImGui::BeginDisabled();
            
            char btnLabel[128];
            if (m_scanner.IsDownloading()) {
                snprintf(btnLabel, sizeof(btnLabel), "%s Downloading Xray... (%.0f%%)", ICON_FA_DOWNLOAD, progress * 100.0f);
            } else if (!canScan) {
                snprintf(btnLabel, sizeof(btnLabel), "%s Install Xray to start", ICON_FA_DOWNLOAD);
            } else {
                snprintf(btnLabel, sizeof(btnLabel), "%s Start Scan", ICON_FA_PLAY);
            }
            
            if (ImGui::Button(btnLabel, ImVec2(-1.0f, btnHeight))) {
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
            ApplyHandCursor();
            if (!canScan) ImGui::EndDisabled();
            ImGui::PopStyleColor(3); // Pop the Green button colors
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(4); // ChildRounding, ChildBorderSize, WindowPadding, ItemSpacing
    ImGui::PopStyleColor(); // ChildBg
    
    ImGui::PopStyleColor(); // Pop WindowBg
    // --- NAVIGATION DRAWER (Animated Overlay) ---
    if (m_drawerAnimProgress > 0.0f) {
        float drawerWidth = ImGui::GetFontSize() * 16.0f; // Dynamic width based on font size (scales automatically with DPI)
        float currentDrawerX = -drawerWidth + (drawerWidth * m_drawerAnimProgress);
        
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        
        // Dark Dimming Overlay
        ImGui::SetCursorPos(ImVec2(0, appBarHeight + 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.5f * m_drawerAnimProgress));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        
        // Draw the scrim as a child so we can color it, and make its contents capture clicks
        if (ImGui::BeginChild("DrawerScrim", ImVec2(displaySize.x, displaySize.y - appBarHeight - 1.0f), false, ImGuiWindowFlags_NoScrollbar)) {
            // Invisible button to close drawer when clicking outside
            if (ImGui::InvisibleButton("CloseDrawerBtn", ImVec2(displaySize.x, displaySize.y))) {
                m_drawerOpen = false;
            }
            ApplyHandCursor(); // Show hand cursor when hovering outside drawer to indicate it closes it
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Drawer Container
        ImGui::SetCursorPos(ImVec2(currentDrawerX, appBarHeight + 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceColor); // 100% Solid background
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // Removed top/bottom padding
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f)); // No spacing between drawer items
        if (ImGui::BeginChild("NavigationDrawer", ImVec2(drawerWidth, displaySize.y - appBarHeight - 1.0f), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
            
            auto DrawDrawerItem = [&](const char* icon, const char* label, int tabIndex) {
                bool isSelected = (m_currentTab == tabIndex);
                
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, m_isDarkTheme ? ImVec4(1, 1, 1, 0.12f) : ImVec4(0, 0, 0, 0.08f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_isDarkTheme ? ImVec4(1, 1, 1, 0.16f) : ImVec4(0, 0, 0, 0.12f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_isDarkTheme ? ImVec4(1, 1, 1, 0.20f) : ImVec4(0, 0, 0, 0.16f));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_isDarkTheme ? ImVec4(1, 1, 1, 0.05f) : ImVec4(0, 0, 0, 0.05f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_isDarkTheme ? ImVec4(1, 1, 1, 0.08f) : ImVec4(0, 0, 0, 0.08f));
                }
                
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.05f, 0.5f)); // Left align text
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f); // NO BORDER - removes button feel
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f); // Sharp corners (0 rounding)
                
                char btnLabel[128];
                snprintf(btnLabel, sizeof(btnLabel), "   %s    %s", icon, label);
                
                // Increase length (height) of the items
                if (ImGui::Button(btnLabel, ImVec2(-1.0f, btnHeight * 1.6f))) {
                    m_currentTab = tabIndex;
                    m_drawerOpen = false; // Close drawer on selection
                }
                ApplyHandCursor();
                ImGui::PopStyleVar(3);
                ImGui::PopStyleColor(3);
                // Removed ImGui::Spacing() to eliminate gap between items
            };
            
            DrawDrawerItem(ICON_FA_GEAR, "Settings", 0);
            DrawDrawerItem(ICON_FA_LIST, "Domains List", 1);
            DrawDrawerItem(ICON_FA_CHECK, "Results", 2);
            
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
    
    ImGui::End();
}

bool App::NeedsContinuousUpdate() const {
    // If the drawer is currently animating, we need high FPS
    if (m_drawerAnimProgress > 0.0f && m_drawerAnimProgress < 1.0f) {
        return true;
    }
    // If scanning or downloading, progress bar is moving, so high FPS
    if (m_scanner.IsRunning() || m_scanner.IsDownloading()) {
        return true;
    }
    return false;
}
