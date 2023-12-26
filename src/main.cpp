#include "globals.h"
#include "utils/ConfigUtils.h"
#include "utils/logger.h"
#include <coreinit/debug.h>
#include <coreinit/mcp.h>
#include <coreinit/title.h>
#include <coreinit/userconfig.h>
#include <cstdio>
#include <malloc.h>
#include <map>
#include <nn/acp.h>
#include <sysapp/switch.h>
#include <sysapp/title.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>

WUPS_PLUGIN_NAME("Region Free Plugin");
WUPS_PLUGIN_DESCRIPTION("Allows the user to load titles from other regions");
WUPS_PLUGIN_VERSION(VERSION_FULL);
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("GPL");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("region_free_plugin");

bool getRealProductArea(MCPRegion *out);

DECL_FUNCTION(int32_t, ACPGetLaunchMetaXml, ACPMetaXml *metaxml) {
    int result = real_ACPGetLaunchMetaXml(metaxml);
    if (metaxml != nullptr) {
        metaxml->region = 0xFFFFFFFF;
    }

    return result;
}

DECL_FUNCTION(int, UCReadSysConfig, int IOHandle, int count, struct UCSysConfig *settings) {
    int result = real_UCReadSysConfig(IOHandle, count, settings);
    auto upid  = OSGetUPID();
    if (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL) {
        return result;
    }

    if (gForceSettingsEnabled) {
        if (result != 0) {
            return result;
        }

        if (std::string_view("cafe.language") == settings->name) {
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: cafe.language found!");
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: forcing language...");
            DEBUG_FUNCTION_LINE("UCReadSysConfig: original lang %d, new %d", *((int *) settings->data), gCurrentLanguage);
            *((int *) settings->data) = gCurrentLanguage;
        }
        if (std::string_view("cafe.cntry_reg") == settings->name) {
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: cafe.cntry_reg found!");
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: forcing cntry_reg...");
            DEBUG_FUNCTION_LINE("UCReadSysConfig: original cntry_reg %d, new %d", *((int *) settings->data), gCurrentCountry);
            *((int *) settings->data) = gCurrentCountry;
        }
    }

    return result;
}

#define CAT_GENERAL_ROOT           "root"
#define CAT_GENERAL_SETTINGS       "general_settings"
#define CAT_TITLE_SETTINGS         "title_settings"

#define VAL_LANGUAGE               "language"
#define VAL_COUNTRY                "cntry_reg"
#define VAL_PRODUCT_AREA           "product_area"

#define VAL_SKIP_OWN_REGION        "skip_own_region"
#define VAL_PREFER_SYSTEM_SETTINGS "prefer_system_settings"
#define VAL_AUTO_DETECTION         "auto_detection"
#define VAL_DEFAULT_LANG_EUR       "default_lang_eur"
#define VAL_DEFAULT_LANG_USA       "default_lang_usa"
#define VAL_DEFAULT_LANG_JPN       "default_lang_jpn"

#define VAL_DEFAULT_COUNTRY_EUR    "default_cntry_reg_eur"
#define VAL_DEFAULT_COUNTRY_USA    "default_cntry_reg_usa"
#define VAL_DEFAULT_COUNTRY_JPN    "default_cntry_reg_jpn"

DECL_FUNCTION(int32_t, ACPGetTitleMetaXmlByDevice, uint32_t titleid_upper, uint32_t titleid_lower, ACPMetaXml *metaxml, uint32_t device) {
    int result = real_ACPGetTitleMetaXmlByDevice(titleid_upper, titleid_lower, metaxml, device);
    if (metaxml != nullptr) {
        metaxml->region = 0xFFFFFFFF;
    }
    return result;
}

