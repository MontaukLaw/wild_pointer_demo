package com.wulala.wildpointerdemo

import android.Manifest
import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.graphics.Color
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.SurfaceView
import android.widget.TextView
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.wulala.wildpointerdemo.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    private var bkJavaPlayer: BKJavaPlayer? = null
    private var surfaceView: SurfaceView? = null
    private var tvState: TextView? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        surfaceView = binding.surfaceView

        bkJavaPlayer = BKJavaPlayer()
        lifecycle.addObserver(bkJavaPlayer!!) // MainActivity被观察者 与 BKPlayer观察者 建立绑定关系
        bkJavaPlayer?.setSurfaceView(surfaceView!!)
        // bkJavaPlayer?.setDataSource("rtsp://192.168.3.204:8554/stream")
        bkJavaPlayer?.setDataSource("rtsp://192.168.1.7:554/stream1")

        bkJavaPlayer!!.setOnErrorListener(object : BKJavaPlayer.OnErrorListener {

            @SuppressLint("SetTextI18n")
            override fun onError(errorCode: String?) {
                runOnUiThread { // Toast.makeText(MainActivity.this, "出错了，错误详情是:" + errorInfo, Toast.LENGTH_SHORT).show();
                    tvState?.setTextColor(Color.RED) // 红色
                    tvState?.text = errorCode
                }
            }
        })
        bkJavaPlayer!!.setOnPreparedListener(object : BKJavaPlayer.OnPreparedListener {

            @SuppressLint("SetTextI18n")
            override fun onPrepared() {

                runOnUiThread {
                    // Toast.makeText(MainActivity.this, "准备成功，即将开始播放", Toast.LENGTH_SHORT).show();
                    tvState?.setTextColor(Color.GREEN) // 绿色
                    tvState!!.text = "RTSP解析成功"
                }
                bkJavaPlayer!!.start() // 调用 C++ 开始播放
            }
        })

        // 动态 6.0及以上的 申请权限
        checkPermission()
    }

    private var permissions = arrayOf<String>(Manifest.permission.WRITE_EXTERNAL_STORAGE) // 如果要申请多个动态权限，此处可以写多个

    var mPermissionList: MutableList<String> = ArrayList()

    private val PERMISSION_REQUEST = 1

    private fun checkPermission() {
        mPermissionList.clear()

        // 判断哪些权限未授予
        for (permission in permissions) {
            if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                mPermissionList.add(permission)
            }
        }

        // 判断是否为空
        if (mPermissionList.isEmpty()) { // 未授予的权限为空，表示都授予了
        } else {
            //请求权限方法
            val permissions = mPermissionList.toTypedArray() //将List转为数组
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String?>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        when (requestCode) {
            PERMISSION_REQUEST -> {}
            else -> super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
    }
}