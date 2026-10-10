#pragma once
#include "game.h"

namespace lasthit {

struct Opt {
    bool  deny      = true;
    bool  harass    = true;
    bool  fake      = true;
    float castPoint = 0.45f;
    float projSpeed = 1200.f;
};

struct Plan {
    const FrameUnit* attack = nullptr;
    bool fake = false;
};

Plan Evaluate(const Frame& f, const Opt& o);

bool ShouldCancel(float simNow, float myLastAtk,
                  unsigned long msSinceOwnSwing, unsigned long msSinceStop);

void RunLastHit(const Frame& f);

}
