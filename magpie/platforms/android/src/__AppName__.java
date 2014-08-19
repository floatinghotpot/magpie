package com.handywit.__AppName__;

import com.handywit.magpie.MagpieActivity;

import org.cocos2dx.lib.Cocos2dxGLSurfaceView;

import android.os.Bundle;
import android.util.Log;

public class __AppName__ extends MagpieActivity {
	
    public Cocos2dxGLSurfaceView onCreateView() {
    	Cocos2dxGLSurfaceView glSurfaceView = new Cocos2dxGLSurfaceView(this);
    	
    	// cdvcxx should create stencil buffer
    	glSurfaceView.setEGLConfigChooser(5, 6, 5, 0, 16, 8);
    	
    	return glSurfaceView;
    }

    static {
        System.loadLibrary("cocos2dcpp");
    }     
}
