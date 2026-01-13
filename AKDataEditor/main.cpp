// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <Imgui/imgui.h>
#include <Imgui/imgui_impl_win32.h>
#include <Imgui/imgui_impl_gdi.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <stdio.h>
#include <commdlg.h>
#include <ShlObj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

#include "EnemyEditor.h"
#include "OperatorEditor.h"
#include "LevelEditor.h"
#include "SkillEditor.h"
#include "Utility.h"

static char solutionPath[512] = "";
static bool pathInitialized = false;
static bool showUnsavedWarning = false;

// Forward declarations of helper functions
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::string BrowseForFolder()
{
    char path[MAX_PATH] = "";

    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = "Select Arknights Solution Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

    if (pidl != 0)
    {
        SHGetPathFromIDListA(pidl, path);

        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc)))
        {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return std::string(path);
    }
    return "";
}

// 경로 변경 함수
void ChangeSolutionPath(const std::string& newPath, EnemyEditor*& enemyEditor, OperatorEditor*& operatorEditor, SkillEditor*& skillEditor, LevelEditor*& levelEditor)
{
    strcpy_s(solutionPath, newPath.c_str());
    pathInitialized = true;

    std::string enemyPath = std::string(solutionPath) + "/gamedata/tables/enemies_table.json";
    std::string operatorPath = std::string(solutionPath) + "/gamedata/tables/operators_table.json";
    std::string skillPath = std::string(solutionPath) + "/gamedata/tables/skills_table.json";
    std::string levelPath = std::string(solutionPath) + "/gamedata/levels/";

    delete enemyEditor;
    delete operatorEditor;
    delete levelEditor;

    enemyEditor = new EnemyEditor(enemyPath);
    operatorEditor = new OperatorEditor(operatorPath);
    skillEditor = new SkillEditor(skillPath, operatorPath);
    levelEditor = new LevelEditor(levelPath, std::string(solutionPath));

    std::cout << "Path set to: " << solutionPath << "\n";
}

// Ctrl+S 단축키 처리 함수
void HandleCtrlS(EnemyEditor* enemyEditor, OperatorEditor* operatorEditor, SkillEditor* skillEditor, LevelEditor* levelEditor)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
    {
        bool saved = false;

        if (enemyEditor->HasUnsavedChanges())
        {
            enemyEditor->SaveEnemies();
            enemyEditor->ClearUnsavedFlag();
            saved = true;
        }

        if (operatorEditor->HasUnsavedChanges())
        {
            operatorEditor->SaveOperators();
            operatorEditor->ClearUnsavedFlag();
            saved = true;
        }

        if (skillEditor->HasUnsavedChanges())
        {
            skillEditor->SaveSkills();
            skillEditor->ClearUnsavedFlag();
            saved = true;
        }

        if (levelEditor->HasUnsavedChanges())
        {
            levelEditor->SaveAllLevels();
            levelEditor->ClearUnsavedFlag();
            saved = true;
        }

        if (saved)
            std::cout << "[Shortcut] Saved with Ctrl+S!\n";
    }
}

