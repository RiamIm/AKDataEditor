#include "OperatorEditor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui/imgui.h>

#include "Utility.h"

namespace fs = std::filesystem;

OperatorEditor::OperatorEditor(const std::string& jsonPath)
    : _jsonPath(jsonPath)
{
    LoadOperators();
}

OperatorEditor::~OperatorEditor() {}

void OperatorEditor::RenderGUI(bool* p_open)
{
    ImGui::Begin("오퍼레이터 편집기", p_open);

    RenderToolbar();
    ImGui::Separator();

    RenderOperatorList();

    ImGui::End();

    if (_showCreateWindow)
        RenderCreateWindow();

    if (_showEditWindow)
        RenderEditWindow();

    if (_showRangeEditor)
        RenderRangeGridEditor();

    if (_showSkillEditor)
        RenderSkillEditor();
}

void OperatorEditor::LoadOperators()
{
    std::ifstream file(_jsonPath);
    if (file.is_open())
    {
        try
        {
            file >> _operatorData;
            std::cout << "Loaded " << _jsonPath << "\n";

            // ===== 버전 체크 및 마이그레이션 =====
            int currentVersion = VERSION;  // 현재 에디터 버전
            int dataVersion = 0;      // JSON 데이터 버전

            if (_operatorData.contains("version"))
            {
                dataVersion = _operatorData["version"];
            }

            // 버전이 낮을 때만 마이그레이션 실행
            if (dataVersion < currentVersion)
            {
                std::cout << "[Migration] Upgrading data from v" << dataVersion
                    << " to v" << currentVersion << "\n";

                bool migrated = false;

                // 마이그레이션 로직
                for (auto& op : _operatorData["operators"])
                {
                    if (op.contains("phases") && !op["phases"].empty())
                    {
                        auto& attrs = op["phases"][0]["attributesKeyFrames"][0]["data"];

                        // baseAttackTime 반올림
                        if (attrs.contains("baseAttackTime"))
                        {
                            float oldValue = attrs["baseAttackTime"];
                            float newValue = Snap2(static_cast<double>(oldValue));
                            if (std::abs(oldValue - newValue) > 0.001f)
                            {
                                attrs["baseAttackTime"] = newValue;
                                migrated = true;
                            }
                        }

                        // magicResistance 반올림
                        if (attrs.contains("magicResistance"))
                        {
                            float oldValue = attrs["magicResistance"];
                            float newValue = Snap2(static_cast<double>(oldValue));
                            if (std::abs(oldValue - newValue) > 0.001f)
                            {
                                attrs["magicResistance"] = newValue;
                                migrated = true;
                            }
                        }
                    }

                    // 스킬 duration 반올림
                    if (op.contains("skills"))
                    {
                        for (auto& skill : op["skills"])
                        {
                            if (skill.contains("duration"))
                            {
                                float oldValue = skill["duration"];
                                float newValue = Snap2(static_cast<double>(oldValue));
                                if (std::abs(oldValue - newValue) > 0.001f)
                                {
                                    skill["duration"] = newValue;
                                    migrated = true;
                                }
                            }
                        }
                    }
                }

                if (migrated)
                {
                    // 버전 업데이트
                    _operatorData["version"] = currentVersion;

                    std::cout << "[Migration] Auto-saving fixed values...\n";
                    SaveOperators();
                    std::cout << "[Migration] Complete!\n";
                }
                else
                {
                    // 변경사항 없어도 버전만 업데이트
                    _operatorData["version"] = currentVersion;
                    SaveOperators();
                }
            }
        }
        catch (const json::exception& e)
        {
            std::cout << "JSON parse error: " << e.what() << "\n";
            _operatorData = { {"version", VERSION}, {"operators", json::array()} };
        }
    }
    else
    {
        std::cout << "File not found, creating new: " << _jsonPath << "\n";
        _operatorData = { {"version", VERSION}, {"operators", json::array()} };
    }
}

void OperatorEditor::SaveOperators()
{
    std::filesystem::path filePath(_jsonPath);
    std::filesystem::create_directories(filePath.parent_path());

    if (!_operatorData.contains("version"))
    {
        _operatorData["version"] = 1;
    }

    // 순서 보장
    json output;
    output["version"] = _operatorData["version"];
    output["operators"] = _operatorData["operators"];

    std::ofstream file(_jsonPath);
    if (file.is_open())
    {
        file << output.dump(2);
        std::cout << "Saved to " << _jsonPath << "\n";
    }
    else
    {
        std::cout << "Failed to save!\n";
    }
}

