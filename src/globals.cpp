#include "globals.h"

bool gPreferSystemSettings = DEFAULT_PREFER_SYSTEM_SETTINGS;
bool gSkipOwnRegion        = DEFAULT_SKIP_OWN_REGION;
bool gAutoDetection        = DEFAULT_AUTO_DETECTION_VALUE;

int gForceSettingsEnabled     = 0;
Lanuages gDefaultLanguage     = LANG_ENGLISH;
int32_t gDefaultCountry       = 78;
MCPRegion gDefaultProductArea = MCP_REGION_EUROPE;

Lanuages gCurrentLanguage     = gDefaultLanguage;
int32_t gCurrentCountry       = gDefaultCountry;
MCPRegion gCurrentProductArea = gDefaultProductArea;

Lanuages gDefaultLangForEUR   = DEFAULT_LANG_FOR_EUR;
int32_t gDefaultCountryForEUR = DEFAULT_COUNTRY_FOR_EUR;
Lanuages gDefaultLangForUSA   = DEFAULT_LANG_FOR_USA;
int32_t gDefaultCountryForUSA = DEFAULT_COUNTRY_FOR_USA;
Lanuages gDefaultLangForJPN   = DEFAULT_LANG_FOR_JPN;
int32_t gDefaultCountryForJPN = DEFAULT_COUNTRY_FOR_JPN;