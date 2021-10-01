#include "ConfigUtils.h"

#include "utils/logger.h"
#include "DrawUtils.h"

#include <string>
#include <vector>
#include <coreinit/screen.h>
#include <memory/mappedmemory.h>
#include <vpad/input.h>
#include <padscore/kpad.h>
#include <coreinit/mcp.h>
#include <map>
#include "globals.h"

#define COLOR_BACKGROUND Color(238, 238, 238, 255)
#define COLOR_TEXT       Color(51,  51,  51,  255)
#define COLOR_TEXT2      Color(72,  72,  72,  255)
#define COLOR_DISABLED   Color(255, 0,   0,   255)
#define COLOR_BORDER     Color(204, 204, 204, 255)
#define COLOR_BORDER_HIGHLIGHTED Color(0x3478e4FF)
#define COLOR_WHITE      Color(0xFFFFFFFF)
#define COLOR_BLACK      Color(0, 0, 0, 255)

#define MAX_BUTTONS_ON_SCREEN 8

static uint32_t remapWiiMoteButtons(uint32_t buttons) {
    uint32_t conv_buttons = 0;

    if (buttons & WPAD_BUTTON_LEFT)
        conv_buttons |= VPAD_BUTTON_LEFT;

    if (buttons & WPAD_BUTTON_RIGHT)
        conv_buttons |= VPAD_BUTTON_RIGHT;

    if (buttons & WPAD_BUTTON_DOWN)
        conv_buttons |= VPAD_BUTTON_DOWN;

    if (buttons & WPAD_BUTTON_UP)
        conv_buttons |= VPAD_BUTTON_UP;

    if (buttons & WPAD_BUTTON_PLUS)
        conv_buttons |= VPAD_BUTTON_PLUS;

    if (buttons & WPAD_BUTTON_B)
        conv_buttons |= VPAD_BUTTON_B;

    if (buttons & WPAD_BUTTON_A)
        conv_buttons |= VPAD_BUTTON_A;

    if (buttons & WPAD_BUTTON_MINUS)
        conv_buttons |= VPAD_BUTTON_MINUS;

    if (buttons & WPAD_BUTTON_HOME)
        conv_buttons |= VPAD_BUTTON_HOME;

    return conv_buttons;
}

static uint32_t remapClassicButtons(uint32_t buttons) {
    uint32_t conv_buttons = 0;

    if (buttons & WPAD_CLASSIC_BUTTON_LEFT)
        conv_buttons |= VPAD_BUTTON_LEFT;

    if (buttons & WPAD_CLASSIC_BUTTON_RIGHT)
        conv_buttons |= VPAD_BUTTON_RIGHT;

    if (buttons & WPAD_CLASSIC_BUTTON_DOWN)
        conv_buttons |= VPAD_BUTTON_DOWN;

    if (buttons & WPAD_CLASSIC_BUTTON_UP)
        conv_buttons |= VPAD_BUTTON_UP;

    if (buttons & WPAD_CLASSIC_BUTTON_PLUS)
        conv_buttons |= VPAD_BUTTON_PLUS;

    if (buttons & WPAD_CLASSIC_BUTTON_X)
        conv_buttons |= VPAD_BUTTON_X;

    if (buttons & WPAD_CLASSIC_BUTTON_Y)
        conv_buttons |= VPAD_BUTTON_Y;

    if (buttons & WPAD_CLASSIC_BUTTON_B)
        conv_buttons |= VPAD_BUTTON_B;

    if (buttons & WPAD_CLASSIC_BUTTON_A)
        conv_buttons |= VPAD_BUTTON_A;

    if (buttons & WPAD_CLASSIC_BUTTON_MINUS)
        conv_buttons |= VPAD_BUTTON_MINUS;

    if (buttons & WPAD_CLASSIC_BUTTON_HOME)
        conv_buttons |= VPAD_BUTTON_HOME;

    if (buttons & WPAD_CLASSIC_BUTTON_ZR)
        conv_buttons |= VPAD_BUTTON_ZR;

    if (buttons & WPAD_CLASSIC_BUTTON_ZL)
        conv_buttons |= VPAD_BUTTON_ZL;

    if (buttons & WPAD_CLASSIC_BUTTON_R)
        conv_buttons |= VPAD_BUTTON_R;

    if (buttons & WPAD_CLASSIC_BUTTON_L)
        conv_buttons |= VPAD_BUTTON_L;

    return conv_buttons;
}

