#
# Copyright 2016 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This contains the module build definitions for the hardware-specific
# components for this device.
#
# As much as possible, those components should be built unconditionally,
# with device-specific names to avoid collisions, to avoid device-specific
# bitrot and build breakages. Building a component unconditionally does
# *not* include it on all devices, so it is safe even with hardware-specific
# components.

LOCAL_PATH := $(call my-dir)

ifneq ($(filter hiaeul hiaeuhl hiaewhl,$(TARGET_DEVICE)),)

include $(call all-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

ADSP_IMAGES := \
    adsp.b00 adsp.b01 adsp.b02 adsp.b03 adsp.b04 adsp.b05 adsp.b06 adsp.b07 \
    adsp.b08 adsp.b09 adsp.b10 adsp.b11 adsp.b12 adsp.b13 adsp.b14 adsp.b15 \
    adsp.mdt adpver.cfg

ADSP_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(ADSP_IMAGES)))
$(ADSP_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "ADSP firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/adsp/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(ADSP_SYMLINKS)

KEYMASTER_IMAGES := \
    keymaste.b00 keymaste.b01 keymaste.b02 keymaste.b03 keymaste.mdt

KEYMASTER_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(KEYMASTER_IMAGES)))
$(KEYMASTER_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Keymaster firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /etc/firmware/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(KEYMASTER_SYMLINKS)

MBA_IMAGES := \
    mba.b00 mba.b01 mba.b02 mba.b03 mba.b04 mba.b05 mba.mbn mba.mdt

MBA_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(MBA_IMAGES)))
$(MBA_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "MBA firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /vendor/firmware/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MBA_SYMLINKS)

MISC_IMAGES := \
    qdsp6m.qdb radiover.cfg version.cfg

MISC_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(MISC_IMAGES)))
$(MISC_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Misc firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /vendor/firmware/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MISC_SYMLINKS)

MODEM_IMAGES := \
    modem.b00 modem.b01 modem.b02 modem.b04 modem.b05 modem.b06 \
    modem.b07 modem.b08 modem.b09 modem.b10 modem.b11 modem.b12 \
    modem.b13 modem.b16 modem.b17 modem.b18 modem.b19 modem.b20 \
    modem.mdt

MODEM_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(MODEM_IMAGES)))
$(MODEM_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Modem firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /vendor/firmware/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(MODEM_SYMLINKS)

WCNSS_IMAGES := \
    wcnss.b00 wcnss.b01 wcnss.b02 wcnss.b03 wcnss.b04 wcnss.b05 wcnss.b06 \
    wcnss.b07 wcnss.b08 wcnss.b09 wcnss.b10 wcnss.b11 wcnss.b12 wcnss.mdt \
    wcnssver.cfg

WCNSS_SYMLINKS := $(addprefix $(TARGET_ROOT_OUT)/firmware/image/,$(notdir $(WCNSS_IMAGES)))
$(WCNSS_SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "WCNSS firmware link: $@"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf /firmware/wcnss/$(notdir $@) $@

ALL_DEFAULT_INSTALLED_MODULES += $(WCNSS_SYMLINKS)

# Create a link for the WCNSS config file, which ends up as a writable
# # version in /data/misc/wifi
$(shell mkdir -p $(TARGET_OUT)/etc/firmware/wlan/prima; \
    ln -sf /data/misc/wifi/WCNSS_qcom_cfg.ini \
	    $(TARGET_OUT)/etc/firmware/wlan/prima/WCNSS_qcom_cfg.ini)

endif
