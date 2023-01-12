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

        if (strcmp(settings->name, "cafe.language") == 0) {
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: cafe.language found!");
            DEBUG_FUNCTION_LINE_VERBOSE("UCReadSysConfig: forcing language...");
            DEBUG_FUNCTION_LINE("UCReadSysConfig: original lang %d, new %d", *((int *) settings->data), gCurrentLanguage);
            *((int *) settings->data) = gCurrentLanguage;
        }
        if (strcmp(settings->name, "cafe.cntry_reg") == 0) {
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

extern "C" void ACPInitialize();
extern "C" void ACPFinalize();

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

    bool forceConfigMenu = false;
    auto *acpMetaXml     = (ACPMetaXml *) memalign(0x40, sizeof(ACPMetaXml));

    memset(acpMetaXml, 0, sizeof(ACPMetaXml));
    uint32_t regionFromXML = 0;
    if (acpMetaXml) {
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

    wups_storage_item_t *root = nullptr;
    auto resa                 = WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root);
    if (resa != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to read %s subitem", CAT_GENERAL_ROOT);
        return;
    }

    wups_storage_item_t *title_settings;
    if (WUPS_GetSubItem(root, CAT_TITLE_SETTINGS, &title_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        if (WUPS_CreateSubItem(root, CAT_TITLE_SETTINGS, &title_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("WUPS_CreateSubItem %s failed", CAT_TITLE_SETTINGS);
            return;
        }
    }

    char buffer[18];
    snprintf(buffer, 17, "%016llX", OSGetTitleID());

    wups_storage_item_t *curTitleItem;
    if (WUPS_GetSubItem(title_settings, buffer, &curTitleItem) != WUPS_STORAGE_ERROR_SUCCESS) {
        if (WUPS_CreateSubItem(title_settings, buffer, &curTitleItem) != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("WUPS_CreateSubItem %s failed", buffer);
            return;
        }
    }

    if (curTitleItem != nullptr) {
        if (WUPS_GetInt(curTitleItem, VAL_LANGUAGE, (int32_t *) &gCurrentLanguage) != WUPS_STORAGE_ERROR_SUCCESS) {
            WUPS_StoreInt(curTitleItem, VAL_LANGUAGE, gDefaultLanguage);
            gCurrentLanguage = gDefaultLanguage;
        }

        /*
        if (WUPS_GetInt(curTitleItem, VAL_COUNTRY, (int32_t *) &gCurrentCountry) != WUPS_STORAGE_ERROR_SUCCESS) {
            WUPS_StoreInt(curTitleItem, VAL_COUNTRY, gDefaultCountry);
            gCurrentCountry = gDefaultCountry;
        }*/


        if (WUPS_GetInt(curTitleItem, VAL_PRODUCT_AREA, (int32_t *) &gCurrentProductArea) != WUPS_STORAGE_ERROR_SUCCESS) {
            WUPS_StoreInt(curTitleItem, VAL_PRODUCT_AREA, gDefaultProductArea);
            gCurrentProductArea = gDefaultProductArea;
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
                // If the want to skip checks for own region and we were able
                // to tell the region of the current title don't show the menu.
                showMenu = false;
                return;
            }
        }

        // this overrides the current settings
        if (forceConfigMenu || (!isWiiUMenu && showMenu)) {
            ConfigUtils::openConfigMenu();
            // Save settings to storage
            WUPS_StoreInt(curTitleItem, VAL_LANGUAGE, gCurrentLanguage);
            //WUPS_StoreInt(curTitleItem, VAL_COUNTRY, gCurrentCountry);
            WUPS_StoreInt(curTitleItem, VAL_PRODUCT_AREA, gCurrentProductArea);
        }

        DEBUG_FUNCTION_LINE("Language will be force to %d", gCurrentLanguage);
        DEBUG_FUNCTION_LINE("Country will be force to %d", gDefaultCountry);
        DEBUG_FUNCTION_LINE("Product Area will be forced to %d", gCurrentProductArea);
    }
}

ON_APPLICATION_START() {
    initLogging();

    WUPS_OpenStorage();

    wups_storage_item_t *root;
    if (WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_CreateSubItem(nullptr, CAT_GENERAL_ROOT, &root);
    }

    wups_storage_item_t *general_settings;
    if (WUPS_GetSubItem(root, CAT_GENERAL_SETTINGS, &general_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_CreateSubItem(root, CAT_GENERAL_SETTINGS, &general_settings);
    }

    if (WUPS_GetInt(general_settings, VAL_AUTO_DETECTION, (int32_t *) &gAutoDetection) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_AUTO_DETECTION, gAutoDetection);
    }
    if (WUPS_GetInt(general_settings, VAL_PREFER_SYSTEM_SETTINGS, (int32_t *) &gPreferSystemSettings) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_PREFER_SYSTEM_SETTINGS, gPreferSystemSettings);
    }

    if (WUPS_GetInt(general_settings, VAL_SKIP_OWN_REGION, (int32_t *) &gSkipOwnRegion) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_SKIP_OWN_REGION, gSkipOwnRegion);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_LANG_EUR, (int32_t *) &gDefaultLangForEUR) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_LANG_EUR, gDefaultLangForEUR);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_COUNTRY_EUR, &gDefaultCountryForEUR) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_COUNTRY_EUR, gDefaultCountryForEUR);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_LANG_USA, (int32_t *) &gDefaultLangForUSA) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_LANG_USA, gDefaultLangForUSA);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_COUNTRY_USA, &gDefaultCountryForUSA) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_COUNTRY_USA, gDefaultCountryForUSA);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_LANG_JPN, (int32_t *) &gDefaultLangForJPN) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_LANG_JPN, gDefaultLangForJPN);
    }

    if (WUPS_GetInt(general_settings, VAL_DEFAULT_COUNTRY_JPN, &gDefaultCountryForJPN) != WUPS_STORAGE_ERROR_SUCCESS) {
        WUPS_StoreInt(general_settings, VAL_DEFAULT_COUNTRY_JPN, gDefaultCountryForJPN);
    }

    gForceSettingsEnabled = 1;
    if (OSGetTitleID() == 0x0005001010040000L || // Wii U Menu JPN
        OSGetTitleID() == 0x0005001010040100L || // Wii U Menu USA
        OSGetTitleID() == 0x0005001010040200L) { // Wii U Menu EUR
        gForceSettingsEnabled = 0;
    }

    bootStuff();

    WUPS_CloseStorage();
}

