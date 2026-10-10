#include "schema_dump.h"
#include "offsets.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

extern "C" HMODULE __stdcall GetModuleHandleA(const char*) { return nullptr; }
extern "C" void* __stdcall GetProcAddress(HMODULE, const char*) { return nullptr; }
extern "C" DWORD __stdcall GetTickCount() { return 0; }
extern "C" BOOL __stdcall GetCursorPos(void*) { return 0; }

static int g_fail = 0;
static void Check(bool ok, const char* what) {
    printf("%s %s\n", ok ? "[ok]  " : "[FAIL]", what);
    if (!ok) ++g_fail;
}

struct Field { const char* name; int off; };
struct Class { const char* name; int size; std::vector<Field> fields; };

static uintptr_t Alloc(size_t n) {
    void* p = calloc(1, n);
    return (uintptr_t)p;
}

static uintptr_t PutStr(const char* s) {
    uintptr_t a = Alloc(strlen(s) + 1);
    strcpy((char*)a, s);
    return a;
}

static uintptr_t BuildClass(const Class& c) {
    uintptr_t ci = Alloc(0x60);
    *(uintptr_t*)(ci + 0x08) = PutStr(c.name);
    *(int32_t*)(ci + 0x18) = c.size;
    *(int16_t*)(ci + 0x1C) = (int16_t)c.fields.size();

    uintptr_t fptr = Alloc(0x30 * c.fields.size() + 0x30);
    *(uintptr_t*)(ci + 0x28) = fptr;
    for (size_t i = 0; i < c.fields.size(); ++i) {
        uintptr_t f = fptr + 0x30 * i;
        *(uintptr_t*)(f + 0x00) = PutStr(c.fields[i].name);
        *(int32_t*)(f + 0x10) = c.fields[i].off;
    }
    return ci;
}

static uintptr_t BuildScope(const std::vector<Class>& classes) {
    uintptr_t scope = Alloc(0x600 + 0x18 * 256);
    uintptr_t hash = scope + 0x560;

    std::vector<uintptr_t> nodes;
    for (size_t i = 0; i < classes.size(); ++i) {
        uintptr_t ci = BuildClass(classes[i]);
        uintptr_t node = Alloc(0x20);
        *(uint32_t*)(node + 0x00) = (uint32_t)i;
        *(uintptr_t*)(node + 0x10) = ci;
        nodes.push_back(node);
    }
    for (size_t i = 0; i < nodes.size(); ++i)
        *(uintptr_t*)(nodes[i] + 0x08) = (i + 1 < nodes.size()) ? nodes[i + 1] : 0;

    if (!nodes.empty())
        *(uintptr_t*)(hash + 0x60 + 0 * 0x18) = nodes[0];
    if (nodes.size() > 1)
        *(uintptr_t*)(hash + 0x60 + 1 * 0x18) = nodes[1];

    return scope;
}

static void TestResolvesKnownClasses() {
    uintptr_t before = off::NPC::m_iAttackRange;
    Class npc{ "C_DOTA_BaseNPC", 0x2000, {
        { "m_iCurrentLevel", 0xC9C }, { "m_bIsAncient", 0xCA0 }, { "m_bIsBossCreature", 0xCA1 },
        { "m_bConsideredHero", 0xCA7 }, { "m_iAttackRange", 0xCC8 }, { "m_iHealthBarOffset", 0xCF4 },
        { "m_flMana", 0xCFC }, { "m_flMaxMana", 0xD00 }, { "m_vecAbilities", 0xD28 },
        { "m_bIsIllusion", 0xD24 }, { "m_flInvisibilityLevel", 0xD5C }, { "m_iszUnitName", 0xD70 },
        { "m_iDamageMin", 0xE18 }, { "m_iDamageMax", 0xE1C }, { "m_iDamageBonus", 0xE20 },
        { "m_iMoveSpeed", 0xCE0 }, { "m_flBaseAttackTime", 0xCE8 }, { "m_flHullRadius", 0xD60 },
        { "m_flCollisionPadding", 0xD64 }, { "m_nPlayerOwnerID", 0x13C0 },
        { "m_flLastAttackTime", 0x13C8 }, { "m_flDeathTime", 0x1624 },
        { "m_flPhysicalArmorValue", 0x162C }, { "m_bSuppressGlow", 0x140C } } };
    Class be{ "C_BaseEntity", 0x600, {
        { "m_pGameSceneNode", 0x330 }, { "m_iMaxHealth", 0x348 }, { "m_iHealth", 0x34C },
        { "m_lifeState", 0x354 }, { "m_flSimulationTime", 0x3B8 }, { "m_flAnimTime", 0x3B4 },
        { "m_iTeamNum", 0x3E7 }, { "m_hOwnerEntity", 0x514 } } };

    uintptr_t scope = BuildScope({ be, npc });
    Check(schema_dump::DumpFromScope(scope), "scope dump returns true");
    Check(off::NPC::m_iAttackRange == before && off::NPC::m_iAttackRange == 0xCC8,
          "NPC m_iAttackRange resolved to 0xCC8");
    Check(off::BaseEntity::m_iHealth == 0x34C, "BaseEntity m_iHealth resolved");
    Check(off::NPC::m_flCollisionPadding == 0xD64, "NPC m_flCollisionPadding resolved");
}

