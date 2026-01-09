#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class OperatorEditor
{
public:
    OperatorEditor(const std::string& jsonPath);
    ~OperatorEditor();

    void RenderGUI(bool* p_open);
    bool HasUnsavedChanges() const { return _hasUnsavedChanges; }
    void ClearUnsavedFlag() { _hasUnsavedChanges = false; }

    void LoadOperators();
    void SaveOperators();

private:
    std::string _jsonPath;
    json _operatorData;

    // Enums
    enum Profession {
        PROF_CASTER = 0,
        PROF_SNIPER = 1,
        PROF_GUARD = 2,
        PROF_DEFENDER = 3,
        PROF_MEDIC = 4,
        PROF_VANGUARD = 5,
        PROF_SUPPORTER = 6,
        PROF_SPECIALIST = 7,
        PROF_MAX
    };

    enum Position {
        POS_RANGED = 0,
        POS_MELEE = 1,
        POS_MAX
    };

    // 변경 감지
    bool _hasUnsavedChanges = false;

    // GUI State
    bool _showCreateWindow = false;
    bool _showEditWindow = false;
    bool _showRangeEditor = false;
    int _selectedOperatorIndex = -1;

    // Delete 확인
    bool _showDeleteConfirm = false;
    int _deleteTargetIndex = -1;
    std::string _deleteTargetName;

    // 입력 버퍼
    char _inputCharId[64] = "";
    char _inputName[64] = "";
    Profession _inputProfession = Profession::PROF_CASTER;
    int _inputRarity = 3;
    Position _inputPosition = Position::POS_RANGED;
    int _inputHp = 1000;
    int _inputAtk = 300;
    int _inputDef = 50;
    int _inputMagicRes = 0;
    int _inputCost = 10;
    int _inputBlockCnt = 1;
    float _inputBaseAttackTime = 1.5f;
    int _inputRespawnTime = 70;

    // 격자판 범위 데이터
    static const int GRID_SIZE = 13;
    static const int CENTER = 6;
    bool _rangeGrid[GRID_SIZE][GRID_SIZE] = {};

    // GUI 서브 함수
    void RenderToolbar();
    void RenderOperatorList();
    void RenderCreateWindow();
    void RenderEditWindow();
    void RenderRangeGridEditor();
    void RenderSkillList();

    // 헬퍼 함수
    json OperatorDataStructure(const std::string& charId, const std::string& name,
        const std::string& profession, int rarity, const std::string& position,
        int hp, int atk, int def, int magicRes,
        int cost, int blockCnt, float baseAttackTime, int respawnTime,
        const json& range);

    // 변환 함수
    Profession StringToProfession(const std::string& profStr);
    Position StringToPosition(const std::string& posStr);
    Position GetPositionFromProfession(Profession prof);
    const char* ProfessionToString(Profession prof);
    const char* PositionToString(Position pos);

    // 격자판 변환
    json GridToRangeJson();
    void RangeJsonToGrid(const json& rangeData);
};
