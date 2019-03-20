package com.sample.render;

import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import com.sample.render.ffmpegp.VideoUtils;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    SurfaceHolder holder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        surfaceView = findViewById(R.id.sufaceView);
        holder = surfaceView.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);

    }


    public void start(View view) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                VideoUtils.render(new File(Environment.getExternalStorageDirectory().getPath(),"input.mp4").getAbsolutePath(),holder.getSurface());
            }
        }).start();

    }
}
