#pragma once

#include "Tracy.hpp"
#include "rx/core/types.h"

enum SubsystemMask : Uint32 {
    SubsystemNeverProfile = 0,
    SubsystemAlwaysProfile = 0xFFFFFFFF,

    SubsystemEngine = 1 << 0,
    SubsystemRenderer = 1 << 1,
    SubsystemStreaming = 1 << 2,
    SubsystemWorldGen = 1 << 3,
    SubsystemScripting = 1 << 4,
    SubsystemSerialization = 1 << 5,
};

#define SUBSYSTEMS_TO_PROFILE 0xFFFFFFFF
