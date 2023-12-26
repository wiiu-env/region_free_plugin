#pragma once

#include "version.h"
#include <coreinit/mcp.h>

#define VERSION      "v0.2.3"
#define VERSION_FULL VERSION VERSION_EXTRA

extern int gForceSettingsEnabled;

enum Lanuages {
    LANG_JAPANESE   = 0,
    LANG_ENGLISH    = 1,
    LANG_FRANCAIS   = 2,
    LANG_DEUTSCH    = 3,
    LANG_ESPANOL    = 5,
    LANG_ITALIANO   = 4,
    LANG_NEDERLANDS = 8,
    LANG_PORTUGUES  = 9,
    LANG_RUSSKI     = 10,
};

#define DEFAULT_AUTO_DETECTION_VALUE true
extern bool gAutoDetection;

#define DEFAULT_PREFER_SYSTEM_SETTINGS true
extern bool gPreferSystemSettings;

#define DEFAULT_SKIP_OWN_REGION true
extern bool gSkipOwnRegion;

extern Lanuages gDefaultLanguage;
extern int32_t gDefaultCountry;
extern MCPRegion gDefaultProductArea;

extern Lanuages gCurrentLanguage;
extern int32_t gCurrentCountry;
extern MCPRegion gCurrentProductArea;

#define DEFAULT_LANG_FOR_EUR    LANG_ENGLISH
#define DEFAULT_COUNTRY_FOR_EUR 110

#define DEFAULT_LANG_FOR_USA    LANG_ENGLISH
#define DEFAULT_COUNTRY_FOR_USA 49

#define DEFAULT_LANG_FOR_JPN    LANG_JAPANESE
#define DEFAULT_COUNTRY_FOR_JPN 1

extern Lanuages gDefaultLangForEUR;
extern int32_t gDefaultCountryForEUR;

extern Lanuages gDefaultLangForUSA;
extern int32_t gDefaultCountryForUSA;

extern Lanuages gDefaultLangForJPN;
extern int32_t gDefaultCountryForJPN;
