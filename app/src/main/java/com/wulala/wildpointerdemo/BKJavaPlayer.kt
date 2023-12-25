package com.wulala.wildpointerdemo

import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent

@Suppress("DEPRECATION")
class BKJavaPlayer : SurfaceHolder.Callback, LifecycleObserver {

    val TAG = "BK Player"
    private var nativePlayerObj: Long? = null // 保存DerryPlayer.cpp对象的地址

    companion object {
        // Used to load the 'wildpointerdemo' library on application startup.
        init {
            System.loadLibrary("wildpointerdemo")
        }
    }

    private var surfaceHolder: SurfaceHolder? = null

    // 媒体源（文件路径， 直播地址rtmp）
    private var dataSource: String? = null
    fun setDataSource(dataSource: String?) {
        this.dataSource = dataSource
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    fun prepare() {
        // 当前Activity处于Resumed状态时调用
        Log.d(TAG, "prepare: ")
        // nativePlayerObj = prepareNative(dataSource!!)
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
    fun stop() {
        Log.d(TAG, "Stop: ")
        nativePlayerObj = null
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        setSurfaceNative(holder.surface, nativePlayerObj!!)

    }

    fun setSurfaceView(surfaceView: SurfaceView) {
        if (surfaceHolder != null) {
            surfaceHolder?.removeCallback(this) // 清除上一次的
        }
        surfaceHolder = surfaceView.holder
        surfaceHolder?.addCallback(this) // 监听
    }
    override fun surfaceDestroyed(holder: SurfaceHolder) {
    }

    private external fun setSurfaceNative(surface: Surface, nativeObj: Long)

}