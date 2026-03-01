/*
 * APPActivity.java
 *
 * Extension to SDLActivity supporting the extensions in SDL_App.
 *
 * There used to be a lot more here, but SDL3 solved the safe area problems.
 * Now we just need to deal with large-screen behavior on Android 15+.
 *
 * @author Walker M. White
 * @date 11/22/25
 */
package org.libsdl.app;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.graphics.Point;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.DisplayCutout;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;
import android.content.res.Resources;
import android.graphics.Rect;
import android.provider.Settings;
import android.provider.Settings.Secure;
import android.view.WindowMetrics;


/**
 * An extension to SDLActivity supporting orientation and insets
 *
 * SDL_app relies on exposing a few extra features on mobile devices. Most of
 * of these features are implemented in C. But on Android they require
 * us to add a few new methods to the activity. That is the purpose of this
 * class.
 */
@SuppressWarnings("deprecation")
public class APPActivity extends SDLActivity {
    /** Object to monitor the device orientation */
    protected static DeviceOrientation mDeviceOrientation;
    /** Object to monitor the display orientation */
    protected static DisplayOrientation mDisplayOrientation;
    /** Whether or not a cutout exists */
    protected static boolean hasCutout;
    
    /**
     * Initializes this activity.
     *
     * This method just calles the initialization for the subclass and then 
     * gives default values to our new attributes.
     */
    public static void initialize() {
        SDLActivity.initialize();
        mDeviceOrientation = null;
        mDisplayOrientation = null;
    }

    /**
     * This method is called by SDL before loading the native shared libraries.
     * It can be overridden to provide names of shared libraries to be loaded.
     * The default implementation returns the defaults. It never returns null.
     * Note that SDL_app compiles SDL and its extensions statically, and makes
     * main the only shared library.
     */
    @Override
    protected String[] getLibraries() {
        return new String[] {
            "main"
        };
    }

    /**
     * Called upon activity creation
     *
     * In addition to the subclass onCreate, this method determines the initial
     * device orientation.
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mDeviceOrientation  = new DeviceOrientation(this);
        mDisplayOrientation = new DisplayOrientation(this);
        
        // Support for older phones
        mSurface.post(() -> { computeSafeInsets(); });
    }

    /**
     * Called when we need to cleanly pause the native thread
     */
    @Override
    protected void pauseNativeThread() {
        // NOTE: This method is hard overridden because the final line is not super() compatible.
        mNextNativeState = NativeState.PAUSED;
        mIsResumedCalled = false;

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        mDeviceOrientation.stop();
        mDisplayOrientation.stop();
        SDLActivity.handleNativeState();
    }

    /**
     * Called when we need to resume the native thread
     */
    @Override
    protected void resumeNativeThread() {
        // NOTE: This method is hard overridden because the final line is not super() compatible.
        mNextNativeState = NativeState.RESUMED;
        mIsResumedCalled = true;

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        mDeviceOrientation.start();
        mDisplayOrientation.start();
        SDLActivity.handleNativeState();
    }

