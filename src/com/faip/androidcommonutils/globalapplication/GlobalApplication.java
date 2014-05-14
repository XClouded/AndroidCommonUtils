package com.faip.androidcommonutils.globalapplication;

import android.app.Activity;
import android.app.Application;
import android.os.Handler;
import android.os.StrictMode;
import android.util.DisplayMetrics;
import android.view.Display;

import com.crashlytics.android.Crashlytics;
import com.faip.androidcommonutils.BuildConfig;
import com.faip.androidcommonutils.debugutils.StrictModeUtils;

/**
 * Global singleton applicatioin subclass.
 */
public final class GlobalApplication extends Application {

	// Singleton.
	private static GlobalApplication mGlobalApplication = null;

	// Record the current running activity.
	private Activity activity = null;

	private Activity currentRunningActivity = null;

	private DisplayMetrics displayMetrics = null;

	private Handler handler = new Handler();

	@Override
	public void onCreate() {
		super.onCreate();
		mGlobalApplication = this;
		Crashlytics.start(this);

		if (BuildConfig.DEBUG) {
			// Remeber to set class instance limit inside this api.
			StrictModeUtils.enableStrictMode();
		}
	}

	public static GlobalApplication getInstance() {
		return mGlobalApplication;
	}

	public Handler getUIHandler() {
		return handler;
	}

	public DisplayMetrics getDisplayMetrics() {
		if (displayMetrics != null) {
			return displayMetrics;
		} else {
			Activity a = getActivity();
			if (a != null) {
				Display display = getActivity().getWindowManager().getDefaultDisplay();
				DisplayMetrics metrics = new DisplayMetrics();
				display.getMetrics(metrics);
				this.displayMetrics = metrics;
				return metrics;
			} else {
				// Default screen is 800x480
				DisplayMetrics metrics = new DisplayMetrics();
				metrics.widthPixels = 480;
				metrics.heightPixels = 800;
				return metrics;
			}
		}
	}

	public Activity getActivity() {
		return activity;
	}

	public void setActivity(Activity activity) {
		this.activity = activity;
	}

	public Activity getCurrentRunningActivity() {
		return currentRunningActivity;
	}

	public void setCurrentRunningActivity(Activity currentRunningActivity) {
		this.currentRunningActivity = currentRunningActivity;
	}

}
