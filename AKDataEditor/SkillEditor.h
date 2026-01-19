#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Skill.h"

using json = nlohmann::ordered_json;

class SkillEditor
{
public:
	SkillEditor(std::string jsonPath, std::string operatorPath);
	~SkillEditor();

	void LoadSkills();
	void SaveSkills();

	void RenderGUI(bool* p_open);

	bool HasUnsavedChanges() const { return _hasUnsavedChanges; }
	void ClearUnsavedFlag() { _hasUnsavedChanges = false; }

private:
	std::string _jsonPath;
	std::string _operatorPath;
	std::vector<Skill> _skills;
	std::vector<std::string> _operatorIds;
	int _selectedOperatorIdx = 0;  
	char _inputSkillSuffix[8] = "1";  
	std::string _dataVersion = "0.0";


	// 변경 감지
	bool _hasUnsavedChanges = false;

	// GUI State
	bool _showCreateWindow = false;
	bool _showEditWindow = false;
	bool _showRangeEditor = false;
	bool _showEffectAddPopup = false;
	int _selectedSkillIndex = -1;

	// 삭제 확인
	bool _showDeleteConfirm = false;
	int _deleteTargetIndex = -1;
	std::string _deleteTargetName = "";

	char _inputSkillName[64] = "";
	char _inputSkillDesc[512] = "";
	int _inputSkillType = 1;
	float _inputDuration = 0.0f;

	int _inputSpType = 1;
	int _inputSpCost = 30;
	int _inputInitSp = 0;

	static const int GRID_SIZE = 9;
	static const int CENTER = 4;
	bool _skillRangeGrid[GRID_SIZE][GRID_SIZE] = {};

	char _inputEffectKey[64];
	float _inputEffectValue = 0.0f;
	int _editingEffectIndex = -1;

	std::vector<BlackboardEntry> _currentEffects;

	// GUI Render
	void RenderToolbar();
	void RenderSkillList();
	void RenderCreateWindow();
	void RenderEditWindow();
	void RenderRangeGridEditor();
	void RenderEffectListPanel();
	void RenderEffectAddPopup();

	// 헬퍼
	void ClearAllInputBuffers();
	void LoadSkillToBuffer(const Skill& skill);
	Skill CreateSkillFromBuffer();

	std::vector<SkillRange> GridToRangeJson();
	void RangeJsonToGrid(const std::vector<SkillRange>& ranges);
	
	void LoadOperatorIds();
	std::string GenerateSkillId(const std::string& operatorId, const std::string& suffix);
	std::string GetOperatorDisplayName(const std::string& operatorId);

	void UpdateOperatorSkillIds();
};

