#pragma once
#include <string>
#include "scanner.h"

class App {
public:
    App();
    ~App();

    void Init();
    void RenderUI();
    bool NeedsContinuousUpdate() const;

private:
    char m_baseUrl[256] = "https://1.2.3.4:54321";
    char m_token[512] = "";
    char m_serverIp[256] = "";
    int m_configPort = 44300;
    char m_domainsInput[65536] = "yahoo.com\n";
    char m_domainsUrl[512] = "";
    std::string m_domainLoadStatus;
    int m_currentTab = 0;
    
    std::string m_apiTestResult;
    
    Scanner m_scanner;
    std::string m_appError;
    
    // Material Design State
    bool m_isDarkTheme = true;
    bool m_drawerOpen = false;
    float m_drawerAnimProgress = 0.0f; // 0.0f (closed) to 1.0f (open)
    
    void ApplyTheme();
};
