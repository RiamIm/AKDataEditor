#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class LevelEditor
{
public:
	LevelEditor(std::string jsonPath, std::string solutionPath);
	~LevelEditor();

	void RenderGUI(bool* p_open);

	void LoadLevels();
	void SaveAllLevels();

	bool HasUnsavedChanges() const { return _hasUnsavedChanges; }
	void ClearUnsavedFlag() { _hasUnsavedChanges = false; }

private:
    // 타일 타입
    enum class TileType
    {
        Forbidden = 0,   // tile_forbidden
        Road = 1,        // tile_road
        Wall = 2,        // tile_wall
        Start = 3,       // tile_start
        End = 4,         // tile_end
        HighGround = 5,  // tile_highground
        Hole = 6,        // tile_hole
        MAX
    };

    // 개별 레벨 데이터 구조체
    struct LevelData
    {
        std::string fileName;              // level_main_00-01.json
        std::string levelId;               // 00-01
        int characterLimit = 8;
        int gridRows = 6;
        int gridCols = 9;
        int maxLifePoint = 3;
        int initialCost = 10;
        int maxCost = 99;
        float costIncreaseTime = 1.0f;

        std::vector<std::vector<int>> gridMap;  // 타일 인덱스 맵
        std::vector<json> tiles;                // 타일 정보
        json fullData;                          // 전체 JSON 데이터

        bool isModified = false;                // 수정 여부

        // 완성 상태 추적
        bool gridCompleted = false;
        bool routeCompleted = false;
        bool waveCompleted = false;
    };

    // edit 상태
    enum class EditMode
    {
        Grid,
        Route,
        Wave,
        MAX
    };

    // 경로 편집 단계
    enum class RouteEditStep
    {
        SetStart,
        SetEnd,
        AddCheckpoints,
        MAX
    };

private:
	std::string _jsonPath;

    // 레벨 목록
    std::vector<LevelData> _levels;

	// 변경 사항 추적
	bool _hasUnsavedChanges = false;

	// gui 상태
	bool _showCreateWindow = false;
	bool _showEditWindow = false;
    int _selectedLevelIndex = -1;

    EditMode _editMode = EditMode::Grid;
    bool _editModeChanged = false;

    // 삭제 확인
    bool _showDeleteConfirm = false;
    int _deleteTargetIndex = -1;
    std::string _deleteTargetName = "";

    // 새 레벨 입력
    char _inputLevelId[64] = ""; // 00-01 형식

    // 그리드 상태
    TileType _selectedTileType = TileType::Road;
    int _selectedGridRow = -1;
    int _selectedGridCol = -1;

    // 경로 편집 상태
    int _selectedRouteIndex = -1;              
    int _editingCheckpointIndex = -1;         
    bool _showRouteDeleteConfirm = false; 
    RouteEditStep _routeEditStep = RouteEditStep::SetStart;
    bool _routeEditMode = false;

    // 웨이브 편집 상태
    int _selectedWaveIndex = -1;
    int _selectedFragmentIndex = -1;
    int _selectedActionIndex = -1;
    bool _showWaveDeleteConfirm = false;
    bool _showFragmentDeleteConfirm = false;
    char _inputEnemyKey[128] = "";  // 적 키 입력용
    std::vector<std::string> _enemyKeys;  // enemy_table의 적 목록
    int _selectedEnemyIndex = 0;  // Combo 선택 인덱스

	// gui render
	void RenderToolbar();
	void RenderLevelsList();
	void RenderCreateWindow();
	void RenderEditWindow();

    // 편집 윈도우 서브 패널
    void RenderGridEditor(LevelData& level);
    void RenderTileInspector(LevelData& level);
    void RenderOptionsPanel(LevelData& level);

    void RenderRouteEditor(LevelData& level);
    void RenderRouteOnGrid(LevelData& level, json& route);

    void RenderWaveEditor(LevelData& level);
    void RenderWaveList(LevelData& level, int waveCount);
    void RenderWaveSetting(LevelData& level, json& wave);
    void RenderFragmentList(json& wave);  
    void RenderFragmentEditor(LevelData& level, json& fragment);  
    void RenderEnemySelector(LevelData& level, json& fragment);  

    // 레벨 파일 관리
    std::vector<std::string> GetLevelFiles() const;
    std::string FormatLevelFileName(const std::string& levelId) const;
    std::string ExtractLevelId(const std::string& fileName) const;
    void LoadEnemyTable(std::string solutionPath);

    // 레벨 데이터 처리
    LevelData LoadLevelFromFile(const std::string& fileName);
    void SaveLevelToFile(const LevelData& level);
    void InitializeEmptyLevel(LevelData& level, const std::string& levelId);

    // JSON 동기화
    void SyncGridFromJson(LevelData& level);
    void SyncJsonFromGrid(LevelData& level);

    // 타일 관련
    json CreateTileData(TileType type);
    const char* TileTypeToString(TileType type);
    const char* TileTypeToTileKey(TileType type);
    int GetTileColor(TileType type);

    // 좌표 변환
    int GameRowToJsonIndex(int gameRow, int totalRows) const { return (totalRows - 1) - gameRow; }
    int JsonIndexToGameRow(int jsonIdx, int totalRows) const { return (totalRows - 1) - jsonIdx; }
};

