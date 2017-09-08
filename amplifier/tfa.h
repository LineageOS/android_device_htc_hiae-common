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

#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

enum {
    TFA9887_MODE_PLAYBACK = 0,
    TFA9887_MODE_VOICE,
    TFA9887_MODE_VOIP,
    TFA9887_MODE_MAX,
};

enum {
    TFA9887_MUTE_OFF = 0,
    TFA9887_MUTE_DIGITAL,
    TFA9887_MUTE_AMPLIFIER,
};

/* possible memory values for DMEM in CF_CONTROLs */
enum {
    TFA9887_DMEM_PMEM = 0,
    TFA9887_DMEM_XMEM,
    TFA9887_DMEM_YMEM,
    TFA9887_DMEM_IOMEM,
};

struct tfa_t {
    int fd;
    int mode;
    uint16_t rev;
    atomic_bool initializing;
    bool clock_enabled;
    bool writing;
    pthread_t write_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

struct tfa_t * tfa_new(void);
int tfa_clock_on(struct tfa_t *amp);
int tfa_clock_off(struct tfa_t *amp);
int tfa_init(struct tfa_t *amp);
int tfa_power(struct tfa_t *amp, bool on);
int tfa_set_mode(struct tfa_t *amp, int mode);
int tfa_set_mute(struct tfa_t *amp, bool on);
void tfa_destroy(struct tfa_t *amp);
