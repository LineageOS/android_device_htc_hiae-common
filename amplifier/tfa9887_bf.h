/*
 * Copyright (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define BF_VDDS 0x0000 /*!< Power-on-reset flag */
#define BF_PLLS 0x0010 /*!< PLL lock */
#define BF_OTDS 0x0020 /*!< Over Temperature Protection alarm */
#define BF_OVDS 0x0030 /*!< Over Voltage Protection alarm */
#define BF_UVDS 0x0040 /*!< Under Voltage Protection alarm */
#define BF_OCDS 0x0050 /*!< Over Current Protection alarm */
#define BF_CLKS 0x0060 /*!< Clocks stable flag */
#define BF_CLIPS 0x0070 /*!< Amplifier clipping */
#define BF_MTPB 0x0080 /*!< MTP busy */
#define BF_NOCLK 0x0090 /*!< Flag lost clock from clock generation unit */
#define BF_SPKS 0x00a0 /*!< Speaker error flag */
#define BF_ACS 0x00b0 /*!< Cold Start flag */
#define BF_SWS 0x00c0 /*!< Flag Engage */
#define BF_WDS 0x00d0 /*!< Flag watchdog reset */
#define BF_AMPS 0x00e0 /*!< Amplifier is enabled by manager */
#define BF_AREFS 0x00f0 /*!< References are enabled by manager */
#define BF_BATS 0x0109 /*!< Battery voltage readout; 0 .. 5.5 [V] */
#define BF_TEMPS 0x0208 /*!< Temperature readout from the temperature sensor */
#define BF_REV 0x030b /*!< Device type number is B97 */
#define BF_RCV 0x0420 /*!< Enable Receiver Mode */
#define BF_CHS12 0x0431 /*!< Channel Selection TDM input for Coolflux */
#define BF_INPLVL 0x0450 /*!< Input level selection control */
#define BF_CHSA 0x0461 /*!< Input selection for amplifier */
#define BF_I2SDOE 0x04b0 /*!< Enable data output */
#define BF_AUDFS 0x04c3 /*!< Audio sample rate setting */
#define BF_BSSCR 0x0501 /*!< Protection Attack Time */
#define BF_BSST 0x0523 /*!< ProtectionThreshold */
#define BF_BSSRL 0x0561 /*!< Protection Maximum Reduction */
#define BF_BSSRR 0x0582 /*!< Battery Protection Release Time */
#define BF_BSSHY 0x05b1 /*!< Battery Protection Hysteresis */
#define BF_BSSR 0x05e0 /*!< battery voltage for I2C read out only */
#define BF_BSSBY 0x05f0 /*!< bypass clipper battery protection */
#define BF_DPSA 0x0600 /*!< Enable dynamic powerstage activation */
#define BF_CFSM 0x0650 /*!< Soft mute in CoolFlux */
#define BF_BSSS 0x0670 /*!< BatSenseSteepness */
#define BF_VOL 0x0687 /*!< volume control (in CoolFlux) */
#define BF_DCVO 0x0702 /*!< Boost Voltage */
#define BF_DCMCC 0x0733 /*!< Max boost coil current - step of 175 mA */
#define BF_DCIE 0x07a0 /*!< Adaptive boost mode */
#define BF_DCSR 0x07b0 /*!< Soft RampUp/Down mode for DCDC controller */
#define BF_DCPAVG 0x07c0 /*!< ctrl_peak2avg for analog part of DCDC */
#define BF_TROS 0x0800 /*!< Select external temperature also the ext_temp will be put on the temp read out */
#define BF_EXTTS 0x0818 /*!< external temperature setting to be given by host */
#define BF_PWDN 0x0900 /*!< Device Mode */
#define BF_I2CR 0x0910 /*!< I2C Reset */
#define BF_CFE 0x0920 /*!< Enable CoolFlux */
#define BF_AMPE 0x0930 /*!< Enable Amplifier */
#define BF_DCA 0x0940 /*!< EnableBoost */
#define BF_SBSL 0x0950 /*!< Coolflux configured */
#define BF_AMPC 0x0960 /*!< Selection on how Amplifier is enabled */
#define BF_DCDIS 0x0970 /*!< DCDC not connected */
#define BF_PSDR 0x0980 /*!< IDDQ test amplifier */
#define BF_DCCV 0x0991 /*!< Coil Value */
#define BF_CCFD 0x09b0 /*!< Selection CoolFlux Clock */
#define BF_INTPAD 0x09c1 /*!< INT pad configuration control */
#define BF_IPLL 0x09e0 /*!< PLL input reference clock selection */
#define BF_MTPK 0x0b07 /*!< 5Ah 90d To access KEY1_Protected registers (Default for engineering) */
#define BF_CVFDLY 0x0c25 /*!< Fractional delay adjustment between current and voltage sense */
#define BF_TDMPRF 0x1011 /*!< TDM_usecase */
#define BF_TDMEN 0x1030 /*!< TDM interface control */
#define BF_TDMCKINV 0x1040 /*!< TDM clock inversion */
#define BF_TDMFSLN 0x1053 /*!< TDM FS length */
#define BF_TDMFSPOL 0x1090 /*!< TDM FS polarity */
#define BF_TDMSAMSZ 0x10a4 /*!< TDM Sample Size for all tdm sinks/sources */
#define BF_TDMSLOTS 0x1103 /*!< Number of slots */
#define BF_TDMSLLN 0x1144 /*!< Slot length */
#define BF_TDMBRMG 0x1194 /*!< Bits remaining */
#define BF_TDMDDEL 0x11e0 /*!< Data delay */
#define BF_TDMDADJ 0x11f0 /*!< Data adjustment */
#define BF_TDMTXFRM 0x1201 /*!< TXDATA format */
#define BF_TDMUUS0 0x1221 /*!< TXDATA format unused slot sd0 */
#define BF_TDMUUS1 0x1241 /*!< TXDATA format unused slot sd1 */
#define BF_TDMSI0EN 0x1270 /*!< TDM sink0 enable */
#define BF_TDMSI1EN 0x1280 /*!< TDM sink1 enable */
#define BF_TDMSI2EN 0x1290 /*!< TDM sink2 enable */
#define BF_TDMSO0EN 0x12a0 /*!< TDM source0 enable */
#define BF_TDMSO1EN 0x12b0 /*!< TDM source1 enable */
#define BF_TDMSO2EN 0x12c0 /*!< TDM source2 enable */
#define BF_TDMSI0IO 0x12d0 /*!< tdm_sink0_io */
#define BF_TDMSI1IO 0x12e0 /*!< tdm_sink1_io */
#define BF_TDMSI2IO 0x12f0 /*!< tdm_sink2_io */
#define BF_TDMSO0IO 0x1300 /*!< tdm_source0_io */
#define BF_TDMSO1IO 0x1310 /*!< tdm_source1_io */
#define BF_TDMSO2IO 0x1320 /*!< tdm_source2_io */
#define BF_TDMSI0SL 0x1333 /*!< sink0_slot [GAIN IN] */
#define BF_TDMSI1SL 0x1373 /*!< sink1_slot [CH1 IN] */
#define BF_TDMSI2SL 0x13b3 /*!< sink2_slot [CH2 IN] */
#define BF_TDMSO0SL 0x1403 /*!< source0_slot [GAIN OUT] */
#define BF_TDMSO1SL 0x1443 /*!< source1_slot [Voltage Sense] */
#define BF_TDMSO2SL 0x1483 /*!< source2_slot [Current Sense] */
#define BF_NBCK 0x14c3 /*!< NBCK */
#define BF_INTOVDDS 0x2000 /*!< flag_por_int_out */
#define BF_INTOPLLS 0x2010 /*!< flag_pll_lock_int_out */
#define BF_INTOOTDS 0x2020 /*!< flag_otpok_int_out */
#define BF_INTOOVDS 0x2030 /*!< flag_ovpok_int_out */
#define BF_INTOUVDS 0x2040 /*!< flag_uvpok_int_out */
#define BF_INTOOCDS 0x2050 /*!< flag_ocp_alarm_int_out */
#define BF_INTOCLKS 0x2060 /*!< flag_clocks_stable_int_out */
#define BF_INTOCLIPS 0x2070 /*!< flag_clip_int_out */
#define BF_INTOMTPB 0x2080 /*!< mtp_busy_int_out */
#define BF_INTONOCLK 0x2090 /*!< flag_lost_clk_int_out */
#define BF_INTOSPKS 0x20a0 /*!< flag_cf_speakererror_int_out */
#define BF_INTOACS 0x20b0 /*!< flag_cold_started_int_out */
#define BF_INTOSWS 0x20c0 /*!< flag_engage_int_out */
#define BF_INTOWDS 0x20d0 /*!< flag_watchdog_reset_int_out */
#define BF_INTOAMPS 0x20e0 /*!< flag_enbl_amp_int_out */
#define BF_INTOAREFS 0x20f0 /*!< flag_enbl_ref_int_out */
#define BF_INTOACK 0x2201 /*!< Interrupt status register output - Corresponding flag */
#define BF_INTIVDDS 0x2300 /*!< flag_por_int_in */
#define BF_INTIPLLS 0x2310 /*!< flag_pll_lock_int_in */
#define BF_INTIOTDS 0x2320 /*!< flag_otpok_int_in */
#define BF_INTIOVDS 0x2330 /*!< flag_ovpok_int_in */
#define BF_INTIUVDS 0x2340 /*!< flag_uvpok_int_in */
#define BF_INTIOCDS 0x2350 /*!< flag_ocp_alarm_int_in */
#define BF_INTICLKS 0x2360 /*!< flag_clocks_stable_int_in */
#define BF_INTICLIPS 0x2370 /*!< flag_clip_int_in */
#define BF_INTIMTPB 0x2380 /*!< mtp_busy_int_in */
#define BF_INTINOCLK 0x2390 /*!< flag_lost_clk_int_in */
#define BF_INTISPKS 0x23a0 /*!< flag_cf_speakererror_int_in */
#define BF_INTIACS 0x23b0 /*!< flag_cold_started_int_in */
#define BF_INTISWS 0x23c0 /*!< flag_engage_int_in */
#define BF_INTIWDS 0x23d0 /*!< flag_watchdog_reset_int_in */
#define BF_INTIAMPS 0x23e0 /*!< flag_enbl_amp_int_in */
#define BF_INTIAREFS 0x23f0 /*!< flag_enbl_ref_int_in */
#define BF_INTIACK 0x2501 /*!< Interrupt register input */
#define BF_INTENVDDS 0x2600 /*!< flag_por_int_enable */
#define BF_INTENPLLS 0x2610 /*!< flag_pll_lock_int_enable */
#define BF_INTENOTDS 0x2620 /*!< flag_otpok_int_enable */
#define BF_INTENOVDS 0x2630 /*!< flag_ovpok_int_enable */
#define BF_INTENUVDS 0x2640 /*!< flag_uvpok_int_enable */
#define BF_INTENOCDS 0x2650 /*!< flag_ocp_alarm_int_enable */
#define BF_INTENCLKS 0x2660 /*!< flag_clocks_stable_int_enable */
#define BF_INTENCLIPS 0x2670 /*!< flag_clip_int_enable */
#define BF_INTENMTPB 0x2680 /*!< mtp_busy_int_enable */
#define BF_INTENNOCLK 0x2690 /*!< flag_lost_clk_int_enable */
#define BF_INTENSPKS 0x26a0 /*!< flag_cf_speakererror_int_enable */
#define BF_INTENACS 0x26b0 /*!< flag_cold_started_int_enable */
#define BF_INTENSWS 0x26c0 /*!< flag_engage_int_enable */
#define BF_INTENWDS 0x26d0 /*!< flag_watchdog_reset_int_enable */
#define BF_INTENAMPS 0x26e0 /*!< flag_enbl_amp_int_enable */
#define BF_INTENAREFS 0x26f0 /*!< flag_enbl_ref_int_enable */
#define BF_INTENACK 0x2801 /*!< Interrupt enable register */
#define BF_INTPOLVDDS 0x2900 /*!< flag_por_int_pol */
#define BF_INTPOLPLLS 0x2910 /*!< flag_pll_lock_int_pol */
#define BF_INTPOLOTDS 0x2920 /*!< flag_otpok_int_pol */
#define BF_INTPOLOVDS 0x2930 /*!< flag_ovpok_int_pol */
#define BF_INTPOLUVDS 0x2940 /*!< flag_uvpok_int_pol */
#define BF_INTPOLOCDS 0x2950 /*!< flag_ocp_alarm_int_pol */
#define BF_INTPOLCLKS 0x2960 /*!< flag_clocks_stable_int_pol */
#define BF_INTPOLCLIPS 0x2970 /*!< flag_clip_int_pol */
#define BF_INTPOLMTPB 0x2980 /*!< mtp_busy_int_pol */
#define BF_INTPOLNOCLK 0x2990 /*!< flag_lost_clk_int_pol */
#define BF_INTPOLSPKS 0x29a0 /*!< flag_cf_speakererror_int_pol */
#define BF_INTPOLACS 0x29b0 /*!< flag_cold_started_int_pol */
#define BF_INTPOLSWS 0x29c0 /*!< flag_engage_int_pol */
#define BF_INTPOLWDS 0x29d0 /*!< flag_watchdog_reset_int_pol */
#define BF_INTPOLAMPS 0x29e0 /*!< flag_enbl_amp_int_pol */
#define BF_INTPOLAREFS 0x29f0 /*!< flag_enbl_ref_int_pol */
#define BF_INTPOLACK 0x2b01 /*!< Interrupt status flags polarity register */
#define BF_CLIP 0x4900 /*!< Bypass clip control */
#define BF_CIMTP 0x62b0 /*!< start copying all the data from i2cregs_mtp to mtp [Key 2 protected] */
#define BF_RST 0x7000 /*!< Reset CoolFlux DSP */
#define BF_DMEM 0x7011 /*!< Target memory for access */
#define BF_AIF 0x7030 /*!< Autoincrement-flag for memory-address */
#define BF_CFINT 0x7040 /*!< Interrupt CoolFlux DSP */
#define BF_REQ 0x7087 /*!< request for access (8 channels) */
#define BF_REQCMD 0x7080 /*!< Firmware event request rpc command */
#define BF_REQRST 0x7090 /*!< Firmware event request reset restart */
#define BF_REQMIPS 0x70a0 /*!< Firmware event request short on mips */
#define BF_REQMUTED 0x70b0 /*!< Firmware event request mute sequence ready */
#define BF_REQVOL 0x70c0 /*!< Firmware event request volume ready */
#define BF_REQDMG 0x70d0 /*!< Firmware event request speaker damage detected */
#define BF_REQCAL 0x70e0 /*!< Firmware event request calibration completed */
#define BF_REQRSV 0x70f0 /*!< Firmware event request reserved */
#define BF_MADD 0x710f /*!< memory-address to be accessed */
#define BF_MEMA 0x720f /*!< activate memory access (24- or 32-bits data is written/read to/from memory */
#define BF_ERR 0x7307 /*!< Coolflux error flags */
#define BF_ACK 0x7387 /*!< acknowledge of requests (8 channels) */
#define BF_MTPOTC 0x8000 /*!< Calibration schedule (key2 protected) */
#define BF_MTPEX 0x8010 /*!< (key2 protected) */