void auto_detection_changed(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE("New value in auto_detection changed: %d", newValue);

    wups_storage_item_t *root;
    if (WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    wups_storage_item_t *general_settings;
    if (WUPS_GetSubItem(root, CAT_GENERAL_SETTINGS, &general_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    WUPS_StoreInt(general_settings, VAL_AUTO_DETECTION, newValue);
    gAutoDetection = newValue;
}

void prefer_system_changed(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE("New value in prefer_system_settings changed: %d", newValue);

    wups_storage_item_t *root;
    if (WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    wups_storage_item_t *general_settings;
    if (WUPS_GetSubItem(root, CAT_GENERAL_SETTINGS, &general_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    WUPS_StoreInt(general_settings, VAL_PREFER_SYSTEM_SETTINGS, newValue);
    gPreferSystemSettings = newValue;
}

void skip_own_region_changed(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE("New value in skip_own_region_changed changed: %d", newValue);

    wups_storage_item_t *root;
    if (WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    wups_storage_item_t *general_settings;
    if (WUPS_GetSubItem(root, CAT_GENERAL_SETTINGS, &general_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    WUPS_StoreInt(general_settings, VAL_SKIP_OWN_REGION, newValue);
    gPreferSystemSettings = newValue;
}

void default_lang_changed(ConfigItemMultipleValues *item, uint32_t newValue) {
    DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);

    wups_storage_item_t *root;
    if (WUPS_GetSubItem(nullptr, CAT_GENERAL_ROOT, &root) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    wups_storage_item_t *general_settings;
    if (WUPS_GetSubItem(root, CAT_GENERAL_SETTINGS, &general_settings) != WUPS_STORAGE_ERROR_SUCCESS) {
        return;
    }

    WUPS_StoreInt(general_settings, item->configId, (int32_t) newValue);
    if (strcmp(item->configId, VAL_DEFAULT_LANG_EUR) == 0) {
        DEBUG_FUNCTION_LINE("Updated default eur lang");
        gDefaultLangForEUR = (Lanuages) newValue;
    } else if (strcmp(item->configId, VAL_DEFAULT_LANG_USA) == 0) {
        DEBUG_FUNCTION_LINE("Updated default usa lang");
        gDefaultLangForUSA = (Lanuages) newValue;
    } else if (strcmp(item->configId, VAL_DEFAULT_LANG_JPN) == 0) {
        DEBUG_FUNCTION_LINE("Updated default jpn lang");
        gDefaultLangForJPN = (Lanuages) newValue;
    }
}

void getConfigInfoForLangMap(std::map<Lanuages, const char *> &curLangMap, ConfigItemMultipleValuesPair *pair, uint32_t default_lang, uint32_t *default_index, uint32_t *len) {
    uint32_t i = 0;
    for (auto &curEntry : curLangMap) {
        if (default_lang == curEntry.first) {
            *default_index = i;
        }
        pair[i].value     = curEntry.first;
        pair[i].valueName = (char *) curEntry.second;
        i++;
    }
    *len = i;
}

WUPS_GET_CONFIG() {
    // We open the storage so we can persist the configuration the user did.
    WUPS_OpenStorage();

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Region Free Plugin");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, VAL_DEFAULT_LANG_USA, "Auto detect region/language", gAutoDetection, &auto_detection_changed);
    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, VAL_SKIP_OWN_REGION, "Force auto detection for in-region titles", gSkipOwnRegion, &skip_own_region_changed);
    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, VAL_PREFER_SYSTEM_SETTINGS, "Prefer system settings for in-region titles", gPreferSystemSettings, &prefer_system_changed);

    std::map<Lanuages, const char *> eur_lang_map{
            {LANG_ENGLISH, "English"},
            {LANG_FRANCAIS, "Francais"},
            {LANG_DEUTSCH, "Deutsch"},
            {LANG_ITALIANO, "Italiano"},
            {LANG_ESPANOL, "Espanol"},
            {LANG_NEDERLANDS, "Nederlands"},
            {LANG_PORTUGUES, "Portugues"},
            {LANG_RUSSKI, "Russki"},
    };

    std::map<Lanuages, const char *> usa_lang_map{
            {LANG_ENGLISH, "English"},
            {LANG_FRANCAIS, "Francais"},
            {LANG_ESPANOL, "Espanol"},
            {LANG_PORTUGUES, "Portugues"}};

    ConfigItemMultipleValuesPair lang_eur_pair[eur_lang_map.size()];
    uint32_t number_lang_eur_values = 0;
    uint32_t default_index_eur      = 0;

    getConfigInfoForLangMap(eur_lang_map, lang_eur_pair, gDefaultLangForEUR, &default_index_eur, &number_lang_eur_values);

    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, VAL_DEFAULT_LANG_EUR, "Default language for EUR", default_index_eur, lang_eur_pair, number_lang_eur_values,
                                                      &default_lang_changed);

    ConfigItemMultipleValuesPair lang_usa_pair[eur_lang_map.size()];
    uint32_t number_lang_usa_values = 0;
    uint32_t default_index_usa      = 0;

    getConfigInfoForLangMap(usa_lang_map, lang_usa_pair, gDefaultLangForUSA, &default_index_usa, &number_lang_usa_values);

    WUPSConfigItemMultipleValues_AddToCategoryHandled(config, cat, VAL_DEFAULT_LANG_USA, "Default language for USA", default_index_usa, lang_usa_pair, number_lang_usa_values,
                                                      &default_lang_changed);

    return config;
}

WUPS_CONFIG_CLOSED() {
    // Save all changes
    WUPS_CloseStorage();
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
