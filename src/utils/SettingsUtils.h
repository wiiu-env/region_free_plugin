#pragma once

#include "globals.h"
#include <cstdint>
#include <optional>

std::optional<int32_t> getRealCountry();

std::optional<Languages> getRealLanguage();

std::optional<MCPRegion> getRealRegion();

std::optional<uint32_t> getRegionForTitle(uint64_t titleId);