/* SPDX-License-Identifier: MIT */
#include "cdi_launcher.h"
#include "cdi_sha256.h"
#include "bios_identity.h"
#include "launcher_profile.h"
#include "recomp_launcher.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

static int verify_bios(const char *path, RecompLauncherCDiscVerify *verdict) {
    uint8_t hash[32];
    int valid = cdi_sha256_file(path, hash) &&
                !memcmp(hash, cdi_compiled_bios_sha256, sizeof hash);
    verdict->verdict = valid ? 1 : 3;
    verdict->iso_ok = valid;
    if (valid) snprintf(verdict->serial, sizeof verdict->serial, "CDI490");
    return 1;
}

int cdi_launcher_run(const char *initial_bios, const char *initial_disc,
                     char *bios, size_t bios_capacity, char *disc, size_t disc_capacity,
                     CdiPlayerConfig *config, const char *config_path) {
    (void)initial_disc;
    if (!disc_capacity) return 2;
    disc[0] = '\0';
    RecompLauncherCSettings settings = {0};
    RecompLauncherCGameInfo game = {0};
    launcher_profile_apply("cdi", &game);
    static const char *patterns[] = {"*.rom", "*.bin"};
    static const char *aspects[] = {"Original 4:3", "Fit to screen"};
    game.name = "Philips CD-i Menu";
    game.rom_noun = "player BIOS";
    game.rom_filter_desc = "CD-i player BIOS";
    game.rom_patterns = patterns;
    game.num_rom_patterns = 2;
    game.known_sha256 = &cdi_compiled_bios_sha256;
    game.num_known_sha256 = 1;
    game.disc_verify = verify_bios;
    game.has_bios = 0;
    game.has_mouse_controls = 1;
    game.has_window_size = 1;
    game.aspect_labels = aspects;
    game.num_aspect_labels = 2;
    game.setup_wizard_supported = 1;
    game.config_path = config_path;

    settings.window_scale = 2;
    settings.window_width = config->window_width;
    settings.aspect_index = config->fit_to_screen;
    settings.fullscreen = config->fullscreen;
    settings.linear_filter = config->linear_filter;
    settings.enable_audio = config->enable_audio;
    settings.audio_freq = 44100;
    settings.volume = config->volume;
    settings.mouse_enabled = config->enable_mouse;
    settings.mouse_sensitivity = config->mouse_sensitivity;
    settings.mouse_invert_x = config->mouse_invert_x;
    settings.mouse_invert_y = config->mouse_invert_y;
    settings.player_src[0] = config->input_source;
    memcpy(settings.mouse_bind, config->mouse_bind, sizeof config->mouse_bind);
    memcpy(settings.player_key_bind[0], config->key_bind, sizeof config->key_bind);
    memcpy(settings.player_pad_bind[0], config->pad_bind, sizeof config->pad_bind);
    snprintf(settings.player_gamepad_guid[0], sizeof settings.player_gamepad_guid[0],
             "%s", config->gamepad_guid);
    char *base = SDL_GetBasePath();
    char assets[1024];
    int length = snprintf(assets, sizeof assets, "%sassets", base ? base : "");
    SDL_free(base);
    if (length < 0 || (size_t)length >= sizeof assets) return 2;

    recomp_launcher_set_preserve_sdl(1);
    int result = recomp_launcher_run_window("Philips CD-i Menu", &settings, &game, assets,
        initial_bios ? initial_bios : config->bios_path, bios, bios_capacity);
    config->fit_to_screen = settings.aspect_index == 1;
    config->window_width = settings.window_width;
    config->fullscreen = settings.fullscreen;
    config->linear_filter = settings.linear_filter;
    config->enable_audio = settings.enable_audio;
    config->volume = settings.volume;
    config->enable_mouse = settings.mouse_enabled;
    config->mouse_sensitivity = settings.mouse_sensitivity;
    config->mouse_invert_x = settings.mouse_invert_x;
    config->mouse_invert_y = settings.mouse_invert_y;
    config->input_source = settings.player_src[0];
    config->gamepad_instance = settings.player_gamepad_instance[0];
    memcpy(config->mouse_bind, settings.mouse_bind, sizeof config->mouse_bind);
    memcpy(config->key_bind, settings.player_key_bind[0], sizeof config->key_bind);
    memcpy(config->pad_bind, settings.player_pad_bind[0], sizeof config->pad_bind);
    snprintf(config->gamepad_guid, sizeof config->gamepad_guid, "%s", settings.player_gamepad_guid[0]);
    if (result == RECOMP_LAUNCHER_RESULT_LAUNCH) {
        uint8_t digest[32];
        if (strlen(bios) >= sizeof config->bios_path ||
            !cdi_sha256_file(bios, digest) || memcmp(digest, cdi_compiled_bios_sha256, sizeof digest)) {
            return 2;
        }
        strcpy(config->bios_path, bios);
    }
    if (!cdi_player_config_save(config_path, config)) return 2;
    if (result == RECOMP_LAUNCHER_RESULT_QUIT) return 1;
    return result == RECOMP_LAUNCHER_RESULT_LAUNCH ? 0 : 2;
}
