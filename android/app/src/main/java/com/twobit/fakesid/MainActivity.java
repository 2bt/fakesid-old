package com.twobit.fakesid;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

import org.libsdl.app.SDLActivity;

public class MainActivity extends SDLActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWritePermission();
    }

    static final int PERMISSION_REQUEST_WRITE_EXTERNAL_STORAGE = 42;

    boolean mWritePermission = false;

    public void getWritePermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            mWritePermission = true;
        }
        else {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    PERMISSION_REQUEST_WRITE_EXTERNAL_STORAGE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        if (requestCode != PERMISSION_REQUEST_WRITE_EXTERNAL_STORAGE) return;
        if (grantResults.length < 1) return;
        if (grantResults[0] == PackageManager.PERMISSION_GRANTED) mWritePermission = true;
    }

}
