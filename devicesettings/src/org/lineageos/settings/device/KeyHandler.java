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

package org.lineageos.settings.device;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;

import com.android.internal.os.DeviceKeyHandler;

public class KeyHandler implements DeviceKeyHandler {

    private static final String TAG = KeyHandler.class.getSimpleName();

    private final boolean DEBUG = false;

    private final Context mContext;
    private boolean mFpHomeEnabled = false;

    public KeyHandler(Context context) {
        mContext = context;

        IntentFilter fpHomeFilter = new IntentFilter(Constants.FP_HOME_INTENT);
        mContext.registerReceiver(mFpHomeReceiver, fpHomeFilter);
    }

    @Override
    public KeyEvent handleKeyEvent(KeyEvent event) {
        if (!hasSetupCompleted()) {
            return event;
        }

        if (event.getKeyCode() == KeyEvent.KEYCODE_HOME && event.getScanCode() == 143) {
            /* Consume the home keypress if not enabled */
            return !mFpHomeEnabled ? null : event;
        }

        return event;
    }

    private boolean hasSetupCompleted() {
        return Settings.Secure.getInt(mContext.getContentResolver(),
                Settings.Secure.USER_SETUP_COMPLETE, 0) != 0;
    }

    private BroadcastReceiver mFpHomeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mFpHomeEnabled = intent.getBooleanExtra(Constants.FP_HOME_INTENT_ENABLED, false);
        }
    };
}
