#pragma once

#include <coreinit/mcp.h>
#include <coreinit/userconfig.h>

extern "C" int (*real_UCReadSysConfig)(int IOHandle, int count, UCSysConfig *settings) __attribute__((section(".data")));
extern "C" int (*real_MCP_GetSysProdSettings)(int IOHandle, MCPSysProdSettings *settings) __attribute__((section(".data")));