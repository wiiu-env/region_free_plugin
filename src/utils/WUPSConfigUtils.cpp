#include "WUPSConfigUtils.h"
#include "globals.h"
#include "logger.h"
#include <wups/config/WUPSConfigItemBoolean.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/storage.h>

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


const char *countryCodeMap[] = {
        NULL, "JP", NULL, NULL, NULL, NULL, NULL, NULL, "AI", "AG", "AR",
        "AW", "BS", "BB", "BZ", "BO", "BR", "VG", "CA", "KY", "CL",
        "CO", "CR", "DM", "DO", "EC", "SV", "GF", "GD", "GP", "GT",
        "GY", "HT", "HN", "JM", "MQ", "MX", "MS", "AN", "NI", "PA",
        "PY", "PE", "KN", "LC", "VC", "SR", "TT", "TC", "US", "UY",
        "VI", "VE", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, "AL", "AU", "AT", "BE", "BA", "BW", "BG",
        "HR", "CY", "CZ", "DK", "EE", "FI", "FR", "DE", "GR", "HU",
        "IS", "IE", "IT", "LV", "LS", "LI", "LT", "LU", "MK", "MT",
        "ME", "MZ", "NA", "NL", "NZ", "NO", "PL", "PT", "RO", "RU",
        "RS", "SK", "SI", "ZA", "ES", "SZ", "SE", "CH", "TR", "GB",
        "ZM", "ZW", "AZ", "MR", "ML", "NE", "TD", "SD", "ER", "DJ",
        "SO", "AD", "GI", "GG", "IM", "JE", "MC", "TW", NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, "KR", NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, "HK", "MO", NULL, NULL, NULL, NULL, NULL,
        NULL, "ID", "SG", "TH", "PH", "MY", NULL, NULL, NULL, "CN",
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, "AE", NULL, "EG",
        "OM", "QA", "KW", "SA", "SY", "BH", "JO", NULL, NULL, NULL,
        NULL, NULL, NULL, "SM", "VA", "BM", "IN", NULL, NULL, NULL,
        NULL, "NG", "AO", "GH", NULL, NULL, NULL, NULL, NULL, NULL};


static std::optional<WUPSStorageSubItem> rootSettingsStorage    = {};
static std::optional<WUPSStorageSubItem> titleSettingsStorage   = {};
static std::optional<WUPSStorageSubItem> generalSettingsStorage = {};

std::optional<TitleRegionInfo> getTitleRegionInfo(uint64_t titleId, Languages default_language, MCPRegion default_product_area, bool allow_default) {
    if (!titleSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to read title settings from storage");
        return {};
    }
    char buffer[18];
    snprintf(buffer, 17, "%016llX", titleId);

    WUPSStorageError storageError;
    auto curTitle = titleSettingsStorage->GetSubItem(buffer, storageError);
    if (!curTitle) {
        if (storageError == WUPS_STORAGE_ERROR_NOT_FOUND) {
            if (allow_default) {
                curTitle = titleSettingsStorage->CreateSubItem(buffer, storageError);
            } else {
                // If we don't allow default values, we return now.
                return {};
            }
        }
        if (!curTitle) {
            DEBUG_FUNCTION_LINE_ERR("titleSettingsStorage->GetOrCreateSubItem(%s) failed: %s", buffer, WUPSStorageAPI_GetStatusStr(storageError));
            return {};
        }
    }

    Languages languages    = default_language;
    MCPRegion product_area = default_product_area;

    storageError = curTitle->GetOrStoreDefault(VAL_LANGUAGE, languages, gCurrentLanguage);
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or store language for title %s: %s", buffer, WUPSStorageAPI_GetStatusStr(storageError));
        return {};
    }
    storageError = curTitle->GetOrStoreDefault(VAL_PRODUCT_AREA, product_area, gCurrentProductArea);
    if (storageError != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or store product area for title %s: %s", buffer, WUPSStorageAPI_GetStatusStr(storageError));
        return {};
    }

    return TitleRegionInfo(titleId, languages, product_area);
}

