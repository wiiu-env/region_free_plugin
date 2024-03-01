#include "globals.h"

std::optional<MCPRegion> gRealRegionOpt   = {};
std::optional<int32_t> gRealCountryOpt    = {};
std::optional<Languages> gRealLanguageOpt = {};

bool gRegionFreeValuesSetupDone = false;

bool gPreferSystemSettings = DEFAULT_PREFER_SYSTEM_SETTINGS;
bool gSkipOwnRegion        = DEFAULT_SKIP_OWN_REGION;
bool gAutoDetection        = DEFAULT_AUTO_DETECTION_VALUE;

Languages gCurrentLanguage    = LANG_ENGLISH;
int32_t gCurrentCountry       = 0;
MCPRegion gCurrentProductArea = MCP_REGION_USA;

Languages gDefaultLangForEUR  = DEFAULT_LANG_FOR_EUR;
int32_t gDefaultCountryForEUR = DEFAULT_COUNTRY_FOR_EUR;
Languages gDefaultLangForUSA  = DEFAULT_LANG_FOR_USA;
int32_t gDefaultCountryForUSA = DEFAULT_COUNTRY_FOR_USA;
Languages gDefaultLangForJPN  = DEFAULT_LANG_FOR_JPN;
int32_t gDefaultCountryForJPN = DEFAULT_COUNTRY_FOR_JPN;