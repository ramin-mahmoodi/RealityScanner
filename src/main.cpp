#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <iostream>
#include "app.h"
#include "IconsFontAwesome6.h"

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

App* g_App = nullptr;

void RenderFrame(GLFWwindow* window) {
    if (!g_App) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    g_App->RenderUI();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    // The clear color will be managed by the app theme now
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

void window_size_callback(GLFWwindow* window, int width, int height) {
    RenderFrame(window);
}

void window_refresh_callback(GLFWwindow* window) {
    RenderFrame(window);
}

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main(int, char**) {
#endif
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    
    float monXScale = 1.0f, monYScale = 1.0f;
    glfwGetMonitorContentScale(primary, &monXScale, &monYScale);
    
    // Scale window size by DPI, but use a smaller base size so it fits on laptop screens
    int windowWidth = (int)(850 * monXScale);
    int windowHeight = (int)(700 * monYScale);
    
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Reality Scanner", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        return -1;
    }
    
    int xpos = (mode->width - windowWidth) / 2;
    int ypos = (mode->height - windowHeight) / 2;
    glfwSetWindowPos(window, xpos, ypos);glfwSetWindowSizeLimits(window, 800, 600, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
#endif

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Disable imgui.ini generation

    // High DPI scaling
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    ImGui::GetStyle().ScaleAllSizes(xscale);
    
    // Load a sharper font for High DPI if available
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f * xscale, &font_config);
    if (font == nullptr) {
        io.FontGlobalScale = xscale; // Fallback
    } else {
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 1;
        icons_config.OversampleV = 1;
        io.Fonts->AddFontFromFileTTF("fonts/fa-solid-900.ttf", 16.0f * xscale, &icons_config, icon_ranges);
        
        // Load Bold font as the second font (AFTER FontAwesome is merged into the first)
        ImFontConfig bold_config = font_config;
        bold_config.MergeMode = false;
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 20.0f * xscale, &bold_config);
    }

    // Professional Dark Theme with Accent Colors
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(20.0f * xscale, 20.0f * xscale);
    style.FramePadding = ImVec2(10.0f * xscale, 8.0f * xscale);
    style.ItemSpacing = ImVec2(12.0f * xscale, 12.0f * xscale);
    style.WindowRounding = 8.0f * xscale;
    style.FrameRounding = 6.0f * xscale;
    style.GrabRounding = 6.0f * xscale;
    style.ScrollbarRounding = 6.0f * xscale;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    App app;
    g_App = &app;
    app.Init();

    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetWindowRefreshCallback(window, window_refresh_callback);

    // Free CPU memory of the font atlas after uploading to OpenGL
    ImGui_ImplOpenGL3_CreateDeviceObjects();
    io.Fonts->ClearTexData();

    while (!glfwWindowShouldClose(window)) {
        if (app.NeedsContinuousUpdate()) {
            glfwPollEvents(); // Run at maximum allowed FPS (VSync) for smooth animations
        } else {
            glfwWaitEvents(); // Block indefinitely until input/event to drop GPU/CPU usage to 0.0%
        }
        RenderFrame(window);
    }

    g_App = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
