#include "SettingsUtils.h"
#include "function_patches.h"
#include "globals.h"
#include "logger.h"
#include <coreinit/userconfig.h>
#include <cstring>
#include <malloc.h>
#include <nn/acp/title.h>
#include <optional>
#include <string_view>

template<typename T>
static std::optional<T> readU32FromConfig(std::string_view config_name) {
    static_assert(sizeof(T) == sizeof(uint32_t), "T must be an enum of size sizeof(uint32_t)");
    auto ucHandle           = UCOpen();
    std::optional<T> result = {};
    if (ucHandle >= 0) {
        UCSysConfig sysConfig = {};
        uint32_t data         = 0xFFFFFFFF;
        sysConfig.dataType    = UC_DATATYPE_UNSIGNED_INT;
        sysConfig.dataSize    = 4;
        sysConfig.data        = &data;
        strncpy(sysConfig.name, config_name.data(), 64);
        if (real_UCReadSysConfig(ucHandle, 1, &sysConfig) >= 0) {
            result = static_cast<T>(*(uint32_t *) sysConfig.data);
        }
        UCClose(ucHandle);
    }
    return result;
}

std::optional<int32_t> getRealCountry() {
    return readU32FromConfig<uint32_t>("cafe.cntry_reg");
}

std::optional<Languages> getRealLanguage() {
    return readU32FromConfig<Languages>("cafe.language");
}

std::optional<MCPRegion> getRealRegion() {
    std::optional<MCPRegion> result = {};
    auto handle                     = MCP_Open();
    if (handle >= 0) {
        __attribute__((aligned(0x40))) MCPSysProdSettings data;
        auto res = real_MCP_GetSysProdSettings(handle, &data);
        if (res >= 0) {
            result = (MCPRegion) data.product_area;
        }
        MCP_Close(handle);
    }
    return result;
}

std::optional<uint32_t> getRegionForTitle(uint64_t titleId) {
    auto *acpMetaXml = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));
    if (!acpMetaXml) {
        DEBUG_FUNCTION_LINE_ERR("Failed to alloc memory for ACPMetaXml");
        return {};
    }
    std::optional<uint32_t> result = {};
    auto res                       = ACPGetTitleMetaXml(titleId, acpMetaXml);
    if (res >= 0) {
        result = (uint32_t) acpMetaXml->region;
    }
    free(acpMetaXml);
    return result;
}