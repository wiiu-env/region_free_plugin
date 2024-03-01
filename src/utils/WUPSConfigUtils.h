#pragma once

#include "globals.h"
#include <coreinit/mcp.h>
#include <memory>
#include <wups/config.h>

typedef struct TitleRegionInfo {
    uint64_t titleId;
    Languages language;
    MCPRegion product_area;
} TitleRegionInfo;

std::optional<TitleRegionInfo> getTitleRegionInfo(uint64_t titleId, Languages default_language, MCPRegion default_product_area, bool allow_default_values);

bool saveTitleRegionInfo(const TitleRegionInfo &titleRegionInfo);

void InitConfigAndStorage();