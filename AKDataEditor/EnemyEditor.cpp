#include "EnemyEditor.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_gdi.h>

#include "Utility.h"

namespace fs = std::filesystem;

EnemyEditor::EnemyEditor(const std::string& jsonPath)
	: _jsonPath(jsonPath)
{
	LoadEnemies();
}

EnemyEditor::~EnemyEditor() {}

void EnemyEditor::RenderGUI(bool* p_open)
{
	ImGui::Begin("적 편집기", p_open);

	RenderToolbar();
	ImGui::Separator();

	RenderEnemyList();

	ImGui::End();

	if (_showCreateWindow)
		RenderCreateWindow();

	if (_showEditWindow)
		RenderEditWindow();
}

void EnemyEditor::LoadEnemies()
{
	std::ifstream file(_jsonPath);
	if (file.is_open())
	{
		try
		{
			file >> _enemyData;
			std::cout << "EnemyData Loaded " << _jsonPath << '\n';

			int currentVersion = VERSION;
			int dataVersion = 0;

			if (_enemyData.contains("version"))
			{
				dataVersion = _enemyData["version"];
			}

			if (dataVersion < currentVersion)
			{
				bool migrated = false;

				if (migrated)
				{
					// 버전 업데이트
					_enemyData["version"] = currentVersion;

					std::cout << "[Migration] Auto-saving fixed values...\n";
					SaveEnemies();
					std::cout << "[Migration] Complete!\n";
				}
				else
				{
					// 변경사항 없어도 버전만 업데이트
					_enemyData["version"] = currentVersion;
					SaveEnemies();
				}
			}
		}
		catch (json::exception e)
		{
			std::cout << "JSON parse Error: " << e.what() << '\n';
			_enemyData = { {"version", VERSION}, {"enemies", json::array()} };
		}
	}
	else
	{
		std::cout << "File not found, creating new: " << _jsonPath << "\n";
		_enemyData = { {"version", VERSION}, {"enemies", json::array()} };
	}
}

void EnemyEditor::SaveEnemies()
{
	fs::path filePath(_jsonPath);
	fs::create_directories(filePath.parent_path());

	// version이 없으면 추가
	if (!_enemyData.contains("version"))
	{
		_enemyData["version"] = VERSION;
	}

	// 순서 보장: 새 JSON 생성
	json output;
	output["version"] = _enemyData["version"];
	output["enemies"] = _enemyData["enemies"];

	std::ofstream file(_jsonPath);
	if (file.is_open())
	{
		file << output.dump(2);  // output 저장
		std::cout << "Saved to " << _jsonPath << "\n";
	}
	else
	{
		std::cout << "Failed to save!\n";
	}
}

void EnemyEditor::RenderToolbar()
{
	// 저장 상태 표시
	if (_hasUnsavedChanges)
	{
		// 노란색 경고 아이콘 + 텍스트
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_YELLOW);
		ImGui::Text("* 저장되지 않은 변경사항");
		ImGui::PopStyleColor();

		ImGui::SameLine();

		// Save All 버튼
		if (ImGui::Button("전체 저장"))
		{
			SaveEnemies();
			_hasUnsavedChanges = false;
			std::cout << "[Enemy] Saved successfully!\n";
		}

		ImGui::SameLine();

		// Discard 버튼 (변경사항 취소)
		if (ImGui::Button("되돌리기"))
		{
			LoadEnemies();  // 파일에서 다시 로드
			_hasUnsavedChanges = false;
			std::cout << "[Enemy] Changes discarded.\n";
		}
	}
	else
	{
		// 초록색 체크 표시
		ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GREEN);
		ImGui::Text("저장완료");
		ImGui::PopStyleColor();
	}

	ImGui::SameLine();
	ImGui::Spacing();
	ImGui::SameLine();

	// Create New 버튼
	if (ImGui::Button("새로운 적 생성"))
	{
		_showCreateWindow = true;

		// 입력 버퍼 초기화
		memset(_inputEnemyKey, 0, sizeof(_inputEnemyKey));
		memset(_inputName, 0, sizeof(_inputName));
		_inputEnemyType = EnemyType::ENEMY_GROUND;
		_inputMaxHp = 100;
		_inputAtk = 50;
		_inputRangeRadius = -1.0f;
		_inputDef = 0;
		_inputMagicRes = 0;
		_inputMoveSpeed = 1.0f;
		_inputBaseAttackTime = 1.5f;
	}

	ImGui::SameLine();

	// Refresh 버튼
	if (ImGui::Button("새로고침"))
	{
		LoadEnemies();
		_hasUnsavedChanges = false;
	}
}

