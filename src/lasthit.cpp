#include "lasthit.h"
#include "hook.h"
#include <cstdlib>

namespace input { void ClickAt(HWND hwnd, int cx, int cy, bool right); void TapKey(int vk); }

namespace lasthit {

namespace {

struct Atk { float next, period, dmg; };

inline void AddAttacker(const FrameUnit& s, const FrameUnit& u, float now,
                        Atk* atk, int& na) {
    if (na >= 16) return;
    float dx = s.pos.x - u.pos.x, dy = s.pos.y - u.pos.y;
    float d = sqrtf(dx * dx + dy * dy);
    bool targeted = s.towerTarget != 0 && s.towerTarget == u.addr;
    if (!targeted && d > (float)s.atkRange + 150.f) return;

    float period = s.bat;
    if (period < 0.6f) period = 0.6f;
    if (period > 4.f)  period = 4.f;
    float proj = (s.kind == UnitKind::Building) ? 750.f :
                 (s.kind == UnitKind::Hero)     ? 1200.f : 900.f;
    float travel = (s.atkRange > 350) ? d / proj : 0.f;
    float first = 0.f;
    if (s.lastAtk > 0.f && now > 0.f) {
        first = (s.lastAtk + period) - now;
        if (first < 0.f) first = 0.f;
    }
    atk[na].next   = first + 0.5f + travel;
    atk[na].period = period;
    atk[na].dmg    = s.dmgAvg;
    ++na;
}

inline void SimTimeline(float hp, float myDmg, Atk* atk, int na,
                        float& Tlethal, float& Tdeath) {
    Tlethal = (hp <= myDmg) ? 0.f : -1.f;
    Tdeath  = -1.f;
    for (float t = 0.05f; t <= 5.f && Tdeath < 0.f; t += 0.05f) {
        for (int i = 0; i < na; ++i) {
            while (atk[i].next <= t) {
                hp -= atk[i].dmg;
                atk[i].next += atk[i].period;
            }
        }
        if (hp <= 0.f) { Tdeath = t; break; }
        if (Tlethal < 0.f && hp <= myDmg) Tlethal = t;
    }
}

}

Plan Evaluate(const Frame& f, const Opt& o) {
    Plan plan;
    if (!f.localAlive || f.atkRange <= 0) return plan;
    const bool myRanged = f.atkRange > 350;

    const FrameUnit* best = nullptr;
    float bestT = 1e9f;
    const FrameUnit* fakeCand = nullptr;
    float fakeGap = 1e9f;

    for (const auto& u : f.units) {
        if (!u.alive || u.kind != UnitKind::Creep) continue;
        const bool isDeny = (u.team == f.localTeam);
        if (isDeny) {
            if (!o.deny || !u.canDeny) continue;
        }
        if (u.dist > (float)f.atkRange + 90.f) continue;
        if (!(u.dmgOut > 1.f)) continue;

        float tMe = o.castPoint + (myRanged ? u.dist / o.projSpeed : 0.f);

        Atk atk[16];
        int na = 0;
        for (const auto& s : f.units) {
            if (!s.alive || s.addr == u.addr || s.dmgAvg <= 1.f) continue;
            if (s.team == u.team) continue;
            AddAttacker(s, u, f.now, atk, na);
        }

        float Tlethal, Tdeath;
        SimTimeline((float)u.hp, u.dmgOut, atk, na, Tlethal, Tdeath);
        if (Tlethal < 0.f) continue;

        if (tMe < Tlethal - 0.10f) {
            if (!isDeny && !myRanged && o.fake) {
                float gap = Tlethal - tMe;
                if (gap > 0.05f && gap < 0.40f && gap < fakeGap) {
                    fakeGap = gap;
                    fakeCand = &u;
                }
            }
            continue;
        }
        if (Tdeath >= 0.f && tMe >= Tdeath) continue;

        if (Tlethal < bestT) { bestT = Tlethal; best = &u; }
    }

    if (best) { plan.attack = best; plan.fake = false; return plan; }

    if (fakeCand) { plan.attack = fakeCand; plan.fake = true; return plan; }

    if (o.harass) {
        const FrameUnit* hero = nullptr;
        for (const auto& u : f.units) {
            if (!u.alive || u.kind != UnitKind::Hero) continue;
            if (u.team == f.localTeam || u.illusion) continue;
            if (u.dist > (float)f.atkRange + 100.f) continue;
            if (!hero || u.dist < hero->dist) hero = &u;
        }
        if (hero) { plan.attack = hero; return plan; }
    }

    return plan;
}

bool ShouldCancel(float simNow, float myLastAtk,
                  unsigned long msSinceOwnSwing, unsigned long msSinceStop) {
    return msSinceOwnSwing > 600 &&
           msSinceStop > 300 &&
           myLastAtk > 0.f &&
           (simNow - myLastAtk) < 0.45f;
}

void RunLastHit(const Frame& f) {
    if (!f.ok) return;
    if (f.gameState == 6 || f.gameState == 7) return;
    if (!cfg::autoLastHit) return;
    if (!f.localAlive) return;
    if (cfg::menuOpen) return;

    HWND hwnd = GameHwnd();
    if (!hwnd || GetForegroundWindow() != hwnd) return;

    static DWORD     s_lhAt = 0, s_swingAt = 0, s_stopAt = 0, s_fakeAt = 0;
    DWORD now = GetTickCount();

    int jit = 0;
    if (cfg::lhHumanizer > 0.f) {
        int h = (int)cfg::lhHumanizer;
        jit = (int)(rand() % (2 * h + 1)) - h;
    }
    int cadence = 250 + jit;
    if (cadence < 120) cadence = 120;
    if (now - s_lhAt < (DWORD)cadence) return;

    Opt o;
    o.deny      = cfg::autoDeny;
    o.harass    = cfg::lhHarass;
    o.fake      = cfg::lhFakeSpam;
    o.castPoint = cfg::lhCastPoint;
    o.projSpeed = cfg::lhProjSpeed;

    Plan p = Evaluate(f, o);

    if (p.attack) {
        Vec3 tgt{ p.attack->pos.x, p.attack->pos.y, p.attack->pos.z + 70.f };
        ImVec2 sp;
        if (view::W2S(tgt, sp) &&
            sp.x >= 0.f && sp.y >= 0.f && sp.x < view::W && sp.y < view::H) {
            input::ClickAt(hwnd, (int)sp.x, (int)sp.y, true);
            s_lhAt = now;
            s_swingAt = now;
            if (p.fake) s_fakeAt = now;
            else s_fakeAt = 0;
        }
        return;
    }

    if (s_fakeAt && now - s_fakeAt >= 120) {
        input::TapKey(0x53);
        s_fakeAt = 0;
        s_stopAt = now;
        s_lhAt = now;
        return;
    }

    if (ShouldCancel(f.now, f.myLastAtk, now - s_swingAt, now - s_stopAt)) {
        input::TapKey(0x53);
        s_stopAt = now;
        s_lhAt = now;
    }
}

}
