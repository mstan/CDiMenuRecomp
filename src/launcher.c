/* SPDX-License-Identifier: MIT */
#include "cdi_launcher.h"
#include "cdi_launcher_settings.h"
#include "cdi_sha256.h"
#include "bios_identity.h"
#include "launcher_profile.h"
#include "recomp_launcher.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include "cdi_launcher_diagnostics.h"

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
    RecompLauncherCModProvider diagnostics={0};
    diagnostics.ctx=config;diagnostics.feature_count=diagnostics_feature_count;
    diagnostics.feature_get=diagnostics_feature_get;diagnostics.feature_enable=diagnostics_feature_enable;
    diagnostics.commit=diagnostics_mod_commit;game.mods=&diagnostics;

    cdi_launcher_settings_from_config(&settings,config);
    char *base = SDL_GetBasePath();
    char assets[1024];
    int length = snprintf(assets, sizeof assets, "%sassets", base ? base : "");
    SDL_free(base);
    if (length < 0 || (size_t)length >= sizeof assets) return 2;

    recomp_launcher_set_preserve_sdl(1);
    int result = recomp_launcher_run_window("Philips CD-i Menu", &settings, &game, assets,
        initial_bios ? initial_bios : config->bios_path, bios, bios_capacity);
    cdi_launcher_settings_to_config(config,&settings);
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
