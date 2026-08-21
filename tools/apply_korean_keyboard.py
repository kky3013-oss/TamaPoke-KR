from pathlib import Path
import re

root = Path('.')
ino = root / 'TamaPoke.ino'
s = ino.read_text(encoding='utf-8')

if '#include "kr_text.h"' not in s:
    s = s.replace('#include "audio.h"', '#include "audio.h"\n#include "kr_text.h"\n#include "kr_keyboard.h"')
s = s.replace('char nameBuf[12] = "";', 'char nameBuf[64] = "";')
s = s.replace('if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");', 'if (!gfx->begin(80000000)) Serial.println("gfx->begin() fallo");\n  krTextBegin();\n  krKeyboardBegin();')
s = s.replace('static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT" };', 'static const char *const LANG_CODES[LANG_COUNT] = { "ES", "EN", "FR", "DE", "IT", "PT", "KO" };')

start = s.index('// ---------- teclado para renombrar ----------')
end = s.index('// ---------- galeria pokedex ----------', start)
new_keyboard = '''// ---------- teclado para renombrar ----------\n\nstatic bool kbKoreanMode = true;\n\nvoid openKeyboard() {\n  kbOpen = true;\n  krKeyboardOpen(pet.nick, nameBuf, sizeof(nameBuf));\n  nameLen = (uint8_t)strlen(nameBuf);\n}\n\nvoid renderKeyboard() {\n  krKeyboardRender(gfx, nameBuf, kbKoreanMode);\n}\n\nvoid keyboardTap(int16_t x, int16_t y) {\n  bool closeKeyboard = false;\n  krKeyboardTap(x, y, kbKoreanMode, nameBuf, sizeof(nameBuf), closeKeyboard);\n  nameLen = (uint8_t)strlen(nameBuf);\n  if (closeKeyboard) {\n    pet.rename(nameBuf);\n    kbOpen = false;\n  }\n}\n\n'''
s = s[:start] + new_keyboard + s[end:]

# Make every normal Arduino_GFX text print/cursor/color pass through the UTF-8 wrapper.
s = s.replace('gfx->setCursor(', 'krSetCursor(gfx, ')
s = s.replace('gfx->setTextColor(', 'krSetTextColor(gfx, ')
s = s.replace('gfx->print(', 'krPrint(gfx, ')
ino.write_text(s, encoding='utf-8')

# Add Korean language as the seventh language and make it the default for new installs.
h = root / 'i18n.h'
hs = h.read_text(encoding='utf-8')
hs = hs.replace('enum Lang : uint8_t { LANG_ES = 0, LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_PT, LANG_COUNT };', 'enum Lang : uint8_t { LANG_ES = 0, LANG_EN, LANG_FR, LANG_DE, LANG_IT, LANG_PT, LANG_KO, LANG_COUNT };')
hs = hs.replace('#define LANG_DEFAULT LANG_EN', '#define LANG_DEFAULT LANG_KO')
h.write_text(hs, encoding='utf-8')

# Add the Korean translation row to the existing table.
ic = root / 'i18n.cpp'
isrc = ic.read_text(encoding='utf-8')
ko = '''  // ---------------- KO ----------------\n  {\n    "진화 중!", "냠냠!", "좋아해요!", "배고파요!", "목욕이 필요해요!",\n    "지쳤어요...", "슬퍼요...", "통통해요...", "반짝이!", "행복해요",\n    "고마워요! 잘 가요", "도망갔어요...", "안녕! 잘 가요...",\n    "알", "전설의 알!?", "희귀한 알!", "알을 눌러보세요...", "움직여요!", "거의 다 됐어요!",\n    "도감 %u/151",\n    "%s%s 레벨 %u",\n    "%s를 놓아줄까요?", "예", "아니오",\n    "%u번 성공", "힘 +%u", "새 기록!", "기록: %u", "빠르게 눌러요!",\n    "점수: %u", "정말 즐거워요!", "+행복",\n    "시간 설정", "시", "분", "위로 밀기: 취소", "언어",\n    "메달!", "최고예요!", "%u일 연속!",\n    "연속 %u  최고 %u", "유대", "열매 ???", "빨간 열매", "파란 열매", "초록 열매",\n    "%s   나이 %lud", "이름을 눌러 이름 변경",\n    "전투", "공격", "방어", "속도", "무게", "힘 훈련",\n    "메달 %d/%d", "눌러서 뒤로",\n    "이름:", "눌러서 돌아가기",\n    "먹이", "기쁨", "에너지", "청결",\n    "최고 %u",\n    "진행", "레벨 %u", "레벨 %u까지 %u분", "진화", "최종 형태",\n    "진화할 준비 완료!", "모든 능력치가 40 이상이어야 진화",\n    "%u레벨 후 진화", "실수: %u",\n    "소리 켜짐", "소리 꺼짐",\n    "진화하기", "%s가 하고 싶은 말이 있어요...", "%s가 버려졌다고 느껴요...",\n    "진화할까요?", "현재 형태 유지", "작별할까요?", "작별하기", "함께 있기",\n    "처음 포켓몬을 선택하세요",\n    "스프라이트 없음", "SD 카드에 넣어주세요",\n  },\n'''
if '// ---------------- KO ----------------' not in isrc:
    isrc = isrc.replace('};\n\n// Nombres de medalla', ko + '};\n\n// Nombres de medalla', 1)

def add_row(table_name, row):
    global isrc
    pat = r'(static const char \*const ' + table_name + r'\[LANG_COUNT\]\[MED_COUNT\] = \{.*?\n\};)'
    m = re.search(pat, isrc, re.S)
    if not m:
        raise RuntimeError(f'Could not locate {table_name}')
    block = m.group(1)
    if row.strip() not in block:
        block = block[:-3] + row + '};'
        isrc = isrc[:m.start()] + block + isrc[m.end():]

add_row('MED_NAME', '  { "레벨10", "레벨25", "레벨50", "열매", "7일 연속", "유대", "최종 형태", "건강" },\n')
add_row('MED_LBL', '  { "Lv10", "Lv25", "Lv50", "열매", "7일", "유대", "최종", "건강" },\n')
add_row('MED_DSC', '  { "레벨 10", "레벨 25", "레벨 50", "열매 발견",\n    "7일 연속", "최대 유대", "최종 형태", "건강한 상태" },\n')
ic.write_text(isrc, encoding='utf-8')

print('Korean language + UTF-8 text + keyboard patch applied.')
