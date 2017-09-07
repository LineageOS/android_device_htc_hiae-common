/*
 * Copyright (C) 2013, The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "rt5501"
//#define LOG_NDEBUG 0

#include "rt5501.h"

#include <linux/rt5506.h>

#include <cutils/log.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

static struct rt55xx_config rt55xx_playback_config = {
    .reg_len = 8,
    .reg = {
        { 0x00, 0xC0, },
        { 0x01, 0x10, }, // gain -12dB
        { 0x02, 0x80, }, // noise gate on
        { 0x08, 0x37, }, // noise gate on
        { 0x07, 0x7F, }, // noise gate setting
        { 0x09, 0x02, }, // noise gate setting
        { 0x0A, 0x03, }, // noise gate setting
        { 0x0B, 0xD8, }, // noise gate -4dB
    },
};

static struct rt55xx_config rt55xx_playback_128_config = {
    .reg_len = 8,
    .reg = {
        { 0x00, 0xC0, },
        { 0x01, 0x12, }, // gain -10dB
        { 0x02, 0x80, }, // noise gate on
        { 0x08, 0x37, }, // noise gate on
        { 0x07, 0x7F, }, // noise gate setting
        { 0x09, 0x02, }, // noise gate setting
        { 0x0A, 0x03, }, // noise gate setting
        { 0x0B, 0xD8, }, // noise gate -4dB
    },
};

static struct rt55xx_config rt55xx_voice_config = {
    .reg_len = 7,
    .reg = {
        { 0x00, 0xC0, },
        { 0x01, 0x18, }, // gain -4dB
        { 0x02, 0x00, }, // noise gate off
        { 0x07, 0x7F, }, // noise gate setting
        { 0x09, 0x01, }, // noise gate setting
        { 0x0A, 0x00, }, // noise gate setting
        { 0x0B, 0xC7, }, // noise gate -35dB
    },
};

static struct rt55xx_config rt55xx_ring_config = {
    .reg_len = 8,
    .reg = {
        { 0x00, 0xC0, },
        { 0x01, 0x0C, }, // gain -16dB
        { 0x02, 0x81, }, // noise gate on
        { 0x08, 0x01, }, // noise gate on
        { 0x07, 0x7F, }, // noise gate setting
        { 0x09, 0x01, }, // noise gate setting
        { 0x0A, 0x00, }, // noise gate setting
        { 0x0B, 0xC7, }, // noise gate -35dB
    },
};

static int rt55xx_configure(struct rt55xx_t *amp)
{
    int rc = 0;
    struct rt55xx_config_data cfg;

    memset(&cfg, 0, sizeof(struct rt55xx_config_data));

    cfg.mode_num = RT55XX_MAX_MODE;
    cfg.cmd_data[RT55XX_MODE_PLAYBACK].config = rt55xx_playback_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK8OH].config = rt55xx_playback_8_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK16OH].config = rt55xx_playback_16_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK32OH].config = rt55xx_playback_32_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK64OH].config = rt55xx_playback_64_config
    cfg.cmd_data[RT55XX_MODE_PLAYBACK128OH].config = rt55xx_playback_128_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK256OH].config = rt55xx_playback_256_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK500OH].config = rt55xx_playback_512_config;
    //cfg.cmd_data[RT55XX_MODE_PLAYBACK1KOH].config = rt55xx_playback_1024_config;
    cfg.cmd_data[RT55XX_MODE_VOICE].config = rt55xx_voice_config;
    //cfg.cmd_data[RT55XX_MODE_TTY].config = rt55xx_tty_config;
    //cfg.cmd_data[RT55XX_MODE_FM].config = rt55xx_fm_config;
    cfg.cmd_data[RT55XX_MODE_RING].config = rt55xx_ring_config;
    //cfg.cmd_data[RT55XX_MODE_MFG].config = rt55xx_mfg_config;
    //cfg.cmd_data[RT55XX_MODE_BEATS_8_64].config = rt55xx_beats_8_64_config;
    //cfg.cmd_data[RT55XX_MODE_BEATS_128_500].config = rt55xx_beats_128_500_config;
    //cfg.cmd_data[RT55XX_MODE_MONO].config = rt55xx_mono_config;
    //cfg.cmd_data[RT55XX_MODE_MONO_BEATS].config = rt55xx_mono_beats_config;

    /* Open the amplifier device */
    /* Load config */
    if ((rc = ioctl(amp->fd, RT55XX_SET_PARAM, &cfg)) < 0) {
        rc = -errno;
        ALOGE("%s: ioctl RT55XX_SET_CONFIG failed. ret = %d\n",
                __func__, rc);
    }

    return rc;
}

struct rt55xx_t * rt55xx_new(void)
{
    struct rt55xx_t *amp;
    int rc;

    amp = calloc(1, sizeof(struct rt55xx_t));
    if (!amp) {
        ALOGE("%s:%d: Unable to allocate memory for RT55XX module\n",
                __func__, __LINE__);
        return NULL;
    }

    if ((amp->fd = open(RT55XX_DEVICE, O_RDWR)) < 0) {
        rc = -errno;
        ALOGE("%s:%d: Error opening amplifier device %s: %d\n",
                __func__, __LINE__, RT55XX_DEVICE, rc);
        free(amp);
        return NULL;
    }

    rc = rt55xx_configure(amp);
    if (rc) {
        ALOGE("%s:%d: Failed to configure amplifier device\n",
                __func__, __LINE__);
        close(amp->fd);
        free(amp);
        return NULL;
    }

    amp->mode = RT55XX_MODE_PLAYBACK;

    return amp;
}

int rt55xx_set_mode(struct rt55xx_t *amp, audio_mode_t mode)
{
    int headsetohm = HEADSET_OM_UNDER_DETECT;
    int rc = 0;

    /* Get impedance of headset */
    if ((rc = ioctl(amp->fd, RT55XX_QUERY_OM, &headsetohm)) < 0) {
        rc = -errno;
        ALOGE("%s: error querying headset impedance: %d\n", __func__, rc);
        return rc;
    }

    switch (mode) {
        default:
        case AUDIO_MODE_NORMAL:
            /* For headsets with a impedance between 128ohm and 1000ohm */
            if (headsetohm >= HEADSET_128OM && headsetohm <= HEADSET_1KOM) {
                ALOGI("%s: Mode: Playback 128\n", __func__);
                amp->mode = RT55XX_MODE_PLAYBACK128OH;
            } else {
                ALOGI("%s: Mode: Playback\n", __func__);
                amp->mode = RT55XX_MODE_PLAYBACK;
            }
            break;
        case AUDIO_MODE_RINGTONE:
            ALOGI("%s: Mode: Ring\n", __func__);
            amp->mode = RT55XX_MODE_RING;
            break;
        case AUDIO_MODE_IN_CALL:
        case AUDIO_MODE_IN_COMMUNICATION:
            ALOGI("%s: Mode: Voice\n", __func__);
            amp->mode = RT55XX_MODE_VOICE;
            break;
    }

    /* Set the selected config */
    if ((rc = ioctl(amp->fd, RT55XX_SET_MODE, &amp->mode)) < 0) {
        rc = -errno;
        ALOGE("%s: ioctl RT55XX_SET_MODE failed. rc = %d\n", __func__, rc);
        return rc;
    }

    return 0;
}

void rt55xx_destroy(struct rt55xx_t *amp)
{
    close(amp->fd);
    free(amp);
}
