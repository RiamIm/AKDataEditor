#include "LevelEditor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_gdi.h>

#include "Utility.h"

namespace fs = std::filesystem;

LevelEditor::LevelEditor(std::string jsonPath)
	: _jsonPath(jsonPath)
{
	LoadLevels();
}

LevelEditor::~LevelEditor()
{
}

void LevelEditor::RenderGUI(bool* p_open)
{
	ImGui::Begin("레벨 편집기", p_open);

	RenderToolbar();
	ImGui::Separator();

	RenderLevelsList();
	ImGui::End();

	if (_showCreateWindow)
		RenderCreateWindow();

	if (_showEditWindow)
		RenderEditWindow();
}

void LevelEditor::LoadLevels()
{
	_levels.clear();

	std::vector<std::string> levelFiles = GetLevelFiles();

	for (const auto& fileName : levelFiles)
	{
		LevelData level = LoadLevelFromFile(fileName);
		_levels.push_back(level);
	}

	std::cout << "[Level] Loaded: " << _levels.size() << " levels.\n";
}

void LevelEditor::SaveAllLevels()
{
	for (auto& level : _levels)
	{
		if (level.isModified)
		{
			SaveLevelToFile(level);
			level.isModified = false;
		}
	}

	std::cout << "[Level] All levels saved.\n";

}

void LevelEditor::RenderToolbar()
{
	if (_hasUnsavedChanges)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_YELLOW);
		ImGui::Text("* 저장되지 않은 변경사항");
		ImGui::PopStyleColor();

		ImGui::SameLine();

		if (ImGui::Button("전체 저장"))
		{
			SaveAllLevels();
			_hasUnsavedChanges = false;
			std::cout << "[Level] Saved successfully!\n";
		}

		if (ImGui::Button("되돌리기"))
		{
			LoadLevels();  // 파일에서 다시 로드
			_hasUnsavedChanges = false;
			std::cout << "[Level] Changes discarded.\n";
		}
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GREEN);
		ImGui::Text("저장완료");
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	if (ImGui::Button("새로운 레벨 생성"))
	{
		_showCreateWindow = true;
		memset(_inputLevelId, 0, sizeof(_inputLevelId));
	}

	ImGui::SameLine();

	if (ImGui::Button("새로고침"))
	{
		LoadLevels();
		_hasUnsavedChanges = false;
	}
}