void OperatorEditor::RenderToolbar()
{
    if (_hasUnsavedChanges)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_YELLOW);
        ImGui::Text("* 저장되지 않은 변경사항");
        ImGui::PopStyleColor();

        ImGui::SameLine();

        if (ImGui::Button("전체 저장"))
        {
            SaveOperators();
            _hasUnsavedChanges = false;
            std::cout << "[Operator] Saved successfully!\n";
        }

        ImGui::SameLine();

        if (ImGui::Button("되돌리기"))
        {
            LoadOperators();
            _hasUnsavedChanges = false;
            std::cout << "[Operator] Changes discarded.\n";
        }
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GREEN);
        ImGui::Text("저장 완료");
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::Button("새로운 오퍼레이터 생성"))
    {
        _showCreateWindow = true;

        // 멤버 변수 초기화
        memset(_inputCharId, 0, sizeof(_inputCharId));
        memset(_inputName, 0, sizeof(_inputName));
        _inputProfession = Profession::PROF_CASTER;
        _inputRarity = 3;
        _inputPosition = Position::POS_RANGED;
        _inputHp = 1000;
        _inputAtk = 300;
        _inputDef = 50;
        _inputMagicRes = 0;
        _inputCost = 10;
        _inputBlockCnt = 1;
        _inputBaseAttackTime = 1.5f;
        _inputRespawnTime = 70;

        // Range 초기화
        memset(_rangeGrid, 0, sizeof(_rangeGrid));
        _rangeGrid[CENTER][CENTER] = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("새로고침"))
    {
        LoadOperators();
        _hasUnsavedChanges = false;
    }
}

