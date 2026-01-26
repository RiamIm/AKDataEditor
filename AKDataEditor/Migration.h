#pragma once
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;
using MigrationFunc = std::function<void(json&)>;

class Migration
{
public:
    enum class DataType
    {
        Operator,
        Enemy,
        Skill,
        Level
    };

    // 마이그레이션 등록
    static void Register(DataType type, const std::string& fromVersion,
        const std::string& toVersion, MigrationFunc func);

    // 버전 체크 + 마이그레이션 실행 + 버전 업데이트
    // 반환값: true면 저장 필요
    static bool CheckAndMigrate(DataType type, json& data);

private:
    static int CompareVersions(const std::string& a, const std::string& b);
    static void RunMigrations(DataType type, json& data,
        const std::string& fromVersion,
        const std::string& toVersion);

    static std::map<DataType, std::vector<std::tuple<std::string, std::string, MigrationFunc>>>& GetRegistry();
};

// 앱 시작 시 호출
void RegisterAllMigrations();