    /**
     * Invokes the initial insets computation
     */
    @Override
    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        computeSafeInsets();
    }

    /**
     * Recomputes the insets when the orientation changes
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mDisplayOrientation.update();
        computeSafeInsets();
    }
    
    /**
     * Returns the surface for this activity
     *
     * @return the surface for this activity
     */
    public SDLSurface getSurface() {
        return mSurface;
    }

    /**
     * Computes the the insets for notched devices
     */
    @SuppressWarnings("deprecation")
    public void computeSafeInsets() {
        if (Build.VERSION.SDK_INT >= 30 /* Android 11 (R) */) {
            // Rely on the SDL3 features
            android.view.WindowInsets insets = getWindow().getDecorView().getRootWindowInsets();
            DisplayCutout cutout = insets.getDisplayCutout();
            hasCutout = (cutout != null);
            return;
        }
        
        DisplayMetrics vmetrics = new DisplayMetrics();
        DisplayMetrics rmetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(vmetrics);
        getWindowManager().getDefaultDisplay().getMetrics(rmetrics);
        Rect safeInsets = new Rect(0,0,0,0);

        if (Build.VERSION.SDK_INT >= 28) {
            android.view.WindowInsets insets = getWindow().getDecorView().getRootWindowInsets();
            if (insets != null) {
                DisplayCutout cutout = insets.getDisplayCutout();
                if (cutout != null) {
                    safeInsets.left  = Math.max(insets.getStableInsetLeft(),cutout.getSafeInsetLeft());
                    safeInsets.right = Math.max(insets.getStableInsetRight(),cutout.getSafeInsetRight());
                    safeInsets.top   = Math.max(insets.getStableInsetTop(),cutout.getSafeInsetTop());
                    safeInsets.bottom = Math.max(insets.getStableInsetBottom(),cutout.getSafeInsetBottom());
                    hasCutout = true;
                } else {
                    safeInsets.left  = insets.getStableInsetLeft();
                    safeInsets.right = insets.getStableInsetRight();
                    safeInsets.top   = insets.getStableInsetTop();
                    safeInsets.bottom = insets.getStableInsetBottom();
                    hasCutout = false;
                }
            }
        } else {
            int statusBar = 0;
            int resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android");
            if (resourceId > 0) {
                statusBar = getResources().getDimensionPixelSize(resourceId);
            }
            safeInsets.top += statusBar;
            hasCutout = statusBar > convertDpToPixel(24);
        }

        int remainW = (rmetrics.widthPixels-vmetrics.widthPixels)- safeInsets.left-safeInsets.right;
        int remainH = (rmetrics.heightPixels-vmetrics.heightPixels)- safeInsets.top-safeInsets.bottom;

        if (remainW > 0) {
            safeInsets.right -= remainW;
        }
        if (remainH > 0) {
            safeInsets.bottom -= remainH;
        }
        
        // Send to SDL3
        SDLActivity.onNativeInsetsChanged(safeInsets.left, safeInsets.right, safeInsets.top, safeInsets.bottom);
    }
    
    /**
     * Returns the number of pixels for the given device independent points
     *
     * @param dp	Measurement in device independent points
     *
     * @return he number of pixels for the given device independent points
     */
    public static int convertDpToPixel (float dp){
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float px = dp * (metrics.densityDpi / 160f);
        return Math.round(px);
    }
    
    /**
     * Returns true if the x-y axes on the accelerometer are swapped
     *
     * @return true if the x-y axes on the accelerometer are swapped
     */
    public static boolean isXYSwapped() {
        return mDeviceOrientation.isSwapXY();
    }
    
    /**
     * Returns true if this device is notched
     *
     * @return true if this device is notched
     */
    public static boolean hasNotch() {
        return hasCutout;
    }
    
    /**
     * Returns the current device orientation
     *
     * @return the current device orientation
     */
    public static int getDeviceOrientation() {
        return mDeviceOrientation.getOrientation() ;
    }

    /**
     * Returns the current configuration orientation
     *
     * @return the current configuration orientation
     */
    public static int getConfigOrientation() {
        return mDisplayOrientation.getConfigOrientation();
    }

    /**
     * Returns the current window orientation
     *
     * @return the current window orientation
     */
    public static int getWindowOrientation() {
        return mDisplayOrientation.getWindowOrientation();
    }
    
    /**
     * Returns the name of this device
     *
     * This value is identical to Build.DEVICE
     *
     * @return the name of this device
     */
    public static String getDeviceName() {
        // https://stackoverflow.com/questions/16704597/how-do-you-get-the-user-defined-device-name-in-android
        String deviceName = Settings.Global.getString(getContext().getContentResolver(), Settings.Global.DEVICE_NAME);
        if(deviceName == null) {
            deviceName = Settings.Secure.getString(getContext().getContentResolver(), "bluetooth_name");
        }
        return deviceName == null ? Build.DEVICE : deviceName;
    }

    /**
     * Returns the model of this device
     *
     * This value is typically Build.MODEL, unless it is missing manufacturer 
     * information. In that case, we append the manufacturer to Build.MODEL.
     *
     * @return the model of this device
     */
    public static String getDeviceModel() {
        String manufacturer = Build.MANUFACTURER;
        String model = Build.MODEL;
        if (model.startsWith(manufacturer)) {
            return model.substring(0,1).toUpperCase() + model.substring(1).toLowerCase();
        } else {
            return (manufacturer.substring(0,1).toUpperCase() + manufacturer.substring(1).toLowerCase() +
                    " " + model);
        }
    }

    /**
     * Returns the OS version of this device
     *
     * This value is the version number (we do not use Android code names) 
     * together with the date of the latest patch.
     *
     * @return the OS version of this device
     */
    public static String getDeviceOSVersion() {
        return Build.VERSION.RELEASE + " ("+Build.VERSION.SECURITY_PATCH+")";
    }

    /**
     * Returns the Android ID of this device
     *
     * This is a unique identifier for the device that does not require special 
     * privileges (like the serial number does). While this number can be 
     * reassigned, it is generally stable enough to use for app tracking.
     *
     * @return the Android ID of this device
     */
    public static String getDeviceID() {
        return Secure.getString(getContext().getContentResolver(), Secure.ANDROID_ID);
    }
}