package cn.vecrates.ffmpeg;

import android.annotation.SuppressLint
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.lifecycle.lifecycleScope
import androidx.media3.common.MediaItem
import androidx.media3.common.Player
import androidx.media3.exoplayer.ExoPlayer
import cn.vecrates.ffmpeg.databinding.ActivityCmdBinding
import cn.vecrates.ffmpeg.ffmpeg.FFmpegCmd
import cn.vecrates.ffmpeg.util.FileUtil
import cn.vecrates.ffmpeg.util.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

class CmdActivity : ComponentActivity() {

    companion object {
        private const val TAG = "CmdActivity"
    }

    private val binding: ActivityCmdBinding by lazy {
        ActivityCmdBinding.inflate(layoutInflater);
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)

        init()
    }

    @SuppressLint("SetTextI18n")
    private fun init() {
        lifecycleScope.launch(Dispatchers.Default) {

            val assertVideo = "video/test.mp4"
            val sdVideo = File(cacheDir, "test.mp4").path
            FileUtil.copyAssets(App.context, assertVideo, sdVideo)

            val assertLogo = "image/bili_logo.png"
            val sdLogo = File(cacheDir, "bili_logo.png").path
            FileUtil.copyAssets(App.context, assertLogo, sdLogo)

            val outputVideo = File(cacheDir, "merged.mp4")
            if (outputVideo.exists()) {
                outputVideo.delete()
            }

            val watermarkWidth = 150.dp()
            val watermarkHeight = (watermarkWidth / (702f / 222)).toInt()
            val margin = 10.dp()

            val cmd = "ffmpeg -hide_banner -i $sdVideo -i $sdLogo " +
                    "-filter_complex [1:v]scale=${watermarkWidth}:${watermarkHeight}[scaleLogo];" +
                    "[0:v][scaleLogo]overlay=x=W-w-$margin:y=H-h-$margin ${outputVideo.path}"

            withContext(Dispatchers.Main) {
                Log.d(TAG, "exe ffmpeg cmd: $cmd")
                binding.tvCmd.text = String.format("cmd=%s\nprogress=%s", cmd, 0)
            }

            val cmdArray = cmd.split(" ").toTypedArray()
            val result = FFmpegCmd.exec(cmdArray) { currentUs, durationUs ->
                runOnUiThread {
                    val progress = (currentUs.toFloat() / durationUs * 100).toInt()
                    binding.tvCmd.text = String.format("cmd=%s\nprogress=%s", cmd, "$progress%")
                }
            }

            Log.e(TAG, "exe ffmpeg cmd result: $result")

            withContext(Dispatchers.Main) {
                binding.tvCmd.text = String.format("cmd=%s\nresult=%d", cmd, result)
                if (result == 0) {
                    initVideo(outputVideo.path)
                }
            }

        }

    }

    private fun initVideo(path: String) {
        val player = ExoPlayer.Builder(this).build()
        player.setMediaItem(MediaItem.fromUri(path))
        binding.viewPlayer.useController = true
        binding.viewPlayer.player = player
        binding.viewPlayer.player!!.playWhenReady = true
        binding.viewPlayer.player!!.addListener(object : Player.Listener {

            override fun onPlaybackStateChanged(playbackState: Int) {
                super.onPlaybackStateChanged(playbackState)
                Log.d(TAG, "onPlaybackStateChanged: $playbackState")
            }

        })
    }


    override fun onDestroy() {
        super.onDestroy()
        binding.viewPlayer.player?.run {
            stop()
            release()
        }
        FFmpegCmd.release()
    }

}