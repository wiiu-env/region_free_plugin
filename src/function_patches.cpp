#include "utils/SettingsUtils.h"
#include "utils/logger.h"
#include <coreinit/debug.h>
#include <coreinit/userconfig.h>
#include <nn/acp/title.h>
#include <string_view>
#include <sysapp/switch.h>
#include <sysapp/title.h>
#include <wups/function_patching.h>

DECL_FUNCTION(int32_t, ACPGetTitleMetaXmlByDevice, uint64_t titleid, ACPMetaXml *metaxml, uint32_t device) {
    int result = real_ACPGetTitleMetaXmlByDevice(titleid, metaxml, device);
    if (metaxml != nullptr) {
        metaxml->region = 0xFFFFFFFF;
    }
    return result;
}

DECL_FUNCTION(int32_t, ACPGetLaunchMetaXml, ACPMetaXml *metaxml) {
    int result = real_ACPGetLaunchMetaXml(metaxml);
    if (metaxml != nullptr) {
        metaxml->region = 0xFFFFFFFF;
    }

    return result;
}

DECL_FUNCTION(int, UCReadSysConfig, int IOHandle, int count, UCSysConfig *settings) {
    int result = real_UCReadSysConfig(IOHandle, count, settings);
    auto upid  = OSGetUPID();

    if (!gRegionFreeValuesSetupDone || result != 0 || (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL)) {
        return result;
    }

    if (std::string_view("cafe.language") == settings->name) {
        DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: cafe.language found!");
        DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: forcing language...");
        DEBUG_FUNCTION_LINE("UCReadSysConfig: original lang %d, new %d", *((int *) settings->data), gCurrentLanguage);
        *((int *) settings->data) = gCurrentLanguage;
    } else if (std::string_view("cafe.cntry_reg") == settings->name) {
        DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: cafe.cntry_reg found!");
        DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: forcing cntry_reg...");
        DEBUG_FUNCTION_LINE("UCReadSysConfig: original cntry_reg %d, new %d", *((int *) settings->data), gCurrentCountry);
        *((int *) settings->data) = gCurrentCountry;
    }

    return result;
}

DECL_FUNCTION(int, MCP_GetSysProdSettings, int IOHandle, MCPSysProdSettings *settings) {
    int result = real_MCP_GetSysProdSettings(IOHandle, settings);
    auto upid  = OSGetUPID();
    if (!gRegionFreeValuesSetupDone || result != 0 || (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL)) {
        return result;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("MCP_GetSysProdSettings: forcing platform region...");
    DEBUG_FUNCTION_LINE("MCP_GetSysProdSettings: original region %d, new %d...", settings->product_area, gCurrentProductArea);
    settings->product_area = gCurrentProductArea;

    return result;
}

DECL_FUNCTION(uint64_t, _SYSGetSystemApplicationTitleIdByProdArea, SYSTEM_APP_ID param_1, MCPRegion param_2) {
    auto upid = OSGetUPID();
    if (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL) {
        return real__SYSGetSystemApplicationTitleIdByProdArea(param_1, param_2);
    }

    return real__SYSGetSystemApplicationTitleIdByProdArea(param_1, gRealRegionOpt.value_or(param_2));
}

WUPS_MUST_REPLACE_FOR_PROCESS(_SYSGetSystemApplicationTitleIdByProdArea, WUPS_LOADER_LIBRARY_SYSAPP, _SYSGetSystemApplicationTitleIdByProdArea, WUPS_FP_TARGET_PROCESS_ALL);
WUPS_MUST_REPLACE(ACPGetTitleMetaXmlByDevice, WUPS_LOADER_LIBRARY_NN_ACP, ACPGetTitleMetaXmlByDevice);
WUPS_MUST_REPLACE(ACPGetLaunchMetaXml, WUPS_LOADER_LIBRARY_NN_ACP, ACPGetLaunchMetaXml);
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_GetSysProdSettings, WUPS_LOADER_LIBRARY_COREINIT, MCP_GetSysProdSettings, WUPS_FP_TARGET_PROCESS_ALL);
WUPS_MUST_REPLACE_FOR_PROCESS(UCReadSysConfig, WUPS_LOADER_LIBRARY_COREINIT, UCReadSysConfig, WUPS_FP_TARGET_PROCESS_ALL);