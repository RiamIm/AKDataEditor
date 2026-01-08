#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct BlackboardEntry
{
	std::string key;
	double value;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(BlackboardEntry, key, value)
};

struct SkillRange
{
	int row;
	int col;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SkillRange, row, col)
};

struct SpData
{
	int spType;		// 1. attack, 2. time, 4. hit
	int spCost;
	int initSp;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpData, spType, spCost, initSp)
};

struct Skill
{
	std::string skillId;
	std::string operatorId;
	std::string name;
	std::string description;
	int skillType;			// 0. passive, 1. manual, 2. auto
	double duration;
	SpData spData;
	std::vector<SkillRange> range;
	std::vector<BlackboardEntry> blackboard;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Skill,
		skillId, operatorId, name, description, skillType,
		duration, spData, range, blackboard)
};