bool saveTitleRegionInfo(const TitleRegionInfo &titleRegionInfo) {
    if (!titleSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to read title settings from storage");
        return false;
    }
    char buffer[18];
    snprintf(buffer, 17, "%016llX", titleRegionInfo.titleId);

    WUPSStorageError storageError;
    auto curTitle = titleSettingsStorage->GetOrCreateSubItem(buffer, storageError);

    if (!curTitle) {
        DEBUG_FUNCTION_LINE_ERR("titleSettingsStorage->GetOrCreateSubItem(%s) failed: %s", buffer, WUPSStorageAPI_GetStatusStr(storageError));
        return false;
    }

    if ((storageError = curTitle->Store(VAL_LANGUAGE, titleRegionInfo.language)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("curTitle->Store: VAL_LANGUAGE, %s, %d failed: %s", buffer, titleRegionInfo.language, WUPSStorageAPI_GetStatusStr(storageError));
    }
    if ((storageError = curTitle->Store(VAL_PRODUCT_AREA, titleRegionInfo.product_area)) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("curTitle->Store: VAL_PRODUCT_AREA, %s, %d failed: %s", buffer, titleRegionInfo.product_area, WUPSStorageAPI_GetStatusStr(storageError));
    }

    return true;
}

static void bool_item_changed(ConfigItemBoolean *item, bool newValue) {
    DEBUG_FUNCTION_LINE("Value changed for %s: %d", item->identifier ? item->identifier : "[UNKNOWN]", newValue);

    if (std::string_view(VAL_AUTO_DETECTION) == item->identifier) {
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
        gSkipOwnRegion = newValue;
        if (generalSettingsStorage) {
            generalSettingsStorage->Store(VAL_SKIP_OWN_REGION, gSkipOwnRegion);
        }
    }
}

static void default_lang_changed(ConfigItemMultipleValues *item, uint32_t newValue) {
    DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->identifier, newValue);

    if (std::string_view(VAL_DEFAULT_LANG_EUR) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default eur lang");
        gDefaultLangForEUR = (Languages) newValue;
    } else if (std::string_view(VAL_DEFAULT_LANG_USA) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default usa lang");
        gDefaultLangForUSA = (Languages) newValue;
    } else if (std::string_view(VAL_DEFAULT_LANG_JPN) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default jpn lang");
        gDefaultLangForJPN = (Languages) newValue;
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

/*
static void default_country_changed(ConfigItemMultipleValues *item, uint32_t newValue) {
    DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->identifier, newValue);

    if (std::string_view(VAL_DEFAULT_COUNTRY_EUR) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default eur country");
        gDefaultCountryForEUR = (int32_t) newValue;
    } else if (std::string_view(VAL_DEFAULT_COUNTRY_USA) == item->identifier) {
        DEBUG_FUNCTION_LINE("Updated default usa country");
        gDefaultCountryForUSA = (int32_t) newValue;
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
*/

static WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    try {
        WUPSConfigCategory root = WUPSConfigCategory(rootHandle);

        root.add(WUPSConfigItemBoolean::Create(VAL_AUTO_DETECTION, "Auto detect region/language",
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

        /*
        auto expertCat = WUPSConfigCategory::Create("Expert parameters");
        std::vector<WUPSConfigItemMultipleValues::ValuePair> valid_country_codes_map;
        for (uint32_t i = 0; i < sizeof(countryCodeMap) / sizeof(countryCodeMap[0]); i++) {
            if (countryCodeMap[i] == nullptr) {
                continue;
            }
            valid_country_codes_map.push_back({i, countryCodeMap[i]});
        }
        expertCat.add(WUPSConfigItemMultipleValues::CreateFromValue(VAL_DEFAULT_COUNTRY_USA, "Default country for USA (out-of-region titles only)",
                                                                    DEFAULT_COUNTRY_FOR_USA, gDefaultCountryForUSA,
                                                                    valid_country_codes_map,
                                                                    default_country_changed));

        expertCat.add(WUPSConfigItemMultipleValues::CreateFromValue(VAL_DEFAULT_COUNTRY_EUR, "Default country for EUR (out-of-region titles only)",
                                                                    DEFAULT_COUNTRY_FOR_EUR, gDefaultCountryForEUR,
                                                                    valid_country_codes_map,
                                                                    default_country_changed));
        root.add(std::move(expertCat));
        */
    } catch (std::exception &e) {
        OSReport("Exception: %s\n", e.what());
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

static void ConfigMenuClosedCallback() {
    // Save all changes
    WUPSStorageError storageError;
    if ((storageError = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to save storage: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
    }
}

void InitConfigAndStorage() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = "Region Free Plugin"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }

    WUPSStorageError storageError;
    rootSettingsStorage = WUPSStorageAPI::GetOrCreateSubItem(CAT_GENERAL_ROOT, storageError);
    if (!rootSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get or create %s subitem", CAT_GENERAL_ROOT);
    }

    titleSettingsStorage = rootSettingsStorage->GetOrCreateSubItem(CAT_TITLE_SETTINGS, storageError);
    if (!titleSettingsStorage) {
        DEBUG_FUNCTION_LINE_ERR("WUPS_CreateSubItem %s failed", CAT_TITLE_SETTINGS);
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

    if ((storageError = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to save storage: %s", WUPSStorageAPI::GetStatusStr(storageError).data());
    }
}