// 메인 UI 렌더링 함수
void RenderMainUI(EnemyEditor*& enemyEditor, OperatorEditor*& operatorEditor, SkillEditor*& skillEditor, LevelEditor*& levelEditor,
    bool& showEnemyEditor, bool& showOperatorEditor, bool& showSkillEditor, bool& showLevelEditor)
{
    ImGui::Begin("Data Editor");

    // === 경로 설정 섹션 ===
    ImGui::SeparatorText("솔루션 경로");

    ImGui::InputText("##SolutionPath", solutionPath, 512);
    ImGui::SameLine();

    if (ImGui::Button("찾아보기..."))
    {
        bool hasUnsaved = enemyEditor->HasUnsavedChanges() || operatorEditor->HasUnsavedChanges() || skillEditor->HasUnsavedChanges() || levelEditor->HasUnsavedChanges();

        if (hasUnsaved)
        {
            showUnsavedWarning = true;
        }
        else
        {
            std::string selected = BrowseForFolder();
            if (!selected.empty())
            {
                ChangeSolutionPath(selected, enemyEditor, operatorEditor, skillEditor, levelEditor);
            }
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("경로 저장"))
    {
        std::ofstream configFile("config.ini");
        if (configFile.is_open())
        {
            configFile << solutionPath;
            configFile.close();
            std::cout << "[Config] Path saved to config.ini\n";
        }
    }

    // 경로 상태 표시
    if (strlen(solutionPath) > 0)
    {
        ImGui::TextColored(COLOR_GREEN, "경로 설정됨");
    }
    else
    {
        ImGui::TextColored(COLOR_RED, "솔루션 경로를 설정하세요!");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === 에디터 버튼들 ===
    ImGui::SeparatorText("편집기");

    bool hasPath = strlen(solutionPath) > 0;

    if (!hasPath) ImGui::BeginDisabled();

    if (ImGui::Button("적 편집기"))
        showEnemyEditor = true;
    if (ImGui::Button("오퍼레이터 편집기"))
        showOperatorEditor = true;
    if (ImGui::Button("스킬 편집기"))
        showSkillEditor = true;
    if (ImGui::Button("레벨 편집기"))
        showLevelEditor = true;

    if (!hasPath) ImGui::EndDisabled();

    ImGui::End();
}

// Unsaved 경고 팝업 렌더링 함수
void RenderUnsavedWarningPopup(EnemyEditor*& enemyEditor, OperatorEditor*& operatorEditor, SkillEditor*& skillEditor, LevelEditor*& levelEditor)
{
    if (showUnsavedWarning)
    {
        ImGui::OpenPopup("경고: 저장되지 않은 변경사항");
        showUnsavedWarning = false;
    }

    if (ImGui::BeginPopupModal("경고: 저장되지 않은 변경사항", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("저장되지 않은 변경사항이 있습니다!");
        ImGui::Text("경로를 바꾸면 변경사항이 사라집니다.");
        ImGui::Separator();

        if (ImGui::Button("계속", ImVec2(120, 0)))
        {
            std::string selected = BrowseForFolder();
            if (!selected.empty())
            {
                ChangeSolutionPath(selected, enemyEditor, operatorEditor, skillEditor, levelEditor);
            }
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 설정 파일에서 경로 로드
void LoadConfig()
{
    std::ifstream configFile("config.ini");
    if (configFile.is_open())
    {
        configFile.getline(solutionPath, sizeof(solutionPath));
        configFile.close();
        pathInitialized = true;
        std::cout << "[Config] Loaded path from config.ini: " << solutionPath << "\n";
    }
}

// Main code
int main(int, char**)
{
    // Create application window
    std::string title = "AK Data Editor v" + std::string(VERSION);
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd;

#ifdef UNICODE
    std::wstring wtitle(title.begin(), title.end());
    hwnd = ::CreateWindow(wc.lpszClassName, wtitle.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
#else
    hwnd = ::CreateWindow(wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);
#endif

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

#ifdef _DEBUG
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "../fonts/malgun.ttf",
        18.0f,
        NULL,
        io.Fonts->GetGlyphRangesKorean()
    );
#else
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "fonts/malgun.ttf",
        18.0f,
        NULL,
        io.Fonts->GetGlyphRangesKorean()
    );
#endif

    if (font == NULL)
    {
        std::cout << "Failed to load Korean font!\n";
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplGDI_Init();

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 설정 로드
    LoadConfig();

    // 에디터 초기화
    std::string enemyPath = std::string(solutionPath) + "/gamedata/tables/enemies_table.json";
    std::string operatorPath = std::string(solutionPath) + "/gamedata/tables/operators_table.json";
    std::string skillPath = std::string(solutionPath) + "/gamedata/tables/skills_table.json";
    std::string levelPath = std::string(solutionPath) + "/gamedata/levels/";

    EnemyEditor* enemyEditor = new EnemyEditor(enemyPath);
    OperatorEditor* operatorEditor = new OperatorEditor(operatorPath);
    SkillEditor* skillEditor = new SkillEditor(skillPath, operatorPath);
    LevelEditor* levelEditor = new LevelEditor(levelPath, std::string(solutionPath));

    bool showEnemyEditor = false;
    bool showOperatorEditor = false;
    bool showSkillEditor = false;
    bool showLevelEditor = false;

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplGDI_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Ctrl+S 처리
        HandleCtrlS(enemyEditor, operatorEditor, skillEditor, levelEditor);

        // 메인 UI
        RenderMainUI(enemyEditor, operatorEditor, skillEditor, levelEditor, showEnemyEditor, showOperatorEditor, showSkillEditor, showLevelEditor);

        // 경고 팝업
        RenderUnsavedWarningPopup(enemyEditor, operatorEditor, skillEditor, levelEditor);

        // 에디터 윈도우들
        if (showEnemyEditor)
            enemyEditor->RenderGUI(&showEnemyEditor);

        if (showOperatorEditor)
            operatorEditor->RenderGUI(&showOperatorEditor);

        if (showSkillEditor)
            skillEditor->RenderGUI(&showSkillEditor);

        if (showLevelEditor)
            levelEditor->RenderGUI(&showLevelEditor);

        // Rendering
        ImGui::Render();
        ImGui_ImplGDI_SetBackgroundColor(&clear_color);
        ImGui_ImplGDI_RenderDrawData(ImGui::GetDrawData());
    }

    // Cleanup
    delete enemyEditor;
    delete operatorEditor;

    ImGui_ImplGDI_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
