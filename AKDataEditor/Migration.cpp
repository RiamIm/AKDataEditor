#include "Migration.h"
#include "Utility.h"
#include <iostream>
#include <sstream>

std::map<Migration::DataType, std::vector<std::tuple<std::string, std::string, MigrationFunc>>>&
Migration::GetRegistry()
{
    static std::map<DataType, std::vector<std::tuple<std::string, std::string, MigrationFunc>>> registry;
    return registry;
}

void Migration::Register(DataType type, const std::string& fromVersion,
    const std::string& toVersion, MigrationFunc func)
{
    GetRegistry()[type].push_back({ fromVersion, toVersion, func });
}

int Migration::CompareVersions(const std::string& a, const std::string& b)
{
    auto parse = [](const std::string& v) -> std::vector<int> {
        std::vector<int> parts;
        std::stringstream ss(v);
        std::string item;
        while (std::getline(ss, item, '.')) {
            try { parts.push_back(std::stoi(item)); }
            catch (...) { parts.push_back(0); }
        }
        return parts;
        };

    auto va = parse(a);
    auto vb = parse(b);

    size_t maxLen = std::max(va.size(), vb.size());
    va.resize(maxLen, 0);
    vb.resize(maxLen, 0);

    for (size_t i = 0; i < maxLen; ++i) {
        if (va[i] < vb[i]) return -1;
        if (va[i] > vb[i]) return 1;
    }
    return 0;
}

void Migration::RunMigrations(DataType type, json& data,
    const std::string& fromVersion,
    const std::string& toVersion)
{
    auto& registry = GetRegistry()[type];

    for (auto& [from, to, func] : registry)
    {
        // fromVersion <= from < to <= toVersion 인 경우 실행
        if (CompareVersions(fromVersion, from) <= 0 &&
            CompareVersions(to, toVersion) <= 0)
        {
            std::cout << "[Migration] Running: " << from << " -> " << to << "\n";
            func(data);
        }
    }
}

bool Migration::CheckAndMigrate(DataType type, json& data)
{
    std::string currentVersion = VERSION;
    std::string dataVersion = data.value("version", std::string("0.0"));

    if (dataVersion == currentVersion)
        return false;  // 이미 최신

    std::cout << "[Migration] Upgrading from v" << dataVersion
        << " to v" << currentVersion << "\n";

    // 등록된 마이그레이션 실행
    RunMigrations(type, data, dataVersion, currentVersion);

    // 버전 업데이트
    data["version"] = currentVersion;

    return true;  // 저장 필요
}

// ===== 마이그레이션 등록 =====
void RegisterAllMigrations()
{
    // 예시: Operator 2.0 -> 2.1 마이그레이션
    // Migration::Register(
    //     Migration::DataType::Operator,
    //     "2.0", "2.1",
    //     [](json& data) {
    //         // 변환 로직
    //     }
    // );

    // 현재는 빈 마이그레이션 (스키마 변경 없음)
    // 나중에 필요할 때 여기에 추가
}
