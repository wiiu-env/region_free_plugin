[![CI-Release](https://github.com/wiiu-env/region_free_plugin/actions/workflows/ci.yml/badge.svg)](https://github.com/wiiu-env/region_free_plugin/actions/workflows/ci.yml)

# Region Free Plugin

This plugin allows you to launch a title of an other region and / or force the language of a title.

## Installation
(`[ENVIRONMENT]` is a placeholder for the actual environment name.)

1. Copy the file `regionfree.wps` into `sd:/wiiu/environments/[ENVIRONMENT]/plugins`.  
2. Requires the [WiiUPluginLoaderBackend](https://github.com/wiiu-env/WiiUPluginLoaderBackend) in `sd:/wiiu/environments/[ENVIRONMENT]/modules`.

At the first launch with the plugin open the config menu (press L, DPAD Down and Minus on the gamepad) and set your default language for each region.

## Usage

The plugin has a built in auto detection, so in most cases it just works out of the box.

Via the plugin config menu (press L, DPAD Down and Minus on the gamepad) you can configure the plugin. The available options are the following:

- **Auto detect region/language**: Enables/Disables the auto detection of the region/language. When you disable it, you need/can set the region and language for a title on each title start. Enabled by default.
- **Force auto detection for in-region titles**: Force auto detection when starting a title of your region, even if auto detection is disabled. Enabled by default.
- **Prefer system settings for in-region titles**: Forces the region, country and language of your console when starting a title of your region (Ignoring "Default language for XXX"). Enabled by default.
- **Default language for EUR**: Sets the default language for EUR titles. Set to english by default.
- **Default language for USA**: Sets the default language for USA titles. Set to english by default.

If the auto detection fails, the user needs to enter a region/language on the title boot.

The plugin keeps tracks the region/language the user has selected for a title.

Scenario:  
 - The User has disabled the auto detection and booted a EUR version of Mario Kart 8 in German on their US console.
 - The User then enables the auto detection and set the default language for EUR titles to English.
 - The EUR version of Mario Kart 8 will still boot in German. To change the language the user has to disable the auto detection and reboot the title.

## Building using the Dockerfile

It's possible to use a docker image for building. This way you don't need anything installed on your host system.

```
# Build docker image (only needed once)
docker build . -t regionfree_plugin-builder

# make 
docker run -it --rm -v ${PWD}:/project regionfree_plugin-builder make

# make clean
docker run -it --rm -v ${PWD}:/project regionfree_plugin-builder make clean
```

## Format the code via docker

`docker run --rm -v ${PWD}:/src wiiuenv/clang-format:13.0.0-2 -r ./src -i`
