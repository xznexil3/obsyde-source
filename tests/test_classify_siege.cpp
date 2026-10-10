#include <cstdio>
#include <cstring>

static bool StartsWith(const char* s, const char* p) {
    return strncmp(s, p, strlen(p)) == 0;
}
static bool Contains(const char* s, const char* p) {
    return strstr(s, p) != nullptr;
}
static bool Streq(const char* a, const char* b) { return strcmp(a, b) == 0; }

enum Kind { Unknown, Creep, Building, Ward, Roshan, Boss, Courier };

static Kind ClassifyByName(const char* name) {
    Kind kind = Unknown;
    if (Streq(name, "npc_dota_roshan")) kind = Roshan;
    else if (Contains(name, "observer_ward") || Contains(name, "sentry_ward") ||
             StartsWith(name, "npc_dota_ward")) kind = Ward;
    else if (StartsWith(name, "npc_dota_creep") || StartsWith(name, "npc_dota_neutral") ||
             Contains(name, "_siege")) kind = Creep;
    else if (Contains(name, "_tower") || Contains(name, "_fort") ||
             Contains(name, "_fountain") || Contains(name, "_healer")) kind = Building;
    else if (Contains(name, "courier")) kind = Courier;
    else if (Contains(name, "tormentor") || Contains(name, "miniboss")) kind = Boss;
    return kind;
}

static int g_fail = 0;
static void Check(bool ok, const char* what) {
    printf("%s %s\n", ok ? "[ok]  " : "[FAIL]", what);
    if (!ok) ++g_fail;
}

int main() {
    Check(ClassifyByName("npc_dota_goodguys_siege") == Creep, "radiant siege -> Creep");
    Check(ClassifyByName("npc_dota_badguys_siege") == Creep, "dire siege -> Creep");
    Check(ClassifyByName("npc_dota_creep_badguys_melee") == Creep, "melee creep -> Creep");
    Check(ClassifyByName("npc_dota_creep_goodguys_ranged") == Creep, "ranged creep -> Creep");
    Check(ClassifyByName("npc_dota_neutral_kobold") == Creep, "neutral -> Creep");
    Check(ClassifyByName("npc_dota_roshan") == Roshan, "roshan unchanged");
    Check(ClassifyByName("npc_dota_observer_ward") == Ward, "ward unchanged");
    Check(ClassifyByName("npc_dota_badguys_tower") == Building, "tower unchanged");
    Check(ClassifyByName("npc_dota_courier") == Courier, "courier unchanged");
    Check(ClassifyByName("npc_dota_hero_sniper") == Unknown, "hero not matched by name branch");

    printf("\n%s (%d failures)\n", g_fail ? "FAILURES" : "ALL PASSED", g_fail);
    return g_fail ? 1 : 0;
}
