#pragma once
#include <imgui/imgui.h>

#define VERSION 1

#define COLOR_RED		ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
#define COLOR_GREEN		ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define COLOR_BLUE		ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define COLOR_YELLOW	ImVec4(0.0f, 1.0f, 1.0f, 1.0f)
#define COLOR_GRAY		ImVec4(0.5f, 0.5f, 0.5f, 1.0f)


#define COLOR_CASTER		ImVec4(0.5f, 0.5f, 1.0f, 1.0f)
#define COLOR_SNIPER		ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
#define COLOR_GUARD			ImVec4(1.0f, 0.5f, 0.5f, 1.0f)
#define COLOR_DEFENDER		ImVec4(0.8f, 0.8f, 0.3f, 1.0f)
#define COLOR_MEDIC			ImVec4(0.3f, 1.0f, 1.0f, 1.0f)
#define COLOR_VANGUARD		ImVec4(1.0f, 0.7f, 0.3f, 1.0f)
#define COLOR_SUPPORTER		ImVec4(1.0f, 0.3f, 1.0f, 1.0f)
#define COLOR_SPECIALIST	ImVec4(0.6f, 0.4f, 1.0f, 1.0f)
#define COLOR_DEFAULT		ImVec4(0.9f, 0.9f, 0.9f, 1.0f)

inline double Snap2(double v) {
    long long x = llround(v * 100.0);
    return x / 100.0;
}

inline double Snap1(double v) {
    long long x = llround(v * 10.0);
    return x / 10.0;
}