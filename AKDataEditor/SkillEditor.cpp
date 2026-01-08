#include "SkillEditor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui/imgui.h>

#include "Utility.h"
#include "ImGuiRAII.h"

namespace fs = std::filesystem;

SkillEditor::SkillEditor(std::string jsonPath, std::string operatorPath)
    : _jsonPath(jsonPath), _operatorPath(operatorPath)
{
    LoadSkills();
    LoadOperatorIds();
}

SkillEditor::~SkillEditor() {}

void SkillEditor::LoadSkills()
{
    std::ifstream file(_jsonPath);
    if (file.is_open())
    {
        try
        {
            json j;
            file >> j;

            _dataVersion = j.value("version", 0);

            if (j.contains("skills")) 
            {
                _skills = j["skills"].get<std::vector<Skill>>();
            }

            std::cout << "[Skill] Loaded " << _skills.size() << " skills (v" << _dataVersion << ")\n";
        }
        catch (json::exception& e)
        {
            std::cout << "[Skill] JSON parse error: " << e.what() << '\n';
            _skills.clear();
        }
    }
    else
    {
        std::cout << "[Skill] File not found, creating new: " << _jsonPath << '\n';
        _skills.clear();
    }
}

void SkillEditor::SaveSkills()
{
    fs::path filePath(_jsonPath);
    fs::create_directories(filePath.parent_path());

    json output;
    output["version"] = _dataVersion;
    output["skills"] = _skills;

    std::ofstream file(_jsonPath);
    if (file.is_open())
    {
        file << output.dump(2);
        std::cout << "Saved to " << _jsonPath << '\n';
        _hasUnsavedChanges = false;
    }
    else
    {
        std::cout << "Failed to save!\n";
    }
}

void SkillEditor::RenderGUI(bool* p_open)
{
    if (ScopedWindow window("스킬 편집기", p_open); window)
    {
        RenderToolbar();
        ImGui::Separator();

        RenderSkillList();
    }

    if (_showCreateWindow)
        RenderCreateWindow();

    if (_showEditWindow)
        RenderEditWindow();
}

