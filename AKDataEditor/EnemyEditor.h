#pragma once
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

class EnemyEditor
{
public:
	EnemyEditor(const std::string& jsonPath);
	~EnemyEditor();

	void RenderGUI(bool* p_open);

	void LoadEnemies();
	void SaveEnemies();

	bool HasUnsavedChanges() const { return _hasUnsavedChanges; }
	void ClearUnsavedFlag() { _hasUnsavedChanges = false; }

private:
	enum class EnemyType
	{
		ENEMY_GROUND = 0,
		ENEMY_FLYING = 1,

		ENEMY_MAX
	};

private:
	std::string _jsonPath;
	json _enemyData;

	// 변경 사항 추적
	bool _hasUnsavedChanges = false;

	// GUI state
	bool _showCreateWindow = false;
	bool _showEditWindow = false;
	int _selectedEnemyIndex = -1;

	// 입력 버퍼
	char _inputEnemyKey[64] = "";
	char _inputName[64] = "";
	EnemyType _inputEnemyType = EnemyType::ENEMY_GROUND;
	int _inputMaxHp = 100;
	int _inputAtk = 50;
	float _inputRangeRadius = -1.0f;
	int _inputDef = 0;
	int _inputMagicRes = 0;
	float _inputMoveSpeed = 1.0f;
	float _inputBaseAttackTime = 1.5f;
	bool _showDeleteConfirm = false;
	int _deleteTargetIndex = -1;
	std::string _deleteTargetName;

	// GUI 헬퍼 함수
	void RenderToolbar();
	void RenderEnemyList();
	void RenderCreateWindow();
	void RenderEditWindow();

	// 기본 헬퍼 함수
	json CreateEnemyDataStructure(const std::string& key, const std::string& name, const std::string& type,
		int hp, int atk, float range, int def, int magicRes,
		float moveSpeed, float baseAttackTime);
	const char* EnemyTypeToString(EnemyType type);
	EnemyType StringToEnemyType(const std::string& str);
};

