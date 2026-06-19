#pragma once
#include <vector>
#include "core/model.h"

class Skeleton;

class IKSolver {
public:
    static void solve(int ik_bone_idx, const std::vector<BoneDef>& defs, Skeleton& skeleton);
};
