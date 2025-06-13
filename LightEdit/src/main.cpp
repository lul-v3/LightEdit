// main.cpp
#include <SDL3/SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_sdl3.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "ImGui/TextEditor.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <cstdlib> // for system()
#include <map>
#include <set>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#endif

namespace fs = std::filesystem;

// Custom TextEditor extension to track filenames and dirty state
class CustomTextEditor : public TextEditor {
public:
    void SetFilePath(const std::string& path) {
        mFilePath = path;
        mIsDirty = false;
    }
    const std::string& GetFilePath() const { return mFilePath; }
    bool IsDirty() const { return mIsDirty; }
    void SetDirty(bool dirty) { mIsDirty = dirty; }

    bool Save() {
        if (mFilePath.empty()) return false;

        std::ofstream out(mFilePath);
        if (out.good()) {
            out << GetText();
            mIsDirty = false;
            return true;
        }
        return false;
    }

private:
    std::string mFilePath;
    bool mIsDirty = false;
};

// Represents a directory node in the project explorer
struct DirectoryNode {
    std::string name;
    std::string fullPath;
    std::map<std::string, DirectoryNode> subdirectories;
    std::vector<std::string> files;
};

// Application state
struct AppState {
    std::string projectPath;
    DirectoryNode projectRoot;
    std::vector<std::unique_ptr<CustomTextEditor>> editors;
    int activeEditorIndex = -1;
    std::string buildOutput;
    bool showDemoWindow = false;
};

// Function declarations
void SetupImGuiStyle();
void BuildDirectoryTree(DirectoryNode& root, const fs::path& currentPath);
void ScanProjectDirectory(AppState& state, const std::string& path);
void RenderDirectoryNode(const DirectoryNode& node, AppState& state);
void RenderProjectExplorer(AppState& state);
void RenderEditorTabs(AppState& state);
void RenderConsole(AppState& state);
void BuildProject(AppState& state);
bool SaveCurrentFile(AppState& state);
void HandleShortcuts(AppState& state);

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow("LightEdit - Lightweight Code Editor", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_StartTextInput(window);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glew_err = glewInit();
    if (glew_err != GLEW_OK) {
        printf("Error: glewInit(): %s\n", glewGetErrorString(glew_err));
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load fonts
    io.Fonts->AddFontFromFileTTF("fonts/Roboto-Medium.ttf", 16.0f);
    io.Fonts->AddFontDefault();

    // Setup custom style
    SetupImGuiStyle();

    // Application state
    AppState state;

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;

            // Handle keyboard shortcuts
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_F5) {
                    BuildProject(state);
                }
                // Ctrl+S save
                else if (event.key.key == SDLK_S &&
                    (SDL_GetModState() & SDL_KMOD_CTRL)) {
                    SaveCurrentFile(state);
                }
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Enable docking
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::Begin("MainDockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Project...")) {
                    // TODO: Implement proper file dialog
                    const char* path = "C:\\Users\\colli\\Documents\\TestProject"; // Replace with actual path
                    if (path) {
                        state.projectPath = path;
                        ScanProjectDirectory(state, path);
                    }
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    SaveCurrentFile(state);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Demo Window", nullptr, &state.showDemoWindow);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::End();

        // Render our windows
        RenderProjectExplorer(state);
        RenderEditorTabs(state);
        RenderConsole(state);

        // Demo window (for testing ImGui features)
        if (state.showDemoWindow)
            ImGui::ShowDemoWindow(&state.showDemoWindow);

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_StopTextInput(window);
    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void SetupImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    // Dark theme colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void BuildDirectoryTree(DirectoryNode& root, const fs::path& currentPath) {
    for (const auto& entry : fs::directory_iterator(currentPath)) {
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();
            DirectoryNode& newNode = root.subdirectories[dirName];
            newNode.name = dirName;
            newNode.fullPath = entry.path().string();
            BuildDirectoryTree(newNode, entry.path());
        }
        else if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".txt" || ext == ".md") {
                root.files.push_back(entry.path().string());
            }
        }
    }
}

void ScanProjectDirectory(AppState& state, const std::string& path) {
    state.projectRoot = DirectoryNode();
    state.projectRoot.name = fs::path(path).filename().string();
    state.projectRoot.fullPath = path;

    try {
        BuildDirectoryTree(state.projectRoot, path);
    }
    catch (const fs::filesystem_error& e) {
        state.buildOutput = "Error scanning directory: " + std::string(e.what()) + "\n";
    }
}

void RenderDirectoryNode(const DirectoryNode& node, AppState& state) {
    // Display directories first
    for (const auto& [name, dirNode] : node.subdirectories) {
        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow)) {
            RenderDirectoryNode(dirNode, state);
            ImGui::TreePop();
        }
    }

    // Then display files
    for (const auto& file : node.files) {
        std::string filename = fs::path(file).filename().string();

        if (ImGui::Selectable(filename.c_str())) {
            // Check if file is already open
            bool found = false;
            for (size_t i = 0; i < state.editors.size(); ++i) {
                if (state.editors[i]->GetFilePath() == file) {
                    state.activeEditorIndex = i;
                    found = true;
                    break;
                }
            }

            // If not open, create a new editor
            if (!found) {
                auto editor = std::make_unique<CustomTextEditor>();
                editor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
                editor->SetShowWhitespaces(false);

                // Load file content
                std::ifstream t(file);
                if (t.good()) {
                    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                    editor->SetText(str);
                    editor->SetFilePath(file);
                    state.editors.push_back(std::move(editor));
                    state.activeEditorIndex = state.editors.size() - 1;
                }
                else {
                    state.buildOutput = "Failed to open file: " + file + "\n";
                }
            }
        }
    }
}

