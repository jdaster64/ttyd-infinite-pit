#pragma once

#include <cstdint>

namespace mod::infinite_pit::battle {

// Apply patches to various battle features.
void ApplyFixedPatches();

// Overrides the default target audience amount to be based on Pit progression.
void SetTargetAudienceAmount();

}