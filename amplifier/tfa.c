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

#define LOG_TAG "tfa9887"
//#define LOG_NDEBUG 0

#include "tfa.h"
#include "tfa9887_bf.h"

#include <linux/tfa9895.h>

#include <cutils/log.h>
#include <tinyalsa/asoundlib.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

typedef struct uint24 {
    uint8_t bytes[3];
} uint24_t;

#define DEFAULT_SAMPLE_RATE 48000
#define MAX_EQ_ITEM_SIZE (sizeof(uint24_t))
#define MAX_EQ_LINE_SIZE 6
#define MAX_EQ_LINES 10
#define MAX_EQ_SIZE (MAX_EQ_ITEM_SIZE * MAX_EQ_LINE_SIZE * MAX_EQ_LINES)
#define MAX_I2C_LENGTH 254
#define MAX_PATCH_SIZE 3072
#define MAX_PARAM_SIZE 768
#define PATCH_HEADER_LENGTH 6

#define TFA9887_DEVICE "/dev/tfa9887"
#define I2S_MIXER_CTL "QUIN_MI2S_RX Audio Mixer MultiMedia1"

/* modules */
#define MODULE_SPEAKERBOOST  1
#define MODULE_BIQUADFILTERBANK 2
#define MODULE_SETRE 9

/* RPC commands */
#define PARAM_SET_RE0 0x00 // Sets the speaker calibration impedance (@25 degrees celsius)
#define PARAM_SET_LSMODEL 0x06 // Load a full model into SpeakerBoost.
#define PARAM_SET_LSMODEL_SEL 0x07 // Select one of the default models present in Tfa9887 ROM.
#define PARAM_SET_EQ 0x0A // 2 Equaliser Filters.
#define PARAM_SET_PRESET 0x0D // Load a preset
#define PARAM_SET_CONFIG 0x0E // Load a config
#define PARAM_SET_DRC 0x0F // Load DRC file
#define PARAM_SET_AGC 0x10 // Load DRC params

#define PARAM_GET_CONFIG_PRESET 0x80
#define PARAM_GET_RE0 0x85  // Gets the speaker calibration impedance (@25 degrees celsius)
#define PARAM_GET_LSMODEL 0x86  // Gets current LoudSpeaker Model.
#define PARAM_GET_STATE 0xC0

#define CONFIG_TFA9887 "/vendor/etc/tfa/tfa9895.config"
#define PATCH_TFA9887 "/vendor/etc/tfa/tfa9895.patch"

#define SPKR "/vendor/etc/tfa/tfa9895.speaker"

#define PRESET_PLAYBACK "/vendor/etc/tfa/playback.preset"
#define PRESET_VOICE "/vendor/etc/tfa/voice.preset"
#define PRESET_VOIP "/vendor/etc/tfa/voip.preset"

#define EQ_PLAYBACK "/vendor/etc/tfa/playback.eq"
#define EQ_VOICE "/vendor/etc/tfa/voice.eq"
#define EQ_VOIP "/vendor/etc/tfa/voip.eq"

#define DRC_PLAYBACK "/vendor/etc/tfa/playback.drc"
#define DRC_VOICE "/vendor/etc/tfa/voice.drc"
#define DRC_VOIP "/vendor/etc/tfa/voip.drc"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define ROUND_DOWN(a,n) (((a)/(n))*(n))

#define UNUSED __attribute__((unused))

struct mode_config_t {
    const char *preset;
    const char *eq;
    const char *drc;
};

const struct mode_config_t mode_configs[TFA9887_MODE_MAX] = {
    {   /* Playback */
        .preset = PRESET_PLAYBACK,
        .eq = EQ_PLAYBACK,
        .drc = DRC_PLAYBACK,
    },
    {   /* Voice */
        .preset = PRESET_VOICE,
        .eq = EQ_VOICE,
        .drc = DRC_VOICE,
    },
    {   /* VOIP */
        .preset = PRESET_VOIP,
        .eq = EQ_VOIP,
        .drc = DRC_VOIP,
    }
};

/*****************************************************************************/

static int i2s_interface_en(bool enable)
{
    enum mixer_ctl_type type;
    struct mixer_ctl *ctl;
    struct mixer *mixer = mixer_open(0);

    if (mixer == NULL) {
        ALOGE("Error opening mixer 0");
        return -1;
    }

    ctl = mixer_get_ctl_by_name(mixer, I2S_MIXER_CTL);
    if (ctl == NULL) {
        mixer_close(mixer);
        ALOGE("%s: Could not find %s", __func__, I2S_MIXER_CTL);
        return -ENODEV;
    }

    type = mixer_ctl_get_type(ctl);
    if (type != MIXER_CTL_TYPE_BOOL) {
        ALOGE("%s: %s is not supported", __func__, I2S_MIXER_CTL);
        mixer_close(mixer);
        return -EINVAL;
    }

    mixer_ctl_set_value(ctl, 0, enable);
    mixer_close(mixer);
    return 0;
}

void * write_dummy_data(void *param)
{
    struct tfa_t *t = (struct tfa_t *) param;
    uint8_t *buffer;
    int size;
    struct pcm *pcm;
    struct pcm_config config = {
        .channels = 2,
        .rate = 48000,
        .period_size = 256,
        .period_count = 2,
        .format = PCM_FORMAT_S16_LE,
        .start_threshold = config.period_size * config.period_count - 1,
        .stop_threshold = UINT_MAX,
        .silence_threshold = 0,
        .avail_min = 1,
    };

    if (i2s_interface_en(true)) {
        ALOGE("%s: Failed to enable I2S interface", __func__);
        return NULL;
    }

    pcm = pcm_open(0, 0, PCM_OUT | PCM_MONOTONIC, &config);
    if (!pcm || !pcm_is_ready(pcm)) {
        ALOGE("pcm_open failed: %s", pcm_get_error(pcm));
        if (pcm) {
            goto err_close_pcm;
        }
        goto err_disable_i2s;
    }

    size = 1024 * 8;
    buffer = calloc(1, size);
    if (!buffer) {
        ALOGE("%s: failed to allocate buffer", __func__);
        goto err_close_pcm;
    }

    bool signaled = false;
    do {
        if (pcm_write(pcm, buffer, size)) {
            ALOGE("%s: pcm_write failed", __func__);
        }
        if (!signaled) {
            pthread_mutex_lock(&t->mutex);
            t->writing = true;
            pthread_cond_signal(&t->cond);
            pthread_mutex_unlock(&t->mutex);
            signaled = true;
        }
    } while (t->initializing);

    t->writing = false;

err_free:
    free(buffer);
err_close_pcm:
    pcm_close(pcm);
err_disable_i2s:
    i2s_interface_en(false);
    return NULL;
}