void ConfigUtils::displayMenu() {
    bool redraw = true;
    uint32_t buttonsTriggered;
    uint32_t buttonsReleased;

    VPADStatus vpad_data{};
    VPADReadError vpad_error;
    KPADStatus kpad_data{};
    int32_t kpad_error;

    auto selectedBtn = 0;

    std::map<MCPRegion, const char *> region_map{
            {MCP_REGION_JAPAN,  "Japan"},
            {MCP_REGION_USA,    "USA"},
            {MCP_REGION_EUROPE, "Europe"},
    };

    std::map<MCPRegion, int32_t> region_map_to_index{
            {MCP_REGION_JAPAN,  0},
            {MCP_REGION_USA,    1},
            {MCP_REGION_EUROPE, 2},
    };


    std::map<int32_t, MCPRegion> region_index_to_map{
            {0, MCP_REGION_JAPAN},
            {1, MCP_REGION_USA},
            {2, MCP_REGION_EUROPE},
    };

    auto curSelectedRegion = gCurrentProductArea;
    DEBUG_FUNCTION_LINE("Current %d", curSelectedRegion);

    std::map<Lanuages, const char *> lang_map{
            {LANG_JAPANESE,   "Japanese"},
            {LANG_ENGLISH,    "English"},
            {LANG_FRANCAIS,   "Francais"},
            {LANG_DEUTSCH,    "Deutsch"},
            {LANG_ITALIANO,   "Italiano"},
            {LANG_ESPANOL,    "Espanol"},
            {LANG_NEDERLANDS, "Nederlands"},
            {LANG_PORTUGUES,  "Portugues"},
            {LANG_RUSSKI,     "Russki"},
    };
    std::map<Lanuages, int32_t> lang_map_to_index{
            {LANG_JAPANESE,   0},
            {LANG_ENGLISH,    1},
            {LANG_FRANCAIS,   2},
            {LANG_DEUTSCH,    3},
            {LANG_ITALIANO,   4},
            {LANG_ESPANOL,    5},
            {LANG_NEDERLANDS, 6},
            {LANG_PORTUGUES,  7},
            {LANG_RUSSKI,     8},
    };
    std::map<int32_t, Lanuages> lang_index_to_map{
            {0, LANG_JAPANESE},
            {1, LANG_ENGLISH},
            {2, LANG_FRANCAIS},
            {3, LANG_DEUTSCH},
            {4, LANG_ITALIANO},
            {5, LANG_ESPANOL},
            {6, LANG_NEDERLANDS},
            {7, LANG_PORTUGUES},
            {8, LANG_RUSSKI},
    };

    auto curSelectedLanguage = gCurrentLanguage;

    int32_t curRegionIndex = region_map_to_index[curSelectedRegion];
    int32_t curLangIndex = lang_map_to_index[curSelectedLanguage];

    while (true) {
        buttonsTriggered = 0;
        buttonsReleased = 0;

        VPADRead(VPAD_CHAN_0, &vpad_data, 1, &vpad_error);
        if (vpad_error == VPAD_READ_SUCCESS) {
            buttonsTriggered = vpad_data.trigger;
            buttonsReleased = vpad_data.release;
        }

        for (int i = 0; i < 4; i++) {
            if (KPADReadEx((KPADChan) i, &kpad_data, 1, &kpad_error) > 0) {
                if (kpad_error == KPAD_ERROR_OK) {
                    if (kpad_data.extensionType == WPAD_EXT_CORE || kpad_data.extensionType == WPAD_EXT_NUNCHUK) {
                        buttonsTriggered |= remapWiiMoteButtons(kpad_data.trigger);
                        buttonsReleased |= remapWiiMoteButtons(kpad_data.release);
                    } else {
                        buttonsTriggered |= remapClassicButtons(kpad_data.classic.trigger);
                        buttonsReleased |= remapClassicButtons(kpad_data.classic.release);
                    }
                }
            }
        }

        if (buttonsTriggered & VPAD_BUTTON_HOME) {
            break;
        }

        if (buttonsTriggered & VPAD_BUTTON_DOWN) {
            selectedBtn++;
            redraw = true;
        } else if (buttonsTriggered & VPAD_BUTTON_UP) {
            selectedBtn--;
            redraw = true;
        }
        if (selectedBtn < 0) {
            selectedBtn = 0;
        }
        if (selectedBtn >= 1) {
            selectedBtn = 1;
        }

        if (selectedBtn == 0) {
            if (buttonsTriggered & VPAD_BUTTON_LEFT) {
                curRegionIndex--;
                redraw = true;
            } else if (buttonsTriggered & VPAD_BUTTON_RIGHT) {
                curRegionIndex++;
                redraw = true;
            }
            if (curRegionIndex < 0) {
                curRegionIndex = 0;
            }
            if (curRegionIndex >= region_map.size()) {
                curRegionIndex = region_map.size() - 1;
            }
            gCurrentProductArea = region_index_to_map[curRegionIndex];
            curSelectedRegion = gCurrentProductArea;
        } else if (selectedBtn == 1) {
            if (buttonsTriggered & VPAD_BUTTON_LEFT) {
                curLangIndex--;
                redraw = true;
            } else if (buttonsTriggered & VPAD_BUTTON_RIGHT) {
                curLangIndex++;
                redraw = true;
            }
            if (curLangIndex < 0) {
                curLangIndex = 0;
            }
            if (curLangIndex >= lang_map.size()) {
                curLangIndex = lang_map.size() - 1;
            }
            gCurrentLanguage = lang_index_to_map[curLangIndex];
            curSelectedLanguage = gCurrentLanguage;
        }

        if (redraw) {
            DrawUtils::beginDraw();
            DrawUtils::clear(COLOR_BACKGROUND);

            DrawUtils::setFontColor(COLOR_TEXT);

            std::string headline = "Region Free Plugin";

            // draw top bar
            DrawUtils::setFontSize(24);
            DrawUtils::print(16, 6 + 24, headline.c_str());
            DrawUtils::drawRectFilled(8, 8 + 24 + 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);
            DrawUtils::setFontSize(18);

            // draw buttons
            uint32_t index = 8 + 24 + 8 + 4;

            DEBUG_FUNCTION_LINE("%d", selectedBtn);

            DrawUtils::setFontColor(COLOR_TEXT);
            DrawUtils::setFontSize(24);

            if (selectedBtn == 0) {
                DrawUtils::drawRect(16, index, SCREEN_WIDTH - 16 * 2, 44, 4, COLOR_BORDER_HIGHLIGHTED);
            } else {
                DrawUtils::drawRect(16, index, SCREEN_WIDTH - 16 * 2, 44, 2, COLOR_BORDER);
            }

            DrawUtils::print(16 * 2, index + 8 + 24, "Region");
            DrawUtils::print(SCREEN_WIDTH - 16 * 2, index + 8 + 24, region_map[curSelectedRegion], true);
            index += 42 + 8;

            if (selectedBtn == 1) {
                DrawUtils::drawRect(16, index, SCREEN_WIDTH - 16 * 2, 44, 4, COLOR_BORDER_HIGHLIGHTED);
            } else {
                DrawUtils::drawRect(16, index, SCREEN_WIDTH - 16 * 2, 44, 2, COLOR_BORDER);
            }
            DrawUtils::print(16 * 2, index + 8 + 24, "Language");

            DrawUtils::print(SCREEN_WIDTH - 16 * 2, index + 8 + 24, lang_map[curSelectedLanguage], true);

            index += 42 + 8;

            // draw bottom bar
            DrawUtils::drawRectFilled(8, SCREEN_HEIGHT - 24 - 8 - 4, SCREEN_WIDTH - 8 * 2, 3, COLOR_BLACK);
            DrawUtils::setFontSize(18);
            DrawUtils::print(16, SCREEN_HEIGHT - 8, "\ue07d Navigate ");
            DrawUtils::print(SCREEN_WIDTH - 16, SCREEN_HEIGHT - 8, "\ue001 Back", true);

            DrawUtils::endDraw();
            redraw = false;
        }
    }
    DrawUtils::beginDraw();
    DrawUtils::clear(COLOR_BLACK);
    DrawUtils::endDraw();

}