void SkillEditor::RenderToolbar()
{
    if (_hasUnsavedChanges)
    {
        SCOPED_COLOR(ImGuiCol_Text, COLOR_YELLOW);
        ImGui::Text("* 저장되지 않은 변경사항");

        ImGui::SameLine();

        if (ImGui::Button("전체 저장"))
        {
            SaveSkills();
            std::cout << "[Skill] Saved successfully!\n";
        }

        ImGui::SameLine();

        if (ImGui::Button("되돌리기"))
        {
            LoadSkills();
            _hasUnsavedChanges = false;
            std::cout << "[Skill] Changes discarded.\n";
        }
    }
    else
    {
        SCOPED_COLOR(ImGuiCol_Text, COLOR_GREEN);
        ImGui::Text("저장완료");
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (ImGui::Button("새로운 스킬 생성"))
    {
        _showCreateWindow = true;
        ClearAllInputBuffers();
    }

    ImGui::SameLine();

    if (ImGui::Button("새로고침"))
    {
        LoadSkills();
        _hasUnsavedChanges = false;
    }
}

void SkillEditor::RenderSkillList()
{
    if (_skills.empty())
    {
        ImGui::TextColored(COLOR_GRAY, "스킬 데이터가 없습니다.");
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_ScrollY;

    if (ScopedTable table("SkillTable", 7, flags); table)
    {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Operator", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("SP", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Effects", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableHeadersRow();

        int index = 0;
        for (const auto& skill : _skills)
        {
            SCOPED_ID(index);

            ImGui::TableNextRow();

            ImU32 bg = (index % 2 == 0)
                ? IM_COL32(25, 25, 25, 255)
                : IM_COL32(40, 40, 40, 255);

            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

            ImGui::TableNextColumn();
            ImGui::Text("%s", skill.skillId.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", skill.operatorId.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", skill.name.c_str());

            ImGui::TableNextColumn();
            const char* types[] = { "Passive", "Manual", "Auto" };
            ImGui::Text("%s", types[skill.skillType]);

            ImGui::TableNextColumn();
            ImGui::Text("%d", skill.spData.spCost);

            ImGui::TableNextColumn();
            ImGui::Text("%d", (int)skill.blackboard.size());

            ImGui::TableNextColumn();

            if (ImGui::SmallButton("편집"))
            {
                _selectedSkillIndex = index;
                LoadSkillToBuffer(skill);
                _showEditWindow = true;
            }

            ImGui::SameLine();

            if (ImGui::SmallButton("삭제"))
            {
                _deleteTargetIndex = index;
                _deleteTargetName = skill.name;
                _showDeleteConfirm = true;
            }

            ++index;
        }
    }

    if (_showDeleteConfirm)
    {
        ImGui::OpenPopup("삭제 확인");
        _showDeleteConfirm = false;
    }

    if (ScopedPopupModal popup("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize); popup)
    {
        ImGui::TextColored(COLOR_RED, "이 스킬을 정말 삭제할까요?");
        ImGui::Separator();
        ImGui::Text("대상: ");
        ImGui::SameLine();
        ImGui::TextColored(COLOR_YELLOW, "%s", _deleteTargetName.c_str());
        ImGui::Separator();

        if (ImGui::Button("예", ImVec2(120, 0)))
        {
            if (_deleteTargetIndex >= 0 && _deleteTargetIndex < (int)_skills.size())
            {
                _skills.erase(_skills.begin() + _deleteTargetIndex);
                _hasUnsavedChanges = true;
                std::cout << "[Skill] Deleted: " << _deleteTargetName << '\n';
            }
            _deleteTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("아니요", ImVec2(120, 0)))
        {
            _deleteTargetIndex = -1;
            ImGui::CloseCurrentPopup();
        }
    }
}

void SkillEditor::RenderCreateWindow()
{
    if (ScopedWindow window("새로운 스킬 생성", &_showCreateWindow); window)
    {
        ImGui::Text("오퍼레이터 선택:");
        const char* previewText = (_selectedOperatorIdx >= 0 && _selectedOperatorIdx < _operatorIds.size())
            ? _operatorIds[_selectedOperatorIdx].c_str()
            : "선택하세요";

        if (ScopedCombo combo("##OperatorCombo", previewText); combo)
        {
            for (int i = 0; i < (int)_operatorIds.size(); ++i)
            {
                SCOPED_ID(i);
                bool isSelected = (_selectedOperatorIdx == i);
                if (ImGui::Selectable(_operatorIds[i].c_str(), isSelected))
                {
                    _selectedOperatorIdx = i;
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }

        ImGui::InputText("스킬 번호", _inputSkillSuffix, sizeof(_inputSkillSuffix));
        ImGui::SameLine();
        ImGui::TextDisabled("(1, 2, 3...)");

        if (_selectedOperatorIdx >= 0 && _selectedOperatorIdx < _operatorIds.size()) {
            std::string previewId = GenerateSkillId(_operatorIds[_selectedOperatorIdx], _inputSkillSuffix);
            ImGui::TextColored(COLOR_GREEN, "생성될 ID: %s", previewId.c_str());
        }

        ImGui::SeparatorText("기본 정보");
        ImGui::InputText("Name", _inputSkillName, 64);
        ImGui::InputTextMultiline("Description", _inputSkillDesc, 512);

        const char* skillTypes[] = { "Passive", "Manual", "Auto" };
        ImGui::Combo("Skill Type", &_inputSkillType, skillTypes, 3);

        ImGui::SeparatorText("SP 정보");

        {
            SCOPED_ITEM_WIDTH(150);

            const char* spTypes[] = { "None", "Attack", "Time", "Reserved", "Hit" };
            ImGui::Combo("SP Type", &_inputSpType, spTypes, 5);
            ImGui::InputInt("SP Cost", &_inputSpCost);
            ImGui::InputInt("Init SP", &_inputInitSp);
            ImGui::InputFloat("Duration", &_inputDuration, 0.1f, 1.0f, "%.1f");
        }

        ImGui::SeparatorText("범위");

        if (ImGui::Button("범위 편집기 열기"))
        {
            _showRangeEditor = true;
        }

        ImGui::SeparatorText("효과 (Blackboard)");
        RenderEffectListPanel();

        ImGui::Separator();

        if (ImGui::Button("생성", ImVec2(120, 0)))
        {
            if (_selectedOperatorIdx >= 0 && _selectedOperatorIdx < (int)_operatorIds.size())
            {
                Skill newSkill = CreateSkillFromBuffer();
                _skills.push_back(newSkill);

                _hasUnsavedChanges = true;
                _showCreateWindow = false;

                std::cout << "[Skill] Created: " << newSkill.name << '\n';

                ClearAllInputBuffers();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(120, 0)))
        {
            _showCreateWindow = false;
            ClearAllInputBuffers();
        }
    }

    if (_showRangeEditor)
    {
        RenderRangeGridEditor();
    }

    RenderEffectAddPopup();
}

void SkillEditor::RenderEditWindow()
{
    if (_selectedSkillIndex < 0 || _selectedSkillIndex >= (int)_skills.size())
    {
        _showEditWindow = false;
        return;
    }

    if (ScopedWindow window("스킬 편집", &_showEditWindow); window)
    {
        Skill& skill = _skills[_selectedSkillIndex];

        ImGui::SeparatorText("기본 정보");
        ImGui::Text("Operators: %s", skill.operatorId.c_str());
        ImGui::Text("Skill ID: %s", skill.skillId.c_str());
        ImGui::Separator();

        ImGui::InputText("Name", _inputSkillName, 64);
        ImGui::InputTextMultiline("Description", _inputSkillDesc, 512, ImVec2(-1, 80));
        const char* skillTypes[] = { "Passive", "Manual", "Auto" };
        ImGui::Combo("Skill Type", &_inputSkillType, skillTypes, 3);
        ImGui::SeparatorText("SP 정보");

        {
            SCOPED_ITEM_WIDTH(150);

            const char* spTypes[] = { "None", "Attack", "Time", "Reserved", "Hit" };
            ImGui::Combo("SP Type", &_inputSpType, spTypes, 5);
            ImGui::InputInt("SP Cost", &_inputSpCost);
            ImGui::InputInt("Init SP", &_inputInitSp);
            ImGui::InputFloat("Duration", &_inputDuration, 0.1f, 1.0f, "%.1f");
        }

        ImGui::SeparatorText("범위");

        if (ImGui::Button("범위 편집기 열기", ImVec2(200, 0)))
        {
            RangeJsonToGrid(skill.range);
            _showRangeEditor = true;
        }

        ImGui::SameLine();

        int rangeCount = (int)skill.range.size();
        ImGui::Text("(%d 칸)", rangeCount);

        ImGui::SeparatorText("효과 (Blackboard)");
        RenderEffectListPanel();

        ImGui::Separator();

        if (ImGui::Button("완료", ImVec2(120, 0)))
        {
            skill.name = _inputSkillName;          
            skill.description = _inputSkillDesc;
            skill.skillType = _inputSkillType;
            skill.duration = Snap1(static_cast<double>(_inputDuration));
            skill.spData.spType = _inputSpType;
            skill.spData.spCost = _inputSpCost;
            skill.spData.initSp = _inputInitSp;
            skill.range = GridToRangeJson();
            skill.blackboard = _currentEffects;

            _hasUnsavedChanges = true;
            _showEditWindow = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(120, 0)))
        {
            _showEditWindow = false;
        }
    }

    if (_showRangeEditor)
    {
        RenderRangeGridEditor();
    }

    RenderEffectAddPopup();
}

void SkillEditor::RenderRangeGridEditor()
{
    ImGui::SetNextWindowSize(ImVec2(500, 550), ImGuiCond_Appearing);

    if (ScopedWindow window("스킬 범위 편집", &_showRangeEditor); window)
    {
        ImGui::Text("칸을 클릭해서 스킬 범위를 설정하세요");

        {
            SCOPED_COLOR(ImGuiCol_Text, COLOR_BLUE);
            ImGui::Text("파랑 = 오퍼레이터");
        }

        {
            SCOPED_COLOR(ImGuiCol_Text, COLOR_YELLOW);
            ImGui::Text("노랑 = 스킬 범위");
        }

        ImGui::Separator();
        ImGui::Spacing();

        const float cellSize = 25.0f;
        const float gridWidth = GRID_SIZE * cellSize;
        const float gridHeight = GRID_SIZE * cellSize;

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();

        for (int row = 0; row < GRID_SIZE; row++)
        {
            for (int col = 0; col < GRID_SIZE; col++)
            {
                ImVec2 p_min(canvas_pos.x + col * cellSize, canvas_pos.y + row * cellSize);
                ImVec2 p_max(p_min.x + cellSize, p_min.y + cellSize);

                if (row == CENTER && col == CENTER)
                {
                    draw_list->AddRectFilled(p_min, p_max, IM_COL32(100, 100, 255, 255));
                }
                else if (_skillRangeGrid[row][col])
                {
                    draw_list->AddRectFilled(p_min, p_max, IM_COL32(255, 255, 100, 255));
                }
                else
                {
                    draw_list->AddRectFilled(p_min, p_max, IM_COL32(50, 50, 50, 255));
                }

                draw_list->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255));
            }
        }

        ImGui::InvisibleButton("canvas", ImVec2(gridWidth, gridHeight));
        if (ImGui::IsItemClicked(0))
        {
            ImVec2 mouse = ImGui::GetMousePos();
            ImVec2 local = ImVec2(mouse.x - canvas_pos.x, mouse.y - canvas_pos.y);

            int col = (int)(local.x / cellSize);
            int row = (int)(local.y / cellSize);

            if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
            {
                if (!(row == CENTER && col == CENTER))
                {
                    _skillRangeGrid[row][col] = !_skillRangeGrid[row][col];
                }
            }
        }

        ImGui::Separator();

        if (ImGui::Button("초기화", ImVec2(100, 0)))
        {
            memset(_skillRangeGrid, 0, sizeof(_skillRangeGrid));
            _skillRangeGrid[CENTER][CENTER] = true;
        }

        ImGui::SameLine();

        if (ImGui::Button("확인", ImVec2(100, 0)))
        {
            _showRangeEditor = false;
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(100, 0)))
        {
            _showRangeEditor = false;
        }
    }
}

void SkillEditor::RenderEffectListPanel()
{
    if (_currentEffects.empty())
    {
        ImGui::TextColored(COLOR_GRAY, "효과가 없습니다.");
    }
    else
    {
        if (ScopedTable table("EffectTable", 3, ImGuiTableFlags_Borders); table)
        {
            ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();

            int index = 0;
            for (auto& effect : _currentEffects)
            {
                SCOPED_ID(index);

                ImGui::TableNextRow();

                ImU32 bg = (index % 2 == 0)
                    ? IM_COL32(25, 25, 25, 255)
                    : IM_COL32(40, 40, 40, 255);

                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

                ImGui::TableNextColumn();
                ImGui::Text("%s", effect.key.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%.2f", effect.value);

                ImGui::TableNextColumn();

                if (ImGui::SmallButton("수정"))
                {
                    strcpy_s(_inputEffectKey, effect.key.c_str());
                    _inputEffectValue = static_cast<float>(effect.value);
                    _editingEffectIndex = index;
                    _showEffectAddPopup = true;
                }

                ImGui::SameLine();

                if (ImGui::SmallButton("삭제"))
                {
                    _currentEffects.erase(_currentEffects.begin() + index);
                    _hasUnsavedChanges = true;
                    break;
                }

                ++index;
            }
        }
    }

    if (ImGui::Button("+ 효과 추가", ImVec2(150, 0)))
    {
        memset(_inputEffectKey, 0, sizeof(_inputEffectKey));
        _inputEffectValue = 0.0f;
        _editingEffectIndex = -1;
        _showEffectAddPopup = true;
    }
}

void SkillEditor::RenderEffectAddPopup()
{
    if (_showEffectAddPopup)
    {
        ImGui::OpenPopup("효과 추가/수정");
        _showEffectAddPopup = false;
    }

    if (ScopedPopupModal popup("효과 추가/수정", nullptr, ImGuiWindowFlags_AlwaysAutoResize); popup)
    {
        ImGui::Text(_editingEffectIndex >= 0 ? "효과 수정" : "효과 추가");
        ImGui::Separator();

        ImGui::InputText("Key", _inputEffectKey, 64);
        ImGui::InputFloat("Value", &_inputEffectValue, 0.01f, 1.0f, "%.2f");

        ImGui::Separator();

        if (ImGui::Button("확인", ImVec2(120, 0)))
        {
            if (strlen(_inputEffectKey) > 0)
            {
                BlackboardEntry entry;
                entry.key = _inputEffectKey;
                entry.value = Snap2(static_cast<double>(_inputEffectValue));

                if (_editingEffectIndex >= 0)
                {
                    _currentEffects[_editingEffectIndex] = entry;
                }
                else
                {
                    _currentEffects.push_back(entry);
                }

                _hasUnsavedChanges = true;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("취소", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
    }
}

void SkillEditor::ClearAllInputBuffers()
{
    memset(_inputSkillName, 0, sizeof(_inputSkillName));
    memset(_inputSkillDesc, 0, sizeof(_inputSkillDesc));
    _inputSkillType = 1;
    _inputDuration = 0.0f;
    _inputSpType = 1;
    _inputSpCost = 30;
    _inputInitSp = 0;
    _currentEffects.clear();
    memset(_skillRangeGrid, 0, sizeof(_skillRangeGrid));
}

void SkillEditor::LoadSkillToBuffer(const Skill& skill)
{
    strcpy_s(_inputSkillName, skill.name.c_str());
    strcpy_s(_inputSkillDesc, skill.description.c_str());
    _inputSkillType = skill.skillType;
    _inputDuration = skill.duration;
    _inputSpType = skill.spData.spType;
    _inputSpCost = skill.spData.spCost;
    _inputInitSp = skill.spData.initSp;

    RangeJsonToGrid(skill.range);

    _currentEffects = skill.blackboard;

    auto it = std::find(_operatorIds.begin(), _operatorIds.end(), skill.operatorId);
    if (it != _operatorIds.end())
    {
        _selectedOperatorIdx = std::distance(_operatorIds.begin(), it);
    }
}

Skill SkillEditor::CreateSkillFromBuffer()
{
    Skill skill;

    skill.operatorId = _operatorIds[_selectedOperatorIdx];
    skill.skillId = GenerateSkillId(skill.operatorId, _inputSkillSuffix);

    skill.name = _inputSkillName;
    skill.description = _inputSkillDesc;
    skill.skillType = _inputSkillType;
    skill.duration = Snap1(static_cast<double>(_inputDuration));

    skill.spData.spType = _inputSpType;
    skill.spData.spCost = _inputSpCost;
    skill.spData.initSp = _inputInitSp;

    skill.range = GridToRangeJson();
    skill.blackboard = _currentEffects;

    return skill;
}

std::vector<SkillRange> SkillEditor::GridToRangeJson()
{
    std::vector<SkillRange> ranges;

    for (int row = 0; row < GRID_SIZE; ++row)
    {
        for (int col = 0; col < GRID_SIZE; ++col)
        {
            if (_skillRangeGrid[row][col])
            {
                SkillRange range;
                range.row = CENTER - row;
                range.col = col - CENTER;
                ranges.push_back(range);
            }
        }
    }

    return ranges;
}

void SkillEditor::RangeJsonToGrid(const std::vector<SkillRange>& ranges)
{
    memset(_skillRangeGrid, 0, sizeof(_skillRangeGrid));

    for (const auto& range : ranges)
    {
        int absRow = CENTER - range.row;
        int absCol = range.col + CENTER;

        if (absRow >= 0 && absRow < GRID_SIZE &&
            absCol >= 0 && absCol < GRID_SIZE)
        {
            _skillRangeGrid[absRow][absCol] = true;
        }
    }
}

void SkillEditor::LoadOperatorIds()
{
    _operatorIds.clear();

    std::ifstream file(_operatorPath);
    if (!file.is_open())
    {
        std::cout << "[Skill] operators_table.json not found: " << _operatorPath << '\n';
        return;
    }
    else
    {
        try
        {
            json j;
            file >> j;

            if (j.contains("operators") && j["operators"].is_array())
            {
                for (const auto& op : j["operators"])
                {
                    if (op.contains("charId"))
                    {
                        std::string charId = op["charId"].get<std::string>();
                        _operatorIds.push_back(charId);
                    }
                }
            }

            std::sort(_operatorIds.begin(), _operatorIds.end());

            std::cout << "[SKill] Loaded " << _operatorIds.size() << " operators.\n";
        }
        catch (json::exception e)
        {
            std::cout << "[Skill] Failed to parse operators_table.josn: " << e.what() << '\n';
        }
    }
}

std::string SkillEditor::GenerateSkillId(const std::string& operatorId, const std::string& suffix)
{
    std::string skillId = "skchr_";

    if (operatorId.find("char_") == 0)
    {
        skillId += operatorId.substr(5);
    }
    else
    {
        skillId += operatorId;
    }

    skillId += '_' + suffix;
    return skillId;
}

std::string SkillEditor::GetOperatorDisplayName(const std::string& operatorId)
{
    return operatorId;
}
