/*
 * DeviceOrientation.java
 *
 * A class to measure physical device orientation.
 *
 * @author Walker M. White
 * @date 11/22/25
 */
package org.libsdl.app;

import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.content.Context;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.*;

@SuppressWarnings( "deprecation" )
/**
 * A class to measure physical device orientation.
 *
 * Device orientation differs from display orientation in that it indicates how 
 * the device is being held, not the orientation of the screen. It is inferred 
 * from either the accelerometer or the gravity sensor.
 *
 * Note that accelerometer results are relative to the home button, as that is 
 * the "natural" position of the device. For the vast majority of devices, this 
 * means that portrait mode is the default device orientation. However, older 
 * Samsung tablets actually have the home button in landscape mode, and this can 
 * affect the results. For legacy devices this class uses the display to detect
 * the natural orientation. Note that this is impossible on modern Android
 * devices as there is no natural orientation, only the initial orientation.
 */
public class DeviceOrientation implements SensorEventListener {
	/** The sensor manager to respond to orientation changes */
	private final SensorManager sensorManager;
	/** The specific sensor (accelerometer or gravity) to use */
    private final Sensor accelOrGravity;
    /** The current device orientation */
    private int orientation = SDLActivity.SDL_ORIENTATION_UNKNOWN;
    /** Whether to swap the axes */
    private boolean swapXY = false;
    /** Whether this has been paused */
    private boolean paused = true;

    /**
     * Creates a new DeviceOrientation for the given context
     *
     * @param context	The application context
     */
    public DeviceOrientation(Context context) {
        sensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);

        // Prefer GRAVITY sensor if available; else fall back to raw accelerometer
        Sensor gravity = sensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
        if (gravity != null) {
            accelOrGravity = gravity;
        } else {
            accelOrGravity = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }
        orientation = SDLActivity.SDL_ORIENTATION_UNKNOWN;
        swapXY = shouldSwapXY(context);
        start();
    }

    /**
     * Returns true if this is a legacy tablet with x/y axes swapped.
     *
     * Old Samsungs like the Galaxy Tab S were designed to be used in landscape 
     * mode; the home button was arranged along that axes. As such, the 
     * designers also decided to swap the accelerometer axes so that holding 
     * the device in landscape is the same as holding any other device in 
     * portrait. This causes the results of this class to be reversed, and in
     * general plays havok with "game-like" controls.
     *
     * Note that this only holds for old devices. Newer devices like the Lenovo 
     * Yoga Tab are also designed to be used in landscape, but they do not swap 
     * the axes. This allows us to use the display API to query the natural 
     * orientation. This is important, because this code does not work on 
     * Android 36+ large screens (when Android no longer recognizes orientation
     * lock).
     *
     * @param context	The application context
     *
     * @return true if this is a legacy tablet with x/y axes swapped.
     */
    private static boolean shouldSwapXY(Context context) {
        // Limit this to older Android versions where the old display APIs behave predictably.
        // SM-T800 is typically around L/M and this is where we have seen it.
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.S) {
            return false;
        }

        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        if (wm == null) {
            return false;
        }
        Display display = wm.getDefaultDisplay();
        if (display == null) {
            return false;
        }

        int rotation = display.getRotation();
        int cfgOrient = context.getResources().getConfiguration().orientation;
        if (cfgOrient != Configuration.ORIENTATION_PORTRAIT && cfgOrient != Configuration.ORIENTATION_LANDSCAPE) {
            return false;
        }

        // Infer natural orientation using the classic pre-large-screen logic:
        boolean naturalPortrait;
        switch (rotation) {
            case Surface.ROTATION_0:
            case Surface.ROTATION_180:
                naturalPortrait = (cfgOrient == Configuration.ORIENTATION_PORTRAIT);
                break;
            case Surface.ROTATION_90:
            case Surface.ROTATION_270:
                naturalPortrait = (cfgOrient == Configuration.ORIENTATION_LANDSCAPE);
                break;
            default:
                naturalPortrait = true;
                break;
        }

        // If the device is landscape-natural, we flip how we interpret X/Y
        return !naturalPortrait;
    }

    /**
     * Returns true if the accelerometer axes are swapped.
     *
     * @return true if the accelerometer axes are swapped.
     */
    public boolean isSwapXY() {
        return swapXY;
    }

    /**
     * Activates the device orientation manager
     */
    public void start() {
    	if (paused) {
			if (accelOrGravity != null) {
				sensorManager.registerListener(this, accelOrGravity, SensorManager.SENSOR_DELAY_UI);
			}
			paused = false;
		}
    }

    /**
     * Deactivates the device orientation manager
     */
    public void stop() {
    	if (!paused) {
			if (accelOrGravity != null) {
				sensorManager.unregisterListener(this);
			}
			paused = true;
		}
    }

    /**
     * Returns the current device orientation.
     *
     * This value returned is a constant defined in SDLActivity
     *
     * @return the current device orientation.
     */
    public int getOrientation() {
        return orientation;
    }

	/** 
	 * Responds to a change in sensor measurements
	 *
	 * @param event	The sensor event
  	 */
    @Override
    public void onSensorChanged(SensorEvent event) {
        float px = event.values[0];
        float py = event.values[1];
        float z = event.values[2];

        // Swap axes on old tablets
        float x = swapXY ? py : px;
        float y = swapXY ? px : py;

        float g = (float)Math.sqrt(x*x + y*y + z*z);
        if (g < 7.0f || g > 12.0f) {
            // Very noisy / unreliable reading, ignore
            return;
        }

        float xy = (float)Math.sqrt(x*x + y*y);
        float planarRatio = xy / g;

        // Classify only if enough of gravity is in the screen plane
        // Value of 0.6-0.7 supports up to a 45° tilt
        final float MIN_PLANAR_RATIO = 0.6f;
        if (planarRatio < MIN_PLANAR_RATIO) {
            return;
        }

        // Hysteresis to avoid jitter when |x| ~ |y|
        final float hysteresis = 1.0f;
        
        int current = SDLActivity.SDL_ORIENTATION_UNKNOWN;
        if (Math.abs(x) > Math.abs(y) + hysteresis) {
            if (x > 0) {
                current = SDLActivity.SDL_ORIENTATION_LANDSCAPE;
            } else {
                current = SDLActivity.SDL_ORIENTATION_LANDSCAPE_FLIPPED;
            }
        } else if (Math.abs(y) > Math.abs(x) + hysteresis) {
            if (y > 0) {
                current = SDLActivity.SDL_ORIENTATION_PORTRAIT;
            } else {
                current = SDLActivity.SDL_ORIENTATION_PORTRAIT_FLIPPED;
            }
        } else {
            // In the “diagonal” zone; don’t change orientation
            return;
        }

        if (current != orientation) {
            orientation = current;
            nativeSetDeviceOrientation(orientation);
		}
    }

	/** 
	 * Responds to a change in accuracy 
	 *
	 * @param sensor	The sensor event
	 * @param accuracy	The new accuracy
	 */
    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }
    
    /**
     * Pushes the orientation to SDL3
     *
     * @param orientation	The device orientation
     */
	private static native void nativeSetDeviceOrientation(int orientation);

}