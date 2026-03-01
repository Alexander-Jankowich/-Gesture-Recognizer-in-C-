/*
 * DeviceOrientation.java
 *
 *  A class to detect the display orientation.
 *
 * @author Walker M. White
 * @date 11/22/25
 */
package org.libsdl.app;

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.Point;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.content.Context;
import android.os.Build;
import android.view.Display;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.view.WindowManager;
import android.view.WindowMetrics;

@SuppressWarnings( "deprecation" )
/**
 * A class to detect the display orientation.
 *
 * While SDL3 provides support for detecting the display orientation, it uses
 * an old understanding of orientation that is incompatible with Android 15+
 * large-screen behavior. This class separates window and configuration
 * orientation to rectify this problem.
 *
 * The most important thing to understand about large-screen behavior is that
 * configuration orientation and display orientation are now decoupled. It is
 * possible to have a letter-boxed portrait display in a landscape 
 * configuration, and vice-versa.
 */
public class DisplayOrientation {
    /** The SDLActivity for the display */
    protected SDLActivity mActivity;
    /** The orientation of the configuration */
    protected int  mConfigOrientation;
    /** The orientation of the window */
    protected int  mWindowOrientation;
    /** The current device rotation */
    protected int mDeviceRotation;
    /** A listener to react to device updates */
    private OrientationEventListener orientationListener;
    /** Whether this has been paused */
    private boolean paused = true;

    /**
     * Creates a new DisplayOrientation for the given activity
     *
     * @param activity	The application activity
     */
    public DisplayOrientation(SDLActivity activity) {
        mActivity = activity;
        mDeviceRotation = -1;
        mConfigOrientation = -1;
        mWindowOrientation = -1;

		// Respond to orientation changed
        orientationListener = new OrientationEventListener(activity) {
        	/**
        	 * Updates the device rotation
        	 *
        	 * @param orientation	the current orientation
        	 */
            @Override
            public void onOrientationChanged(int orientation) {
                if (orientation == ORIENTATION_UNKNOWN) {
                    return; // flat or sensor unhappy
                }

                int newRotation;
                if (orientation >= 315 || orientation < 45) {
                    newRotation = Surface.ROTATION_0;
                } else if (orientation >= 45 && orientation < 135) {
                    newRotation = Surface.ROTATION_90;
                } else if (orientation >= 135 && orientation < 225) {
                    newRotation = Surface.ROTATION_180;
                } else {
                    newRotation = Surface.ROTATION_270;
                }

                if (newRotation != mDeviceRotation) {
                    mDeviceRotation = newRotation;
					updateWindowOrientation();
                }
            }
        };

        start();
    }

   	/**
     * Activates the display orientation manager
     */
    public void start() {
        if (paused) {
            if (orientationListener != null && orientationListener.canDetectOrientation()) {
                orientationListener.enable();
            }
            update();
            paused = false;
        }
    }

   	/**
     * Deactivates the display orientation manager
     */
    public void stop() {
        if (!paused) {
            if (orientationListener != null) {
                orientationListener.disable();
            }
            paused = true;
        }
    }

	/**
	 * Returns the configuration orientation.
	 *
	 * This method does not update the configuration orientation. Call 
	 * {@link #updateConfigOrientation} to make sure this is the most recent
	 * orientation value.
	 *
	 * Note that a configuration orientation is always one of 
	 * SDL_ORIENTATION_LANDSCAPE or SDL_ORIENTATION_PORTRAIT. The configuration
	 * is never unknown and never flipped.
	 *
	 * @return the configuration orientation.
	 */
    public int getConfigOrientation() {
        return mConfigOrientation;
    }

	/**
	 * Returns the window orientation.
	 *
	 * This method does not update the window orientation. Call 
	 * {@link #updateWindowOrientation} to make sure this is the most recent
	 * orientation value.
	 *
	 * @return the window orientation.
	 */
    public int getWindowOrientation() {
        return mWindowOrientation;
    }

	/**
	 * Updates both the configuration and window orientation
	 *
	 * @return true if either orientation changed in the update
	 */
    public boolean update() {
        boolean changed = updateConfigOrientation();
        return updateWindowOrientation() || changed;
    }
    
    /**
     * Updates the configuration orientation
	 *
	 * @return true if the orientation changed in the update
	 */
    public boolean updateConfigOrientation() {
        int result = SDLActivity.SDL_ORIENTATION_UNKNOWN;

        Configuration config = mActivity.getResources().getConfiguration();
        if (config != null && config.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            result = SDLActivity.SDL_ORIENTATION_LANDSCAPE;
        } else if (config != null) {
        	// Just assume portrait if square or unknown
            result = SDLActivity.SDL_ORIENTATION_PORTRAIT;
        }

        boolean changed = mConfigOrientation != result;
        mConfigOrientation = result;
        if (changed) {
			nativeSetConfigOrientation(mConfigOrientation);
        }
        return changed;
    }

    /**
     * Updates the window orientation
	 *
	 * @return true if the orientation changed in the update
	 */
    public boolean updateWindowOrientation() {
        SDLSurface surface = ((APPActivity)mActivity).getSurface();
        int width = surface.getWidth();
        int height = surface.getHeight();

        if (width <= 0 || height <= 0) {
            // Fallback: query window metrics if view not laid out yet
            WindowManager wm = (WindowManager) mActivity.getSystemService(Context.WINDOW_SERVICE);

            if (Build.VERSION.SDK_INT >= 30) {
                WindowMetrics metrics = wm.getCurrentWindowMetrics();
                Rect b = metrics.getBounds();
                width = b.width();
                height = b.height();
            } else {
                Display d = wm.getDefaultDisplay();
                Point p = new Point();
                d.getSize(p);
                width = p.x;
                height = p.y;
            }
        }

        boolean landscape = width >= height;

        // Get rotation of THIS window, not default display
        int rotation = Surface.ROTATION_0;
        Display display = surface.getDisplay();
        if (display != null) {
            rotation = display.getRotation();
        }

        int result = SDLActivity.SDL_ORIENTATION_UNKNOWN;
        if (!landscape) {
            // Portrait family
            if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_90) {
                result = SDLActivity.SDL_ORIENTATION_PORTRAIT;
            } else {
                result = SDLActivity.SDL_ORIENTATION_PORTRAIT_FLIPPED;
            }
        } else {
            // Landscape family
            if (rotation == Surface.ROTATION_0 || rotation == Surface.ROTATION_90) {
                result = SDLActivity.SDL_ORIENTATION_LANDSCAPE;
            } else {
                result = SDLActivity.SDL_ORIENTATION_LANDSCAPE_FLIPPED;
            }
        }

        boolean changed = result != mWindowOrientation;
        mWindowOrientation = result;
        if (changed) {
			nativeSetWindowOrientation(mWindowOrientation);
        }
        return changed;
    }
    
	/**
     * Pushes the configuration orientation to SDL3
     *
     * @param orientation	The configuration orientation
     */
	private static native void nativeSetConfigOrientation(int orientation);

	/**
     * Pushes the window orientation to SDL3
     *
     * @param orientation	The window orientation
     */
	private static native void nativeSetWindowOrientation(int orientation);
}