/*****************************************************************************/

/* Utility functions */

static void bytes2data(const uint8_t bytes[], int num_bytes,
        int32_t data[])
{
    int i; /* index for data */
    int k; /* index for bytes */
    uint32_t d;
    int num_data = num_bytes / 3;

    for (i = 0, k = 0; i < num_data; i++, k += 3) {
        d = (bytes[k] << 16) | (bytes[k + 1] << 8) | (bytes[k + 2]);
        /* sign bit was set*/
        if (bytes[k] & 0x80) {
            d = - ((1 << 24) - d);
        }
        data[i] = d;
    }
}

static void data2bytes(const int32_t data[], int num_data, uint8_t bytes[])
{
    int i; /* index for data */
    int k; /* index for bytes */
    uint32_t d;

    for (i = 0, k = 0; i < num_data; i++, k += 3) {
        if (data[i] >= 0) {
            d = MIN(data[i], (1 << 23) - 1);
        } else {
            d = (1 << 24) - MIN(-data[i], 1 << 23);
        }
        bytes[k] = (d >> 16) & 0xff;
        bytes[k + 1] = (d >> 8) & 0xff;
        bytes[k + 2] = d & 0xff;
    }
}

static int read_file(const char *file_name, uint8_t *buf, size_t len, int seek)
{
    int rc;
    int fd;

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        rc = -errno;
        ALOGE("%s: unable to open file %s: %d", __func__, file_name, rc);
        return rc;
    }

    lseek(fd, seek, SEEK_SET);

    rc = read(fd, buf, len);
    if (rc < 0) {
        rc = -errno;
        ALOGE("%s: error reading from file %s: %d", __func__, file_name, rc);
    }

    close(fd);
    return rc;
}

static int read_eq_file(const char *file_name, uint8_t *buf, size_t len)
{
    const float disabled[5] = { 1.0, 0.0, 0.0, 0.0, 0.0 };
    float line[5];
    int32_t line_data[6];
    float max;
    FILE *f;
    int i, j;
    int idx, space, rc;

    memset(buf, 0, len);

    f = fopen(file_name, "r");
    if (!f) {
        rc = -errno;
        ALOGE("%s: Unable to open file %s: %d", __func__, file_name, rc);
        return rc;
    }

    for (i = 0; i < MAX_EQ_LINES; i++) {
        rc = fscanf(f, "%d %f %f %f %f %f", &idx, &line[0], &line[1],
                &line[2], &line[3], &line[4]);
        if (rc != 6) {
            ALOGE("%s: %s has bad format: line must be 6 values\n",
                    __func__, file_name);
            fclose(f);
            return -EINVAL;
        }

        if (idx != i + 1) {
            ALOGE("%s: %s has bad format: index mismatch\n",
                    __func__, file_name);
            fclose(f);
            return -EINVAL;
        }

        if (!memcmp(disabled, line, 5)) {
            /* skip */
            continue;
        } else {
            max = (float) fabs(line[0]);
            /* Find the max */
            for (j = 1; j < 5; j++) {
                if (fabs(line[j]) > max) {
                    max = (float) fabs(line[j]);
                }
            }
            space = (int) ceil(log(max + pow(2.0, -23)) / log(2.0));
            if (space > 8) {
                fclose(f);
                ALOGE("%s: Invalid value encountered\n", __func__);
                return -EINVAL;
            }
            if (space < 0) {
                space = 0;
            }

            /* Pack line into bytes */
            line_data[0] = space;
            line_data[1] = (int32_t) (-line[4] * (1 << (23 - space)));
            line_data[2] = (int32_t) (-line[3] * (1 << (23 - space)));
            line_data[3] = (int32_t) (line[2] * (1 << (23 - space)));
            line_data[4] = (int32_t) (line[1] * (1 << (23 - space)));
            line_data[5] = (int32_t) (line[0] * (1 << (23 - space)));
            data2bytes(line_data, MAX_EQ_LINE_SIZE,
                    &buf[i * MAX_EQ_LINE_SIZE * MAX_EQ_ITEM_SIZE]);
        }
    }

    fclose(f);
    return MAX_EQ_SIZE;
}

/*****************************************************************************/

/* TFA9895-specific IOCTL commands */

static int tfa9895_write_reg(struct tfa_t *t, uint8_t reg,
        uint16_t val)
{
    int rc;
    struct tfa9895_i2c_buffer cmd_buf;

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 4;
    cmd_buf.buffer[0] = reg;
    cmd_buf.buffer[1] = (0xff00 & val) >> 8;
    cmd_buf.buffer[2] = (0x00ff & val);
    cmd_buf.buffer[3] = 0;
    rc = ioctl(t->fd, TFA9895_WRITE_CONFIG(cmd_buf), &cmd_buf);
    if (rc) {
        rc = -errno;
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed: %d", rc);
        return rc;
    }

    return 0;
}