void bootStuff() {
    MCPRegion real_product_area;
    auto real_product_area_valid = getRealProductArea(&real_product_area);
    if (real_product_area_valid) {
        if (real_product_area == MCP_REGION_EUROPE) {
            gDefaultProductArea = MCP_REGION_EUROPE;
            gDefaultLanguage    = gDefaultLangForEUR;
            gDefaultCountry     = gDefaultCountryForEUR;
        } else if (real_product_area == MCP_REGION_JAPAN) {
            gDefaultProductArea = MCP_REGION_JAPAN;
            gDefaultLanguage    = gDefaultLangForJPN;
            gDefaultCountry     = gDefaultCountryForJPN;
        }
        if (real_product_area == MCP_REGION_USA) {
            gDefaultProductArea = MCP_REGION_USA;
            gDefaultLanguage    = gDefaultLangForUSA;
            gDefaultCountry     = gDefaultCountryForUSA;
        }
    }

    bool forceConfigMenu   = false;
    auto *acpMetaXml       = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));
    uint32_t regionFromXML = 0;
    if (acpMetaXml) {
        memset(acpMetaXml, 0, sizeof(ACPMetaXml));
        ACPInitialize();
        auto res = ACPGetTitleMetaXml(OSGetTitleID(), acpMetaXml);
        if (res >= 0) {
            regionFromXML = acpMetaXml->region;
            if (real_product_area_valid && (regionFromXML & real_product_area) == real_product_area) {
                gCurrentProductArea = real_product_area;
            } else {
                auto curTitleId = OSGetTitleID();
                if (curTitleId == 0x0005001010040000L || regionFromXML == 1) {
                    DEBUG_FUNCTION_LINE("Set default to JAPAN");
                    gDefaultProductArea = MCP_REGION_JAPAN;
                    gDefaultLanguage    = gDefaultLangForJPN;
                    gDefaultCountry     = gDefaultCountryForJPN;
                } else if (curTitleId == 0x0005001010040100L || regionFromXML == 2) {
                    DEBUG_FUNCTION_LINE("Set default to USA");
                    gDefaultProductArea = MCP_REGION_USA;
                    gDefaultLanguage    = gDefaultLangForUSA;
                    gDefaultCountry     = gDefaultCountryForUSA;
                } else if (curTitleId == 0x0005001010040200L || curTitleId == 0x0005001001004E200L || regionFromXML == 4) {
                    DEBUG_FUNCTION_LINE("Set default to EUR");
                    gDefaultProductArea = MCP_REGION_EUROPE;
                    gDefaultLanguage    = gDefaultLangForEUR;
                    gDefaultCountry     = gDefaultCountryForEUR;
                } else {
                    DEBUG_FUNCTION_LINE_ERR("Unknown area %08X, force menu", acpMetaXml->region);
                    forceConfigMenu = true;
                }
            }
        } else {
            DEBUG_FUNCTION_LINE_ERR("ACPGetTitleMetaXml failed");
            forceConfigMenu = true;
        }
        ACPFinalize();
        free(acpMetaXml);
    } else {
        DEBUG_FUNCTION_LINE_ERR("Failed to allocate acpMetaXml");
        forceConfigMenu = true;
    }

    // Get region and lang from console and set these as default.
    gCurrentLanguage    = gDefaultLanguage;
    gCurrentCountry     = gDefaultCountry;
    gCurrentProductArea = gDefaultProductArea;

    if (gPreferSystemSettings && real_product_area_valid) {
        if ((regionFromXML & real_product_area) == real_product_area) {
            gCurrentProductArea = real_product_area;

            auto ucHandle = UCOpen();
            if (ucHandle >= 0) {
                UCSysConfig sysConfig;
                memset((void *) &sysConfig, 0, sizeof(sysConfig));
                uint32_t data      = 0xFFFFFFFF;
                sysConfig.dataType = UC_DATATYPE_UNSIGNED_INT;
                sysConfig.dataSize = 4;
                sysConfig.data     = &data;
                strncpy(sysConfig.name, "cafe.language", 64);
                int ucRes = real_UCReadSysConfig(ucHandle, 1, &sysConfig);

                if (ucRes >= 0) {
                    DEBUG_FUNCTION_LINE("Force default language to system language for own region");
                    gCurrentLanguage = static_cast<Lanuages>(*(uint32_t *) sysConfig.data);
                    gDefaultLanguage = static_cast<Lanuages>(*(uint32_t *) sysConfig.data);
                } else {
                    DEBUG_FUNCTION_LINE_ERR("UCReadSysConfig failed");
                }

                memset((void *) &sysConfig, 0, sizeof(sysConfig));
                data               = 0xFFFFFFFF;
                sysConfig.dataType = UC_DATATYPE_UNSIGNED_INT;
                sysConfig.dataSize = 4;
                sysConfig.data     = &data;
                strncpy(sysConfig.name, "cafe.cntry_reg", 64);
                ucRes = real_UCReadSysConfig(ucHandle, 1, &sysConfig);

                if (ucRes >= 0) {
                    DEBUG_FUNCTION_LINE("Force default country to system country for own region");
                    gCurrentCountry = (int32_t) * (uint32_t *) sysConfig.data;
                    gDefaultCountry = (int32_t) * (uint32_t *) sysConfig.data;
                } else {
                    DEBUG_FUNCTION_LINE_ERR("UCReadSysConfig failed");
                }
                UCClose(ucHandle);
            } else {
                DEBUG_FUNCTION_LINE_ERR("UCOpen failed");
            }
        }
    }
    WUPSStorageError storageError;
    auto rootStorage = WUPSStorageAPI::GetSubItem(CAT_GENERAL_ROOT, storageError);
    if (!rootStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to read %s subitem", CAT_GENERAL_ROOT);
        return;
    }
    auto titleSettings = rootStorage->GetOrCreateSubItem(CAT_TITLE_SETTINGS, storageError);
    if (!titleSettings) {
        DEBUG_FUNCTION_LINE_ERR("WUPS_CreateSubItem %s failed", CAT_TITLE_SETTINGS);
        return;
    }

    char buffer[18];
    snprintf(buffer, 17, "%016llX", OSGetTitleID());

    auto curTitle = titleSettings->GetOrCreateSubItem(buffer, storageError);

    if (!curTitle) {
        DEBUG_FUNCTION_LINE_ERR("WUPS_CreateSubItem %s failed", buffer);
        return;
    }

    storageError = curTitle->GetOrStoreDefault(VAL_LANGUAGE, gCurrentLanguage, gCurrentLanguage);
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or store current Language");
    }
    storageError = curTitle->GetOrStoreDefault(VAL_PRODUCT_AREA, gCurrentProductArea, gCurrentProductArea);
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or store current Language");
    }

    bool isWiiUMenu = false;
    if (OSGetTitleID() == 0x0005001010040000L || // Wii U Menu JPN
        OSGetTitleID() == 0x0005001010040100L || // Wii U Menu USA
        OSGetTitleID() == 0x0005001010040200L) { // Wii U Menu EUR
        isWiiUMenu = true;
    }

    bool showMenu = !gAutoDetection;

    if (real_product_area_valid && (regionFromXML & real_product_area) == real_product_area) {
        if (gSkipOwnRegion && !forceConfigMenu) {
            // If the want to skip checks for own region, and we were able
            // to tell the region of the current title don't show the menu.
            showMenu = false;
            return;
        }
    }

    // this overrides the current settings
    if (forceConfigMenu || (!isWiiUMenu && showMenu)) {
        ConfigUtils::openConfigMenu();
        // Save settings to storage
        curTitle->Store(VAL_LANGUAGE, gCurrentLanguage);
        curTitle->Store(VAL_PRODUCT_AREA, gCurrentProductArea);
    }

    DEBUG_FUNCTION_LINE("Language will be force to %d", gCurrentLanguage);
    DEBUG_FUNCTION_LINE("Country will be force to %d", gDefaultCountry);
    DEBUG_FUNCTION_LINE("Product Area will be forced to %d", gCurrentProductArea);
}

