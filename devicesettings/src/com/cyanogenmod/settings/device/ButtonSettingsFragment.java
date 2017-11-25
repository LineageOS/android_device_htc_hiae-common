/*
 * Copyright (C) 2017 The LineageOS Project
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

package com.cyanogenmod.settings.device;

import android.os.Bundle;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.support.v14.preference.PreferenceFragment;
import android.util.Log;

public class ButtonSettingsFragment extends PreferenceFragment
        implements SharedPreferences.OnSharedPreferenceChangeListener {

    private static final String TAG = ButtonSettingsFragment.class.getSimpleName();

    private final boolean DEBUG = false;

    private static final String FP_HOME_KEY = "fp_home";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        addPreferencesFromResource(R.xml.button_panel);

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
        doFpHomePreferenceChange(prefs);
        prefs.registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPrefs, String key) {
        if (FP_HOME_KEY.equals(key)) {
            doFpHomePreferenceChange(sharedPrefs);
        }
    }

    private void doFpHomePreferenceChange(SharedPreferences sharedPrefs) {
        boolean enabled = sharedPrefs.getBoolean(FP_HOME_KEY, false);
        broadcastFpEnabled(enabled);
    }

    private void broadcastFpEnabled(boolean enabled) {
        final Intent intent = new Intent(Constants.FP_HOME_INTENT);
        intent.putExtra(Constants.FP_HOME_INTENT_ENABLED, enabled);
        getContext().sendBroadcastAsUser(intent, UserHandle.CURRENT);
    }
}
