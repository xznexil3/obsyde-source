#pragma once

#include "common.h"

namespace schema_dump {

bool DumpFromScope(uintptr_t scope);

bool MatrixValid(uintptr_t addr);
uintptr_t ScanMatrix(uintptr_t lo, uintptr_t hi, uintptr_t prefer);
bool HealMatrix(uintptr_t clientBase, uint32_t imageSize);

int WritableRanges(uintptr_t base, uint32_t imageSize, uintptr_t* out, int maxPairs);

void Run(uintptr_t clientBase, uint32_t imageSize);

int Resolved();
int Wanted();

}