void OperatorEditor::RenderOperatorList()
{
    if (!_operatorData.contains("operators") || _operatorData["operators"].empty())
    {
        ImGui::TextColored(COLOR_GRAY, "오퍼레이터 데이터가 없습니다.");
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("OperatorTable", 7, flags))
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Class", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("HP/ATK", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Cost", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableHeadersRow();

        int index = 0;
        for (const auto& op : _operatorData["operators"])
        {
            ImGui::TableNextRow();

            ImU32 bg = (index % 2 == 0)
                ? IM_COL32(25, 25, 25, 255)
                : IM_COL32(40, 40, 40, 255);

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

            // ID
            ImGui::TableNextColumn();
            std::string id = op["charId"];
            ImGui::Text("%s", id.c_str());

            // Name
            ImGui::TableNextColumn();
            std::string name = op["name"];
            ImGui::Text("%s", name.c_str());

            // Profession
            ImGui::TableNextColumn();
            std::string profession = op["profession"];

            ImVec4 profColor;
            if (profession == "CASTER") profColor = COLOR_CASTER;
            else if (profession == "SNIPER") profColor = COLOR_SNIPER;
            else if (profession == "GUARD") profColor = COLOR_GUARD;
            else if (profession == "DEFENDER") profColor = COLOR_DEFENDER;
            else if (profession == "MEDIC") profColor = COLOR_MEDIC;
            else if (profession == "VANGUARD") profColor = COLOR_VANGUARD;
            else if (profession == "SUPPORTER") profColor = COLOR_SUPPORTER;
            else if (profession == "SPECIALIST") profColor = COLOR_SPECIALIST;
            else profColor = COLOR_DEFAULT;

            ImGui::TextColored(profColor, "%s", profession.c_str());

            // Rarity
            ImGui::TableNextColumn();
            int rarity = op["rarity"];
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "%d", rarity);

            // HP/ATK
            ImGui::TableNextColumn();
            int hp = op["phases"][0]["attributesKeyFrames"][0]["data"]["maxHp"];
            int atk = op["phases"][0]["attributesKeyFrames"][0]["data"]["atk"];
            ImGui::Text("%d/%d", hp, atk);

            // Cost
            ImGui::TableNextColumn();
            int cost = op["phases"][0]["attributesKeyFrames"][0]["data"]["cost"];
            ImGui::Text("%d", cost);

            // Actions
            ImGui::TableNextColumn();

            ImGui::PushID(index);

            if (ImGui::SmallButton("편집"))
            {
                _selectedOperatorIndex = index;
                _showEditWindow = true;

                // 백버퍼 로드
                auto& attrs = op["phases"][0]["attributesKeyFrames"][0]["data"];

                std::string charId = op["charId"];
                std::string opName = op["name"];
                std::string profession = op["profession"];
                std::string position = op["position"];

                strcpy_s(_inputCharId, sizeof(_inputCharId), charId.c_str());
                strcpy_s(_inputName, sizeof(_inputName), opName.c_str());

                _inputProfession = StringToProfession(profession);
                _inputPosition = StringToPosition(position);
                _inputRarity = op["rarity"];
                _inputHp = attrs["maxHp"];
                _inputAtk = attrs["atk"];
                _inputDef = attrs["def"];
                _inputMagicRes = attrs["magicResistance"];
                _inputCost = attrs["cost"];
                _inputBlockCnt = attrs["blockCnt"];
                _inputBaseAttackTime = attrs["baseAttackTime"];
                _inputRespawnTime = attrs["respawnTime"];

                if (op.contains("range"))
                {
                    RangeJsonToGrid(op["range"]);
                }
                else
                {
                    memset(_rangeGrid, 0, sizeof(_rangeGrid));
                    _rangeGrid[CENTER][CENTER] = true;
                }
            }

            ImGui::SameLine();

            if (ImGui::SmallButton("삭제"))
            {
                _deleteTargetIndex = index;
                _deleteTargetName = name;
                _showDeleteConfirm = true;
            }

            ImGui::PopID();

            ++index;
        }
        ImGui::EndTable();
    }

    // Delete Confirm Popup
    if (_showDeleteConfirm)
    {
        ImGui::OpenPopup("삭제 확인");
        _showDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("'%s'을(를) 삭제할까요?", _deleteTargetName.c_str());
        ImGui::Text("이 작업은 되돌릴 수 없습니다.");
        ImGui::Separator();

        if (ImGui::Button("예", ImVec2(120, 0)))
        {
            if (_deleteTargetIndex >= 0 && _deleteTargetIndex < (int)_operatorData["operators"].size())
            {
                _operatorData["operators"].erase(_operatorData["operators"].begin() + _deleteTargetIndex);
                _hasUnsavedChanges = true;
                std::cout << "[Operator] Deleted: " << _deleteTargetName << "\n";
            }
            _deleteTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("아니오", ImVec2(120, 0)))
        {
            _deleteTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void OperatorEditor::RenderCreateWindow()
{
    ImGui::Begin("새 오퍼레이터 생성", &_showCreateWindow);

    ImGui::SeparatorText("기본 정보");
    ImGui::InputText("오퍼레이터 ID", _inputCharId, 64);
    ImGui::InputText("이름", _inputName, 64);

    ImGui::Spacing();

    // Profession Combo
    const char* professions[] = {
        "캐스터", "스나이퍼", "가드", "디펜더",
        "메딕", "뱅가드", "서포터", "스페셜리스트"
    };

    if (ImGui::Combo("포지션", (int*)&_inputProfession, professions, IM_ARRAYSIZE(professions)))
    {
        _inputPosition = GetPositionFromProfession(_inputProfession);
    }

    ImGui::Text("배치: %s (자동)", PositionToString(_inputPosition));

    ImGui::Spacing();

    ImGui::SliderInt("레어도", &_inputRarity, 3, 6);

    ImGui::SeparatorText("능력치");

    ImGui::InputInt("최대 HP", &_inputHp);
    ImGui::InputInt("공격력", &_inputAtk);
    ImGui::InputInt("방어력", &_inputDef);
    ImGui::SliderInt("마법 저항", &_inputMagicRes, 0, 100);
    ImGui::SameLine();
    ImGui::Text("%%");
    ImGui::InputInt("배치 코스트", &_inputCost);
    ImGui::InputInt("저지 가능 수", &_inputBlockCnt);
    ImGui::InputFloat("공격 속도 (초)", &_inputBaseAttackTime, 0.05f, 1.0f, "%.2f");
    ImGui::InputInt("재배치 시간", &_inputRespawnTime);

    ImGui::Spacing();
    ImGui::Separator();

    // Range
    ImGui::Text("공격 범위");
    ImGui::SameLine();
    if (ImGui::SmallButton("범위 선택..."))
    {
        _showRangeEditor = true;
    }

    int rangeCount = 0;
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (_rangeGrid[r][c])
                rangeCount++;

    ImGui::SameLine();
    ImGui::Text("(%d 칸)", rangeCount);

    ImGui::Spacing();

    ImGui::Separator();
    ImGui::Text("스킬");
    ImGui::Spacing();

    if (ImGui::Button("스킬 추가"))
    {
        _showSkillEditor = true;
        _isEditingSkill = false;
        _selectedSkillIndex = -1;
        ClearSkillInputBuffers();
    }

    ImGui::Spacing();

    RenderTempSkillList();

    ImGui::Separator();
    ImGui::Spacing();

    // Buttons
    if (ImGui::Button("생성", ImVec2(120, 0)))
    {
        if (strlen(_inputCharId) == 0 || strlen(_inputName) == 0)
        {
            std::cout << "[Error] ID and Name are required!\n";
        }
        else
        {
            json newOperator = OperatorDataStructure(
                _inputCharId, _inputName,
                ProfessionToString(_inputProfession),
                _inputRarity,
                PositionToString(_inputPosition),
                _inputHp, _inputAtk, _inputDef, _inputMagicRes,
                _inputCost, _inputBlockCnt,
                _inputBaseAttackTime, _inputRespawnTime,
                GridToRangeJson()
            );

            if (!_tempSkills.empty())
            {
                newOperator["skills"] = _tempSkills;
            }

            _operatorData["operators"].push_back(newOperator);
            _hasUnsavedChanges = true;

            std::cout << "[Operator] Created: " << _inputName << "\n";
            _showCreateWindow = false;
            _tempSkills.clear();
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("취소", ImVec2(120, 0)))
    {
        _showCreateWindow = false;
        _tempSkills.clear();
    }

    ImGui::End();
}

void OperatorEditor::RenderEditWindow()
{
    if (_selectedOperatorIndex < 0 || _selectedOperatorIndex >= (int)_operatorData["operators"].size())
    {
        _showEditWindow = false;
        return;
    }

    ImGui::Begin("오퍼레이터 편집", &_showEditWindow);

    auto& op = _operatorData["operators"][_selectedOperatorIndex];
    auto& attrs = op["phases"][0]["attributesKeyFrames"][0]["data"];

    // ID (읽기 전용)
    ImGui::Text("ID: %s", op["charId"].get<std::string>().c_str());
    ImGui::Separator();

    // Name
    char nameBuffer[64];
    std::string name = op["name"];
    strcpy_s(nameBuffer, name.c_str());

    if (ImGui::InputText("이름", nameBuffer, 64))
    {
        op["name"] = std::string(nameBuffer);
        _hasUnsavedChanges = true;
    }

    // Profession
    const char* professions[] = {
        "캐스터", "스나이퍼", "가드", "디펜더",
        "메딕", "뱅가드", "서포터", "스페셜리스트"
    };

    std::string profStr = op["profession"];
    int profIndex = static_cast<int>(StringToProfession(profStr));

    if (ImGui::Combo("포지션", &profIndex, professions, IM_ARRAYSIZE(professions)))
    {
        Profession newProf = static_cast<Profession>(profIndex);
        op["profession"] = ProfessionToString(newProf);
        op["position"] = PositionToString(GetPositionFromProfession(newProf));
        _hasUnsavedChanges = true;
    }

    // Position (Auto)
    ImGui::Text("배치: %s (자동)", op["position"].get<std::string>().c_str());

    // Rarity
    int rarity = op["rarity"];
    if (ImGui::SliderInt("레어도", &rarity, 3, 6))
    {
        op["rarity"] = rarity;
        _hasUnsavedChanges = true;
    }

    ImGui::SeparatorText("능력치");

    // Stats
    int hp = attrs["maxHp"];
    if (ImGui::InputInt("최대 HP", &hp))
    {
        attrs["maxHp"] = hp;
        _hasUnsavedChanges = true;
    }

    int atk = attrs["atk"];
    if (ImGui::InputInt("공격력", &atk))
    {
        attrs["atk"] = atk;
        _hasUnsavedChanges = true;
    }

    int def = attrs["def"];
    if (ImGui::InputInt("방어력", &def))
    {
        attrs["def"] = def;
        _hasUnsavedChanges = true;
    }

    int magicResInt = static_cast<int>(attrs["magicResistance"].get<float>() * 100);
    if (ImGui::SliderInt("마법 저항", &magicResInt, 0, 100))
    {
        attrs["magicResistance"] = Snap2(magicResInt / 100.0);
        _hasUnsavedChanges = true;
    }
    ImGui::SameLine();
    ImGui::Text("%%");

    int cost = attrs["cost"];
    if (ImGui::InputInt("배치 코스트", &cost))
    {
        attrs["cost"] = cost;
        _hasUnsavedChanges = true;
    }

    int blockCnt = attrs["blockCnt"];
    if (ImGui::InputInt("저지 가능 수", &blockCnt))
    {
        attrs["blockCnt"] = blockCnt;
        _hasUnsavedChanges = true;
    }

    float baseAttackTime = static_cast<float>(attrs["baseAttackTime"].get<double>());
    if (ImGui::InputFloat("공격 속도 (초)", &baseAttackTime, 0.1f, 1.0f, "%.2f"))
    {
        attrs["baseAttackTime"] = Snap2(baseAttackTime);
        _hasUnsavedChanges = true;
    }

    int respawnTime = attrs["respawnTime"];
    if (ImGui::InputInt("재배치 시간", &respawnTime))
    {
        attrs["respawnTime"] = respawnTime;
        _hasUnsavedChanges = true;
    }

    ImGui::Separator();

    // Range Edit
    ImGui::Text("공격 범위");
    ImGui::SameLine();
    if (ImGui::SmallButton("범위 편집..."))
    {
        if (op.contains("range"))
        {
            RangeJsonToGrid(op["range"]);
        }
        _showRangeEditor = true;
    }

    ImGui::Separator();

    ImGui::Text("스킬");
    ImGui::Spacing();

    RenderSkillList();

    ImGui::Spacing();

    if (ImGui::Button("새로운 스킬 추가", ImVec2(150, 0)))
    {
        ClearSkillInputBuffers();

        _isEditingSkill = false;
        _showSkillEditor = true;
    }

    ImGui::Separator();

    if (ImGui::Button("완료"))
    {
        // Range 저장
        op["range"] = GridToRangeJson();
        _showEditWindow = false;
    }

    ImGui::End();
}

void OperatorEditor::RenderRangeGridEditor()
{
    ImGui::SetNextWindowSize(ImVec2(500, 550), ImGuiCond_Appearing);

    if (ImGui::Begin("범위 그리드 편집", &_showRangeEditor))
    {
        ImGui::Text("칸을 클릭해서 공격 범위를 설정하세요");
        ImGui::TextColored(COLOR_BLUE, "파랑 = 오퍼레이터");
        ImGui::TextColored(COLOR_YELLOW, "노랑 = 공격 범위");

        ImGui::Separator();
        ImGui::Spacing();

        const float cellSize = 25.0f;
        const float spacing = 2.0f;

        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                if (col > 0)
                    ImGui::SameLine(0, spacing);

                ImGui::PushID(row * GRID_SIZE + col);

                ImVec4 cellColor;

                if (row == CENTER && col == CENTER)
                {
                    cellColor = COLOR_BLUE;
                }
                else if (_rangeGrid[row][col])
                {
                    cellColor = COLOR_YELLOW;
                }
                else
                {
                    cellColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, cellColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                    ImVec4(cellColor.x * 1.2f, cellColor.y * 1.2f, cellColor.z * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                    ImVec4(cellColor.x * 0.8f, cellColor.y * 0.8f, cellColor.z * 0.8f, 1.0f));

                if (ImGui::Button("##cell", ImVec2(cellSize, cellSize)))
                {
                    if (!(row == CENTER && col == CENTER))
                    {
                        _rangeGrid[row][col] = !_rangeGrid[row][col];
                    }
                }

                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("전체 해제", ImVec2(120, 0)))
        {
            memset(_rangeGrid, 0, sizeof(_rangeGrid));
            _rangeGrid[CENTER][CENTER] = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("적용", ImVec2(120, 0)))
        {
            _showRangeEditor = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(120, 0)))
        {
            _showRangeEditor = false;
        }
    }

    ImGui::End();
}

void OperatorEditor::RenderSkillList()
{
    auto& op = _operatorData["operators"][_selectedOperatorIndex];

    if (!op.contains("skills") || op["skills"].empty())
    {
        ImGui::TextColored(COLOR_GRAY, "추가된 스킬이 없습니다.");
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg;

    if (ImGui::BeginTable("Skill Table", 5, flags, ImVec2(0, 150)))
    {
        ImGui::TableSetupColumn("Skill ID", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("SP", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        int index = 0;
        for (auto& skill : op["skills"])
        {
            ImGui::TableNextRow();

            ImU32 bg = (index % 2 == 0)
                ? IM_COL32(25, 25, 25, 255)
                : IM_COL32(40, 40, 40, 255);

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

            // skill id
            ImGui::TableNextColumn();
            std::string skillId = skill["skillId"];
            ImGui::Text("%s", skillId.c_str());

            // name
            ImGui::TableNextColumn();
            std::string name = skill["name"];
            ImGui::Text("%s", name.c_str());

            ImGui::TableNextColumn();
            int skillType = skill["skillType"];
            const char* typeStr = (skillType == 0) ? "패시브" :
                (skillType == 1) ? "수동" : "자동";
            ImGui::Text("%s", typeStr);

            // sp
            ImGui::TableNextColumn();
            int spCost = skill["spData"]["spCost"];
            ImGui::Text("%d", spCost);

            // action
            ImGui::TableNextColumn();

            ImGui::PushID(index);

            if (ImGui::SmallButton("편집"))
            {
                _selectedSkillIndex = index;
                _isEditingSkill = true;

                LoadSkillToBuffer(skill); // 입력 버퍼 로드

                _showSkillEditor = true;
            }

            ImGui::SameLine();

            if (ImGui::SmallButton("삭제"))
            {
                _deleteSkillTargetIndex = index;
                _deleteSkillTargetName = name;
                _showSkillDeleteConfirm = true;
            }

            ImGui::PopID();

            ++index;
        }

        ImGui::EndTable();
    }

    if (_showSkillDeleteConfirm)
    {
        ImGui::OpenPopup("삭제 확인");
        _showSkillDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("'%s을(를) 삭제할까요?", _deleteSkillTargetName.c_str());
        ImGui::Text("이 작업은 되돌릴 수 없습니다.");
        ImGui::Separator();

        if (ImGui::Button("예", ImVec2(120, 0)))
        {
            if (_deleteSkillTargetIndex >= 0 && _deleteSkillTargetIndex < (int)op["skills"].size())
            {
                op["skills"].erase(op["skills"].begin() + _deleteSkillTargetIndex);
                _hasUnsavedChanges = true;
                std::cout << "[Operator] Deleted: " << _deleteSkillTargetName << '\n';
            }
            _deleteSkillTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("아니오", ImVec2(120, 0)))
        {
            _deleteSkillTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void OperatorEditor::RenderSkillEditor()
{
    ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_Appearing);

    const char* title = _isEditingSkill ? "스킬 편집" : "새로운 스킬 추가";

    if (ImGui::Begin(title, &_showSkillEditor))
    {
        ImGui::SeparatorText("기본 정보");

        ImGui::InputText("스킬 ID", _inputSkillId, 64);
        ImGui::InputText("이름", _inputSkillName, 64);
        ImGui::Text("설명");
        ImGui::InputTextMultiline("##Description", _inputSkillDesc, 256, ImVec2(-1, 60));

        ImGui::Spacing();
        ImGui::SeparatorText("스킬 타입");

        const char* skillTypes[] = { "패시브", "수동", "자동" };
        ImGui::Combo("발동 조건", &_inputSkillType, skillTypes, IM_ARRAYSIZE(skillTypes));

        ImGui::Spacing();
        ImGui::SeparatorText("SP 데이터");

        const char* spTypes[] = { "공격 시", "초당 회복", "피격 시" };
        int spTypeIndex = (_inputSpType == 1) ? 0 :
            (_inputSpType == 2) ? 1 : 2;
        if (ImGui::Combo("SP 회복", &spTypeIndex, spTypes, IM_ARRAYSIZE(spTypes)))
        {
            _inputSpType = (spTypeIndex == 0) ? 1 :
                (spTypeIndex == 1) ? 2 : 4;
        }

        ImGui::InputInt("SP 비용", &_inputSpCost);
        ImGui::InputInt("초기 SP", &_inputInitSp);
        ImGui::InputFloat("지속시간 (초)", &_inputDuration, 1.0f, 5.0f, "%.1f");

        ImGui::Spacing();
        ImGui::SeparatorText("효과");

        const char* effectTypes[] = { "공격", "방어", "공격 속도", "DP회복", "힐" };
        ImGui::Combo("효과 타입", &_inputEffectType, effectTypes, IM_ARRAYSIZE(effectTypes));

        if (_inputEffectType == 0 || _inputEffectType == 1) // atk, def
        {
            ImGui::InputFloat("배율", &_inputEffectValue, 0.1f, 0.5f, "%.2f");
            ImGui::Text("(e.g., 0.4 = +40%)");
        }
        else if (_inputEffectType == 3) // cost
        {
            if (_inputDuration > 0)
            {
                ImGui::InputFloat("초당 DP", &_inputEffectValue, 0.1f, 0.5f, "%.1f");
            }
            else
            {
                ImGui::InputFloat("값", &_inputEffectValue, 1.0f, 10.0f, "%.0f");
            }
        }
        else
        {
            ImGui::InputFloat("값", &_inputEffectValue, 1.0f, 10.0f, "%.0f");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("저장", ImVec2(120, 0)))
        {
            json skillData = CreateSkillData();

            if (_showCreateWindow)
            {
                if (_isEditingSkill && _selectedSkillIndex >= 0)
                {
                    _tempSkills[_selectedSkillIndex] = skillData;
                }
                else
                {
                    _tempSkills.push_back(skillData);
                }
            }
            else if (_showEditWindow)
            {
                auto& op = _operatorData["operators"][_selectedOperatorIndex];

                if (!op.contains("skills"))
                {
                    op["skills"] = json::array();
                }

                if (_isEditingSkill && _selectedSkillIndex >= 0)
                {
                    op["skills"][_selectedSkillIndex] = skillData;
                }
                else
                {
                    op["skills"].push_back(skillData);
                }

                _hasUnsavedChanges = true;
            }

            _showSkillEditor = false;
            ClearSkillInputBuffers();
        }

        ImGui::SameLine();

        if (ImGui::Button("완료", ImVec2(120, 0)))
        {
            _showSkillEditor = false;
        }
    }

    ImGui::End();
}

void OperatorEditor::RenderTempSkillList()
{
    if (_tempSkills.empty())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "추가된 스킬이 없습니다.");
        return;
    }

    ImGui::Text("추가된 스킬: %d", (int)_tempSkills.size());

    ImGuiTableFlags flags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("Temp Skill Table", 5, flags, ImVec2(0, 150)))
    {
        ImGui::TableSetupColumn("Skill ID", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("SP", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        int index = 0;
        for (auto& skill : _tempSkills)
        {
            ImGui::TableNextRow();

            ImU32 bg = (index % 2 == 0)
                ? IM_COL32(25, 25, 25, 255)
                : IM_COL32(40, 40, 40, 255);

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

            // skill id
            ImGui::TableNextColumn();
            std::string skillId = skill["skillId"];
            ImGui::Text("%s", skillId.c_str());

            // name
            ImGui::TableNextColumn();
            std::string name = skill["name"];
            ImGui::Text("%s", name.c_str());

            ImGui::TableNextColumn();
            int skillType = skill["skillType"];
            const char* typeStr = (skillType == 0) ? "Passive" :
                (skillType == 1) ? "Manual" : "Auto";
            ImGui::Text("%s", typeStr);

            // sp
            ImGui::TableNextColumn();
            int spCost = skill["spData"]["spCost"];
            ImGui::Text("%d", spCost);

            // action
            ImGui::TableNextColumn();

            ImGui::PushID(index);

            if (ImGui::SmallButton("편집"))
            {
                _selectedSkillIndex = index;
                _isEditingSkill = true;

                LoadSkillToBuffer(skill); // 입력 버퍼 로드

                _showSkillEditor = true;
            }

            ImGui::SameLine();

            if (ImGui::SmallButton("삭제"))
            {
                _deleteSkillTargetIndex = index;
                _deleteSkillTargetName = name;
                _showSkillDeleteConfirm = true;
            }

            ImGui::PopID();

            ++index;
        }

        ImGui::EndTable();
    }

    if (_showSkillDeleteConfirm)
    {
        ImGui::OpenPopup("삭제 확인");
        _showSkillDeleteConfirm = false;
    }

    if (ImGui::BeginPopupModal("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("'%s'을(를) 삭제할까요?", _deleteSkillTargetName.c_str());
        ImGui::Text("이 작업은 되돌릴 수 없습니다.");
        ImGui::Separator();

        if (ImGui::Button("예", ImVec2(120, 0)))
        {
            if (_deleteSkillTargetIndex >= 0 && _deleteSkillTargetIndex < (int)_tempSkills.size())
            {
                _tempSkills.erase(_tempSkills.begin() + _deleteSkillTargetIndex);
                std::cout << "[Operator] Deleted skill: " << _deleteSkillTargetName << '\n';
            }
            _deleteSkillTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("아니오", ImVec2(120, 0)))
        {
            _deleteSkillTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void OperatorEditor::ClearSkillInputBuffers()
{
    memset(_inputSkillId, 0, sizeof(_inputSkillId));
    memset(_inputSkillName, 0, sizeof(_inputSkillName));
    memset(_inputSkillDesc, 0, sizeof(_inputSkillDesc));
    _inputSkillType = 1;
    _inputSpType = 1;
    _inputSpCost = 30;
    _inputInitSp = 0;
    _inputDuration = 0.0f;
    _inputEffectType = 0;
    _inputEffectValue = 0.0f;
}

json OperatorEditor::OperatorDataStructure(const std::string& charId, const std::string& name,
    const std::string& profession, int rarity, const std::string& position,
    int hp, int atk, int def, int magicRes,
    int cost, int blockCnt, float baseAttackTime, int respawnTime,
    const json& range)
{
    return {
        {"charId", charId},
        {"name", name},
        {"description", ""},
        {"profession", profession},
        {"rarity", rarity},
        {"position", position},
        {"range", range},
        {"phases", json::array({
            {
                {"phase", 0},
                {"attributesKeyFrames", json::array({
                    {
                        {"level", 0},
                        {"data", {
                            {"maxHp", hp},
                            {"atk", atk},
                            {"def", def},
                            {"magicResistance", Snap2(magicRes / 100.0)},
                            {"cost", cost},
                            {"blockCnt", blockCnt},
                            {"baseAttackTime", Snap2(baseAttackTime)},
                            {"respawnTime", respawnTime}
                        }}
                    }
                })}
            }
        })},
        {"skills", json::array()}
    };
}

OperatorEditor::Profession OperatorEditor::StringToProfession(const std::string& profStr)
{
    if (profStr == "CASTER") return Profession::PROF_CASTER;
    if (profStr == "SNIPER") return Profession::PROF_SNIPER;
    if (profStr == "GUARD") return Profession::PROF_GUARD;
    if (profStr == "DEFENDER") return Profession::PROF_DEFENDER;
    if (profStr == "MEDIC") return Profession::PROF_MEDIC;
    if (profStr == "VANGUARD") return Profession::PROF_VANGUARD;
    if (profStr == "SUPPORTER") return Profession::PROF_SUPPORTER;
    if (profStr == "SPECIALIST") return Profession::PROF_SPECIALIST;
    return Profession::PROF_CASTER;
}

OperatorEditor::Position OperatorEditor::StringToPosition(const std::string& posStr)
{
    if (posStr == "MELEE") return Position::POS_MELEE;
    return Position::POS_RANGED;
}

OperatorEditor::Position OperatorEditor::GetPositionFromProfession(Profession prof)
{
    switch (prof)
    {
    case Profession::PROF_CASTER:
    case Profession::PROF_SNIPER:
    case Profession::PROF_MEDIC:
    case Profession::PROF_SUPPORTER:
        return Position::POS_RANGED;

    case Profession::PROF_GUARD:
    case Profession::PROF_DEFENDER:
    case Profession::PROF_VANGUARD:
    case Profession::PROF_SPECIALIST:
        return Position::POS_MELEE;

    default:
        return Position::POS_RANGED;
    }
}

const char* OperatorEditor::ProfessionToString(Profession prof)
{
    switch (prof)
    {
    case Profession::PROF_CASTER: return "CASTER";
    case Profession::PROF_SNIPER: return "SNIPER";
    case Profession::PROF_GUARD: return "GUARD";
    case Profession::PROF_DEFENDER: return "DEFENDER";
    case Profession::PROF_MEDIC: return "MEDIC";
    case Profession::PROF_VANGUARD: return "VANGUARD";
    case Profession::PROF_SUPPORTER: return "SUPPORTER";
    case Profession::PROF_SPECIALIST: return "SPECIALIST";
    default: return "CASTER";
    }
}

const char* OperatorEditor::PositionToString(Position pos)
{
    return (pos == Position::POS_RANGED) ? "RANGED" : "MELEE";
}

json OperatorEditor::GridToRangeJson()
{
    json grids = json::array();

    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            if (_rangeGrid[row][col])
            {
                int relRow = CENTER - row;
                int relCol = col - CENTER;

                grids.push_back({
                    {"row", relRow},
                    {"col", relCol}
                    });
            }
        }
    }

    return grids;
}

void OperatorEditor::RangeJsonToGrid(const json& rangeData)
{
    memset(_rangeGrid, 0, sizeof(_rangeGrid));

    for (const auto& cell : rangeData)
    {
        int relRow = cell["row"];
        int relCol = cell["col"];

        int absRow = CENTER - relRow;
        int absCol = relCol + CENTER;

        if (absRow >= 0 && absRow < GRID_SIZE &&
            absCol >= 0 && absCol < GRID_SIZE)
        {
            _rangeGrid[absRow][absCol] = true;
        }
    }
}

json OperatorEditor::CreateSkillData()
{
    const char* effectKeys[] = { "atk", "def", "attack_speed", "cost", "heal" };
    std::string effectKey = effectKeys[_inputEffectType];

    return {
        {"skillId", std::string(_inputSkillId)},
        {"name", std::string(_inputSkillName)},
        {"description", std::string(_inputSkillDesc)},
        {"skillType", _inputSkillType},
        {"spData", {
            {"spType", _inputSpType},
            {"spCost", _inputSpCost},
            {"initSp", _inputInitSp}
        }},
        {"duration", Snap1(_inputDuration)},
        {"effects", json::array({
            {{"key", effectKey}, {"value", _inputEffectValue}}
        })}
    };
}

void OperatorEditor::LoadSkillToBuffer(const json& skillData)
{
    strcpy_s(_inputSkillId, skillData["skillId"].get<std::string>().c_str());
    strcpy_s(_inputSkillName, skillData["name"].get<std::string>().c_str());
    strcpy_s(_inputSkillDesc, skillData["description"].get<std::string>().c_str());

    _inputSkillType = skillData["skillType"];
    _inputSpType = skillData["spData"]["spType"];
    _inputSpCost = skillData["spData"]["spCost"];
    _inputInitSp = skillData["spData"]["initSp"];
    _inputDuration = skillData["duration"];

    if (!skillData["effects"].empty())
    {
        std::string effectKey = skillData["effects"][0]["key"];
        _inputEffectValue = skillData["effects"][0]["value"];

        if (effectKey == "atk") _inputEffectType = 0;
        else if (effectKey == "def") _inputEffectType = 1;
        else if (effectKey == "attack_speed") _inputEffectType = 2;
        else if (effectKey == "cost") _inputEffectType = 3;
        else if (effectKey == "heal") _inputEffectType = 4;
    }
}
