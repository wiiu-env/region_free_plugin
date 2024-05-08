#include "globals.h"
#include "utils/ConfigUtils.h"
#include "utils/SettingsUtils.h"
#include "utils/WUPSConfigUtils.h"
#include "utils/logger.h"
#include <coreinit/title.h>
#include <nn/acp/client.h>
#include <notifications/notifications.h>
#include <wups.h>

WUPS_PLUGIN_NAME("Region Free Plugin");
WUPS_PLUGIN_DESCRIPTION("Allows the user to load titles from other regions");
WUPS_PLUGIN_VERSION(VERSION_FULL);
WUPS_PLUGIN_AUTHOR("Maschell");
WUPS_PLUGIN_LICENSE("GPL");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("region_free_plugin");

void SetupRegionSpoofing() {
    ACPInitialize(); // We need to this for getting the region of the current title

    // By default, always use the real values
    gCurrentLanguage    = gRealLanguageOpt.value();
    gCurrentProductArea = gRealRegionOpt.value();
    gCurrentCountry     = gRealCountryOpt.value();

    /**
     * 1. No do any region/language/country changes for the Wii U Menu
     * 2. Try to get the region from the xml of the title that is currently running
     *      2a. If we are able to determine the region of the title, set the current region/language/country to the default region/language/country of that region
     *      2b. If we are NOT able to determine the region of the title, make sure to force open the config menu
     *    If reading the XML of the current title fails, make sure to force open the config menu
     * 3. If the current region is equal to the real region
     *      - make sure to reset the country to the real country
     *      - set the language back to the system language if requested (see "Prefer system settings for in region title"-setting)
     * 4. Try to read the region/language for the currently running title from the storage.
     *      - If the settings for these settings do no exists, create an entry with the current values (unless the menu is forced opened)
     *      - The values from the storage always have the highest priority
     *      - Currently the country is NOT loaded from the storage (is not stored to storage).
     * 5. Open the region free config menu if the auto detection is enabled, or we failed to determine the region of the currently running title.
     */

    bool forceOpenConfigMenu = false;
    bool isWiiUMenuOrHS      = false;
    auto curTitleID          = OSGetTitleID();
    if (curTitleID == 0x0005001010040000L || // Wii U Menu JPN
        curTitleID == 0x0005001010040100L || // Wii U Menu USA
        curTitleID == 0x0005001010040200L || // Wii U Menu EUR
        curTitleID == 0x000500101004E000L || // Health and Safety Information JPN
        curTitleID == 0x000500101004E100L || // Health and Safety Information USA
        curTitleID == 0x000500101004E200L) { // Health and Safety Information EUR
        isWiiUMenuOrHS = true;
    }

    if (!isWiiUMenuOrHS) {
        // Set current values based on title xml
        auto regionForCurrentTitleOpt = getRegionForTitle(curTitleID);
        if (regionForCurrentTitleOpt) {
            auto region_from_xml = regionForCurrentTitleOpt.value();
            if ((region_from_xml & gCurrentProductArea) == gCurrentProductArea) {
                // get current product area
            } else if (region_from_xml == 1) {
                gCurrentProductArea = MCP_REGION_JAPAN;
                gCurrentLanguage    = gDefaultLangForJPN;
                gCurrentCountry     = gDefaultCountryForJPN;
            } else if (region_from_xml == 2) {
                gCurrentProductArea = MCP_REGION_USA;
                gCurrentLanguage    = gDefaultLangForUSA;
                gCurrentCountry     = gDefaultCountryForUSA;
            } else if (region_from_xml == 4) {
                gCurrentProductArea = MCP_REGION_EUROPE;
                gCurrentLanguage    = gDefaultLangForEUR;
                gCurrentCountry     = gDefaultCountryForEUR;
            } else {
                // Unknown region in xml, force open the config menu
                forceOpenConfigMenu = true;
            }
        } else {
            // Failed to get region for current tile, force open the config menu
            forceOpenConfigMenu = true;
        }

        // If the launched title is for real region, force the country (and maybe language) to be the real one
        if (gRealRegionOpt.value() == gCurrentProductArea) {
            // Check if we want to keep the system language for in-region titles.
            if (gPreferSystemSettings) {
                gCurrentLanguage = gRealLanguageOpt.value();
            }
            // For now always use the real country
            gCurrentCountry = gRealCountryOpt.value();
        }

        // Read per-title settings. Give current lang/region as default values.
        // If we don't have values yet for this title, we only create them if we could get the region for the current title
        bool allowDefaultValues = !forceOpenConfigMenu;
        auto titleRegionInfoOpt = getTitleRegionInfo(curTitleID, gCurrentLanguage, gCurrentProductArea, allowDefaultValues);
        if (titleRegionInfoOpt) {
            gCurrentProductArea = titleRegionInfoOpt->product_area;
            gCurrentLanguage    = titleRegionInfoOpt->language;
        }
    }

    // show menu if we don't use auto-detection
    bool showMenu = !gAutoDetection;
    if (gCurrentProductArea == gRealRegionOpt.value()) {
        if (gSkipOwnRegion && !forceOpenConfigMenu) {
            // If the want to skip checks for own region, and we were able
            // to tell the region of the current title don't show the menu.
            showMenu = false;
        }
    }

    if (forceOpenConfigMenu || (!isWiiUMenuOrHS && showMenu)) {
        ConfigUtils::openConfigMenu();
        // Save (updated) title settings to the storage
        if (!saveTitleRegionInfo({curTitleID, gCurrentLanguage, gCurrentProductArea})) {
            DEBUG_FUNCTION_LINE_ERR("Failed to save current title region info to storage");
        } else {
            WUPSStorageError err;
            if ((err = WUPSStorageAPI::SaveStorage()) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to save storage: %s (%d)", WUPSStorageAPI_GetStatusStr(err), err);
            }
        }
    }

    DEBUG_FUNCTION_LINE("Language will be forced to %d", gCurrentLanguage);
    DEBUG_FUNCTION_LINE("Country will be forced to %d", gCurrentCountry);
    DEBUG_FUNCTION_LINE("Product Area will be forced to %d", gCurrentProductArea);
}