void RenderProjectExplorer(AppState& state) {
    ImGui::Begin("Project Explorer");

    if (!state.projectPath.empty()) {
        ImGui::Text("Project: %s", state.projectPath.c_str());
        ImGui::Separator();

        if (ImGui::TreeNodeEx(state.projectRoot.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow)) {
            RenderDirectoryNode(state.projectRoot, state);
            ImGui::TreePop();
        }
    }
    else {
        ImGui::Text("No project loaded");
        if (ImGui::Button("Open Project")) {
            // TODO: Implement proper file dialog
            const char* path = "C:\\Users\\colli\\Documents\\TestProject"; // Replace with actual path
            if (path) {
                state.projectPath = path;
                ScanProjectDirectory(state, path);
            }
        }
    }

    ImGui::End();
}

void RenderEditorTabs(AppState& state) {
    ImGui::Begin("Editor");

    if (!state.editors.empty()) {
        // Tab bar for all open editors
        if (ImGui::BeginTabBar("EditorTabs")) {
            for (size_t i = 0; i < state.editors.size(); ++i) {
                const auto& editor = state.editors[i];
                std::string filename = fs::path(editor->GetFilePath()).filename().string();
                std::string tabName = filename + (editor->IsDirty() ? " *" : "");

                bool tabOpen = true;
                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (editor->IsDirty()) flags |= ImGuiTabItemFlags_UnsavedDocument;

                if (ImGui::BeginTabItem(tabName.c_str(), &tabOpen, flags)) {
                    state.activeEditorIndex = i;

                    // Get the available space for the editor
                    ImVec2 contentSize = ImGui::GetContentRegionAvail();

                    // Render the text editor
                    editor->Render("TextEditor", contentSize);

                    // Check for modifications
                    if (editor->IsTextChanged()) {
                        editor->SetDirty(true);
                    }

                    ImGui::EndTabItem();
                }

                if (!tabOpen) {
                    // Save before closing if dirty
                    if (editor->IsDirty()) {
                        if (editor->Save()) {
                            state.buildOutput += "Saved: " + editor->GetFilePath() + "\n";
                        }
                        else {
                            state.buildOutput += "Failed to save: " + editor->GetFilePath() + "\n";
                        }
                    }

                    state.editors.erase(state.editors.begin() + i);
                    if (state.activeEditorIndex >= static_cast<int>(state.editors.size())) {
                        state.activeEditorIndex = state.editors.empty() ? -1 : state.editors.size() - 1;
                    }
                    --i; // Adjust index after removal
                }
            }
            ImGui::EndTabBar();
        }
    }
    else {
        ImGui::Text("No files open");
    }

    ImGui::End();
}

void RenderConsole(AppState& state) {
    ImGui::Begin("Console");

    // Build button
    if (ImGui::Button("Build (F5)")) {
        BuildProject(state);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        state.buildOutput.clear();
    }

    ImGui::Separator();

    // Console output
    ImGui::BeginChild("ConsoleOutput");
    ImGui::TextUnformatted(state.buildOutput.c_str());
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

bool SaveCurrentFile(AppState& state) {
    if (state.activeEditorIndex >= 0 && state.activeEditorIndex < static_cast<int>(state.editors.size())) {
        auto& editor = state.editors[state.activeEditorIndex];
        if (editor->Save()) {
            state.buildOutput += "Saved: " + editor->GetFilePath() + "\n";
            return true;
        }
        else {
            state.buildOutput += "Failed to save: " + editor->GetFilePath() + "\n";
            return false;
        }
    }
    return false;
}

void BuildProject(AppState& state) {
    if (state.projectPath.empty()) {
        state.buildOutput = "No project loaded\n";
        return;
    }

    // Save all open files first
    for (auto& editor : state.editors) {
        if (editor->IsDirty()) {
            if (editor->Save()) {
                state.buildOutput += "Saved: " + editor->GetFilePath() + "\n";
            }
            else {
                state.buildOutput += "Failed to save: " + editor->GetFilePath() + "\n";
            }
        }
    }

    // Execute build command
    state.buildOutput += "Building project...\n";

    // Create build directory if it doesn't exist
    std::string buildDir = state.projectPath + "/build";
    if (!fs::exists(buildDir)) {
        fs::create_directory(buildDir);
    }

    // Execute CMake commands
    std::string cmd = "cd " + buildDir + " && cmake .. && cmake --build .";

    // Execute command and capture output
    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        state.buildOutput += "Failed to execute build command\n";
        return;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    int returnCode = pclose(pipe);

    state.buildOutput += result;
    state.buildOutput += returnCode == 0 ? "Build succeeded!\n" : "Build failed!\n";
}