static void TestInheritsNothingButResolvesSeparateClasses() {
    Class ctrl{ "C_DOTAPlayerController", 0xC00, {
        { "m_nPlayerID", 0x910 }, { "m_hAssignedHero", 0x914 },
        { "m_hActiveAbility", 0x99C }, { "m_flCameraZoomAmount", 0xC94 } } };
    Class baseCtrl{ "CBasePlayerController", 0x900, {
        { "m_bIsLocalPlayerController", 0x778 } } };
    Class hero{ "C_DOTA_BaseNPC_Hero", 0x1C00, {
        { "m_flRespawnTime", 0x1AD8 }, { "m_iPlayerID", 0x1B68 },
        { "m_bLifeState", 0x1B34 }, { "m_bDisableWearables", 0x1AC0 } } };

    uintptr_t scope = BuildScope({ ctrl, baseCtrl, hero });
    Check(schema_dump::DumpFromScope(scope), "controller/hero scope dump returns true");
    Check(off::Ctrl::m_nPlayerID == 0x910, "Ctrl m_nPlayerID resolved");
    Check(off::Camera::m_flCameraZoomAmount == 0xC94, "Camera zoom shares controller class");
    Check(off::Ctrl::m_bIsLocalPlayerController == 0x778, "CBasePlayerController resolves separately");
    Check(off::Hero::m_flRespawnTime == 0x1AD8, "Hero m_flRespawnTime resolved");
}

static void TestPartialClassIsRejected() {
    Class partial{ "C_DOTAGamerules", 0x1000, {
        { "m_nStartingGold", 0x58 }, { "m_nGameState", 0x7C } } };

    uintptr_t savedState = off::Rules::m_nGameState;
    uintptr_t savedGold = off::Rules::m_nStartingGold;
    uintptr_t savedMode = off::Rules::m_iGameMode;

    uintptr_t scope = BuildScope({ partial });
    schema_dump::DumpFromScope(scope);
    Check(off::Rules::m_nGameState == savedState &&
          off::Rules::m_nStartingGold == savedGold &&
          off::Rules::m_iGameMode == savedMode,
          "incomplete class leaves all offsets untouched");
}

static void TestOutOfRangeFieldIsRejected() {
    Class bad{ "C_DOTABaseGameMode", 0x600, {
        { "m_bFogOfWarDisabled", 0x605 }, { "m_bUseUnseenFOW", 0x9999 } } };

    uintptr_t savedFow = off::GameMode::m_bUseUnseenFOW;
    uintptr_t scope = BuildScope({ bad });
    schema_dump::DumpFromScope(scope);
    Check(off::GameMode::m_bUseUnseenFOW == savedFow,
          "field offset beyond class size rejected");
}

static void TestMatrixValid() {
    uintptr_t m = Alloc(64);
    float* f = (float*)m;
    for (int i = 0; i < 16; ++i) f[i] = 0.f;
    f[0] = 1.f; f[5] = 1.f; f[10] = -1.f; f[11] = 1.f; f[14] = -100.f;
    Check(schema_dump::MatrixValid(m), "valid viewProj accepted");

    f[3] = 5.f;
    Check(!schema_dump::MatrixValid(m), "nonzero m[3] rejected");
    f[3] = 0.f;

    f[11] = 0.f;
    Check(!schema_dump::MatrixValid(m), "m[11] != 1 rejected");
    f[11] = 1.f;

    f[0] = 0.f;
    Check(!schema_dump::MatrixValid(m), "zero m[0] rejected");
}

static void TestScanMatrix() {
    uintptr_t buf = Alloc(4096);
    float* f = (float*)(buf + 1024);
    for (int i = 0; i < 16; ++i) f[i] = 0.f;
    f[0] = 1.f; f[5] = 1.f; f[10] = -1.f; f[11] = 1.f; f[14] = -50.f;

    uintptr_t got = schema_dump::ScanMatrix(buf, buf + 4096, buf);
    Check(got == buf + 1024, "ScanMatrix finds the valid matrix offset");

    uintptr_t none = schema_dump::ScanMatrix(buf + 2048, buf + 4096, buf);
    Check(none == 0, "ScanMatrix returns 0 when none present");
}

static void TestWantedCount() {
    Check(schema_dump::Wanted() == 78, "wanted field count is 78");
}

int main() {
    TestResolvesKnownClasses();
    TestInheritsNothingButResolvesSeparateClasses();
    TestPartialClassIsRejected();
    TestOutOfRangeFieldIsRejected();
    TestMatrixValid();
    TestScanMatrix();
    TestWantedCount();
    printf("\n%s (%d failures)\n", g_fail ? "FAILURES" : "ALL PASSED", g_fail);
    return g_fail ? 1 : 0;
}
