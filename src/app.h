#pragma once
#include <string>
#include "scanner.h"

class App {
public:
    App();
    ~App();

    void Init();
    void RenderUI();

private:
    char m_baseUrl[256] = "https://1.2.3.4:54321";
    char m_token[512] = "";
    char m_serverIp[256] = "";
    int m_configPort = 44300;
    char m_domainsInput[4096] = "yahoo.com\n";
    
    std::string m_apiTestResult;
    
    Scanner m_scanner;
    std::string m_appError;
};
