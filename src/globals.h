#pragma once

#include <coreinit/mcp.h>

extern int gForceSettingsEnabled;

enum Lanuages {
    LANG_JAPANESE = 0,
    LANG_ENGLISH = 1,
    LANG_FRANCAIS = 2,
    LANG_DEUTSCH = 3,
    LANG_ESPANOL = 5,
    LANG_ITALIANO = 4,
    LANG_NEDERLANDS = 8,
    LANG_PORTUGUES = 9,
    LANG_RUSSKI = 10,
};


extern int gPreferSystemSettings;
extern int gAutoDetection;
extern Lanuages gDefaultLanguage;
extern int32_t gDefaultCountry;
extern MCPRegion gDefaultProductArea;

extern Lanuages gCurrentLanguage;
extern int32_t gCurrentCountry;
extern MCPRegion gCurrentProductArea;

extern Lanuages gDefaultLangForEUR;
extern int32_t gDefaultCountryForEUR;
extern Lanuages gDefaultLangForUSA;
extern int32_t gDefaultCountryForUSA;
extern Lanuages gDefaultLangForJPN;
extern int32_t gDefaultCountryForJPN;