void EnemyEditor::RenderEnemyList()
{
	if (!_enemyData.contains("enemies") || _enemyData["enemies"].empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "적 데이터가 없습니다.");
		return;
	}

	// 테이블 플래그
	ImGuiTableFlags flags = ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY;

	if (ImGui::BeginTable("EnemyTable", 6, flags))
	{
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("ATK", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("Range", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableHeadersRow();

		int index = 0;
		for (const auto& enemy : _enemyData["enemies"])
		{
			ImGui::TableNextRow();

			ImU32 bg = (index % 2 == 0)
				? IM_COL32(25, 25, 25, 255)
				: IM_COL32(40, 40, 40, 255);

			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bg);

			// Key
			ImGui::TableNextColumn();
			std::string key = enemy["key"];
			ImGui::Text("%s", key.c_str());

			// Name
			ImGui::TableNextColumn();
			std::string name = enemy["value"][0]["enemyData"]["name"]["m_value"];
			ImGui::Text("%s", name.c_str());

			// HP
			ImGui::TableNextColumn();
			int hp = enemy["value"][0]["enemyData"]["attributes"]["maxHp"]["m_value"];
			ImGui::Text("%d", hp);

			// ATK
			ImGui::TableNextColumn();
			int atk = enemy["value"][0]["enemyData"]["attributes"]["atk"]["m_value"];
			ImGui::Text("%d", atk);

			// Range
			ImGui::TableNextColumn();
			float range = enemy["value"][0]["enemyData"]["rangeRadius"]["m_value"];
			ImGui::Text("%.1f", range);

			// Actions
			ImGui::TableNextColumn();

			ImGui::PushID(index);

			// Edit 버튼
			if (ImGui::SmallButton("편집"))
			{
				_selectedEnemyIndex = index;
				_showEditWindow = true;

				// 현재 입력값 백버퍼 저장
				strcpy_s(_inputEnemyKey, sizeof(_inputEnemyKey), key.c_str());
				strcpy_s(_inputName, sizeof(_inputName), name.c_str());
				_inputEnemyType = StringToEnemyType(enemy["value"][0]["enemyData"]["type"]["m_value"]);
				_inputMaxHp = hp;
				_inputAtk = atk;
				_inputRangeRadius = range;
				_inputDef = enemy["value"][0]["enemyData"]["attributes"]["def"]["m_value"];
				_inputMagicRes = enemy["value"][0]["enemyData"]["attributes"]["magicResistance"]["m_value"];
				_inputMoveSpeed = enemy["value"][0]["enemyData"]["attributes"]["moveSpeed"]["m_value"];
				_inputBaseAttackTime = enemy["value"][0]["enemyData"]["attributes"]["baseAttackTime"]["m_value"];
			}

			ImGui::SameLine();

			// 삭제 버튼
			if (ImGui::SmallButton("삭제"))
			{
				_deleteTargetIndex = index;      // 인덱스 저장
				_deleteTargetName = name;         // 이름 저장
				_showDeleteConfirm = true;        // 팝업 표시 플래그
			}

			ImGui::PopID();

			++index;
		}
		ImGui::EndTable();
	}

	if (_showDeleteConfirm)
	{
		ImGui::OpenPopup("삭제 확인");
		_showDeleteConfirm = false;  // 한 번만 OpenPopup 호출
		std::cout << "[DEBUG] Delete target: " << _deleteTargetName << " (index: " << _deleteTargetIndex << ")\n";
	}

	if (ImGui::BeginPopupModal("삭제 확인", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(COLOR_RED, "이 적을 정말 삭제할까요?");
		ImGui::Separator();

		ImGui::Text("대상: ");
		ImGui::SameLine();
		ImGui::TextColored(COLOR_YELLOW, "%s", _deleteTargetName.c_str());

		ImGui::Separator();

		if (ImGui::Button("예", ImVec2(120, 0)))
		{
			if (_deleteTargetIndex >= 0 && _deleteTargetIndex < (int)_enemyData["enemies"].size())
			{
				_enemyData["enemies"].erase(_enemyData["enemies"].begin() + _deleteTargetIndex);
				_hasUnsavedChanges = true;
				std::cout << "[Enemy] Deleted: " << _deleteTargetName << "\n";
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

void EnemyEditor::RenderCreateWindow()
{
	ImGui::Begin("새로운 적 생성", &_showCreateWindow);

	// 기본 정보
	ImGui::SeparatorText("기본 정보");
	ImGui::InputText("적 ID", _inputEnemyKey, 64);
	ImGui::InputText("이름", _inputName, 64);

	const char* enemyTypes[] = { "지상", "공중" };
	ImGui::Combo("타입", (int*)&_inputEnemyType, enemyTypes, IM_ARRAYSIZE(enemyTypes));

	ImGui::Separator();

	// 스텟
	ImGui::SeparatorText("능력치");
	ImGui::InputInt("최대 HP", &_inputMaxHp);
	ImGui::InputInt("공격력", &_inputAtk);
	ImGui::InputFloat("공격 범위", &_inputRangeRadius, 0.1f, 1.0f, "%.1f");
	ImGui::Text("(-1.0 = 근거리, 0.9~2.0 = 원거리)");

	ImGui::InputInt("방어력", &_inputDef);
	ImGui::SliderInt("마법 저항", &_inputMagicRes, 0, 100);
	ImGui::SameLine();
	ImGui::Text("%%");
	ImGui::InputFloat("이동 속도", &_inputMoveSpeed, 0.1f, 1.0f, "%.1f");
	ImGui::InputFloat("공격 속도 (초)", &_inputBaseAttackTime, 0.05f, 1.0f, "%.2f");

	ImGui::Separator();

	// 버튼
	if (ImGui::Button("생성", ImVec2(120, 0)))
	{
		if (strlen(_inputEnemyKey) == 0 || strlen(_inputName) == 0)
		{
			std::cout << "[Error] Key and Name are required!\n";
		}
		else
		{
			// JSON 생성
			json newEnemy = CreateEnemyDataStructure(
				_inputEnemyKey, _inputName, EnemyTypeToString(_inputEnemyType), _inputMaxHp, _inputAtk,
				_inputRangeRadius, _inputDef, _inputMagicRes,
				_inputMoveSpeed, _inputBaseAttackTime
			);

			_enemyData["enemies"].push_back(newEnemy);

			// 플래그 설정
			_hasUnsavedChanges = true;

			std::cout << "[Enemy] Created: " << _inputName << " (not saved yet)\n";

			_showCreateWindow = false;
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("취소", ImVec2(120, 0)))
	{
		_showCreateWindow = false;
	}

	ImGui::End();
}

void EnemyEditor::RenderEditWindow()
{
	if (_selectedEnemyIndex < 0 || _selectedEnemyIndex >= (int)_enemyData["enemies"].size())
	{
		_showEditWindow = false;
		return;
	}

	ImGui::Begin("적 편집", &_showEditWindow);

	auto& enemy = _enemyData["enemies"][_selectedEnemyIndex];
	auto& enemyData = enemy["value"][0]["enemyData"];
	auto& attrs = enemyData["attributes"];

	// key (읽기 전용)
	ImGui::Text("ID: %s", enemy["key"].get<std::string>().c_str());
	ImGui::Separator();

	// Name
	char nameBuffer[64];
	std::string name = enemyData["name"]["m_value"];
	strcpy_s(nameBuffer, name.c_str());

	if (ImGui::InputText("이름", nameBuffer, 128))
	{
		enemyData["name"]["m_value"] = std::string(nameBuffer);
		_hasUnsavedChanges = true;  // 변경 플래그
	}

	const char* enemyTypes[] = { "지상", "공중" };
	if (ImGui::Combo("타입", (int*)&_inputEnemyType, enemyTypes, IM_ARRAYSIZE(enemyTypes)))
	{
		enemyData["type"]["m_value"] = EnemyTypeToString(_inputEnemyType);
		_hasUnsavedChanges = true;
	}

	// HP
	int hp = attrs["maxHp"]["m_value"];
	if (ImGui::InputInt("최대 HP", &hp))
	{
		attrs["maxHp"]["m_value"] = hp;
		_hasUnsavedChanges = true;
	}

	// ATK
	int atk = attrs["atk"]["m_value"];
	if (ImGui::InputInt("공격력", &atk))
	{
		attrs["atk"]["m_value"] = atk;
		_hasUnsavedChanges = true;
	}

	// Range
	float range = enemyData["rangeRadius"]["m_value"];
	if (ImGui::InputFloat("공격 범위", &range, 0.1f, 1.0f, "%.1f"))
	{
		enemyData["rangeRadius"]["m_value"] = Snap1(static_cast<double>(range));
		_hasUnsavedChanges = true;
	}

	// DEF
	int def = attrs["def"]["m_value"];
	if (ImGui::InputInt("방어력", &def))
	{
		attrs["def"]["m_value"] = def;
		_hasUnsavedChanges = true;
	}

	// Magic Resistance
	int magicRes = static_cast<int>(attrs["magicResistance"]["m_value"].get<float>() * 100);
	if (ImGui::SliderInt("마법 저항", &magicRes, 0, 100))
	{
		attrs["magicResistance"]["m_value"] = Snap2(magicRes / 100.0);
		_hasUnsavedChanges = true;
	}
	ImGui::SameLine();
	ImGui::Text("%%");

	// Move Speed
	float moveSpeed = attrs["moveSpeed"]["m_value"];
	if (ImGui::InputFloat("이동 속도", &moveSpeed, 0.1f, 1.0f, "%.1f"))
	{
		attrs["moveSpeed"]["m_value"] = Snap1(static_cast<double>(moveSpeed));
		_hasUnsavedChanges = true;
	}

	// Base Attack Time
	float baseAttackTime = attrs["baseAttackTime"]["m_value"];
	if (ImGui::InputFloat("공격 속도 (초)", &baseAttackTime, 0.05f, 1.0f, "%.2f"))
	{
		attrs["baseAttackTime"]["m_value"] = Snap2(static_cast<double>(baseAttackTime));
		_hasUnsavedChanges = true;
	}

	ImGui::Separator();

	if (ImGui::Button("완료"))
	{
		_showEditWindow = false;
	}

	ImGui::End();
}

json EnemyEditor::CreateEnemyDataStructure(const std::string& key, const std::string& name, const std::string& type, int hp, int atk, float rangeRadius, int def, int magicRes, float moveSpeed, float baseAttackTime)
{
	return {
		{"key", key},
		{"value", json::array({
			{
				{"level", 0},
				{"enemyData", {
					{"name", {{"m_defined", true}, {"m_value", name}}},
					{"type", {{"m_defined", true}, {"m_value", type}}},
					{"attributes", {
						{"maxHp", {{"m_defined", true}, {"m_value", hp}}},
						{"atk", {{"m_defined", true}, {"m_value", atk}}},
						{"def", {{"m_defined", true}, {"m_value", def}}},
						{"magicResistance", {{"m_defined", true}, {"m_value", Snap2(magicRes / 100.0)}}},
						{"moveSpeed", {{"m_defined", true}, {"m_value", Snap1(static_cast<double>(moveSpeed))}}},
						{"baseAttackTime", {{"m_defined", true}, {"m_value", Snap2(static_cast<double>(baseAttackTime))}}}
					}},
					{"rangeRadius", {{"m_defined", true}, {"m_value", Snap1(static_cast<double>(rangeRadius))}}},
					{"skills", json::array()}
				}}
			}
		})}
	};
}

const char* EnemyEditor::EnemyTypeToString(EnemyType type)
{
	switch (type)
	{
	case EnemyEditor::EnemyType::ENEMY_GROUND:
		return "GROUND";
	case EnemyEditor::EnemyType::ENEMY_FLYING:
		return "FLYING";
	case EnemyEditor::EnemyType::ENEMY_MAX:
	default:
		return "GROUND";
	}
}

EnemyEditor::EnemyType EnemyEditor::StringToEnemyType(const std::string& str)
{
	return (str == "FLYING") ? EnemyType::ENEMY_FLYING : EnemyType::ENEMY_GROUND;
}
