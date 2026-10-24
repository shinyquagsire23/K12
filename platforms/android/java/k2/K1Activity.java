// KOTOR 1 (K2P_GAME=k1): K2Activity with a different native entry. K1's engine defines SDL_main
// itself (src/port/Mac/MacMain.cpp: on the device it is the app), so SDL calls k1_main instead
// (android/main_android.cpp), which mounts the expansion files the way Aspyr's Java did and then
// enters the engine's SDL_main.
package k2;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.DisplayMetrics;

public class K1Activity extends K2Activity {
    // SDL_main reads DisplayMetrics.xdpi / ydpi for its tap and drag tolerances; the stub JNIEnv
    // (compat/android_compat.cpp) answers fields from K2_JNI_<field>.
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        DisplayMetrics dm = getResources().getDisplayMetrics();
        try {
            Os.setenv("K2_JNI_xdpi", Float.toString(dm.xdpi), false);
            Os.setenv("K2_JNI_ydpi", Float.toString(dm.ydpi), false);
        } catch (ErrnoException e) {
        }
        super.onCreate(savedInstanceState);
    }

    @Override
    protected String getMainFunction() {
        return "k1_main";
    }
}