void LevelEditor::RenderLevelsList()
{
	if (_levels.empty())
	{
		ImGui::TextColored(COLOR_GRAY, "레벨 데이터가 없습니다.");
		return;
	}

	ImGuiTableFlags flags = ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY;

	if (ImGui::BeginTable("LevelTable", 6, flags))
	{
		ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Grid Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Init DP", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Max Life", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 80.0f);
		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableHeadersRow();

		int index = 0;
		for (auto& level : _levels)
		{
			ImGui::TableNextRow();

			ImU32 bg = (index % 2 == 0)
				? IM_COL32(25, 25, 25, 255)
				: IM_COL32(40, 40, 40, 255);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

			// id
			ImGui::TableNextColumn();
			ImGui::Text("%s", level.levelId.c_str());

			// grid size
			ImGui::TableNextColumn();
			ImGui::Text("%dx%d", level.gridRows, level.gridCols);

			// init dp
			ImGui::TableNextColumn();
			ImGui::Text("%d", level.initialCost);

			// max life
			ImGui::TableNextColumn();
			ImGui::Text("%d", level.maxLifePoint);

			// state
			ImGui::TableNextColumn();
			if (level.isModified)
			{
				ImGui::TextColored(COLOR_YELLOW, "수정됨");
			}
			else
			{
				ImGui::TextColored(COLOR_GREEN, "저장됨");
			}

			// action;
			ImGui::TableNextColumn();

			ImGui::PushID(index);
			if (ImGui::SmallButton("펀집"))
			{
				_selectedLevelIndex = index;
				_showEditWindow = true;

				// 편집 상태 초기화
				_selectedGridRow = -1;
				_selectedGridCol = -1;
				_selectedTileType = TileType::Road;
			}

			ImGui::SameLine();
			if (ImGui::SmallButton("삭제"))
			{
				_deleteTargetIndex = index;
				_deleteTargetName = level.levelId;
				_showDeleteConfirm = true;
			}

			ImGui::PopID();

			++index;
		}

		ImGui::EndTable();
	}

	if (_showDeleteConfirm)
	{
		ImGui::OpenPopup("삭제 확인");
		_showDeleteConfirm = false;
	}

	if (ImGui::BeginPopupModal("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(COLOR_RED, "이 레벨을 정말 삭제할까요?");
		ImGui::Separator();

		ImGui::Text("대상: ");
		ImGui::SameLine();
		ImGui::TextColored(COLOR_YELLOW, "%s", _deleteTargetName.c_str());

		ImGui::Separator();

		if (ImGui::Button("예", ImVec2(120, 0)))
		{
			if (_deleteTargetIndex >= 0 && _deleteTargetIndex < (int)_levels.size())
			{
				std::string filePath = _jsonPath + "/" + _levels[_deleteTargetIndex].fileName;
				fs::remove(filePath);

				_levels.erase(_levels.begin() + _deleteTargetIndex);
				std::cout << "[Level] Deleted: " << _deleteTargetName << "\n";
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

		ImGui::EndPopup();
	}
}

void LevelEditor::RenderCreateWindow()
{
	ImGui::Begin("새로운 레벨 생성", &_showCreateWindow);

	ImGui::SeparatorText("레벨 ID 입력");
	ImGui::Text("형식: 00-01, 01-05 등");

	ImGui::InputTextWithHint("##LevelID", "00-01", _inputLevelId, 64);

	// 미리보기
	if (strlen(_inputLevelId) > 0)
	{
		std::string previewName = FormatLevelFileName(_inputLevelId);
		ImGui::Text("생성될 파일: %s", previewName.c_str());
	}

	ImGui::Separator();

	if (ImGui::Button("생성", ImVec2(120, 0)))
	{
		if (strlen(_inputLevelId) > 0)
		{
			LevelData newLevel;
			InitializeEmptyLevel(newLevel, _inputLevelId);

			_levels.push_back(newLevel);

			std::sort(_levels.begin(), _levels.end(), [](const LevelData& a, const LevelData& b)
				{
					return a.levelId < b.levelId;
				});

			_hasUnsavedChanges = true;
			_showCreateWindow = false;

			std::cout << "[Level] Created: " << newLevel.levelId << "\n";
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("취소", ImVec2(120, 0)))
	{
		_showCreateWindow = false;
	}

	ImGui::End();
}

void LevelEditor::RenderEditWindow()
{
	if (_selectedLevelIndex < 0 || _selectedLevelIndex >= (int)_levels.size())
	{
		_showEditWindow = false;
		return;
	}

	LevelData& level = _levels[_selectedLevelIndex];

	if (!ImGui::Begin("레벨 편집", &_showEditWindow, ImGuiWindowFlags_MenuBar))
	{
		ImGui::End();
		return;
	}

	// 메뉴바
	if (ImGui::BeginMenuBar())
	{
		ImGui::Text("레벨: %s", level.levelId.c_str());
		ImGui::EndMenuBar();
	}

	// 사용 가능한 영역 크기 계산
	ImVec2 availRegion = ImGui::GetContentRegionAvail();
	float buttonHeight = 30.0f;  // 하단 버튼 높이
	float childHeight = availRegion.y - buttonHeight - 10.0f;  // 여백 포함

	// 왼쪽 70%: 타일 에디터
	ImGui::BeginChild("GridPane", ImVec2(availRegion.x * 0.7f, childHeight), true);
	RenderGridEditor(level);
	ImGui::EndChild();

	ImGui::SameLine();

	// 오른쪽 30%: 옵션 + 타일 정보
	ImGui::BeginChild("InfoPane", ImVec2(0, childHeight), true);
	RenderOptionsPanel(level);
	ImGui::Separator();
	RenderTileInspector(level);
	ImGui::EndChild();

	ImGui::Separator();

	// 하단 버튼
	if (ImGui::Button("완료", ImVec2(120, 0)))
	{
		_showEditWindow = false;
	}

	ImGui::End();
}


void LevelEditor::RenderGridEditor(LevelData& level)
{
	ImGui::SeparatorText("격자판 편집");

	// 그리드 크기 조정
	ImGui::PushItemWidth(100);
	if (ImGui::InputInt("행 (세로)", &level.gridRows, 1, 1))
	{
		level.gridRows = std::max(1, std::min(20, level.gridRows));

		// 그리드 맵 리사이즈
		level.gridMap.resize(level.gridRows, std::vector<int>(level.gridCols, 0));

		// 타일 재생성
		level.tiles.clear();
		for (int i = 0; i < level.gridRows * level.gridCols; i++)
		{
			level.tiles.push_back(CreateTileData(TileType::Forbidden));
		}

		SyncJsonFromGrid(level);
		level.isModified = true;
		_hasUnsavedChanges = true;
	}
	ImGui::SameLine();
	if (ImGui::InputInt("열 (가로)", &level.gridCols, 1, 1))
	{
		level.gridCols = std::max(1, std::min(20, level.gridCols));

		// 그리드 맵 리사이즈
		for (auto& row : level.gridMap)
			row.resize(level.gridCols, 0);

		// 타일 재생성
		level.tiles.clear();
		for (int i = 0; i < level.gridRows * level.gridCols; i++)
		{
			level.tiles.push_back(CreateTileData(TileType::Forbidden));
		}

		SyncJsonFromGrid(level);
		level.isModified = true;
		_hasUnsavedChanges = true;
	}
	ImGui::PopItemWidth();

	// 타일 브러시 선택
	ImGui::Separator();
	ImGui::Text("타일 브러시:");
	const char* tileTypes[] = { "금지", "도로", "벽", "시작", "종료", "고지", "구멍" };
	for (int i = 0; i < (int)TileType::MAX; i++)
	{
		if (ImGui::RadioButton(tileTypes[i], (int)_selectedTileType == i))
			_selectedTileType = (TileType)i;
		if (i < (int)TileType::MAX - 1)
			ImGui::SameLine();
	}

	ImGui::Separator();

	// 격자판 렌더링
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();

	float cellSize = std::min(
		canvas_size.x / level.gridCols,
		canvas_size.y / level.gridRows
	) * 0.9f;

	// 그리드 그리기 (중요: 좌표 변환!)
	for (int jsonRow = 0; jsonRow < level.gridRows; jsonRow++)
	{
		int gameRow = JsonIndexToGameRow(jsonRow, level.gridRows);

		for (int col = 0; col < level.gridCols; col++)
		{
			// 화면 위치
			ImVec2 p_min(
				canvas_pos.x + col * cellSize,
				canvas_pos.y + jsonRow * cellSize
			);
			ImVec2 p_max(p_min.x + cellSize, p_min.y + cellSize);

			// 타일 인덱스 가져오기
			int tileIndex = jsonRow * level.gridCols + col;

			// 타일 타입 결정
			TileType tileType = TileType::Forbidden;
			if (tileIndex < (int)level.tiles.size())
			{
				std::string tileKey = level.tiles[tileIndex].value("tileKey", "tile_forbidden");
				if (tileKey == "tile_road") tileType = TileType::Road;
				else if (tileKey == "tile_wall") tileType = TileType::Wall;
				else if (tileKey == "tile_start") tileType = TileType::Start;
				else if (tileKey == "tile_end") tileType = TileType::End;
				else if (tileKey == "tile_highground") tileType = TileType::HighGround;
				else if (tileKey == "tile_hole") tileType = TileType::Hole;
			}

			// 색상 적용
			ImU32 color = GetTileColor(tileType);
			draw_list->AddRectFilled(p_min, p_max, color);

			// 그리드 선
			draw_list->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255));

			// 좌표 표시 
			char coordText[16];
			snprintf(coordText, sizeof(coordText), "(%d, %d)", col, gameRow);
			if (cellSize > 30)
			{
				draw_list->AddText(ImVec2(p_min.x + 2, p_min.y + 2),
					IM_COL32(255, 255, 255, 150), coordText);
			}

			// 마우스 클릭/드래그로 타일 편집
			if (ImGui::IsMouseClicked(0) || ImGui::IsMouseDown(0))
			{
				ImVec2 mouse = ImGui::GetMousePos();
				if (mouse.x >= p_min.x && mouse.x <= p_max.x &&
					mouse.y >= p_min.y && mouse.y <= p_max.y)
				{
					// 타일 생성
					json newTile = CreateTileData(_selectedTileType);

					// 타일 교체
					if (tileIndex < (int)level.tiles.size())
						level.tiles[tileIndex] = newTile;

					_selectedGridRow = gameRow;
					_selectedGridCol = col;

					level.isModified = true;
					_hasUnsavedChanges = true;

					SyncJsonFromGrid(level);
				}
			}

			// 선택된 셀 하이라이트
			if (gameRow == _selectedGridRow && col == _selectedGridCol)
			{
				draw_list->AddRect(p_min, p_max, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f);
			}
		}
	}

	// 캔버스 영역
	ImGui::InvisibleButton("canvas", ImVec2(level.gridCols * cellSize, level.gridRows * cellSize));
}


void LevelEditor::RenderTileInspector(LevelData& level)
{
	ImGui::SeparatorText("타일 정보");

	if (_selectedGridRow < 0 || _selectedGridCol < 0)
	{
		ImGui::TextDisabled("타일을 선택하세요");
		return;
	}

	ImGui::Text("게임 좌표:");
	ImGui::Text("  Row: %d (y=%d)", _selectedGridRow, _selectedGridRow);
	ImGui::Text("  Col: %d (x=%d)", _selectedGridCol, _selectedGridCol);

	ImGui::Separator();

	// 타일 인덱스 계산
	int jsonRow = GameRowToJsonIndex(_selectedGridRow, level.gridRows);
	int tileIndex = jsonRow * level.gridCols + _selectedGridCol;

	if (tileIndex >= 0 && tileIndex < (int)level.tiles.size())
	{
		auto& tile = level.tiles[tileIndex];

		std::string tileKey = tile.value("tileKey", "tile_forbidden");
		int heightType = tile.value("heightType", 0);
		int buildableType = tile.value("buildableType", 0);
		int passableMask = tile.value("passableMask", 0);

		ImGui::Text("타일 종류: %s", tileKey.c_str());
		ImGui::Text("높이: %s", heightType == 0 ? "낮음" : "높음");
		ImGui::Text("배치 가능: %s",
			buildableType == 0 ? "불가" :
			buildableType == 1 ? "일반" : "벽 전용");
		ImGui::Text("통과 가능: %s", passableMask == 3 ? "가능" : "불가");
	}
}

void LevelEditor::RenderOptionsPanel(LevelData& level)
{
	ImGui::SeparatorText("레벨 옵션");

	ImGui::PushItemWidth(150);

	if (ImGui::InputInt("최대 라이프", &level.maxLifePoint))
	{
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	if (ImGui::InputInt("시작 DP", &level.initialCost))
	{
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	if (ImGui::InputInt("최대 DP", &level.maxCost))
	{
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	if (ImGui::InputFloat("DP 증가 속도", &level.costIncreaseTime, 0.1f, 1.0f, "%.1f"))
	{
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	ImGui::PopItemWidth();
}

std::vector<std::string> LevelEditor::GetLevelFiles() const
{
	std::vector<std::string> levelFiles;

	if (!fs::exists(_jsonPath))
		return levelFiles;

	for (const auto& entry : fs::directory_iterator(_jsonPath))
	{
		if (entry.is_regular_file())
		{
			std::string filename = entry.path().filename().string();

			// level_main_*.json 패턴 필터링
			if (filename.find("level_main_") == 0 && filename.ends_with(".json"))
			{
				levelFiles.push_back(filename);
			}
		}
	}

	// 오름차순 정렬
	std::sort(levelFiles.begin(), levelFiles.end());

	return levelFiles;
}

std::string LevelEditor::FormatLevelFileName(const std::string& levelId) const
{
	// "00-01" → "level_main_00-01.json"
	return "level_main_" + levelId + ".json";
}

std::string LevelEditor::ExtractLevelId(const std::string& fileName) const
{
	// "level_main_00-01.json" → "00-01"
	if (fileName.length() > 16)
	{
		return fileName.substr(11, fileName.length() - 16);
	}
	return fileName;
}

LevelEditor::LevelData LevelEditor::LoadLevelFromFile(const std::string& fileName)
{
	LevelData level;
	level.fileName = fileName;
	level.levelId = ExtractLevelId(fileName);

	std::string filePath = _jsonPath + "/" + fileName;
	std::ifstream file(filePath);

	if (file.is_open())
	{
		try
		{
			file >> level.fullData;

			// 옵션 불러오기
			if (level.fullData.contains("options"))
			{
				auto& opts = level.fullData["options"];
				level.maxLifePoint = opts.value("maxLifePoint", 3);
				level.initialCost = opts.value("initialCost", 10);
				level.maxCost = opts.value("maxCost", 99);
				level.costIncreaseTime = opts.value("costIncreaseTime", 1.0f);
			}

			// 그리드 데이터 동기화
				SyncGridFromJson(level);

			level.isModified = false;

			std::cout << "[Level] Loaded level: " << level.levelId << "\n";
		}
		catch (json::exception& e)
		{
			std::cout << "[Level] JSON parse error for " << fileName << ": " << e.what() << "\n";
			// 실패 시 빈 레벨 초기화
			InitializeEmptyLevel(level, level.levelId);
		}
	}
	else
	{
		std::cout << "[Level] File not found: " << filePath << "\n";
		InitializeEmptyLevel(level, level.levelId);
	}

	return level;
}

void LevelEditor::SaveLevelToFile(const LevelData& level)
{
	// JSON 업데이트
	json saveData = level.fullData;

	// 옵션 업데이트
	saveData["options"]["maxLifePoint"] = level.maxLifePoint;
	saveData["options"]["initialCost"] = level.initialCost;
	saveData["options"]["maxCost"] = level.maxCost;
	saveData["options"]["costIncreaseTime"] = Snap1(static_cast<double>(level.costIncreaseTime));

	// 맵 데이터 업데이트
	saveData["mapData"] = level.fullData["mapData"];

	// 파일 저장
	fs::create_directories(_jsonPath);

	std::string filepath = _jsonPath + "/" + level.fileName;
	std::ofstream file(filepath);

	if (file.is_open())
	{
		file << saveData.dump(2);
		std::cout << "[LevelEditor] Saved: " << level.levelId << "\n";
	}
	else
	{
		std::cout << "[LevelEditor] Failed to save: " << level.levelId << "\n";
	}
}

void LevelEditor::InitializeEmptyLevel(LevelData& level, const std::string& levelId)
{
	level.fileName = FormatLevelFileName(levelId);
	level.levelId = levelId;
	level.gridRows = 6;
	level.gridCols = 9;
	level.maxLifePoint = 3;
	level.initialCost = 10;
	level.maxCost = 99;
	level.costIncreaseTime = 1.0f;
	level.isModified = true;

	// JSON 기본 구조 생성
	level.fullData = {
		{"options", {
			{"characterLimit", 8},
			{"maxLifePoint", level.maxLifePoint},
			{"initialCost", level.initialCost},
			{"maxCost", level.maxCost},
			{"costIncreaseTime", Snap1(static_cast<double>(level.costIncreaseTime))},
			{"moveMultiplier", 0.5},
			{"steeringEnabled", true},
			{"isTrainingLevel", false},
			{"isHardTrainingLevel", false},
			{"isPredefinedCardsSelectable", false},
			{"displayRestTime", false},
			{"maxPlayTime", -1.0},
			{"functionDisableMask", 0},
			{"configBlackBoard", json::array()},
			{"enemyTauntLevelPow", 0}
		}},
		{"levelId", nullptr},
		{"mapId", nullptr},
		{"bgmEvent", ""},
		{"environmentSe", nullptr},
		{"mapData", {
			{"map", json::array()},
			{"tiles", json::array()},
			{"blockEdges", json::array()},
			{"tags", json::array()},
			{"effects", json::array()},
			{"layerRects", json::array()}
		}},
		{"tilesDisallowToLocate", json::array()},
		{"runes", json::array()},
		{"globalBuffs", json::array()},
		{"routes", json::array()},
		{"enemies", json::array()},
		{"enemyDbRefs", json::array()},
		{"waves", json::array()},
		{"branches", json::object()},
		{"predefines", {
			{"characterInsts", json::array()},
			{"tokenInsts", json::array()},
			{"characterCards", json::array()},
			{"tokenCards", json::array()}
		}},
		{"hardPredefines", nullptr},
		{"excludeCharIdList", json::array()},
		{"randomSeed", 0},
		{"operaConfig", nullptr}
	};

	// 그리드 맵 초기화
	level.gridMap.clear();
	level.gridMap.resize(level.gridRows, std::vector<int>(level.gridCols, 0));

	// 타일 생성
	level.tiles.clear();
	for (int i = 0; i < level.gridRows * level.gridCols; i++)
	{
		level.tiles.push_back(CreateTileData(TileType::Forbidden));
	}

	SyncJsonFromGrid(level);
}

void LevelEditor::SyncGridFromJson(LevelData& level)
{
	if (!level.fullData.contains("mapData") || !level.fullData["mapData"].contains("map"))
		return;

	auto& mapArray = level.fullData["mapData"]["map"];

	if (mapArray.empty())
		return;

	level.gridRows = (int)mapArray.size();
	level.gridCols = (int)mapArray[0].size();

	// 그리드 맵 초기화
	level.gridMap.clear();
	level.gridMap.resize(level.gridRows, std::vector<int>(level.gridCols, 0));

	// 맵 데이터 복사
	for (int jsonRow = 0; jsonRow < level.gridRows; jsonRow++)
	{
		for (int col = 0; col < level.gridCols; col++)
		{
			level.gridMap[jsonRow][col] = mapArray[jsonRow][col];
		}
	}

	// 타일 배열 가져오기
	if (level.fullData["mapData"].contains("tiles"))
	{
		level.tiles.clear();
		for (auto& tile : level.fullData["mapData"]["tiles"])
		{
			level.tiles.push_back(tile);
		}
	}
}

void LevelEditor::SyncJsonFromGrid(LevelData& level)
{
	// 맵 배열 생성
	level.fullData["mapData"]["map"] = json::array();

	for (int jsonRow = 0; jsonRow < level.gridRows; jsonRow++)
	{
		json row = json::array();
		for (int col = 0; col < level.gridCols; col++)
		{
			int tileIndex = jsonRow * level.gridCols + col;
			row.push_back(tileIndex);
		}
		level.fullData["mapData"]["map"].push_back(row);
	}

	// 타일 배열 동기화
	level.fullData["mapData"]["tiles"] = level.tiles;
}

json LevelEditor::CreateTileData(TileType type)
{
	const char* tileKey = TileTypeToTileKey(type);

	json tile = {
		{"tileKey", tileKey},
		{"heightType", (type == TileType::HighGround) ? 1 : 0},
		{"buildableType",
			(type == TileType::Road || type == TileType::HighGround) ? 1 :
			(type == TileType::Wall) ? 2 : 0},
		{"passableMask",
			(type == TileType::Road || type == TileType::Start || type == TileType::End) ? 3 : 2},
		{"playerSideMask", 0},
		{"blackboard", json::array()},
		{"effects", json::array()}
	};

	return tile;
}

const char* LevelEditor::TileTypeToString(TileType type)
{
	switch (type)
	{
	case TileType::Forbidden: return "금지";
	case TileType::Road: return "도로";
	case TileType::Wall: return "벽";
	case TileType::Start: return "시작";
	case TileType::End: return "종료";
	case TileType::HighGround: return "고지";
	case TileType::Hole: return "구멍";
	default: return "알 수 없음";
	}
}

const char* LevelEditor::TileTypeToTileKey(TileType type)
{
	switch (type)
	{
	case TileType::Forbidden: return "tile_forbidden";
	case TileType::Road: return "tile_road";
	case TileType::Wall: return "tile_wall";
	case TileType::Start: return "tile_start";
	case TileType::End: return "tile_end";
	case TileType::HighGround: return "tile_highground";
	case TileType::Hole: return "tile_hole";
	default: return "tile_forbidden";
	}
}

int LevelEditor::GetTileColor(TileType type)
{
	switch (type)
	{
	case TileType::Forbidden: return IM_COL32(40, 40, 40, 255);       // 어두운 회색
	case TileType::Road: return IM_COL32(180, 140, 100, 255);         // 갈색
	case TileType::Wall: return IM_COL32(100, 100, 100, 255);         // 회색
	case TileType::Start: return IM_COL32(0, 255, 0, 255);            // 초록
	case TileType::End: return IM_COL32(255, 0, 0, 255);              // 빨강
	case TileType::HighGround: return IM_COL32(150, 180, 150, 255);   // 연두
	case TileType::Hole: return IM_COL32(20, 20, 60, 255);            // 어두운 파랑
	default: return IM_COL32(0, 0, 0, 255);                           // 검정
	}
}
