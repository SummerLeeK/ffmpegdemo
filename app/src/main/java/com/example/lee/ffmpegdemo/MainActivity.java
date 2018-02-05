package com.example.lee.ffmpegdemo;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private String filename = "GALA.mp4";
    private static String name = "yyy";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        String filepath = Environment.getExternalStorageDirectory() + "/" + filename;

        String output = Environment.getExternalStorageDirectory() + "/" + "ffmpeg.mp4";

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);

        CPrintMessage();
        tv.setText(test(filepath));
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    //
    public native int Transfrom(String filepath, String output);

    public native String test(String filepath);

    public static native void CPrintMessage();

    public static void printMessage(String msg) {
        Log.e("ffmpeg", msg);

        Log.e("ffmpeg", name);
    }
}