static int tfa9895_read_reg(struct tfa_t *t, uint8_t reg, uint16_t *val)
{
    int rc;
    struct tfa9895_i2c_buffer cmd_buf;

    /* kernel uses unsigned int */
    unsigned int reg_val[2];
    uint8_t buf[2];

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 1;
    cmd_buf.buffer[0] = reg;
    rc = ioctl(t->fd, TFA9895_WRITE_CONFIG(cmd_buf), &cmd_buf);
    if (rc) {
        rc = -errno;
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed: %d", rc);
        return rc;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 2;
    rc = ioctl(t->fd, TFA9895_READ_CONFIG(cmd_buf), &cmd_buf);
    if (rc) {
        rc = -errno;
        ALOGE("ioctl TFA9895_READ_CONFIG failed: %d", rc);
        return rc;
    }

    *val = ((cmd_buf.buffer[0] << 8) | cmd_buf.buffer[1]);
    return 0;
}

static int tfa9895_read_data(struct tfa_t *t, int addr, uint8_t *buf,
        size_t len, unsigned alignment)
{
    int rc;
    size_t bytes_read = 0;
    /* Round chunk size down to nearest alignment */
    size_t chunk_size = ROUND_DOWN(MAX_I2C_LENGTH, alignment);
    struct tfa9895_i2c_buffer cmd_buf;

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    memset(&cmd_buf, 0, sizeof(cmd_buf));
    cmd_buf.size = 1;
    cmd_buf.buffer[0] = (0xFF & addr);
    rc = ioctl(t->fd, TFA9895_WRITE_CONFIG(cmd_buf), &cmd_buf);
    if (rc) {
        rc = -errno;
        ALOGE("ioctl TFA9895_WRITE_CONFIG failed: %d", rc);
        return rc;
    }

    do {
        memset(&cmd_buf, 0, sizeof(cmd_buf));
        cmd_buf.size = MIN(len - bytes_read, chunk_size);
        rc = ioctl(t->fd, TFA9895_READ_CONFIG(cmd_buf), &cmd_buf);
        if (rc) {
            rc = -errno;
            ALOGE("ioctl TFA9895_READ_CONFIG failed: %d", rc);
            return rc;
        }
        memcpy(buf + bytes_read, cmd_buf.buffer, cmd_buf.size);
        bytes_read += cmd_buf.size;
    } while (bytes_read < len);

    return 0;
}

static int tfa9895_write_data(struct tfa_t *t, int addr,
        const uint8_t *buf, size_t len, unsigned alignment)
{
    int rc;
    size_t bytes_written = 0;
    size_t to_write = 0;
    size_t chunk_size = ROUND_DOWN(MAX_I2C_LENGTH, alignment);
    struct tfa9895_i2c_buffer cmd_buf;

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    do {
        /* Write the address out over I2C */
        memset(&cmd_buf, 0, sizeof(cmd_buf));
        to_write = MIN(len - bytes_written, chunk_size - 1);
        cmd_buf.size = to_write + 1;
        cmd_buf.buffer[0] = (0xFF & addr);
        memcpy(cmd_buf.buffer + 1, buf + bytes_written, to_write);
        rc = ioctl(t->fd, TFA9895_WRITE_CONFIG(cmd_buf), &cmd_buf);
        if (rc) {
            rc = -errno;
            ALOGE("ioctl TFA9895_WRITE_CONFIG failed: %d", rc);
            return rc;
        }
        bytes_written += to_write;
    } while (bytes_written < len);

    return 0;
}

static int tfa9895_lock(struct tfa_t *t, bool lock)
{
    int rc;
    int cmd = lock ? 1 : 0;

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    rc = ioctl(t->fd, TFA9895_KERNEL_LOCK(cmd), &cmd);
    if (rc) {
        rc = -errno;
        ALOGE("%s: Failed to lock amplifier: %d", __func__, rc);
        return rc;
    }

    return 0;
}

static int tfa9895_enable_dsp(struct tfa_t *t, bool enable)
{
    int rc;
    int cmd = enable ? 1 : 0;

    if (!t) {
        return -ENODEV;
    }

    if (t->fd < 0) {
        return -ENODEV;
    }

    rc = ioctl(t->fd, TFA9895_ENABLE_DSP(cmd), &cmd);
    if (rc) {
        rc = -errno;
        ALOGE("%s: Failed to enable DSP mode: %d", __func__, rc);
        return rc;
    }

    ALOGI("%s: Set DSP enable to %d", __func__, enable);

    return 0;
}

/*****************************************************************************/

/* Bitfield manipulation functions */

static inline void bf_set_bf_value(const uint16_t bf, const uint16_t bf_val,
        uint16_t *p_reg_val)
{
    uint16_t reg_val, mask;

    /*
     * bitfield enum
     * - 0..3  : len
     * - 4..7  : pos
     * - 8..15 : addr
     */

    uint8_t len = bf & 0x0f;
    uint8_t pos = (bf >> 4) & 0x0f;

    reg_val = *p_reg_val;

    mask = ((1 << (len + 1)) - 1) << pos;
    reg_val &= ~mask;
    reg_val |= bf_val << pos;

    *p_reg_val = reg_val;
}

static inline uint16_t bf_get_bf_value(const uint16_t bf, const uint16_t reg_val)
{
    uint16_t mask, val;

    /*
     * bitfield enum
     * - 0..3  : len
     * - 4..7  : pos
     * - 8..15 : addr
     */
    uint8_t len = bf & 0x0f;
    uint8_t pos = (bf >> 4) & 0x0f;

    mask = ((1 << (len + 1)) - 1) << pos;
    val = (reg_val & mask) >> pos;

    return val;
}

/*****************************************************************************/

/* Bitfield register operations */

static int tfa_set_bf(struct tfa_t *t, const uint16_t bf, const uint16_t val, bool force)
{
    int rc;
    uint16_t reg_val, mask, old_val;

    /*
     * bitfield enum
     * - 0..3  : len
     * - 4..7  : pos
     * - 8..15 : addr
     */
    uint8_t len = bf & 0x0f;
    uint8_t pos = (bf >> 4) & 0x0f;
    uint8_t addr = (bf >> 8) & 0xff;

    rc = tfa9895_read_reg(t, addr, &reg_val);
    if (rc) {
        ALOGE("%s: Failed to read reg: 0x%02x", __func__, addr);
        return rc;
    }

    old_val = reg_val;
    mask = ((1 << (len + 1)) - 1) << pos;
    reg_val &= ~mask;
    reg_val |= val << pos;

    if (force || old_val != reg_val) {
        rc = tfa9895_write_reg(t, addr, reg_val);
        if (rc) {
            ALOGE("%s: Failed to write reg: 0x%02x", __func__, addr);
            return rc;
        }
    }

    return 0;
}

static int tfa_get_bf(struct tfa_t *t, const uint16_t bf, uint16_t *val)
{
    int rc;
    uint16_t reg_val, mask;

    /*
     * bitfield enum
     * - 0..3  : len
     * - 4..7  : pos
     * - 8..15 : addr
     */
    uint8_t len = bf & 0x0f;
    uint8_t pos = (bf >> 4) & 0x0f;
    uint8_t addr = (bf >> 8) & 0xff;

    rc = tfa9895_read_reg(t, addr, &reg_val);
    if (rc) {
        ALOGE("%s: Failed to read reg: 0x%02x", __func__, addr);
        return rc;
    }
    mask = ((1 << (len + 1)) - 1) << pos;
    reg_val &= mask;
    *val = reg_val >> pos;

    return 0;
}

static int tfa_write_reg(struct tfa_t *t, const uint16_t bf, const uint16_t val)
{
    int rc;
    uint8_t addr = (bf >> 8) & 0xff;
    rc = tfa9895_write_reg(t, addr, val);
    if (rc) {
        ALOGE("%s: Failed to write reg: 0x%02x", __func__, addr);
        return rc;
    }

    return 0;
}

static int tfa_read_reg(struct tfa_t *t, const uint16_t bf, uint16_t *val)
{
    uint8_t addr = (bf >> 8) & 0xff;
    return tfa9895_read_reg(t, addr, val);
}

static int tfa_read_mem(struct tfa_t *t, uint16_t start_addr, size_t num_words, int32_t *data)
{
    /* memory word is 24 bits */
    uint8_t tmp[num_words * 3];
    int rc;

    tfa_set_bf(t, BF_DMEM, TFA9887_DMEM_XMEM, false);
    tfa_write_reg(t, BF_MADD, start_addr);
    rc = tfa9895_read_data(t, 0x72, tmp, num_words * 3, 3);
    if (rc) {
        ALOGE("%s: Failed to read data", __func__);
        return rc;
    }
    /* Assume data has enough memory allocated */
    bytes2data(tmp, num_words * 3, data);

    return 0;
}

static int tfa_write_mem(struct tfa_t *t, uint16_t addr, size_t len, uint8_t *buf)
{
    uint16_t cf_ctrl;

    tfa_read_reg(t, BF_DMEM, &cf_ctrl);
    /* Set autoincrement */
    bf_set_bf_value(BF_AIF, 0, &cf_ctrl);
    /* Set to DMEM */
    bf_set_bf_value(BF_DMEM, TFA9887_DMEM_XMEM, &cf_ctrl);
    tfa_write_reg(t, BF_DMEM, cf_ctrl);
    tfa_write_reg(t, BF_MADD, addr);
    return tfa9895_write_data(t, 0x72, buf, len, 3);
}

/*****************************************************************************/

/* TFA amplifier functions */

static void tfa9887_specific_init(struct tfa_t *t)
{
    tfa_set_bf(t, BF_AMPE, 1, true);

    tfa9895_write_reg(t, 0x05, 0x13AB);
    tfa9895_write_reg(t, 0x06, 0x001F);
    tfa9895_write_reg(t, 0x08, 0x3C4E);
    tfa9895_write_reg(t, 0x09, 0x024D);
    /* TODO: HTC's driver doesn't do this */
    /* tfa9895_write_reg(t, 0x0A, 0x3EC3); */
    tfa9895_write_reg(t, 0x41, 0x0308);
    tfa9895_write_reg(t, 0x49, 0x0E82);
}

static void htc_specific_init(struct tfa_t *t)
{
    uint16_t i2s_sel;
    tfa_set_bf(t, BF_DCVO, 0x5, true);
    tfa_set_bf(t, BF_DCCV, 0x1, true);

    /* not sure what this is doing */
    tfa9895_read_reg(t, 0x0a, &i2s_sel);
    i2s_sel = (i2s_sel & 0xf9fe) | 0x10;
    tfa9895_write_reg(t, 0x0a, i2s_sel);
}

static int tfa_hw_power(struct tfa_t *t, bool on)
{
    /* Set power down to true => power down */
    return tfa_set_bf(t, BF_PWDN, on ? 0 : 1, true);
}

static int tfa_mute(struct tfa_t *t, int mute_mode)
{
    int rc;
    uint16_t audioctrl_val, sysctrl_val;
    rc = tfa_read_reg(t, BF_CFSM, &audioctrl_val);
    if (rc) {
        ALOGE("%s: Failed to read reg: 0x%04x", __func__, BF_CFSM);
        return rc;
    }
    rc = tfa_read_reg(t, BF_AMPE, &sysctrl_val);
    if (rc) {
        ALOGE("%s: Failed to read reg: 0x%04x", __func__, BF_AMPE);
        return rc;
    }

    switch (mute_mode) {
        case TFA9887_MUTE_OFF:
            bf_set_bf_value(BF_CFSM, 0, &audioctrl_val);
            bf_set_bf_value(BF_AMPE, 1, &sysctrl_val);
            bf_set_bf_value(BF_DCA, 1, &sysctrl_val);
            break;
        case TFA9887_MUTE_DIGITAL:
            bf_set_bf_value(BF_CFSM, 1, &audioctrl_val);
            bf_set_bf_value(BF_AMPE, 1, &sysctrl_val);
            bf_set_bf_value(BF_DCA, 0, &sysctrl_val);
            break;
        case TFA9887_MUTE_AMPLIFIER:
            bf_set_bf_value(BF_CFSM, 0, &audioctrl_val);
            bf_set_bf_value(BF_AMPE, 0, &sysctrl_val);
            bf_set_bf_value(BF_DCA, 0, &sysctrl_val);
            break;
        default:
            ALOGE("%s: Bad mute mode: %d", __func__, mute_mode);
            break;
    }

    rc = tfa_write_reg(t, BF_CFSM, audioctrl_val);
    if (rc) {
        ALOGE("%s: Failed to write reg: 0x%04x", __func__, BF_CFSM);
        return rc;
    }
    rc = tfa_write_reg(t, BF_AMPE, sysctrl_val);
    if (rc) {
        ALOGE("%s: Failed to write reg: 0x%04x", __func__, BF_AMPE);
        return rc;
    }

    return 0;
}

static int tfa_set_channels(struct tfa_t *t, int channels)
{
    /*
     * valid channels: 0, 1, 2
     * bitfield len: 2
     * 0x1 => chan 0, 0x2 => chan 1, 0x3 => chan 2
     */
    if (channels < 0 || channels > 2) {
        ALOGE("%s: Invalid channel: %d", __func__, channels);
        return -EINVAL;
    }
    return tfa_set_bf(t, BF_CHS12, channels + 1, false);
}

static int tfa_select_input(struct tfa_t *t, int input)
{
    /*
     * valid inputs: 1, 2
     * bitfield len: 2
     * 0x1 => 1, 0x2 => 2
     */
    if (input < 1 || input > 2) {
        ALOGE("%s: Invalid input: %d", __func__, input);
        return -EINVAL;
    }
    return tfa_set_bf(t, BF_CHSA, input, false);
}

static int tfa_set_sample_rate(struct tfa_t *t, int rate)
{
    /*
     * valid rates: 48k, 44.1k, 32k, 24k, 22.05k, 16k, 12k, 11.025k, 8k
     * bitfield len: 4
     * 0x8 => 48k, 0x7 => 44.1k, 0x6 => 32k ... 0x0 => 8k
     */
    uint8_t bf_rate;
    switch (rate) {
        case 48000:
            bf_rate = 8;
            break;
        case 44100:
            bf_rate = 7;
            break;
        case 32000:
            bf_rate = 6;
            break;
        case 24000:
            bf_rate = 5;
            break;
        case 22050:
            bf_rate = 4;
            break;
        case 16000:
            bf_rate = 3;
            break;
        case 12000:
            bf_rate = 2;
            break;
        case 11025:
            bf_rate = 1;
            break;
        case 8000:
            bf_rate = 0;
            break;
        default:
            ALOGE("%s: Unsupported sample rate %d", __func__, rate);
            return -EINVAL;
    }
    return tfa_set_bf(t, BF_I2SSR, bf_rate, false);
}

static int tfa_set_coolflux_enabled(struct tfa_t *t, bool enable)
{
    /* equivalent to set_configured */
    ALOGI("%s: Set Coolflux state: %d", __func__, enable);
    return tfa_set_bf(t, BF_SBSL, enable ? 1 : 0, true);
}

static int tfa_get_swvstep(struct tfa_t *t, uint16_t *swvstep)
{
    uint16_t val;
    tfa_get_bf(t, BF_SWVSTEP, &val);
    *swvstep = val - 1;

    ALOGD("%s: swvstep: 0x%04x", __func__, *swvstep);

    return 0;
}

static int tfa_key2_lock(struct tfa_t *t, bool lock)
{
    /* unhide lock registers */
    tfa9895_write_reg(t, 0x40, 0x5a6b);
    tfa_write_reg(t, BF_MTPK, lock ? 0 : 0x5a);
    /* hide lock registers */
    tfa9895_write_reg(t, 0x40, 0);

    return 0;
}

/*****************************************************************************/

/* DSP functions */

static int tfa_dsp_reset(struct tfa_t *t, bool reset)
{
    return tfa_set_bf(t, BF_RST, reset ? 1 : 0, true);
}

static bool tfa_dsp_system_stable(struct tfa_t *t)
{
    uint16_t status;
    int rc;

    rc = tfa_read_reg(t, BF_AREFS, &status);
    if (rc) {
        ALOGE("%s: Failed to read reg: 0x%04x", __func__, BF_AREFS);
        return false;
    }
    /* Require AREFS and CLKS to both be set */
    return (bf_get_bf_value(BF_AREFS, status) &&
            bf_get_bf_value(BF_CLKS, status));
}

static bool tfa_check_ic_rom_version(struct tfa_t *t, uint8_t* patch_header)
{
    int32_t check_val, val;
    uint16_t check_addr;
    uint16_t rev_id;
    uint8_t check_rev = patch_header[0];
    uint8_t lsb_rev = t->rev & 0xff;

    if (check_rev != 0xff && check_rev != lsb_rev) {
        ALOGE("%s: Bad patch revision: 0x%02x", __func__, check_rev);
        return false;
    }

    check_addr = (patch_header[1] << 8) + (patch_header[2] << 0);
    check_val = (patch_header[3] << 16) + (patch_header[4] << 8) +
        (patch_header[5] << 0);

    if (check_addr != 0xffff) {
        if (!tfa_dsp_system_stable(t)) {
            ALOGE("%s: DSP system not stable!", __func__);
            return false;
        }

        tfa_read_mem(t, check_addr, 1, &val);
        if (val != check_val) {
            ALOGE("%s: Bad patch check value: 0x%04x, expected: 0x%04x",
                    __func__, check_val, val);
            return false;
        }
    } else {
        if (check_val != 0xffffff && check_val != 0) {
            rev_id = (patch_header[5] << 8) | (patch_header[0] << 0);
            if (rev_id != t->rev) {
                ALOGE("%s: Bad patch full revision: 0x%04x", __func__, rev_id);
                return false;
            }
        }
    }
    ALOGI("%s: Verified IC ROM version", __func__);
    return true;
}

static int tfa_dsp_patch(struct tfa_t *t, uint8_t *patch_buf, size_t len)
{
    int rc;
    unsigned index = 0;
    uint16_t size;
    uint8_t buffer[MAX_I2C_LENGTH] = {0};
    uint8_t *patch_data = patch_buf + PATCH_HEADER_LENGTH;
    size_t patch_data_len = len - PATCH_HEADER_LENGTH;

    if (!tfa_check_ic_rom_version(t, patch_buf)) {
        ALOGE("%s: Invalid patch version", __func__);
        return -EINVAL;
    }

    while (index < patch_data_len) {
        /* extract little endian length */
        size = patch_data[index] | (patch_data[index + 1] << 8);
        index += 2;
        if ((index + size) > patch_data_len) {
            /* outside the buffer, error in the input data */
            ALOGE("%s: Error in input data\n", __func__);
            return -EINVAL;
        }
        memcpy(buffer, patch_data + index, size);
        tfa9895_write_data(t, buffer[0], &buffer[1], size, 1);
        ALOGV("%s: %d %d", __func__, buffer[0], size);
        index += size;
    }

    return 0;
}

static int tfa_dsp_cold_boot(struct tfa_t *t, int state)
{
    uint8_t bytes[3];
    uint16_t acs;
    int tries;

    data2bytes(&state, 1, bytes);
    for (tries = 0; tries < 10; tries++) {
        tfa_set_bf(t, BF_DMEM, TFA9887_DMEM_IOMEM, false);
        tfa_write_reg(t, BF_MADD, 0x8100);
        tfa9895_write_data(t, 0x72, bytes, 3, 1);
        tfa_get_bf(t, BF_ACS, &acs);
        if (acs == state) {
            ALOGI("%s: Cold boot complete, ACS ready", __func__);
            return 0;
        }
    }
    ALOGE("%s: ACS did not become ready", __func__);
    return -EBUSY;
}

static bool tfa_dsp_check_rpc_status(struct tfa_t *t)
{
    uint8_t mem[3];
    uint16_t cf_ctrl = 0;

    bf_set_bf_value(BF_DMEM, TFA9887_DMEM_XMEM, &cf_ctrl);
    tfa_write_reg(t, BF_DMEM, cf_ctrl);
    /* Status => 0x0 */
    tfa_write_reg(t, BF_MADD, 0x0);
    tfa9895_read_data(t, 0x72, mem, sizeof(mem), 1);

    return ((mem[0] << 16) | (mem[1] << 8) | (mem[2] << 0) == 0);
}

static bool tfa_dsp_msg_status(struct tfa_t *t, int retries)
{
    uint16_t cf_status;
    int tries = 0;

    for (tries = 0; tries < retries; tries++) {
        usleep(1 * 1000);
        tfa_get_bf(t, BF_ACK, &cf_status);
        if (cf_status & 0x1) {
            if (tfa_dsp_check_rpc_status(t)) {
                return true;
            }
        }
    }

    ALOGE("%s: DSP did not ACK message after %d retries",
            __func__, tries);
    return false;
}

static int tfa_dsp_msg_read_id(struct tfa_t *t, uint8_t module_id,
        uint8_t param_id, uint8_t *data, size_t len)
{
    int rc;
    uint8_t id[3];
    uint16_t cf_ctrl = 0;

    bf_set_bf_value(BF_DMEM, TFA9887_DMEM_XMEM, &cf_ctrl);
    tfa_write_reg(t, BF_DMEM, cf_ctrl);
    /* IDs => 0x1 */
    tfa_write_reg(t, BF_MADD, 0x1);

    /* Set msg */
    id[0] = 0;
    id[1] = module_id + 128;
    id[2] = param_id;
    tfa9895_write_data(t, 0x72, id, 3, 3);

    /* Notify DSP */
    bf_set_bf_value(BF_REQCMD, 1, &cf_ctrl);
    bf_set_bf_value(BF_CFINT, 1, &cf_ctrl);
    tfa_write_reg(t, BF_CFINT, cf_ctrl);

    /* Check RPC status */
    if (!tfa_dsp_msg_status(t, 100)) {
        ALOGE("%s: Failed to read message from DSP", __func__);
        return -EBUSY;
    }

    /* Parameters => 0x2 */
    tfa_write_reg(t, BF_MADD, 0x2);
    int32_t words[len / 3];
    rc = tfa_read_mem(t, 0x72, len / 3, words);
    if (rc) {
        ALOGE("%s: Failed to read memory", __func__);
        return rc;
    }
    /* Assume we have enough space in data */
    data2bytes(words, len / 3, data);

    return 0;
}

static int tfa_dsp_msg_write_id(struct tfa_t *t, uint8_t module_id,
        uint8_t param_id, uint8_t *data, size_t len)
{
    uint8_t id[3];
    uint16_t cf_ctrl = 0;

    /* Set to DMEM */
    bf_set_bf_value(BF_DMEM, TFA9887_DMEM_XMEM, &cf_ctrl);
    tfa_write_reg(t, BF_DMEM, cf_ctrl);
    /* IDs => 0x1 */
    tfa_write_reg(t, BF_MADD, 0x1);

    /* Set msg */
    id[0] = 0;
    id[1] = module_id + 128;
    id[2] = param_id;
    tfa9895_write_data(t, 0x72, id, 3, 3);

    /* Write data */
    tfa9895_write_data(t, 0x72, data, len, 3);

    /* Notify DSP */
    bf_set_bf_value(BF_REQCMD, 1, &cf_ctrl);
    bf_set_bf_value(BF_CFINT, 1, &cf_ctrl);
    tfa_write_reg(t, BF_CFINT, cf_ctrl);

    /* Check RPC status */
    if (!tfa_dsp_msg_status(t, 100)) {
        ALOGE("%s: Failed to read message from DSP", __func__);
        return -EBUSY;
    }

    return 0;
}

static int tfa_dsp_reset_agc(struct tfa_t *t)
{
    uint8_t data[3];
    int32_t agc = 0;
    data2bytes(&agc, 1, data);
    return tfa_dsp_msg_write_id(t, MODULE_SPEAKERBOOST, PARAM_SET_AGC,
            data, 3);
}

static int tfa_dsp_load_file(struct tfa_t *t, const char *param_file)
{
    int rc;
    uint8_t param_id, module_id;
    uint8_t data[MAX_PARAM_SIZE];
    size_t len;
    char *suffix;

    suffix = strrchr(param_file, '.');
    if (suffix == NULL) {
        ALOGE("%s: Failed to determine parameter file type", __func__);
        return -EINVAL;
    } else if (strcmp(suffix, ".speaker") == 0) {
        param_id = PARAM_SET_LSMODEL;
        module_id = MODULE_SPEAKERBOOST;
        rc = read_file(param_file, data, MAX_PARAM_SIZE, 0);
        if (rc < 0) {
            goto load_file_failed;
        }
        len = rc;
    } else if (strcmp(suffix, ".config") == 0) {
        param_id = PARAM_SET_CONFIG;
        module_id = MODULE_SPEAKERBOOST;
        rc = read_file(param_file, data, MAX_PARAM_SIZE, 0);
        if (rc < 0) {
            goto load_file_failed;
        }
        len = rc;
    } else if (strcmp(suffix, ".preset") == 0) {
        param_id = PARAM_SET_PRESET;
        module_id = MODULE_SPEAKERBOOST;
        rc = read_file(param_file, data, MAX_PARAM_SIZE, 0);
        if (rc < 0) {
            goto load_file_failed;
        }
        len = rc;
    } else if (strcmp(suffix, ".drc") == 0) {
        param_id = PARAM_SET_DRC;
        module_id = MODULE_SPEAKERBOOST;
        rc = read_file(param_file, data, MAX_PARAM_SIZE, 0);
        if (rc < 0) {
            goto load_file_failed;
        }
        len = rc;
    } else if (strcmp(suffix, ".eq") == 0) {
        param_id = PARAM_SET_EQ;
        module_id = MODULE_BIQUADFILTERBANK;
        rc = read_eq_file(param_file, data, MAX_EQ_SIZE);
        if (rc < 0) {
            goto load_file_failed;
        }
        len = rc;
    } else {
        ALOGE("%s: Invalid DSP param file %s", __func__, param_file);
        return -EINVAL;
    }

    return tfa_dsp_msg_write_id(t, module_id, param_id, data, len);

load_file_failed:
    ALOGE("%s: Failed to load file %s: %d", __func__, param_file, rc);
    return rc;
}

static int tfa_dsp_set_mode(struct tfa_t *t, int mode)
{
    int rc;
    uint8_t buf[3];
    const struct mode_config_t *config;

    config = mode_configs;
    ALOGV("%s: Setting mode to %d", __func__, mode);

    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0; /* Should be 0 for voice or voip cases only, 1 otherwise */
    rc = tfa_write_mem(t, 0x0293, 3, buf);
    if (rc) {
        ALOGE("%s: Unable to write memory at 0x293", __func__);
        return rc;
    }
    ALOGI("%s: Prepared memory\n", __func__);

    rc = tfa_dsp_load_file(t, config[mode].preset);
    if (rc) {
        ALOGE("%s: Unable to load preset data", __func__);
        return rc;
    }
    ALOGI("%s: Loaded preset\n", __func__);

    rc = tfa_dsp_load_file(t, config[mode].eq);
    if (rc) {
        ALOGE("%s: Unable to load EQ data", __func__);
        return rc;
    }
    ALOGI("%s: Loaded EQ\n", __func__);

    rc = tfa_dsp_load_file(t, config[mode].drc);
    if (rc) {
        ALOGE("%s: Unable to load DRC data", __func__);
        return rc;
    }
    ALOGI("%s: Loaded DRC", __func__);

    ALOGI("%s: Set DSP mode to %u", __func__, mode);

    return 0;
}

static int tfa_dsp_get_calibration_impedance(struct tfa_t *t)
{
    int rc;
    uint16_t val;
    int32_t word;
    uint8_t bytes[3];
    float re_25c;

    tfa_get_bf(t, BF_MTPOTC, &val);
    if (val) {
        ALOGD("%s: OTC set, read from MTP", __func__);
    } else {
        rc = tfa_dsp_msg_read_id(t, MODULE_SPEAKERBOOST,
                PARAM_SET_RE0, bytes, 3);
        if (rc) {
            ALOGE("%s: Failed to read speakerboost RE0", __func__);
            return -EIO;
        }
        bytes2data(bytes, 3, &word);
        re_25c = ((float) word) / (1 << (23 - 9));
        ALOGI("%s: Speaker impedance: %0.2f", __func__, re_25c);
    }

    return 0;
}

/*****************************************************************************/

/* Startup functions */

static int tfa_startup(struct tfa_t *t)
{
    int rc, tries;
    uint16_t val;

    /* Reset all I2C registers to default */
    val = 0;
    bf_set_bf_value(BF_I2CR, 1, &val);
    tfa_write_reg(t, BF_I2CR, val);

    /* Put DSP in reset */
    tfa_dsp_reset(t, true);

    /* Perform amp-specific init */
    tfa9887_specific_init(t);

    /* Set up input properties */
    rc = tfa_set_sample_rate(t, DEFAULT_SAMPLE_RATE);
    if (rc) {
        ALOGE("%s: Unable to set sample rate: %d", __func__, rc);
        return rc;
    }
    rc = tfa_set_channels(t, 2);
    if (rc) {
        ALOGE("%s: Unable to select channel: %d", __func__, rc);
        return rc;
    }
    rc = tfa_select_input(t, 2);
    if (rc) {
        ALOGE("%s: Unable to select input: %d", __func__, rc);
        return rc;
    }

    /* Power up */
    rc = tfa_hw_power(t, true);
    if (rc) {
        ALOGE("%s: Unable to power up: %d", __func__, rc);
        return rc;
    }

    /* Wait for PLL ready */
    for (tries = 0; tries < 100; tries++) {
        /* Wait 10ms */
        usleep(10 * 1000);

        if (tfa_dsp_system_stable(t)) {
            ALOGD("%s: DSP system stable, continuing startup", __func__);
            return 0;
        }
    }

    ALOGE("%s: Timed out waiting for DSP system stable", __func__);
    return -EBUSY;
}

static int tfa_dsp_startup(struct tfa_t *t)
{
    int rc;
    uint8_t bytes[3];
    int32_t count_boot = 0;
    uint8_t patch_buf[MAX_PATCH_SIZE];
    size_t patch_size;

    rc = read_file(PATCH_TFA9887, patch_buf, MAX_PATCH_SIZE, 0);
    if (rc < 0) {
        ALOGE("%s: Failed to read patch file %s", __func__, PATCH_TFA9887);
        return rc;
    }
    patch_size = rc;

    rc = tfa_dsp_patch(t, patch_buf, patch_size);
    if (rc) {
        ALOGE("%s: Unable to load DSP patch: %d", __func__, rc);
        return rc;
    }
    ALOGI("%s: Loaded patch file", __func__);

    /* Clear count_boot */
    tfa_set_bf(t, BF_DMEM, TFA9887_DMEM_XMEM, false);
    tfa_write_reg(t, BF_MADD, 0x200);
    data2bytes(&count_boot, 1, bytes);
    tfa9895_write_data(t, 0x72, bytes, 3, 1);

    /* Take DSP out of reset - paired with call in tfa_startup */
    tfa_dsp_reset(t, false);

    return 0;
}

static int tfa_speaker_startup(struct tfa_t *t, bool force) {
    int rc;

    if (!force) {
        tfa_startup(t);
        tfa_dsp_startup(t);
    }

    rc = tfa_dsp_load_file(t, SPKR);
    if (rc) {
        ALOGE("%s: Unable to load speaker data: %d", __func__, rc);
        return rc;
    }
    ALOGI("%s: Loaded speaker file", __func__);

    rc = tfa_dsp_reset_agc(t);
    if (rc) {
        ALOGE("%s: Unable to reset AGC: %d", __func__, rc);
        return rc;
    }
    ALOGI("%s: Reset AGC after speaker load", __func__);

    rc = tfa_dsp_load_file(t, CONFIG_TFA9887);
    if (rc) {
        ALOGE("%s: Unable to load config data: %d", __func__, rc);
        return rc;
    }
    ALOGI("%s: Loaded config file\n", __func__);

    rc = tfa_dsp_reset_agc(t);
    if (rc) {
        ALOGE("%s: Unable to reset AGC: %d", __func__, rc);
        return rc;
    }
    ALOGI("%s: Reset AGC after config load", __func__);

    rc = tfa_dsp_set_mode(t, t->mode);
    if (rc) {
        ALOGE("%s: Failed to load DSP config", __func__);
        return rc;
    }

    ALOGI("%s: Speaker startup complete", __func__);
    return 0;
}

static void cold_startup(struct tfa_t *t)
{
    tfa_startup(t);
    tfa_dsp_cold_boot(t, 1);
    tfa_dsp_startup(t);
}

static int wait_calibration(struct tfa_t *t)
{
    int rc, tries;
    uint16_t val;
    int32_t calibrated;

    tfa_get_bf(t, BF_MTPOTC, &val);
    if (val) {
        /* Wait for MTPB to clear */
        for (tries = 0; tries < 50; tries++) {
            usleep(10 * 1000);
            tfa_get_bf(t, BF_MTPB, &val);
            if (!val) {
                break;
            }
        }
        if (tries >= 50) {
            ALOGE("%s: Timed out waiting for MTPB to clear", __func__);
            return -EBUSY;
        }

        /* Wait for MTPEX to be set */
        for (tries = 0; tries < 50; tries++) {
            usleep(50 * 1000);
            tfa_get_bf(t, BF_MTPEX, &val);
            if (val) {
                ALOGI("%s: OTC complete", __func__);
                return 0;
            }
        }
    } else {
        /* Read whether calibration finished */
        for (tries = 0; tries < 40; tries++) {
            usleep(10 * 1000);
            rc = tfa_read_mem(t, 231, 1, &calibrated);
            if (rc) {
                continue;
            }
            ALOGD("%s: read calibration: 0x%04x", __func__, calibrated);
            if (calibrated) {
                ALOGI("%s: Calibration complete", __func__);
                return 0;
            }
        }
    }
    ALOGE("%s: Timed out waiting for calibration", __func__);
    return -EBUSY;
}

static int speaker_calibration(struct tfa_t *t)
{
    int rc;
    uint16_t val;

    rc = tfa_set_coolflux_enabled(t, true);
    if (rc) {
        ALOGE("%s: Failed to enable Coolflux", __func__);
        return rc;
    }

    tfa_get_bf(t, BF_MTPOTC, &val);
    if (val == 1) {
        ALOGD("%s: MTPOTC is set, performing key unlock", __func__);
        tfa_key2_lock(t, false);
    }

    rc = wait_calibration(t);

    rc = tfa_dsp_get_calibration_impedance(t);
    if (rc) {
        ALOGE("%s: Failed to read speaker calibration impedance", __func__);
    }

    tfa_get_bf(t, BF_MTPOTC, &val);
    if (val == 1) {
        ALOGD("%s: MTPOTC is set, performing key lock", __func__);
        tfa_key2_lock(t, true);
    }

    return 0;
}

static int speaker_boost(struct tfa_t *t, bool force)
{
    int rc;
    uint16_t acs;

    if (force) {
        cold_startup(t);
    }

    tfa_get_bf(t, BF_ACS, &acs);

    if (acs == 0x1) {
        tfa_speaker_startup(t, true);

        rc = speaker_calibration(t);
        if (rc) {
            ALOGE("%s: Failed to run speaker calibration", __func__);
            return rc;
        }
    }

    return 0;
}

/*****************************************************************************/

/* Public interface */

struct tfa_t * tfa_new(void)
{
    int rc;
    struct tfa_t *t;

    t = calloc(1, sizeof(struct tfa_t));
    if (!t) {
        ALOGE("%s:%d: Failed to allocate TFA9887 amplifier device memory",
                __func__, __LINE__);
        return NULL;
    }

    t->mode = TFA9887_MODE_PLAYBACK;
    t->initializing = true;
    t->writing = false;
    t->clock_enabled = false;
    pthread_mutex_init(&t->mutex, NULL);
    pthread_cond_init(&t->cond, NULL);

    t->fd = open(TFA9887_DEVICE, O_RDWR);
    if (t->fd < 0) {
        ALOGE("%s:%d: Failed to open amplifier device: %d\n",
                __func__, __LINE__, -errno);
        free(t);
        return NULL;
    }

    /* Read chip revision */
    rc = tfa_read_reg(t, BF_REV, &t->rev);
    if (rc) {
        ALOGE("%s: Failed to read TFA revision", __func__);
        close(t->fd);
        free(t);
        return NULL;
    }

    ALOGI("%s: Instantiated driver for TFA rev 0x%04x", __func__, t->rev);

    return t;
}

int tfa_clock_on(struct tfa_t *t)
{
    if (t->clock_enabled) {
        ALOGW("%s: clocks already on\n", __func__);
        return -EBUSY;
    }

    pthread_create(&t->write_thread, NULL, write_dummy_data, t);
    pthread_mutex_lock(&t->mutex);
    while (!t->writing) {
        pthread_cond_wait(&t->cond, &t->mutex);
    }
    pthread_mutex_unlock(&t->mutex);
    t->clock_enabled = true;

    ALOGI("%s: clocks enabled\n", __func__);

    return 0;
}

int tfa_clock_off(struct tfa_t *t)
{
    if (!t->clock_enabled) {
        ALOGW("%s: clocks already off\n", __func__);
        return 0;
    }

    t->initializing = false;
    pthread_join(t->write_thread, NULL);
    t->clock_enabled = false;

    ALOGI("%s: clocks disabled\n", __func__);

    return 0;
}

int tfa_init(struct tfa_t *t)
{
    int rc = 0;
    uint16_t value = 0;

    rc = tfa9895_enable_dsp(t, false);
    if (rc) {
        ALOGE("%s: Failed to disable DSP mode: %d", __func__, rc);
        return rc;
    }

    /* Enable I2S output for devices without TDM */
    tfa_set_bf(t, BF_I2SDOE, 1, false);

    rc = speaker_boost(t, true);
    if (rc) {
        ALOGE("%s: Failed to start speaker boost", __func__);
        tfa_hw_power(t, false);
        return rc;
    }

    rc = tfa9895_enable_dsp(t, true);
    if (rc) {
        ALOGE("%s: Failed to enable DSP mode: %d", __func__, rc);
        tfa_hw_power(t, false);
        return rc;
    }

    htc_specific_init(t);

    tfa_hw_power(t, false);

    return 0;
}

int tfa_power(struct tfa_t *t, bool on)
{
    int rc;

    rc = tfa_hw_power(t, on);
    if (rc) {
        ALOGE("Unable to power on amp: %d\n", rc);
        return rc;
    }

    ALOGI("%s: Set amplifier power to %d\n", __func__, on);

    return 0;
}

int tfa_set_mode(struct tfa_t *t, int mode)
{
    int rc;

    if (mode == t->mode) {
        ALOGV("No mode change needed, already mode %d", mode);
        return 0;
    }
    tfa9895_lock(t, true);
    tfa_mute(t, TFA9887_MUTE_DIGITAL);
    rc = tfa_dsp_set_mode(t, mode);
    if (rc == 0) {
        /* Only count DSP mode switches that were successful */
        t->mode = mode;
    }
    tfa_mute(t, TFA9887_MUTE_OFF);
    tfa9895_lock(t, false);

    ALOGV("%s: Set amplifier audio mode to %d\n", __func__, mode);

    return 0;
}

int tfa_set_mute(struct tfa_t *t, bool on)
{
    int rc;

    rc = tfa_mute(t, on ? TFA9887_MUTE_DIGITAL : TFA9887_MUTE_OFF);
    if (rc) {
        ALOGE("Unable to mute: %d\n", rc);
    }

    ALOGI("%s: Set mute to %d\n", __func__, on);

    return 0;
}

void tfa_destroy(struct tfa_t *t)
{
    tfa_hw_power(t, false);
    close(t->fd);
    free(t);
}
