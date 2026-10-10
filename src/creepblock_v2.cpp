#include "creepblock_v2.h"
#include "hook.h"

#include <cmath>
#include <cstring>
#include <cstdlib>

namespace input {
void ClickAt(HWND hwnd, int cx, int cy, bool right);
void TapKey(int vk);
}

namespace creepblock_v2 {

static DWORD s_lastOrder = 0;
static DWORD s_lastStop = 0;
static DWORD s_lastCamera = 0;
static Vec3 s_dir{};
static bool s_haveDir = false;

static bool Has(const char* text, const char* part) {
    return text && part && std::strstr(text, part) != nullptr;
}

static bool IsLaneCreep(const FrameUnit& u) {
    return u.kind == UnitKind::Creep &&
           (Has(u.name, "npc_dota_creep") || Has(u.name, "siege") || Has(u.cls, "Creep"));
}

static bool IsRanged(const FrameUnit& u) {
    return Has(u.name, "_ranged") || Has(u.name, "ranged") || Has(u.name, "siege");
}

static bool Normalize(Vec3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.5f) return false;
    v.x /= len;
    v.y /= len;
    v.z = 0.f;
    return true;
}

static bool FindDirection(const Frame& f, Vec3& out) {
    Vec3 measured{};
    int count = 0;
    for (const auto& u : f.units) {
        if (!u.alive || u.team != f.localTeam || !IsLaneCreep(u) || !u.mdirOk)
            continue;
        if (u.dist > 750.f) continue;
        measured.x += u.mdir.x;
        measured.y += u.mdir.y;
        ++count;
    }
    if (count > 0 && Normalize(measured)) {
        if (s_haveDir) {
            out.x = s_dir.x * 0.65f + measured.x * 0.35f;
            out.y = s_dir.y * 0.65f + measured.y * 0.35f;
            Normalize(out);
        } else {
            out = measured;
        }
        s_dir = out;
        s_haveDir = true;
        return true;
    }
    if (s_haveDir) { out = s_dir; return true; }

    const FrameUnit* own = nullptr;
    const FrameUnit* enemy = nullptr;
    float ownBest = 1e30f;
    float enemyBest = 1e30f;
    for (const auto& u : f.units) {
        if (!u.alive) continue;
        if (u.team == f.localTeam && IsLaneCreep(u)) {
            if (u.dist < ownBest) { ownBest = u.dist; own = &u; }
        } else if (u.team != f.localTeam &&
                   (u.kind == UnitKind::Creep || u.kind == UnitKind::Building)) {
            float d = own ? VecDist(own->pos, u.pos) : u.dist;
            if (d < enemyBest) { enemyBest = d; enemy = &u; }
        }
    }
    if (!own || !enemy) return false;
    out = Vec3{ enemy->pos.x - own->pos.x, enemy->pos.y - own->pos.y, 0.f };
    if (!Normalize(out)) return false;
    s_dir = out;
    s_haveDir = true;
    return true;
}

static const FrameUnit* PickLead(const Frame& f, const Vec3& dir) {
    bool hasMelee = false;
    for (const auto& u : f.units) {
        if (u.alive && u.team == f.localTeam && IsLaneCreep(u) &&
            u.dist <= 650.f && !IsRanged(u)) { hasMelee = true; break; }
    }

    const FrameUnit* best = nullptr;
    float bestProjection = -1e30f;
    for (const auto& u : f.units) {
        if (!u.alive || u.team != f.localTeam || !IsLaneCreep(u) || u.dist > 650.f)
            continue;
        if (hasMelee && IsRanged(u)) continue;
        float dx = u.pos.x - f.localPos.x;
        float dy = u.pos.y - f.localPos.y;
        float projection = dx * dir.x + dy * dir.y;
        if (!best || projection > bestProjection) {
            best = &u;
            bestProjection = projection;
        }
    }
    return best;
}

static bool IsSafePoint(const Frame& f, const Vec3& point, ImVec2& screen) {
    if (!view::W2S(point, screen)) return false;
    for (const auto& u : f.units) {
        if (!u.alive) continue;
        if (u.team == f.localTeam && u.kind == UnitKind::Hero) continue;
        ImVec2 unitScreen{};
        if (!view::W2S(u.pos, unitScreen)) continue;
        float dx = screen.x - unitScreen.x;
        float dy = screen.y - unitScreen.y;
        if (dx * dx + dy * dy < 30.f * 30.f) return false;
    }
    return screen.x >= 10.f && screen.y >= 10.f &&
           screen.x < view::W - 10.f && screen.y < view::H - 10.f;
}

void Run(const Frame& f) {
    if (!cfg::creepBlock || !f.ok || !f.localAlive || cfg::menuOpen) return;
    if (f.gameState == 6 || f.gameState == 7) return;

    HWND hwnd = GameHwnd();
    if (!hwnd || GetForegroundWindow() != hwnd) return;

    DWORD now = GetTickCount();

    int jitter = (std::rand() % 15) - 7;
    DWORD orderDelay = (DWORD)(220 + jitter);
    if (now - s_lastOrder < orderDelay) return;

    if (cfg::cbAutoCam && now - s_lastCamera > 2200) {
        input::TapKey(0x70);
        s_lastCamera = now;
    }

    Vec3 dir{};
    if (!FindDirection(f, dir)) return;
    const FrameUnit* lead = PickLead(f, dir);
    if (!lead) return;

    Vec3 side{ -dir.y, dir.x, 0.f };
    float dx = f.localPos.x - lead->pos.x;
    float dy = f.localPos.y - lead->pos.y;
    float ahead = dx * dir.x + dy * dir.y;
    float lateral = fabsf(dx * side.x + dy * side.y);
    bool inPosition = ahead >= 92.f && ahead <= 185.f && lateral <= 110.f;

    Vec3 destination{};
    if (inPosition) {
        destination = f.localPos;
        if (now - s_lastStop > 700) {
            input::TapKey(0x53);
            s_lastStop = now;
        }
    } else {
        destination = Vec3{
            lead->pos.x + dir.x * 190.f,
            lead->pos.y + dir.y * 190.f,
            lead->pos.z
        };
    }

    ImVec2 screen{};
    if (!IsSafePoint(f, destination, screen)) {
        Vec3 alternatives[4] = {
            { destination.x + side.x * 42.f, destination.y + side.y * 42.f, destination.z },
            { destination.x - side.x * 42.f, destination.y - side.y * 42.f, destination.z },
            { destination.x + side.x * 78.f, destination.y + side.y * 78.f, destination.z },
            { destination.x - side.x * 78.f, destination.y - side.y * 78.f, destination.z },
        };
        bool found = false;
        for (const auto& candidate : alternatives) {
            if (IsSafePoint(f, candidate, screen)) {
                destination = candidate;
                found = true;
                break;
            }
        }
        if (!found) return;
    }

    ImVec2 raw{};
    if (!view::W2SRaw(destination, raw)) return;
    if (raw.x < 0.f || raw.y < 0.f || raw.x >= view::W || raw.y >= view::H) return;
    input::ClickAt(hwnd, (int)raw.x, (int)raw.y, true);
    s_lastOrder = now;
}

}