void ConfigUtils::openConfigMenu() {
    OSScreenInit();

    uint32_t screen_buf0_size = OSScreenGetBufferSizeEx(SCREEN_TV);
    uint32_t screen_buf1_size = OSScreenGetBufferSizeEx(SCREEN_DRC);
    void *screenbuffer0 = MEMAllocFromMappedMemoryForGX2Ex(screen_buf0_size, 0x100);
    void *screenbuffer1 = MEMAllocFromMappedMemoryForGX2Ex(screen_buf1_size, 0x100);

    if (!screenbuffer0 || !screenbuffer1) {
        DEBUG_FUNCTION_LINE("Failed to alloc buffers");
        goto error_exit;
    }

    OSScreenSetBufferEx(SCREEN_TV, screenbuffer0);
    OSScreenSetBufferEx(SCREEN_DRC, screenbuffer1);

    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);

    // Clear screens
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    // Flip buffers
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    DrawUtils::initBuffers(screenbuffer0, screen_buf0_size, screenbuffer1, screen_buf1_size);
    DrawUtils::initFont();

    displayMenu();

    DrawUtils::deinitFont();

    error_exit:

    if (screenbuffer0) {
        MEMFreeToMappedMemory(screenbuffer0);
    }

    if (screenbuffer1) {
        MEMFreeToMappedMemory(screenbuffer1);
    }
}