static bool disableRegionFreePlugin = false;

ON_APPLICATION_START() {
    initLogging();
    if (disableRegionFreePlugin) {
        gRegionFreeValuesSetupDone = false;
    } else {
        SetupRegionSpoofing();
        gRegionFreeValuesSetupDone = true;
    }
}

INITIALIZE_PLUGIN() {
    initLogging();

    if (NotificationModule_InitLibrary() != NOTIFICATION_MODULE_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("NotificationModule_InitLibrary failed");
    } else {
        NotificationModule_SetDefaultValue(NOTIFICATION_MODULE_NOTIFICATION_TYPE_ERROR, NOTIFICATION_MODULE_DEFAULT_OPTION_DURATION_BEFORE_FADE_OUT, 10.0f);
    }

    gRegionFreeValuesSetupDone = false;

    gRealRegionOpt   = getRealRegion();
    gRealCountryOpt  = getRealCountry();
    gRealLanguageOpt = getRealLanguage();

    if (!gRealRegionOpt.has_value() || !gRealCountryOpt.has_value() || !gRealLanguageOpt.has_value()) {
        DEBUG_FUNCTION_LINE_ERR("Failed to get the real region, country or language. Disabling the region free plugin");
        disableRegionFreePlugin = true;
        NotificationModuleStatus err;
        if ((err = NotificationModule_AddErrorNotification("Failed to init Region Free Plugin. The plugin will be disabled until next boot")) != NOTIFICATION_MODULE_RESULT_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to add error notification: %s", NotificationModule_GetStatusStr(err));
        }
    }

    InitConfigAndStorage();

    deinitLogging();
}


DEINITIALIZE_PLUGIN() {
    NotificationModule_DeInitLibrary();
}

ON_APPLICATION_ENDS() {
    gRegionFreeValuesSetupDone = false;
    deinitLogging();
}
