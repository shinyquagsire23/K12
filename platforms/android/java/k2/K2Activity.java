// The APK's activity is SDL2's own SDLActivity (org.libsdl.app, compiled from the same SDL2 release
// as libSDL2.so) with two additions, both made before SDL loads libmain.so:
//   * K2_OBB_DIR = getObbDir(), where main_android.cpp finds the expansion files;
//   * the `k2env` intent extra, "KEY=VALUE;KEY=VALUE" -> the process environment, so a launch can
//     set any K2_* switch:  adb shell am start -n <package>/k2.K2Activity --es k2env "K2_STUBS=soft"
// Everything else -- the surface, input, audio, controllers -- is SDL's own Java, unmodified.
package k2;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;

import org.libsdl.app.SDLActivity;

public class K2Activity extends SDLActivity {
    private static void setenv(String key, String value) {
        try {
            Os.setenv(key, value, true);
            Log.i("k2", "env " + key + "=" + value);
        } catch (ErrnoException e) {
            Log.w("k2", "setenv " + key + ": " + e);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        if (getObbDir() != null) {
            setenv("K2_OBB_DIR", getObbDir().getAbsolutePath());
        }
        String env = getIntent() != null ? getIntent().getStringExtra("k2env") : null;
        if (env != null) {
            for (String kv : env.split(";")) {
                int eq = kv.indexOf('=');
                if (eq > 0) {
                    setenv(kv.substring(0, eq).trim(), kv.substring(eq + 1));
                }
            }
        }
        super.onCreate(savedInstanceState);
    }

    // main_android.cpp's entry; the engine's SDL_main (MacMain.cpp) is the shipped app's.
    @Override
    protected String getMainFunction() {
        return "k2_main";
    }
}
