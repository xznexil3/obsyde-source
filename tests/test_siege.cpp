#include "../chk5/src/creepblock_v1.h"
#include <cstdio>
#include <cstring>

static int g_fail = 0;
static void Check(bool ok, const char* what) {
    printf("%s %s\n", ok ? "[ok]  " : "[FAIL]", what);
    if (!ok) ++g_fail;
}

static FrameUnit Mk(UnitKind k, const char* name, const char* cls) {
    FrameUnit u;
    u.kind = k;
    u.alive = true;
    strncpy(u.name, name, sizeof(u.name) - 1);
    u.cls = cls;
    return u;
}

int main() {
    FrameUnit radiantSiege = Mk(UnitKind::Creep, "npc_dota_goodguys_siege", nullptr);
    FrameUnit direSiege    = Mk(UnitKind::Creep, "npc_dota_badguys_siege", nullptr);
    FrameUnit melee        = Mk(UnitKind::Creep, "npc_dota_creep_badguys_melee", nullptr);
    FrameUnit ranged       = Mk(UnitKind::Creep, "npc_dota_creep_badguys_ranged", nullptr);
    FrameUnit neutral      = Mk(UnitKind::Creep, "npc_dota_neutral_kobold", nullptr);
    FrameUnit roshan       = Mk(UnitKind::Roshan, "npc_dota_roshan", nullptr);

    Check(creepblock_v1::IsLaneCreep(radiantSiege), "v1: radiant siege is a lane creep (cls unknown)");
    Check(creepblock_v1::IsLaneCreep(direSiege), "v1: dire siege is a lane creep (cls unknown)");
    Check(creepblock_v1::IsLaneCreep(melee), "v1: melee creep still a lane creep");
    Check(creepblock_v1::IsLaneCreep(ranged), "v1: ranged creep still a lane creep");
    Check(!creepblock_v1::IsLaneCreep(neutral), "v1: neutral is not a lane creep");
    Check(!creepblock_v1::IsLaneCreep(roshan), "v1: roshan is not a lane creep");

    Check(creepblock_v1::IsRangedCreepName(radiantSiege.name), "v1: siege counts as ranged (690 range)");
    Check(creepblock_v1::IsRangedCreepName(ranged.name), "v1: ranged still ranged");
    Check(!creepblock_v1::IsRangedCreepName(melee.name), "v1: melee is not ranged");

    FrameUnit siegeWithCls = Mk(UnitKind::Creep, "npc_dota_goodguys_siege",
                                "C_DOTA_BaseNPC_Creep_Siege");
    FrameUnit meleeWithCls = Mk(UnitKind::Creep, "npc_dota_creep_badguys_melee",
                                "C_DOTA_BaseNPC_Creep_Lane_Melee");
    FrameUnit neutralCls   = Mk(UnitKind::Creep, "npc_dota_neutral_kobold",
                                "C_DOTA_BaseNPC_Creep_Neutral");
    Check(creepblock_v1::IsLaneCreep(siegeWithCls), "v1: siege class accepted when cls known");
    Check(creepblock_v1::IsLaneCreep(meleeWithCls), "v1: lane melee class still accepted");
    Check(!creepblock_v1::IsLaneCreep(neutralCls), "v1: neutral class rejected when cls known");

    printf("\n%s (%d failures)\n", g_fail ? "FAILURES" : "ALL PASSED", g_fail);
    return g_fail ? 1 : 0;
}
