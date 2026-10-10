#include "schema_dump.h"
#include "mem.h"
#include "offsets.h"
#include <cstring>

namespace schema_dump {

struct Want { const char* cls; const char* field; uintptr_t* dst; };

static const Want kWants[] = {
    { "C_BaseEntity", "m_pGameSceneNode",          &off::BaseEntity::m_pGameSceneNode },
    { "C_BaseEntity", "m_iMaxHealth",              &off::BaseEntity::m_iMaxHealth },
    { "C_BaseEntity", "m_iHealth",                 &off::BaseEntity::m_iHealth },
    { "C_BaseEntity", "m_lifeState",               &off::BaseEntity::m_lifeState },
    { "C_BaseEntity", "m_flSimulationTime",        &off::BaseEntity::m_flSimulationTime },
    { "C_BaseEntity", "m_flAnimTime",              &off::BaseEntity::m_flAnimTime },
    { "C_BaseEntity", "m_iTeamNum",                &off::BaseEntity::m_iTeamNum },
    { "C_BaseEntity", "m_hOwnerEntity",            &off::BaseEntity::m_hOwnerEntity },

    { "C_BaseModelEntity", "m_iViewerID",              &off::ModelEntity::m_iViewerID },
    { "C_BaseModelEntity", "m_iTeamVisibilityBitmask", &off::ModelEntity::m_iTeamVisibilityBitmask },
    { "C_BaseModelEntity", "m_bVisibilityDirtyFlag",   &off::ModelEntity::m_bVisibilityDirtyFlag },
    { "C_BaseModelEntity", "m_Glow",                   &off::ModelEntity::m_Glow },

    { "C_DOTA_BaseNPC_Tower", "m_hTowerAttackTarget", &off::Tower::m_hTowerAttackTarget },

    { "C_DOTA_BaseNPC", "m_iCurrentLevel",        &off::NPC::m_iCurrentLevel },
    { "C_DOTA_BaseNPC", "m_bIsAncient",           &off::NPC::m_bIsAncient },
    { "C_DOTA_BaseNPC", "m_bIsBossCreature",      &off::NPC::m_bIsBossCreature },
    { "C_DOTA_BaseNPC", "m_bConsideredHero",      &off::NPC::m_bConsideredHero },
    { "C_DOTA_BaseNPC", "m_iAttackRange",         &off::NPC::m_iAttackRange },
    { "C_DOTA_BaseNPC", "m_iHealthBarOffset",     &off::NPC::m_iHealthBarOffset },
    { "C_DOTA_BaseNPC", "m_flMana",               &off::NPC::m_flMana },
    { "C_DOTA_BaseNPC", "m_flMaxMana",            &off::NPC::m_flMaxMana },
    { "C_DOTA_BaseNPC", "m_vecAbilities",         &off::NPC::m_vecAbilities },
    { "C_DOTA_BaseNPC", "m_bIsIllusion",          &off::NPC::m_bIsIllusion },
    { "C_DOTA_BaseNPC", "m_flInvisibilityLevel",  &off::NPC::m_flInvisibilityLevel },
    { "C_DOTA_BaseNPC", "m_iszUnitName",          &off::NPC::m_iszUnitName },
    { "C_DOTA_BaseNPC", "m_iDamageMin",           &off::NPC::m_iDamageMin },
    { "C_DOTA_BaseNPC", "m_iDamageMax",           &off::NPC::m_iDamageMax },
    { "C_DOTA_BaseNPC", "m_iDamageBonus",         &off::NPC::m_iDamageBonus },
    { "C_DOTA_BaseNPC", "m_iMoveSpeed",           &off::NPC::m_iMoveSpeed },
    { "C_DOTA_BaseNPC", "m_flBaseAttackTime",     &off::NPC::m_flBaseAttackTime },
    { "C_DOTA_BaseNPC", "m_flHullRadius",         &off::NPC::m_flHullRadius },
    { "C_DOTA_BaseNPC", "m_flCollisionPadding",   &off::NPC::m_flCollisionPadding },
    { "C_DOTA_BaseNPC", "m_nPlayerOwnerID",       &off::NPC::m_nPlayerOwnerID },
    { "C_DOTA_BaseNPC", "m_flLastAttackTime",     &off::NPC::m_flLastAttackTime },
    { "C_DOTA_BaseNPC", "m_flDeathTime",          &off::NPC::m_flDeathTime },
    { "C_DOTA_BaseNPC", "m_flPhysicalArmorValue", &off::NPC::m_flPhysicalArmorValue },
    { "C_DOTA_BaseNPC", "m_bSuppressGlow",        &off::NPC::m_bSuppressGlow },

    { "C_DOTA_BaseNPC_Hero", "m_flRespawnTime",     &off::Hero::m_flRespawnTime },
    { "C_DOTA_BaseNPC_Hero", "m_iPlayerID",         &off::Hero::m_iPlayerID },
    { "C_DOTA_BaseNPC_Hero", "m_bLifeState",        &off::Hero::m_bLifeState },
    { "C_DOTA_BaseNPC_Hero", "m_bDisableWearables", &off::Hero::m_bDisableWearables },

    { "C_DOTAWearableItem", "m_bHiddenByEquipmentSlot", &off::Wearable::m_bHiddenByEquipmentSlot },

    { "C_DOTABaseAbility", "m_bHidden",          &off::Ability::m_bHidden },
    { "C_DOTABaseAbility", "m_iLevel",           &off::Ability::m_iLevel },
    { "C_DOTABaseAbility", "m_bInAbilityPhase",  &off::Ability::m_bInAbilityPhase },
    { "C_DOTABaseAbility", "m_fCooldown",        &off::Ability::m_fCooldown },
    { "C_DOTABaseAbility", "m_flCooldownLength", &off::Ability::m_flCooldownLength },
    { "C_DOTABaseAbility", "m_iManaCost",        &off::Ability::m_iManaCost },
    { "C_DOTABaseAbility", "m_flCastStartTime",  &off::Ability::m_flCastStartTime },

    { "C_DOTAPlayerController", "m_nPlayerID",           &off::Ctrl::m_nPlayerID },
    { "C_DOTAPlayerController", "m_hAssignedHero",       &off::Ctrl::m_hAssignedHero },
    { "C_DOTAPlayerController", "m_hActiveAbility",      &off::Ctrl::m_hActiveAbility },
    { "C_DOTAPlayerController", "m_flCameraZoomAmount",  &off::Camera::m_flCameraZoomAmount },

    { "CBasePlayerController", "m_bIsLocalPlayerController", &off::Ctrl::m_bIsLocalPlayerController },

    { "C_DOTA_RoshanSpawner", "m_iLastKillerTeam",    &off::RoshanSpawner::m_iLastKillerTeam },
    { "C_DOTA_RoshanSpawner", "m_iKillCount",         &off::RoshanSpawner::m_iKillCount },
    { "C_DOTA_RoshanSpawner", "m_vRoshanAltLocation", &off::RoshanSpawner::m_vRoshanAltLocation },
    { "C_DOTA_RoshanSpawner", "m_hRoshan",            &off::RoshanSpawner::m_hRoshan },

    { "C_DOTAGamerulesProxy", "m_pGameRules", &off::RulesProxy::m_pGameRules },

    { "C_DOTAGamerules", "m_nStartingGold",              &off::Rules::m_nStartingGold },
    { "C_DOTAGamerules", "m_iGameMode",                  &off::Rules::m_iGameMode },
    { "C_DOTAGamerules", "m_nGameState",                 &off::Rules::m_nGameState },
    { "C_DOTAGamerules", "m_flGameStartTime",            &off::Rules::m_flGameStartTime },
    { "C_DOTAGamerules", "m_nRoshanRespawnPhase",        &off::Rules::m_nRoshanRespawnPhase },
    { "C_DOTAGamerules", "m_flRoshanRespawnPhaseEndTime",&off::Rules::m_flRoshanRespawnPhaseEndTime },
    { "C_DOTAGamerules", "m_hGameModeEntity",            &off::Rules::m_hGameModeEntity },

    { "C_DOTABaseGameMode", "m_bFogOfWarDisabled", &off::GameMode::m_bFogOfWarDisabled },
    { "C_DOTABaseGameMode", "m_bUseUnseenFOW",     &off::GameMode::m_bUseUnseenFOW },

    { "C_DOTA_DataNonSpectator", "m_roshanSpawnInfo",    &off::Data::m_roshanSpawnInfo },
    { "C_DOTA_DataNonSpectator", "m_nNextPowerRuneType", &off::Data::m_nNextPowerRuneType },

    { "CRoshanPhaseInfo", "m_eRoshanPhase",          &off::RoshanPhase::m_eRoshanPhase },
    { "CRoshanPhaseInfo", "m_flRoshanPhaseStartTime",&off::RoshanPhase::m_flRoshanPhaseStartTime },
    { "CRoshanPhaseInfo", "m_flRoshanPhaseEndTime",  &off::RoshanPhase::m_flRoshanPhaseEndTime },

    { "C_PlayerVisibility", "m_flVisibilityStrength",      &off::PlayerVisibility::m_flVisibilityStrength },
    { "C_PlayerVisibility", "m_flFogDistanceMultiplier",   &off::PlayerVisibility::m_flFogDistanceMultiplier },
    { "C_PlayerVisibility", "m_flFogMaxDensityMultiplier", &off::PlayerVisibility::m_flFogMaxDensityMultiplier },
    { "C_PlayerVisibility", "m_bStartDisabled",            &off::PlayerVisibility::m_bStartDisabled },
    { "C_PlayerVisibility", "m_bIsEnabled",                &off::PlayerVisibility::m_bIsEnabled },
};

static const int kWantCount = (int)(sizeof(kWants) / sizeof(kWants[0]));

static int g_resolved = 0;

int Resolved() { return g_resolved; }
int Wanted()   { return kWantCount; }

static void TryResolveClass(uintptr_t ci, const char* nm) {
    int32_t clsSize = 0;
    int16_t fcount = 0;
    uintptr_t fptr = 0;
    if (!mem::Read(ci + 0x18, clsSize)) return;
    if (!mem::Read(ci + 0x1C, fcount)) return;
    if (!mem::Read(ci + 0x28, fptr) || !mem::ValidPtr(fptr)) return;
    if (clsSize <= 0 || clsSize > 0x20000) return;
    if (fcount <= 0 || fcount > 4096) return;

    for (int start = 0; start < kWantCount; ) {
        int end = start;
        while (end < kWantCount && strcmp(kWants[end].cls, kWants[start].cls) == 0) ++end;
        if (strcmp(kWants[start].cls, nm) != 0) { start = end; continue; }

        int groupN = end - start;
        if (groupN > 64) { start = end; continue; }

        uintptr_t tmp[64];
        bool have[64];
        for (int w = 0; w < groupN; ++w) { have[w] = false; tmp[w] = 0; }

        for (int f = 0; f < fcount; ++f) {
            uintptr_t fa = fptr + (uintptr_t)f * 0x30;
            uintptr_t fnp = 0;
            if (!mem::Read(fa, fnp) || !mem::ValidPtr(fnp)) continue;
            char fn[128];
            if (!mem::ReadStr(fnp, fn, sizeof(fn))) continue;
            int32_t offv = -1;
            if (!mem::Read(fa + 0x10, offv)) continue;
            if (offv < 8 || offv >= clsSize) continue;
            for (int w = 0; w < groupN; ++w) {
                if (!have[w] && strcmp(kWants[start + w].field, fn) == 0) {
                    have[w] = true;
                    tmp[w] = (uintptr_t)offv;
                }
            }
        }

        bool full = true;
        for (int w = 0; w < groupN; ++w)
            if (!have[w]) { full = false; break; }
        if (full) {
            for (int w = 0; w < groupN; ++w)
                *kWants[start + w].dst = tmp[w];
            g_resolved += groupN;
        }
        start = end;
    }
}

bool DumpFromScope(uintptr_t scope) {
    if (!mem::ValidPtr(scope)) return false;
    uintptr_t hash = scope + 0x560;
    int walked = 0;

    for (int b = 0; b < 256; ++b) {
        uintptr_t node = 0;
        if (!mem::Read(hash + 0x60 + (uintptr_t)b * 0x18, node)) continue;
        int guard = 0;
        while (mem::ValidPtr(node) && guard++ < 200000 && walked < 600000) {
            ++walked;
            uintptr_t ci = 0;
            if (!mem::Read(node + 0x10, ci) || !mem::ValidPtr(ci)) break;
            uintptr_t np = 0;
            if (mem::Read(ci + 8, np) && mem::ValidPtr(np)) {
                char nm[128];
                if (mem::ReadStr(np, nm, sizeof(nm)) && nm[0])
                    TryResolveClass(ci, nm);
            }
            uintptr_t next = 0;
            if (!mem::Read(node + 8, next)) break;
            node = next;
        }
    }

    return g_resolved > 0;
}

bool MatrixValid(uintptr_t addr) {
    float m[16];
    if (!mem::Read(addr, m)) return false;
    for (int i = 0; i < 16; ++i) {
        float v = m[i];
        if (!(v == v)) return false;
        if (v > 1e6f || v < -1e6f) return false;
    }
    if (m[3] > 1e-3f || m[3] < -1e-3f) return false;
    if (m[7] > 1e-3f || m[7] < -1e-3f) return false;
    if (m[15] > 1e-3f || m[15] < -1e-3f) return false;
    if (m[11] < 0.999f || m[11] > 1.001f) return false;
    if (m[0] < 1e-4f || m[0] > 10.f) return false;
    if (m[5] < 1e-4f || m[5] > 10.f) return false;
    return true;
}

uintptr_t ScanMatrix(uintptr_t lo, uintptr_t hi, uintptr_t prefer) {
    uintptr_t best = 0;
    uintptr_t bestD = ~(uintptr_t)0;
    for (uintptr_t a = (lo + 15) & ~(uintptr_t)15; a + 64 <= hi; a += 16) {
        if (!MatrixValid(a)) continue;
        uintptr_t d = a > prefer ? a - prefer : prefer - a;
        if (d < bestD) { bestD = d; best = a; }
    }
    return best;
}

int WritableRanges(uintptr_t base, uint32_t imageSize, uintptr_t* out, int maxPairs) {
    if (!base || !imageSize) return 0;
    int n = 0;
    __try {
        auto dos = (const IMAGE_DOS_HEADER*)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
        auto nt = (const IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
        uintptr_t sec = (uintptr_t)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader;
        int cnt = nt->FileHeader.NumberOfSections;
        if (cnt < 0 || cnt > 96) cnt = 96;
        for (int i = 0; i < cnt && n < maxPairs; ++i) {
            auto sh = (const IMAGE_SECTION_HEADER*)(sec + (uintptr_t)i * sizeof(IMAGE_SECTION_HEADER));
            if (!(sh->Characteristics & IMAGE_SCN_MEM_WRITE)) continue;
            uint32_t vsz = sh->Misc.VirtualSize ? sh->Misc.VirtualSize : sh->SizeOfRawData;
            if (!vsz) continue;
            uintptr_t from = base + sh->VirtualAddress;
            uintptr_t to = from + vsz;
            if (to > base + imageSize) to = base + imageSize;
            if (to <= from) continue;
            out[n * 2] = from;
            out[n * 2 + 1] = to;
            ++n;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return n;
}

bool HealMatrix(uintptr_t clientBase, uint32_t imageSize) {
    uintptr_t cur = clientBase + off::dwViewMatrix;
    if (MatrixValid(cur)) return true;

    uintptr_t rg[64];
    int n = WritableRanges(clientBase, imageSize, rg, 32);
    uintptr_t best = 0;
    uintptr_t bestD = ~(uintptr_t)0;
    for (int i = 0; i < n; ++i) {
        uintptr_t a = ScanMatrix(rg[i * 2], rg[i * 2 + 1], cur);
        if (!a) continue;
        uintptr_t d = a > cur ? a - cur : cur - a;
        if (d < bestD) { bestD = d; best = a; }
    }
    if (!best) return false;
    off::dwViewMatrix = best - clientBase;
    return true;
}

typedef void* (*CreateInterfaceFn)(const char*, int*);
typedef void* (*FindScopeFn)(void*, const char*);

static bool s_schemaOk = false;

void Run(uintptr_t clientBase, uint32_t imageSize) {
    (void)clientBase;
    (void)imageSize;
    if (s_schemaOk) return;
    __try {
        uintptr_t ss = mem::ModuleBase("schemasystem.dll");
        if (!ss) return;
        auto ci = (CreateInterfaceFn)GetProcAddress((HMODULE)ss, "CreateInterface");
        if (!ci) return;
        void* sys = ci("SchemaSystem_001", nullptr);
        if (!sys) return;
        uintptr_t vt = 0;
        if (!mem::Read((uintptr_t)sys, vt)) return;
        FindScopeFn fs = nullptr;
        if (!mem::Read(vt + 13 * sizeof(void*), fs) || !fs) return;
        void* scope = fs(sys, "client.dll");
        if (scope && DumpFromScope((uintptr_t)scope)) s_schemaOk = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

}
