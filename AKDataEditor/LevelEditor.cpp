#include "LevelEditor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_gdi.h>

#include "Utility.h"

namespace fs = std::filesystem;

LevelEditor::LevelEditor(std::string jsonPath, std::string solutionPath)
	: _jsonPath(jsonPath)
{
	LoadLevels();
	LoadEnemyTable(solutionPath);
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
			if (ImGui::SmallButton("편집"))
			{
				_selectedLevelIndex = index;
				_editMode = EditMode::Grid;  // 항상 그리드부터 시작
				_editModeChanged = true;
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

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 진행상태 표시
		if (level.gridCompleted)
		{
			ImGui::TextColored(COLOR_GREEN, "그리드 완료");
		}
		else
		{
			ImGui::TextColored(COLOR_GRAY, "그리드 미완료");
		}

		ImGui::SameLine();
		ImGui::Text("|");
		ImGui::SameLine();

		if (level.routeCompleted)
		{
			ImGui::TextColored(COLOR_GREEN, "경로 완료");
		}
		else
		{
			ImGui::TextColored(COLOR_GRAY, "경로 미완료");
		}

		ImGui::SameLine();
		ImGui::Text("|");
		ImGui::SameLine();

		if (level.waveCompleted)
		{
			ImGui::TextColored(COLOR_GREEN, "웨이브 완료");
		}
		else
		{
			ImGui::TextColored(COLOR_GRAY, "웨이브 미완료");
		}

		ImGui::EndMenuBar();
	}

	// 사용 가능한 영역 크기 계산
	ImVec2 availRegion = ImGui::GetContentRegionAvail();
	float buttonHeight = 30.0f; 
	float childHeight = availRegion.y - buttonHeight - 10.0f;  

	if (ImGui::BeginTabBar("LevelEditTabs"))
	{
		// ========== 그리드 탭 (항상 활성화) ==========
		ImGuiTabItemFlags gridFlags = 0;
		if (_editMode == EditMode::Grid && _editModeChanged)
			gridFlags = ImGuiTabItemFlags_SetSelected;

		if (ImGui::BeginTabItem("그리드", nullptr, gridFlags))
		{
			_editMode = EditMode::Grid;
			_editModeChanged = false;

			// 기존 레이아웃
			ImGui::BeginChild("GridPane", ImVec2(availRegion.x * 0.7f, childHeight), true);
			RenderGridEditor(level);
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("InfoPane", ImVec2(0, childHeight), true);
			RenderOptionsPanel(level);
			ImGui::Separator();
			RenderTileInspector(level);
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		// ========== 경로 탭 (Grid 완성 후에만 활성화) ==========
		ImGuiTabItemFlags routeFlags = 0;
		if (_editMode == EditMode::Route && _editModeChanged)
			routeFlags = ImGuiTabItemFlags_SetSelected;

		// Grid 완성 여부에 따라 활성화/비활성화
		if (level.gridCompleted)
		{
			if (ImGui::BeginTabItem("경로", nullptr, routeFlags))
			{
				_editMode = EditMode::Route;
				_editModeChanged = false;

				RenderRouteEditor(level);

				ImGui::EndTabItem();
			}
		}
		else
		{
			// 비활성화 상태로 표시
			ImGui::BeginDisabled();
			if (ImGui::BeginTabItem("경로"))
			{
				ImGui::EndTabItem();
			}
			ImGui::EndDisabled();
		}

		// ========== 웨이브 탭 (Route 완성 후에만 활성화) ==========
		ImGuiTabItemFlags waveFlags = 0;
		if (_editMode == EditMode::Wave && _editModeChanged)
			waveFlags = ImGuiTabItemFlags_SetSelected;

		// Route 완성 여부에 따라 활성화/비활성화
		if (level.routeCompleted)
		{
			if (ImGui::BeginTabItem("웨이브", nullptr, waveFlags))
			{
				_editMode = EditMode::Wave;
				_editModeChanged = false;

				RenderWaveEditor(level);

				ImGui::EndTabItem();
			}
		}
		else
		{
			// 비활성화 상태로 표시
			ImGui::BeginDisabled();
			if (ImGui::BeginTabItem("웨이브"))
			{
				ImGui::EndTabItem();
			}
			ImGui::EndDisabled();
		}

		ImGui::EndTabBar();
	}

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
		ImU32 color = GetTileColor((TileType)i);
		ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);
		ImGui::PushStyleColor(ImGuiCol_Text, colorVec);

		if (ImGui::RadioButton(tileTypes[i], (int)_selectedTileType == i))
		{
			_selectedTileType = (TileType)i;

			_selectedGridRow = -2;  // -2는 "브러시 선택" 상태를 의미
			_selectedGridCol = -2;
		}

		ImGui::PopStyleColor();

		if (i < (int)TileType::MAX - 1)
		{
			ImGui::SameLine();
		}
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

	// 그리드 그리기 
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

	ImGui::Separator();
	
	if (!level.gridCompleted)
	{
		ImGui::TextColored(COLOR_YELLOW, "그리드 편집을 완료하면 경로 편집을 시작할 수 있습니다.");

		if (ImGui::Button("그리드 편집 완료", ImVec2(120, 0)))
		{
			level.gridCompleted = true;
			level.isModified = true;
			_hasUnsavedChanges = true;

			std::cout << "[Level] Grid completed for " << level.levelId << "\n";

			// 자동으로 경로 탭으로 전환
			_editMode = EditMode::Route;
			_editModeChanged = true;
		}
	}
	else
	{
		ImGui::TextColored(COLOR_GREEN, "그리드 편집 완료됨");

		if (ImGui::Button("그리드 다시 편집", ImVec2(200, 0)))
		{
			level.gridCompleted = false;
			level.isModified = true;
			_hasUnsavedChanges = true;
		}
	}
}