std::optional<WUPSStorageSubItem> rootSettingsStorage    = {};
std::optional<WUPSStorageSubItem> generalSettingsStorage = {};

ON_APPLICATION_START() {
    initLogging();

    WUPSStorageError storageError;
    rootSettingsStorage = WUPSStorageAPI::GetOrCreateSubItem(CAT_GENERAL_ROOT, storageError);
    if (!rootSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create %s subitem", CAT_GENERAL_ROOT);
    }

    generalSettingsStorage = rootSettingsStorage->GetOrCreateSubItem(CAT_GENERAL_SETTINGS, storageError);
    if (!generalSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create %s subitem", CAT_GENERAL_SETTINGS);
    } else {
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_AUTO_DETECTION, gAutoDetection, DEFAULT_AUTO_DETECTION_VALUE)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_PREFER_SYSTEM_SETTINGS, gPreferSystemSettings, DEFAULT_PREFER_SYSTEM_SETTINGS)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_SKIP_OWN_REGION, gSkipOwnRegion, DEFAULT_SKIP_OWN_REGION)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }

        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_LANG_EUR, gDefaultLangForEUR, DEFAULT_LANG_FOR_EUR)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_COUNTRY_EUR, gDefaultCountryForEUR, DEFAULT_COUNTRY_FOR_EUR)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }

        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_LANG_USA, gDefaultLangForUSA, DEFAULT_LANG_FOR_USA)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_COUNTRY_USA, gDefaultCountryForUSA, DEFAULT_COUNTRY_FOR_USA)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }

        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_LANG_JPN, gDefaultLangForJPN, DEFAULT_LANG_FOR_JPN)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
        if ((storageError = generalSettingsStorage->GetOrStoreDefault(VAL_DEFAULT_COUNTRY_JPN, gDefaultCountryForJPN, DEFAULT_COUNTRY_FOR_JPN)) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_WARN("Failed to get or store default: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
        }
    }

    gForceSettingsEnabled = 1;
    if (OSGetTitleID() == 0x0005001010040000L || // Wii U Menu JPN
        OSGetTitleID() == 0x0005001010040100L || // Wii U Menu USA
        OSGetTitleID() == 0x0005001010040200L) { // Wii U Menu EUR
        gForceSettingsEnabled = 0;
    }

    bootStuff();

    WUPSStorageAPI::SaveStorage();
}

