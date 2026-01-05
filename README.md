# AK Data Editor

Dear ImGui 기반의 **게임 데이터 편집 툴**입니다.  
프로젝트 내부의 JSON 테이블(`gamedata/tables`)을 읽어와 UI에서 수정하고 다시 저장할 수 있습니다.

> ⚠️ **주의**: 저장 시 JSON 파일에 **덮어쓰기**됩니다. 사용 전 원본 데이터를 백업하세요.

---

## ✨ 주요 기능

- **Enemy Editor**: 적 데이터 목록/스탯 편집
- **Operator Editor**: 오퍼레이터 데이터 편집
- **Level Editor**: 개발 중(WIP)
- 프로젝트(솔루션) 경로 선택 및 저장 → 다음 실행 시 자동 로드
- JSON 로드/수정/저장

---

## ✅ 지원 환경

- Windows (x64 권장)
- Visual C++ 런타임이 필요할 수 있습니다(PC 환경에 따라).

---

## 🚀 설치 및 실행 방법

1. 릴리즈 ZIP을 다운로드 후 원하는 위치에 **압축 해제**
2. `AK Data Editor/` 폴더에서 `AKDataEditor.exe` 실행
3. 첫 실행 시 **Browse...** 버튼으로 **프로젝트(솔루션) 폴더**를 선택
4. **Save Path** 버튼으로 경로 저장 → 이후 실행부터 자동으로 불러옵니다

### 🧷 백업 권장
- 편집 전 `gamedata/` 폴더를 통째로 복사해 백업해두면 안전합니다.

---

## 📁 데이터 경로 규칙

기본적으로 아래 경로의 JSON 테이블을 대상으로 합니다.

```
gamedata/
├─ levels/
│  ├─ level_main_*.json
└─ tables/
   ├─ operators_table.json
   └─ enemies_table.json

```

- 파일이 없거나 경로가 다르면 목록이 비어 보일 수 있습니다.
- 저장은 해당 JSON 파일에 직접 반영됩니다.

---

## 🕹️ 사용 흐름(기본)

1. 메인 화면에서 편집할 에디터 선택  
   - Enemy Editor / Operator Editor
2. 목록에서 항목 선택 → 우측(또는 팝업) 편집 UI에서 값 수정
3. 저장 버튼으로 JSON 반영

---

## 📄 라이선스 & 크레딧

- Dear ImGui — UI
- nlohmann/json — JSON parsing/serialization