void LevelEditor::RenderTileInspector(LevelData& level)
{
	ImGui::SeparatorText("타일 정보");

	if (_selectedGridRow == -2 && _selectedGridCol == -2)
	{
		// 선택된 브러시 정보 표시
		ImGui::TextColored(COLOR_YELLOW, "선택된 브러시:");

		json tempTile = CreateTileData(_selectedTileType);
		std::string tileKey = tempTile.value("tileKey", "tile_forbidden");
		int heightType = tempTile.value("heightType", 0);
		int buildableType = tempTile.value("buildableType", 0);
		int passableMask = tempTile.value("passableMask", 0);

		ImGui::Text("타일 종류: %s", tileKey.c_str());
		ImGui::Text("높이: %s", heightType == 0 ? "낮음" : "높음");
		ImGui::Text("배치 가능: %s",
			buildableType == 0 ? "불가" :
			buildableType == 1 ? "일반" : "벽 전용");
		ImGui::Text("통과 가능: %s", passableMask == 3 ? "가능" : "불가");
		return;
	}

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

	if (ImGui::InputInt("오퍼레이터 최대 배치 수", &level.characterLimit))
	{
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

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

void LevelEditor::RenderRouteEditor(LevelData& level)
{
	ImGui::SeparatorText("경로 편집");

	int routeCount = 0;
	if (level.fullData.contains("routes"))
	{
		routeCount = (int)level.fullData["routes"].size();
	}

	ImGui::Text("현재 경로 개수: %d", routeCount);

	if (_selectedRouteIndex >= 0 && _selectedRouteIndex < routeCount)
	{
		ImGui::SameLine();
		ImGui::TextColored(COLOR_YELLOW, " | 선택된 경로: Route %d", _selectedRouteIndex);
	}

	ImGui::Separator();

	ImVec2 availRegion = ImGui::GetContentRegionAvail();

	// 좌 30%
	ImGui::BeginChild("RouteList", ImVec2(availRegion.x * 0.3f, availRegion.y - 60), true);

	ImGui::Text("경로 목록");
	ImGui::Separator();

	if (routeCount == 0)
	{
		ImGui::TextColored(COLOR_GRAY, "경로가 없습니다.");
	}
	else
	{
		for (int i = 0; i < routeCount; ++i)
		{
			ImGui::PushID(i);

			char routeName[32];
			snprintf(routeName, sizeof(routeName), "Route %d", i);

			bool isSelected = (_selectedRouteIndex == i);
			if (isSelected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
			}

			if (ImGui::Selectable(routeName, false))
			{
				_selectedRouteIndex = i;
				std::cout << "[Route] Selected route " << i << "\n";
			}

			if (isSelected)
			{
				ImGui::PopStyleColor();
			}

			ImGui::PopID();
		}
	}

	ImGui::Separator();

	if (ImGui::Button("경로 추가", ImVec2(-1, 0)))
	{
		json newRoute = {
			{"motionMode", 0},  // 0=지상, 2=비행
			{"startPosition", {{"row", -1}, {"col", -1}}},
			{"endPosition", {{"row", -1}, {"col", -1}}},
			{"spawnRandomRange", {{"x", 0.0}, {"y", 0.0}}},
			{"spawnOffset", {{"x", 0.0}, {"y", 0.0}}},
			{"checkpoints", json::array()},
			{"allowDiagonalMove", true},
			{"visitEveryTileCenter", false},
			{"visitEveryNodeCenter", false},
			{"visitEveryCheckPoint", false}
		};

		level.fullData["routes"].push_back(newRoute);
		_selectedRouteIndex = routeCount;

		level.isModified = true;
		_hasUnsavedChanges = true;

		std::cout << "[Route] Added new route (total: " << (routeCount + 1) << ")\n";
	}

	if (_selectedRouteIndex >= 0 && _selectedRouteIndex < routeCount)
	{
		if (ImGui::Button("경로 삭제", ImVec2(-1, 0)))
		{
			_showRouteDeleteConfirm = true;
		}
	}
	
	ImGui::EndChild();

	ImGui::SameLine();

	// 우 70%
	ImGui::BeginChild("RouteEditArea", ImVec2(0, availRegion.y - 60), true);

	if (_selectedRouteIndex >= 0 && _selectedRouteIndex < routeCount)
	{
		auto& route = level.fullData["routes"][_selectedRouteIndex];

		// 경로 설정
		ImGui::Text("경로 설정");
		ImGui::Separator();

		int motionMode = route.value("motionMode", 0);
		const char* motionModes[] = { "지상", "비행" };
		int motionModeIndex = (motionMode == 2) ? 1 : 0;

		ImGui::Text("이동 모드:");
		ImGui::SameLine();
		if (ImGui::Combo("##MotionMode", &motionModeIndex, motionModes, 2))
		{
			route["motionMode"] = (motionModeIndex == 1) ? 2 : 0;
			level.isModified = true;
			_hasUnsavedChanges = true;
		}

		ImGui::Separator();

		ImGui::Text("타일 범례:");
		ImGui::Spacing();

		const char* tileTypes[] = { "금지", "도로", "벽", "시작", "종료", "고지", "구멍" };

		// 폰트 크기 1.2배로 키우기
		ImGui::SetWindowFontScale(1.2f);

		for (int i = 0; i < (int)TileType::MAX; i++)
		{
			ImU32 color = GetTileColor((TileType)i);
			ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);
			colorVec.x = colorVec.x * 0.6f + 0.4f;  // B
			colorVec.y = colorVec.y * 0.6f + 0.4f;  // G
			colorVec.z = colorVec.z * 0.6f + 0.4f;  // R
			colorVec.w = 1.0f;  // A

			// 색상 박스 그리기
			ImGui::PushStyleColor(ImGuiCol_Button, colorVec);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorVec);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorVec);
			ImGui::Button("##", ImVec2(20, 20));  
			ImGui::PopStyleColor(3);

			ImGui::SameLine(0.0f, 5.0f);  

			// 텍스트
			ImGui::PushStyleColor(ImGuiCol_Text, colorVec);
			ImGui::Text("%s", tileTypes[i]);
			ImGui::PopStyleColor();

			if (i < (int)TileType::MAX - 1)
			{
				ImGui::SameLine(0.0f, 15.0f);
			}
		}

		// 폰트 크기 원래대로
		ImGui::SetWindowFontScale(1.0f);

		ImGui::Separator();

		if (!_routeEditMode)
		{
			if (ImGui::Button("경로 편집 시작", ImVec2(-1, 0)))
			{
				_routeEditMode = true;
				_routeEditStep = RouteEditStep::SetStart;
				std::cout << "[Route] Route edit mode started\n";
			}

			auto& startPos = route["startPosition"];
			auto& endPos = route["endPosition"];
			int startRow = startPos.value("row", 0);
			int startCol = startPos.value("col", 0);
			int endRow = endPos.value("row", 0);
			int endCol = endPos.value("col", 0);

			ImGui::Separator();
			ImGui::Text("시작 위치: (%d, %d)", startCol, startRow);
			ImGui::Text("종료 위치: (%d, %d)", endCol, endRow);
		}
		else
		{
			// 현재 단계 표시
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
			if (ImGui::Button("경로 편집 종료", ImVec2(-1, 0)))
			{
				_routeEditMode = false;
				std::cout << "[Route] Route edit mode ended\n";
			}
			ImGui::PopStyleColor();

			ImGui::Separator();

			if (_routeEditStep == RouteEditStep::SetStart)
			{
				ImGui::TextColored(COLOR_GREEN, "1단계: 시작 위치를 그리드에서 클릭하세요");
			}
			else if (_routeEditStep == RouteEditStep::SetEnd)
			{
				ImGui::TextColored(COLOR_GREEN, "2단계: 종료 위치를 그리드에서 클릭하세요");
			}
			else if (_routeEditStep == RouteEditStep::AddCheckpoints)
			{
				ImGui::TextColored(COLOR_GREEN, "3단계: 체크포인트를 그리드에서 클릭하세요");
				ImGui::TextColored(COLOR_YELLOW, "우클릭: 마지막 체크포인트 제거");
			}
		}

		ImGui::Separator();

		// 체크포인트 목록
		ImGui::Text("체크포인트");

		int checkpointCount = (int)route["checkpoints"].size();
		ImGui::Text("개수: %d", checkpointCount);

		ImGui::Separator();

		// 그리드 시각화
		ImGui::Separator();
		ImGui::Text("경로 미리보기");
		ImGui::Separator();

		RenderRouteOnGrid(level, route);
	}
	else
	{
		ImGui::TextColored(COLOR_GRAY, "경로를 선택하세요.");
	}

	ImGui::EndChild();

	// 완료 버튼
	ImGui::Separator();

	if (!level.routeCompleted)
	{
		ImGui::TextColored(COLOR_YELLOW, "경로 편집을 완료하면 웨이브 편집을 시작할 수 있습니다.");

		if (routeCount > 0)
		{
			if (ImGui::Button("경로 편집 완료", ImVec2(200, 0)))
			{
				// 경로 편집 모드 종료
				_routeEditMode = false;
				_selectedRouteIndex = -1;

				level.routeCompleted = true;
				level.isModified = true;
				_hasUnsavedChanges = true;

				std::cout << "[Level] Route Completed for " << level.levelId << '\n';

				_editMode = EditMode::Wave;
				_editModeChanged = true;
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("경로 편집 완료", ImVec2(200, 0));
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::TextColored(COLOR_RED, "최소 1개 이상의 경로가 필요합니다.");
		}
	}
	else
	{
		ImGui::TextColored(COLOR_GREEN, "경로 편집 완료됨");

		if (ImGui::Button("경로 다시 편집", ImVec2(200, 0)))
		{
			level.routeCompleted = false;
			level.isModified = true;
			_hasUnsavedChanges = true;
		}
	}

	// 삭제 팝업
	if (_showRouteDeleteConfirm)
	{
		ImGui::OpenPopup("경로 삭제 확인");
		_showRouteDeleteConfirm = false;
	}

	if (ImGui::BeginPopupModal("경로 삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(COLOR_RED, "이 경로를 정말 삭제할까요?");
		ImGui::Separator();

		ImGui::Text("대상: Route %d", _selectedRouteIndex);

		ImGui::Separator();

		if (ImGui::Button("예", ImVec2(120, 0)))
		{
			level.fullData["routes"].erase(level.fullData["routes"].begin() + _selectedRouteIndex);
			_selectedRouteIndex = -1;
			level.isModified = true;
			_hasUnsavedChanges = true;

			std::cout << "[Route] Route deleted\n";
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("아니요", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void LevelEditor::RenderRouteOnGrid(LevelData& level, json& route)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();

	float cellSize = std::min(
		canvas_size.x / level.gridCols,
		canvas_size.y / level.gridRows
	) * 0.9f;

	// 그리드 배경 + 타일 정보 그리기
	for (int jsonRow = 0; jsonRow < level.gridRows; ++jsonRow)
	{
		int gameRow = JsonIndexToGameRow(jsonRow, level.gridRows);

		for (int col = 0; col < level.gridCols; ++col)
		{
			ImVec2 p_min(
				canvas_pos.x + col * cellSize,
				canvas_pos.y + jsonRow * cellSize
			);
			ImVec2 p_max(p_min.x + cellSize, p_min.y + cellSize);

			// 타일 타입에 따른 색상 (연하게)
			int tileIndex = jsonRow * level.gridCols + col;
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

			// 타일 색상 (투명도 낮춤)
			ImU32 baseColor = GetTileColor(tileType);
			ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(baseColor);
			colorVec.x = colorVec.x * 0.6f + 0.4f;  // B
			colorVec.y = colorVec.y * 0.6f + 0.4f;  // G
			colorVec.z = colorVec.z * 0.6f + 0.4f;  // R
			colorVec.w = 1.0f;  // A
			ImU32 tileColor = ImGui::ColorConvertFloat4ToU32(colorVec);

			draw_list->AddRectFilled(p_min, p_max, tileColor);
			draw_list->AddRect(p_min, p_max, IM_COL32(100, 100, 100, 255));

			// 마우스 클릭 처리 (경로 편집 모드일 때만)
			if (_routeEditMode && ImGui::IsMouseClicked(0))
			{
				ImVec2 mouse = ImGui::GetMousePos();
				if (mouse.x >= p_min.x && mouse.x <= p_max.x &&
					mouse.y >= p_min.y && mouse.y <= p_max.y)
				{
					if (_routeEditStep == RouteEditStep::SetStart)
					{
						// 시작 위치 설정
						route["startPosition"]["row"] = gameRow;
						route["startPosition"]["col"] = col;
						level.isModified = true;
						_hasUnsavedChanges = true;

						std::cout << "[Route] Start position set to (" << col << ", " << gameRow << ")\n";
						_routeEditStep = RouteEditStep::SetEnd;
					}
					else if (_routeEditStep == RouteEditStep::SetEnd)
					{
						// 종료 위치 설정
						route["endPosition"]["row"] = gameRow;
						route["endPosition"]["col"] = col;
						level.isModified = true;
						_hasUnsavedChanges = true;

						std::cout << "[Route] End position set to (" << col << ", " << gameRow << ")\n";
						_routeEditStep = RouteEditStep::AddCheckpoints;
					}
					else if (_routeEditStep == RouteEditStep::AddCheckpoints)
					{
						// 체크포인트 추가
						json newCheckpoint = {
							{"type", 0},
							{"time", 0.0},
							{"position", {{"row", gameRow}, {"col", col}}},
							{"reachOffset", {{"x", 0.0}, {"y", 0.0}}},
							{"randomizeReachOffset", false},
							{"reachDistance", 0.0}
						};

						route["checkpoints"].push_back(newCheckpoint);
						level.isModified = true;
						_hasUnsavedChanges = true;

						std::cout << "[Route] Added checkpoint at (" << col << ", " << gameRow << ")\n";
					}
				}
			}

			// 우클릭 Undo - 그리드 전체에서 동작하도록 수정
			if (_routeEditMode && ImGui::IsMouseClicked(1))
			{
				// 캔버스 영역 체크
				ImVec2 mouse = ImGui::GetMousePos();
				ImVec2 canvas_end(
					canvas_pos.x + level.gridCols * cellSize,
					canvas_pos.y + level.gridRows * cellSize
				);

				if (mouse.x >= canvas_pos.x && mouse.x <= canvas_end.x &&
					mouse.y >= canvas_pos.y && mouse.y <= canvas_end.y)
				{
					if (_routeEditStep == RouteEditStep::AddCheckpoints && !route["checkpoints"].empty())
					{
						// 체크포인트 제거
						route["checkpoints"].erase(route["checkpoints"].end() - 1);
						level.isModified = true;
						_hasUnsavedChanges = true;
						std::cout << "[Route] Undo - removed last checkpoint\n";
					}
					else if (_routeEditStep == RouteEditStep::AddCheckpoints && route["checkpoints"].empty())
					{
						// 체크포인트가 없으면 종료 위치 제거
						route["endPosition"]["row"] = -1;
						route["endPosition"]["col"] = -1;
						_routeEditStep = RouteEditStep::SetEnd;
						level.isModified = true;
						_hasUnsavedChanges = true;
						std::cout << "[Route] Undo - removed end position\n";
					}
					else if (_routeEditStep == RouteEditStep::SetEnd)
					{
						// 종료 위치 단계에서는 시작 위치로 돌아감
						route["endPosition"]["row"] = -1;
						route["endPosition"]["col"] = -1;
						_routeEditStep = RouteEditStep::SetStart;
						level.isModified = true;
						_hasUnsavedChanges = true;
						std::cout << "[Route] Undo - back to start position\n";
					}
					else if (_routeEditStep == RouteEditStep::SetStart &&
						route["startPosition"]["row"].get<int>() != -1)
					{
						// 시작 위치 제거
						route["startPosition"]["row"] = -1;
						route["startPosition"]["col"] = -1;
						level.isModified = true;
						_hasUnsavedChanges = true;
						std::cout << "[Route] Undo - removed start position\n";
					}
				}
			}
		}
	}

	// 시작 위치 그리기 (초록 원)
	auto& startPos = route["startPosition"];
	int startRow = startPos.value("row", 0);
	int startCol = startPos.value("col", 0);
	int startJsonRow = GameRowToJsonIndex(startRow, level.gridRows);

	ImVec2 startCenter(
		canvas_pos.x + (startCol + 0.5f) * cellSize,
		canvas_pos.y + (startJsonRow + 0.5f) * cellSize
	);
	draw_list->AddCircleFilled(startCenter, cellSize * 0.3f, IM_COL32(0, 255, 0, 255));
	draw_list->AddText(ImVec2(startCenter.x - 10, startCenter.y - 10), IM_COL32(255, 255, 255, 255), "S");

	// 종료 위치 그리기 (빨간 원)
	auto& endPos = route["endPosition"];
	int endRow = endPos.value("row", 0);
	int endCol = endPos.value("col", 0);
	int endJsonRow = GameRowToJsonIndex(endRow, level.gridRows);

	ImVec2 endCenter(
		canvas_pos.x + (endCol + 0.5f) * cellSize,
		canvas_pos.y + (endJsonRow + 0.5f) * cellSize
	);
	draw_list->AddCircleFilled(endCenter, cellSize * 0.3f, IM_COL32(255, 0, 0, 255));
	draw_list->AddText(ImVec2(endCenter.x - 10, endCenter.y - 10), IM_COL32(255, 255, 255, 255), "E");

	// 체크포인트 그리기 (노란 원 + 번호)
	auto& checkpoints = route["checkpoints"];
	for (size_t i = 0; i < checkpoints.size(); ++i)
	{
		auto& cp = checkpoints[i];
		int cpRow = cp["position"].value("row", 0);
		int cpCol = cp["position"].value("col", 0);
		int cpJsonRow = GameRowToJsonIndex(cpRow, level.gridRows);

		ImVec2 cpCenter(
			canvas_pos.x + (cpCol + 0.5f) * cellSize,
			canvas_pos.y + (cpJsonRow + 0.5f) * cellSize
		);

		draw_list->AddCircleFilled(cpCenter, cellSize * 0.25f, IM_COL32(255, 255, 0, 255));

		char cpText[8];
		snprintf(cpText, sizeof(cpText), "%d", (int)i);
		draw_list->AddText(ImVec2(cpCenter.x - 5, cpCenter.y - 8), IM_COL32(0, 0, 0, 255), cpText);
	}

	// 시작 위치가 유효한 경우에만 그리기
	if (startRow >= 0 && startCol >= 0)
	{
		ImVec2 prevCenter = startCenter;

		// 종료 위치까지 선이 설정되어 있으면 시작→종료 선 그리기
		if (_routeEditStep == RouteEditStep::AddCheckpoints ||
			_routeEditStep == RouteEditStep::SetEnd)
		{
			// 체크포인트들 연결
			for (auto& cp : checkpoints)
			{
				int cpRow = cp["position"].value("row", 0);
				int cpCol = cp["position"].value("col", 0);
				int cpJsonRow = GameRowToJsonIndex(cpRow, level.gridRows);

				ImVec2 cpCenter(
					canvas_pos.x + (cpCol + 0.5f) * cellSize,
					canvas_pos.y + (cpJsonRow + 0.5f) * cellSize
				);

				draw_list->AddLine(prevCenter, cpCenter, IM_COL32(100, 200, 255, 255), 2.0f);
				prevCenter = cpCenter;
			}

			// 종료 위치가 유효하면 마지막 선 그리기
			if (endRow >= 0 && endCol >= 0)
			{
				draw_list->AddLine(prevCenter, endCenter, IM_COL32(100, 200, 255, 255), 2.0f);
			}
		}
	}
	
	// 캔버스 영역
	ImGui::InvisibleButton("routeCanvas", ImVec2(level.gridCols * cellSize, level.gridRows * cellSize));
}

void LevelEditor::RenderWaveEditor(LevelData& level)
{
	ImGui::SeparatorText("웨이브 편집");

	// 웨이브 개수 표시
	int waveCount = 0;
	if (level.fullData.contains("waves"))
	{
		waveCount = (int)level.fullData["waves"].size();
	}

	ImGui::Text("현재 웨이브 개수: %d", waveCount);
	ImGui::Separator();

	// 좌 30%
	ImVec2 availRegion = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("WaveList", ImVec2(availRegion.x * 0.3f, availRegion.y - 60), true);
	RenderWaveList(level, waveCount);
	ImGui::EndChild();

	ImGui::BeginChild("WaveEditArea", ImVec2(availRegion.x * 0.3f, availRegion.y - 60), true);
	if (_selectedWaveIndex >= 0 && _selectedWaveIndex < waveCount)
	{
		auto& wave = level.fullData["waves"][_selectedWaveIndex];

		// wave 설정
		RenderWaveSetting(level, wave);

		ImGui::Separator();

		RenderFragmentList(wave);

		ImGui::Separator();

		// fragment 편집
		int fragmentCount = (int)wave["fragments"].size();
		if (_selectedFragmentIndex >= 0 && _selectedFragmentIndex < fragmentCount)
		{
			auto& fragment = wave["fragments"][_selectedFragmentIndex];
			RenderFragmentEditor(level, fragment);

			ImGui::Separator();

			RenderEnemySelector(level, fragment);
		}
	}
	else
	{
		ImGui::TextColored(COLOR_GRAY, "웨이브를 선택하세요.");
	}

	ImGui::EndChild();

	ImGui::Separator();

	if (!level.waveCompleted)
	{
		ImGui::TextColored(COLOR_YELLOW, "웨이브 편집을 완료하면 레벨 제작이 완료됩니다.");

		if (waveCount > 0)
		{
			if (ImGui::Button("웨이브 편집 완료", ImVec2(200, 0)))
			{
				_selectedWaveIndex = -1;
				_selectedFragmentIndex = -1;
				_selectedActionIndex = -1;

				level.waveCompleted = true;
				level.isModified = true;
				_hasUnsavedChanges = true;

				std::cout << "[Level] Wave completed for " << level.levelId << "\n";
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("웨이브 편집 완료", ImVec2(200, 0));
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::TextColored(COLOR_RED, "최소 1개 이상의 웨이브가 필요합니다.");
		}
	}
	else
	{
		ImGui::TextColored(COLOR_GREEN, "웨이브 편집 완료됨");
		ImGui::TextColored(COLOR_GREEN, "레벨 제작 완료!");

		if (ImGui::Button("웨이브 다시 편집", ImVec2(200, 0)))
		{
			level.waveCompleted = false;
			level.isModified = true;
			_hasUnsavedChanges = true;
		}
	}

	// 삭제 확인 팝업
	if (_showWaveDeleteConfirm)
	{
		ImGui::OpenPopup("웨이브 삭제 확인");
		_showWaveDeleteConfirm = false;
	}

	if (ImGui::BeginPopupModal("웨이브 삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(COLOR_RED, "이 웨이브를 정말 삭제할까요?");
		ImGui::Separator();
		ImGui::Text("대상: Wave %d", _selectedWaveIndex);
		ImGui::Separator();

		if (ImGui::Button("예", ImVec2(120, 0)))
		{
			level.fullData["waves"].erase(level.fullData["waves"].begin() + _selectedWaveIndex);
			_selectedWaveIndex = -1;
			_selectedFragmentIndex = -1;
			level.isModified = true;
			_hasUnsavedChanges = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("아니요", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (_showFragmentDeleteConfirm)
	{
		ImGui::OpenPopup("Fragment 삭제 확인");
		_showFragmentDeleteConfirm = false;
	}

	if (ImGui::BeginPopupModal("Fragment 삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(COLOR_RED, "이 Fragment를 정말 삭제할까요?");
		ImGui::Separator();
		ImGui::Text("대상: Fragment %d", _selectedFragmentIndex);
		ImGui::Separator();

		if (ImGui::Button("예", ImVec2(120, 0)))
		{
			auto& wave = level.fullData["waves"][_selectedWaveIndex];
			wave["fragments"].erase(wave["fragments"].begin() + _selectedFragmentIndex);
			_selectedFragmentIndex = -1;
			level.isModified = true;
			_hasUnsavedChanges = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("아니요", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void LevelEditor::RenderWaveList(LevelData& level, int waveCount)
{
	ImGui::Text("웨이브 목록");
	ImGui::Separator();

	if (waveCount == 0)
	{
		ImGui::TextColored(COLOR_GRAY, "웨이브가 없습니다.");
	}
	else
	{
		for (int i = 0; i < waveCount; ++i)
		{
			ImGui::PushID(i);

			char waveName[32];
			snprintf(waveName, sizeof(waveName), "Wave %d", i);

			bool isSelected = (_selectedWaveIndex == i);
			if (isSelected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
			}
			
			if (ImGui::Selectable(waveName, isSelected))
			{
				_selectedWaveIndex = i;
				_selectedFragmentIndex = -1;
				_selectedActionIndex = -1;
			}

			if (isSelected)
			{
				ImGui::PopStyleColor();
			}

			ImGui::PopID();
		}
	}

	ImGui::Separator();

	if (ImGui::Button("웨이브 추가", ImVec2(-1, 0)))
	{
		json newWave = {
			{"preDelay", 0.0},
			{"postDelay", 0.0},
			{"maxTimeWaitingForNextWave", -1.0},
			{"fragments", json::array()},
			{"advancedWaveTag", nullptr}
		};

		level.fullData["waves"].push_back(newWave);
		_selectedWaveIndex = waveCount;
		_selectedFragmentIndex = -1;
		_selectedActionIndex = -1;

		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	if (_selectedWaveIndex >= 0 && _selectedWaveIndex < waveCount)
	{
		if (ImGui::Button("웨이브 삭제", ImVec2(-1, 0)))
		{
			_showWaveDeleteConfirm = true;
		}
	}
}

void LevelEditor::RenderWaveSetting(LevelData& level, json& wave)
{
	ImGui::Text("웨이브 설정");
	ImGui::Separator();

	float preDelay = wave.value("preDelay", 0.0f);
	float postDelay = wave.value("postDelay", 0.0f);

	ImGui::PushItemWidth(150);
	if (ImGui::InputFloat("시작 지연 시간", &preDelay, 0.1f, 1.0f, "%.1f"))
	{
		wave["preDelay"] = preDelay;
		level.isModified = true;
		_hasUnsavedChanges = true;
	}

	if (ImGui::InputFloat("종료 지연 시간", &postDelay, 0.1f, 1.0f, "%.1f"))
	{
		wave["postDelay"] = postDelay;
		level.isModified = true;
		_hasUnsavedChanges = true;
	}
	ImGui::PopItemWidth();
}

void LevelEditor::RenderFragmentList(json& wave)
{
	int fragmentCount = (int)wave["fragments"].size();
	ImGui::Text("Fragment 개수: %d", fragmentCount);

	if (_selectedFragmentIndex >= 0 && _selectedFragmentIndex < fragmentCount)
	{
		ImGui::SameLine();
		ImGui::TextColored(COLOR_YELLOW, " | 선택됨: Fragment %d", _selectedFragmentIndex);
	}

	ImGui::Separator();

	ImGui::BeginChild("FragmentList", ImVec2(0, 150), true);

	if (fragmentCount == 0)
	{
		ImGui::TextColored(COLOR_GRAY, "Fragment가 없습니다.");
	}
	else
	{
		for (int i = 0; i < fragmentCount; ++i)
		{
			ImGui::PushID(i);

			auto& frag = wave["fragments"][i];
			int actionCount = (int)frag["actions"].size();

			char fragName[64];
			snprintf(fragName, sizeof(fragName), "Fragment %d (적: %d)", i, actionCount);

			bool isSelected = (_selectedFragmentIndex == i);
			if (isSelected)
			{
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.5f, 0.6f, 1.0f));
			}

			if (ImGui::Selectable(fragName, isSelected))
			{
				_selectedFragmentIndex = i;
				_selectedActionIndex = -1;
			}

			if (isSelected)
			{
				ImGui::PopStyleColor();
			}

			ImGui::PopID();
		}
	}

	ImGui::EndChild();

	if (ImGui::Button("Fragment 추가", ImVec2(150, 0)))
	{
		json newFragment = {
			{"preDelay", 0.0},
			{"actions", json::array()}
		};

		wave["fragments"].push_back(newFragment);
		_selectedFragmentIndex = fragmentCount;
		_selectedActionIndex = -1;
	}

	ImGui::SameLine();

	if (_selectedFragmentIndex >= 0 && _selectedFragmentIndex < fragmentCount)
	{
		if (ImGui::Button("Fragment 삭제", ImVec2(150, 0)))
		{
			_showFragmentDeleteConfirm = true;
		}
	}
}

void LevelEditor::RenderFragmentEditor(LevelData& level, json& fragment)
{
	ImGui::Text("Fragment %d 편집", _selectedFragmentIndex);
	ImGui::Separator();

	// Fragment 설정
	float fragPreDelay = fragment.value("preDelay", 0.0f);
	ImGui::PushItemWidth(150);
	if (ImGui::InputFloat("Fragment 시작 지연", &fragPreDelay, 0.1f, 1.0f, "%.1f"))
	{
		fragment["preDelay"] = fragPreDelay;
		level.isModified = true;
		_hasUnsavedChanges = true;
	}
	ImGui::PopItemWidth();

	ImGui::Separator();

	// Action 목록
	int actionCount = (int)fragment["actions"].size();
	ImGui::Text("적 스폰 목록: %d", actionCount);
	ImGui::Separator();

	ImGui::BeginChild("ActionList", ImVec2(0, 200), true);

	if (actionCount == 0)
	{
		ImGui::TextColored(COLOR_GRAY, "적이 없습니다.");
	}
	else
	{
		for (int i = 0; i < actionCount; ++i)
		{
			ImGui::PushID(i);

			auto& action = fragment["actions"][i];
			std::string key = action.value("key", "unknown");
			int count = action.value("count", 1);
			int routeIndex = action.value("routeIndex", 0);

			char actionText[128];
			snprintf(actionText, sizeof(actionText), "%d. %s x%d (경로:%d)",
				i, key.c_str(), count, routeIndex);

			ImGui::BulletText("%s", actionText);

			ImGui::SameLine(400);
			if (ImGui::SmallButton("삭제"))
			{
				fragment["actions"].erase(fragment["actions"].begin() + i);
				level.isModified = true;
				_hasUnsavedChanges = true;
				ImGui::PopID();
				break;
			}

			ImGui::PopID();
		}
	}

	ImGui::EndChild();
}

void LevelEditor::RenderEnemySelector(LevelData& level, json& fragment)
{
	ImGui::Text("새 적 추가");
	ImGui::Separator();

	// Combo로 적 선택
	if (_enemyKeys.empty())
	{
		ImGui::TextColored(COLOR_RED, "enemy_table.json을 로드할 수 없습니다!");
		ImGui::Text("경로: gamedata/tables/enemy_table.json");
	}
	else
	{
		ImGui::Text("적 선택:");

		// Combo 미리보기용 텍스트
		const char* preview = _selectedEnemyIndex >= 0 && _selectedEnemyIndex < (int)_enemyKeys.size()
			? _enemyKeys[_selectedEnemyIndex].c_str()
			: "적을 선택하세요";

		if (ImGui::BeginCombo("##EnemyCombo", preview))
		{
			for (int i = 0; i < (int)_enemyKeys.size(); ++i)
			{
				bool isSelected = (_selectedEnemyIndex == i);
				if (ImGui::Selectable(_enemyKeys[i].c_str(), isSelected))
				{
					_selectedEnemyIndex = i;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		static int inputCount = 1;
		static int inputRouteIndex = 0;
		static float inputPreDelay = 0.0f;
		static float inputInterval = 0.5f;

		ImGui::PushItemWidth(150);
		ImGui::InputInt("개수", &inputCount);
		inputCount = std::max(1, inputCount);

		// 경로 선택
		int routeCount = level.fullData.contains("routes") ?
			(int)level.fullData["routes"].size() : 0;

		if (routeCount > 0)
		{
			ImGui::InputInt("경로 번호", &inputRouteIndex);
			inputRouteIndex = std::max(0, std::min(routeCount - 1, inputRouteIndex));
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::InputInt("경로 번호", &inputRouteIndex);
			ImGui::EndDisabled();
			ImGui::TextColored(COLOR_RED, "경로가 없습니다!");
		}

		ImGui::InputFloat("시작 지연", &inputPreDelay, 0.1f, 1.0f, "%.1f");
		ImGui::InputFloat("스폰 간격", &inputInterval, 0.1f, 1.0f, "%.1f");
		ImGui::PopItemWidth();

		ImGui::Separator();

		if (_selectedEnemyIndex >= 0 && routeCount > 0)
		{
			if (ImGui::Button("적 추가", ImVec2(150, 0)))
			{
				json newAction = {
					{"actionType", 0},
					{"managedByScheduler", true},
					{"key", _enemyKeys[_selectedEnemyIndex]},
					{"count", inputCount},
					{"preDelay", inputPreDelay},
					{"interval", inputInterval},
					{"routeIndex", inputRouteIndex},
					{"blockFragment", false},
					{"autoPreviewRoute", false},
					{"autoDisplayEnemyInfo", false},
					{"isUnharmfulAndAlwaysCountAsKilled", false},
					{"hiddenGroup", nullptr},
					{"randomSpawnGroupKey", nullptr},
					{"randomSpawnGroupPackKey", nullptr},
					{"randomType", 0},
					{"refreshType", 0},
					{"weight", 0},
					{"dontBlockWave", false},
					{"forceBlockWaveInBranch", false},
					{"isValid", false},
					{"notCountInTotal", false},
					{"extraMeta", nullptr},
					{"actionId", nullptr}
				};

				fragment["actions"].push_back(newAction);
				level.isModified = true;
				_hasUnsavedChanges = true;

				// 입력 초기화
				inputCount = 1;
				inputPreDelay = 0.0f;
				inputInterval = 0.5f;

				std::cout << "[Wave] Added enemy: " << _enemyKeys[_selectedEnemyIndex] << "\n";
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("적 추가", ImVec2(150, 0));
			ImGui::EndDisabled();
		}
	}
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

void LevelEditor::LoadEnemyTable(std::string solutionPath)
{
	_enemyKeys.clear();

	std::string enemyTablePath = solutionPath + "/gamedata/tables/enemies_table.json";

	std::ifstream file(enemyTablePath);
	if (!file.is_open())
	{
		std::cout << "[Level] enemies_table.json not found at: " << enemyTablePath << "\n";
		return;
	}

	try
	{
		json enemyTable;
		file >> enemyTable;

		if (enemyTable.contains("enemies") && enemyTable["enemies"].is_array())
		{
			for (auto& enemy : enemyTable["enemies"])
			{
				if (enemy.contains("key"))
				{
					std::string key = enemy["key"].get<std::string>();
					_enemyKeys.push_back(key);
				}
			}
		}

		std::sort(_enemyKeys.begin(), _enemyKeys.end());

		std::cout << "[level] Loaded " << _enemyKeys.size() << " enemies from enemy_table.json\n";
	}
	catch (json::exception& e)
	{
		std::cout << "[level] Failed to parse enemies_table.json: " << e.what() << "\n";
	}
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
				level.characterLimit = opts.value("characterLimit", 8);
				level.maxLifePoint = opts.value("maxLifePoint", 3);
				level.initialCost = opts.value("initialCost", 10);
				level.maxCost = opts.value("maxCost", 99);
				level.costIncreaseTime = opts.value("costIncreaseTime", 1.0f);
			}

			// 커스텀 데이터 불러오기
			if (level.fullData.contains("editorMetadata"))
			{
				auto& meta = level.fullData["editorMetadata"];
				level.gridCompleted = meta.value("gridCompleted", false);
				level.routeCompleted = meta.value("routeCompleted", false);
				level.waveCompleted = meta.value("waveCompleted", false);
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
	saveData["options"]["characterLimit"] = level.characterLimit;
	saveData["options"]["maxLifePoint"] = level.maxLifePoint;
	saveData["options"]["initialCost"] = level.initialCost;
	saveData["options"]["maxCost"] = level.maxCost;
	saveData["options"]["costIncreaseTime"] = Snap1(static_cast<double>(level.costIncreaseTime));

	// 맵 데이터 업데이트
	saveData["mapData"] = level.fullData["mapData"];

	// 커스텀 필드
	saveData["editorMetadata"] = {
		{"gridCompleted", level.gridCompleted},
		{"routeCompleted", level.routeCompleted},
		{"waveCompleted", level.waveCompleted}
	};

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
	level.characterLimit = 8;
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
			{"characterLimit", level.characterLimit},
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