/* TFA9887 specific fields */
#define BF_I2SF 0x402 /* I2SFormat data 1 input: */
#define BF_CHS3 0x450 /* ChannelSelection data 2 input (coolflux input the DCDC converter gets the other signal) */
#define BF_I2SSR 0x4c3 /* sample rate setting */
#define BF_TFA9887_BSSBY 0x500 /* */
#define BF_TFA9887_BSSCR 0x511 /* 00 0.56 dB/Sample */
#define BF_TFA9887_BSST 0x532 /* 000 2.92V */
#define BF_I2SDOC 0x5f0 /* selection data out */
#define BF_DOLS 0xa02 /* Output selection dataout left channel */
#define BF_DORS 0xa32 /* Output selection dataout right channel */
#define BF_SPKL 0xa62 /* Selection speaker induction */
#define BF_SPKR 0xa91 /* Selection speaker impedance */
#define BF_DCFG 0xab3 /* DCDC speaker current compensation gain */
#define BF_PWMDEL 0x4134 /* PWM DelayBits to set the delay */
#define BF_PWMSH 0x4180 /* PWM Shape */
#define BF_PWMRE 0x4190 /* PWM Bitlength in noise shaper */
#define BF_TCC 0x48e1 /* sample & hold track time: */

/* SW VSTEP */
#define BF_SWPROFIL 0x8045
#define BF_SWVSTEP 0x80a5
