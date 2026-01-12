// RAII (Resource Acquisiton Is Initialization) Pattern
#pragma once
#include <imgui/imgui.h>
#include <string>

// 복사 이동 방지
struct NonCopyMove 
{
	NonCopyMove() = default;
	NonCopyMove(const NonCopyMove&) = delete;
	NonCopyMove& operator=(const NonCopyMove&) = delete;
	NonCopyMove(NonCopyMove&&) = delete;
	NonCopyMove& operator=(NonCopyMove&&) = delete;
};

// ===== Window RAII =====
class ScopedWindow : public NonCopyMove
{
public:
	ScopedWindow(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
		: _isOpend(ImGui::Begin(name, p_open, flags)) {
	}

	virtual ~ScopedWindow()
	{
		ImGui::End();
	}

	// bool 변환 연산자 (if 문에서 사용)
	operator bool() const { return _isOpend; }

private:
	bool _isOpend;
};

// ===== StyleColor RAII =====
class ScopedStyleColor : public NonCopyMove
{
public:
	ScopedStyleColor(ImGuiCol idx, ImU32 col)
	{
		ImGui::PushStyleColor(idx, col);
	}

	ScopedStyleColor(ImGuiCol idx, const ImVec4& col)
	{
		ImGui::PushStyleColor(idx, col);
	}

	virtual ~ScopedStyleColor()
	{
		ImGui::PopStyleColor();
	}
};

// ----- ID RAII =====
class ScopedID : public NonCopyMove
{
public:
	ScopedID(int id)
	{
		ImGui::PushID(id);
	}

	ScopedID(const char* str_id)
	{
		ImGui::PushID(str_id);
	}

	ScopedID(const void* ptr_id)
	{
		ImGui::PushID(ptr_id);
	}

	virtual ~ScopedID()
	{
		ImGui::PopID();
	}
};

// ===== Table RAII =====
class ScopedTable : public NonCopyMove
{
public:
	ScopedTable(const char* str_id, int columns, ImGuiTableFlags flags = 0)
		:_isOpened(ImGui::BeginTable(str_id, columns, flags)) {
	}

	virtual ~ScopedTable()
	{
		if (_isOpened)
		{
			ImGui::EndTable();
		}
	}

	operator bool() const { return _isOpened; }

private:
	bool _isOpened;
};

// ===== Combo RAII =====
class ScopedCombo : public NonCopyMove
{
public:
	ScopedCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0) 
		: _isOpened(ImGui::BeginCombo(label, preview_value, flags)) {}
	
	virtual ~ScopedCombo()
	{
		if (_isOpened)
		{
			ImGui::EndCombo();
		}

	}

	operator bool() const { return _isOpened; }

private:
	bool _isOpened;
};

// ===== Child RAII =====
class ScopedChild : public NonCopyMove
{
public:
	ScopedChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), ImGuiChildFlags flags = 0)
		: _isOpened(ImGui::BeginChild(str_id, size, flags)) {
	}

	virtual ~ScopedChild()
	{
		ImGui::EndChild();
	}

	operator bool() const { return _isOpened; }

private:
	bool _isOpened;
};

// ===== Popup Modal RAII =====
class ScopedPopupModal : public NonCopyMove
{
public:
	ScopedPopupModal(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0)
		: _isOpend(ImGui::BeginPopupModal(name, p_open, flags)) {}

	virtual ~ScopedPopupModal()
	{
		if (_isOpend)
		{
			ImGui::EndPopup();
		}
	}

	operator bool() const { return _isOpend; }

private:
	bool _isOpend;
};

// ===== Disalbed RAII =====
class ScopedDisabled : public NonCopyMove
{
public:
	ScopedDisabled(bool disabled = true)
		: _disabled(disabled)
	{
		if (_disabled)
		{
			ImGui::BeginDisabled();
		}
	}

	virtual ~ScopedDisabled()
	{
		if (_disabled)
		{
			ImGui::EndDisabled();
		}
	}

private:
	bool _disabled;
};

// ===== ItemWidth RAII =====
class ScopedItemWidth : public NonCopyMove
{
public:
	ScopedItemWidth(float width)
	{
		ImGui::PushItemWidth(width);
	}

	virtual ~ScopedItemWidth()
	{
		ImGui::PopItemWidth();
	}
};

// ===== 고유 변수 이름 생성 헬퍼 =====
#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define UNIQUE_NAME(prefix) CONCAT(prefix, __LINE__)

// ===== RAII 매크로들 =====
#define SCOPED_WINDOW(name) \
    ScopedWindow UNIQUE_NAME(_window_)(name)

#define SCOPED_WINDOW_EX(name, p_open, flags) \
    ScopedWindow UNIQUE_NAME(_window_)(name, p_open, flags)

#define SCOPED_ID(id) \
    ScopedID UNIQUE_NAME(_id_)(id)

#define SCOPED_COLOR(idx, col) \
    ScopedStyleColor UNIQUE_NAME(_color_)(idx, col)

#define SCOPED_TABLE(id, columns, flags) \
    ScopedTable UNIQUE_NAME(_table_)(id, columns, flags)

#define SCOPED_CHILD(id, size, flags) \
    ScopedChild UNIQUE_NAME(_child_)(id, size, flags)

#define SCOPED_DISABLED(condition) \
    ScopedDisabled UNIQUE_NAME(_disabled_)(condition)

#define SCOPED_ITEM_WIDTH(width) \
    ScopedItemWidth UNIQUE_NAME(_itemwidth_)(width)