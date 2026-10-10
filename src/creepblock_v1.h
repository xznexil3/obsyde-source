#pragma once
#include "game.h"

namespace creepblock_v1 {

void RunCreepBlock(const Frame& f);
const FrameUnit* PickLeadCreep(const Frame& f);

inline bool IsLaneCreepName(const char* nm) {
    return nm && (strstr(nm, "npc_dota_creep") != nullptr ||
                  strstr(nm, "siege") != nullptr);
}
inline bool IsLaneCreep(const FrameUnit& u) {
    if (u.kind != UnitKind::Creep) return false;
    if (u.cls) return strstr(u.cls, "Creep_Lane") != nullptr ||
                      strstr(u.cls, "Creep_Siege") != nullptr;
    return IsLaneCreepName(u.name);
}
inline bool IsRangedCreepName(const char* nm) {
    return nm && (strstr(nm, "ranged") != nullptr || strstr(nm, "siege") != nullptr);
}
inline Vec3 BlockPoint(const Vec3& creepPos, const Vec3& walkDir,
                       float creepHull, float heroHull) {
    float d = creepHull + heroHull + 5.f;
    return Vec3{ creepPos.x + walkDir.x * d, creepPos.y + walkDir.y * d, creepPos.z };
}
inline Vec3 BasePos(int team) {
    return team == 2 ? Vec3{ -7200.f, -7200.f, 0.f } : Vec3{ 7200.f, 7200.f, 0.f };
}

}
