#include "creepblock_v1.h"
#include "hook.h"
#include <cstdlib>

namespace input { void ClickAt(HWND hwnd, int cx, int cy, bool right); void TapKey(int vk); }

namespace creepblock_v1 {

const FrameUnit* PickLeadCreep(const Frame& f) {
    Vec3 enemyBase = BasePos(f.localTeam == 2 ? 3 : 2);
    const FrameUnit* lead = nullptr;
    float bestD = 1e30f;
    for (const auto& u : f.units) {
        if (!u.alive || !IsLaneCreep(u)) continue;
        if (u.team != f.localTeam) continue;
        if (cfg::cbIgnoreRanged && IsRangedCreepName(u.name)) continue;

        float dxh = u.pos.x - f.localPos.x, dyh = u.pos.y - f.localPos.y;
        if (dxh * dxh + dyh * dyh > 450.f * 450.f) continue;

        float dxe = u.pos.x - enemyBase.x, dye = u.pos.y - enemyBase.y;
        float de = dxe * dxe + dye * dye;
        if (de < bestD) { bestD = de; lead = &u; }
    }
    return lead;
}

void RunCreepBlock(const Frame& f) {
    if (!f.ok) return;
    if (f.gameState == 6 || f.gameState == 7) return;
    if (!cfg::creepBlock) return;
    if (!f.localAlive) return;
    if (cfg::menuOpen) return;

    HWND hwnd = GameHwnd();
    if (!hwnd || GetForegroundWindow() != hwnd) return;

    static DWORD s_lastCmd = 0;
    static DWORD s_lastCam = 0;
    DWORD now = GetTickCount();
    int jitter = (rand() % 14) - 7;
    int dyn = (int)cfg::cbDelay + jitter;
    if (dyn < 30) dyn = 30;
    if (now - s_lastCmd < (DWORD)dyn) return;

    if (cfg::cbAutoCam && now - s_lastCam > 2000) {
        input::TapKey(0x70);
        s_lastCam = now;
    }

    const FrameUnit* lead = PickLeadCreep(f);
    if (!lead) return;

    Vec3 wdir = lead->mdirOk ? lead->mdir : Vec3{ 0.f, 0.f, 0.f };
    float wl = sqrtf(wdir.x * wdir.x + wdir.y * wdir.y);
    if (wl < 0.5f) {
        Vec3 eb = BasePos(lead->team == 2 ? 3 : 2);
        float dx = eb.x - lead->pos.x, dy = eb.y - lead->pos.y;
        float l = sqrtf(dx * dx + dy * dy);
        if (l > 1.f) { wdir = Vec3{ dx / l, dy / l, 0.f }; wl = 1.f; }
    }
    if (wl < 0.5f) return;

    float heroHull = 24.f;
    float face = lead->hullR + heroHull + 10.f;

    float hx = f.localPos.x - lead->pos.x, hy = f.localPos.y - lead->pos.y;
    float ahead = hx * wdir.x + hy * wdir.y;
    float latx = hx - wdir.x * ahead, laty = hy - wdir.y * ahead;
    float lateral = sqrtf(latx * latx + laty * laty);

    static DWORD s_lastStop = 0;

    if (!lead->mdirOk && ahead < face + 60.f && lateral < face + 60.f) {
        if (now - s_lastStop > 500) { input::TapKey(0x53); s_lastStop = now; }
        s_lastCmd = now;
        return;
    }

    if (ahead < face - 5.f || lateral > face + 15.f) {
        Vec3 click{ lead->pos.x + wdir.x * (face + 40.f),
                    lead->pos.y + wdir.y * (face + 40.f), lead->pos.z };

        Vec3 perp{ -wdir.y, wdir.x, 0.f };
        const float kOff[5] = { 0.f, 25.f, -25.f, 50.f, -50.f };
        bool placed = false;
        for (float off : kOff) {
            Vec3 cand{ click.x + perp.x * off, click.y + perp.y * off, click.z };
            ImVec2 cs;
            if (!view::W2S(cand, cs)) continue;
            bool bad = false;
            for (const auto& e2 : f.units) {
                if (!e2.alive) continue;
                if (e2.team == f.localTeam && e2.kind == UnitKind::Hero) continue;
                ImVec2 es;
                if (!view::W2S(e2.pos, es)) continue;
                float dx = cs.x - es.x, dy = cs.y - es.y;
                if (dx * dx + dy * dy < 30.f * 30.f) { bad = true; break; }
            }
            if (!bad) { click = cand; placed = true; break; }
        }
        if (!placed) return;

        Vec3 tp{ click.x, click.y, click.z + 40.f };
        ImVec2 sp;
        if (view::W2SRaw(tp, sp) &&
            sp.x >= 0.f && sp.y >= 0.f && sp.x < view::W && sp.y < view::H) {
            input::ClickAt(hwnd, (int)sp.x, (int)sp.y, true);
            s_lastCmd = now;
        }
    } else {
        if (now - s_lastStop > 500) { input::TapKey(0x53); s_lastStop = now; }
        s_lastCmd = now;
    }
}

}