void bool_item_changed(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE("Value changed for %s: %d", item->identifier ? item->identifier : "[UNKNOWN]", newValue);

    if (std::string_view(VAL_DEFAULT_LANG_USA) == item->identifier) {
        gAutoDetection = newValue;
        if (generalSettingsStorage) {
            generalSettingsStorage->Store(VAL_AUTO_DETECTION, gAutoDetection);
        }
    } else if (std::string_view(VAL_PREFER_SYSTEM_SETTINGS) == item->identifier) {
        gPreferSystemSettings = newValue;
        if (generalSettingsStorage) {
            generalSettingsStorage->Store(VAL_PREFER_SYSTEM_SETTINGS, gPreferSystemSettings);
        }
    } else if (std::string_view(VAL_SKIP_OWN_REGION) == item->identifier) {
        gPreferSystemSettings = newValue;
        if (generalSettingsStorage) {
            generalSettingsStorage->Store(VAL_SKIP_OWN_REGION, gSkipOwnRegion);
        }
    }
}

void default_lang_changed(ConfigItemMultipleValues *item, uint32_t newValue) {
    DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);

    if (std::string_view(VAL_DEFAULT_LANG_EUR) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default eur lang");
        gDefaultLangForEUR = (Lanuages) newValue;
    } else if (std::string_view(VAL_DEFAULT_LANG_USA) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default usa lang");
        gDefaultLangForUSA = (Lanuages) newValue;
    } else if (std::string_view(VAL_DEFAULT_LANG_JPN) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default jpn lang");
        gDefaultLangForJPN = (Lanuages) newValue;
    } else {
        DEBUG_FUNCTION_LINE_WARN("Unexpected identifier for default_lang_changed");
        return;
    }
    if (generalSettingsStorage) {
        generalSettingsStorage->Store(item->identifier, (int32_t) newValue);
    } else {
        DEBUG_FUNCTION_LINE_WARN("Failed to store into general settings: sub-item was nullopt");
    }
}

static WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    try {
        WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

        root.add(WUPSConfigItemBoolean::Create(VAL_DEFAULT_LANG_USA, "Auto detect region/language",
                                               DEFAULT_AUTO_DETECTION_VALUE, gAutoDetection,
                                               &bool_item_changed));
        root.add(WUPSConfigItemBoolean::Create(VAL_SKIP_OWN_REGION, "Force auto detection for in-region titles",
                                               DEFAULT_SKIP_OWN_REGION, gSkipOwnRegion,
                                               &bool_item_changed));
        root.add(WUPSConfigItemBoolean::Create(VAL_PREFER_SYSTEM_SETTINGS, "Prefer system settings for in-region titles",
                                               DEFAULT_PREFER_SYSTEM_SETTINGS, gPreferSystemSettings,
                                               &bool_item_changed));

        constexpr WUPSConfigItemMultipleValues::ValuePair eur_lang_map[] = {
                {LANG_ENGLISH, "English"},
                {LANG_FRANCAIS, "Francais"},
                {LANG_DEUTSCH, "Deutsch"},
                {LANG_ITALIANO, "Italiano"},
                {LANG_ESPANOL, "Espanol"},
                {LANG_NEDERLANDS, "Nederlands"},
                {LANG_PORTUGUES, "Portugues"},
                {LANG_RUSSKI, "Russki"},
        };

        root.add(WUPSConfigItemMultipleValues::CreateFromValue(VAL_DEFAULT_LANG_EUR, "Default language for EUR",
                                                               DEFAULT_LANG_FOR_EUR, gDefaultLangForEUR,
                                                               eur_lang_map,
                                                               default_lang_changed));

        constexpr WUPSConfigItemMultipleValues::ValuePair usa_lang_map[] = {
                {LANG_ENGLISH, "English"},
                {LANG_FRANCAIS, "Francais"},
                {LANG_ESPANOL, "Espanol"},
                {LANG_PORTUGUES, "Portugues"}};

        root.add(WUPSConfigItemMultipleValues::CreateFromValue(VAL_DEFAULT_LANG_USA, "Default language for USA",
                                                               DEFAULT_LANG_FOR_USA, gDefaultLangForUSA,
                                                               usa_lang_map,
                                                               default_lang_changed));

    } catch (std::exception &e) {
        OSReport("Exception T_T : %s\n", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void ConfigMenuClosedCallback() {
    // Save all changes
    WUPSStorageAPI::SaveStorage();
}

INITIALIZE_PLUGIN() {
    initLogging();

    WUPSConfigAPIOptionsV1 configOptions = {.name = "Region Free Plugin"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }
    deinitLogging();
}

DECL_FUNCTION(int, MCP_GetSysProdSettings, int IOHandle, struct MCPSysProdSettings *settings) {
    int result = real_MCP_GetSysProdSettings(IOHandle, settings);

    auto upid = OSGetUPID();
    if (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL) {
        return result;
    }

    if (gForceSettingsEnabled) {
        if (result != 0) {
            return result;
        }

        DEBUG_FUNCTION_LINE_VERBOSE("MCP_GetSysProdSettings: forcing platform region...");
        DEBUG_FUNCTION_LINE("MCP_GetSysProdSettings: original region %d, new %d...", settings->product_area, gCurrentProductArea);
        settings->product_area = gCurrentProductArea;
    }

    return result;
}

static const uint64_t
        sSysAppTitleId[][3] =
                {
                        {
                                // Updater
                                0x0005001010040000ull,
                                0x0005001010040100ull,
                                0x0005001010040200ull,
                        },

                        {
                                // System Settings
                                0x0005001010047000ull,
                                0x0005001010047100ull,
                                0x0005001010047200ull,
                        },

                        {
                                // Parental Controls
                                0x0005001010048000ull,
                                0x0005001010048100ull,
                                0x0005001010048200ull,
                        },

                        {
                                // User Settings
                                0x0005001010049000ull,
                                0x0005001010049100ull,
                                0x0005001010049200ull,
                        },

                        {
                                // Mii Maker
                                0x000500101004A000ull,
                                0x000500101004A100ull,
                                0x000500101004A200ull,
                        },

                        {
                                // Account Settings
                                0x000500101004B000ull,
                                0x000500101004B100ull,
                                0x000500101004B200ull,
                        },

                        {
                                // Daily log
                                0x000500101004C000ull,
                                0x000500101004C100ull,
                                0x000500101004C200ull,
                        },

                        {
                                // Notifications
                                0x000500101004D000ull,
                                0x000500101004D100ull,
                                0x000500101004D200ull,
                        },

                        {
                                // Health and Safety Information
                                0x000500101004E000ull,
                                0x000500101004E100ull,
                                0x000500101004E200ull,
                        },

                        {
                                // Electronic Manual
                                0x0005001B10059000ull,
                                0x0005001B10059100ull,
                                0x0005001B10059200ull,
                        },

                        {
                                // Wii U Chat
                                0x000500101005A000ull,
                                0x000500101005A100ull,
                                0x000500101005A200ull,
                        },

                        {
                                // "Software/Data Transfer"
                                0x0005001010062000ull,
                                0x0005001010062100ull,
                                0x0005001010062200ull,
                        },
};

bool getRealProductArea(MCPRegion *out) {
    if (out == nullptr) {
        return false;
    }
    auto handle = MCP_Open();
    if (handle >= 0) {
        auto data = (struct MCPSysProdSettings *) memalign(0x40, sizeof(struct MCPSysProdSettings));
        auto res  = real_MCP_GetSysProdSettings(handle, data);
        if (res >= 0) {
            *out = data->product_area;
        }
        MCP_Close(handle);
        free(data);
        return res >= 0;
    }
    return false;
}

DECL_FUNCTION(uint64_t, _SYSGetSystemApplicationTitleIdByProdArea, SYSTEM_APP_ID param_1, MCPRegion param_2) {
    auto upid = OSGetUPID();
    if (upid != SYSAPP_PFID_WII_U_MENU && upid != SYSAPP_PFID_DOWNLOAD_GAME && upid != SYSAPP_PFID_EMANUAL) {
        return real__SYSGetSystemApplicationTitleIdByProdArea(param_1, param_2);
    }
    MCPRegion cur_product_area;
    if (!getRealProductArea(&cur_product_area)) {
        DEBUG_FUNCTION_LINE("Failed to get real region");
        cur_product_area = param_2;
    }

    auto regionIdx = 1u;

    if (cur_product_area == MCP_REGION_JAPAN) {
        regionIdx = 0u;
    } else if (cur_product_area == MCP_REGION_EUROPE || cur_product_area == 0x08) {
        regionIdx = 2u;
    }

    uint64_t res = sSysAppTitleId[param_1][regionIdx];
    return res;
}

ON_APPLICATION_ENDS() {
    gCurrentLanguage    = gDefaultLanguage;
    gCurrentCountry     = gDefaultCountry;
    gCurrentProductArea = gDefaultProductArea;
    deinitLogging();
}

WUPS_MUST_REPLACE(ACPGetTitleMetaXmlByDevice, WUPS_LOADER_LIBRARY_NN_ACP, ACPGetTitleMetaXmlByDevice);
WUPS_MUST_REPLACE(ACPGetLaunchMetaXml, WUPS_LOADER_LIBRARY_NN_ACP, ACPGetLaunchMetaXml);
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_GetSysProdSettings, WUPS_LOADER_LIBRARY_COREINIT, MCP_GetSysProdSettings, WUPS_FP_TARGET_PROCESS_ALL);
WUPS_MUST_REPLACE_FOR_PROCESS(UCReadSysConfig, WUPS_LOADER_LIBRARY_COREINIT, UCReadSysConfig, WUPS_FP_TARGET_PROCESS_ALL);
WUPS_MUST_REPLACE_FOR_PROCESS(_SYSGetSystemApplicationTitleIdByProdArea, WUPS_LOADER_LIBRARY_SYSAPP, _SYSGetSystemApplicationTitleIdByProdArea, WUPS_FP_TARGET_PROCESS_ALL);
