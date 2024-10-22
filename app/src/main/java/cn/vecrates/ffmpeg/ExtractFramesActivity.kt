package cn.vecrates.ffmpeg

import android.content.Intent
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.util.Log
import android.widget.ImageView
import android.widget.LinearLayout.LayoutParams
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import cn.vecrates.ffmpeg.databinding.ActivtiyExtractFramesBinding
import cn.vecrates.ffmpeg.ffmpeg.FFmpegCmd
import cn.vecrates.ffmpeg.util.FileUtil
import cn.vecrates.ffmpeg.util.UriUtil
import com.bumptech.glide.Glide
import com.bumptech.glide.load.DataSource
import com.bumptech.glide.load.engine.DiskCacheStrategy
import com.bumptech.glide.load.engine.GlideException
import com.bumptech.glide.request.RequestListener
import com.bumptech.glide.request.target.Target
import com.zhihu.matisse.Matisse
import com.zhihu.matisse.MimeType
import com.zhihu.matisse.engine.impl.GlideEngine
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File

class ExtractFramesActivity : AppCompatActivity() {

    private val TAG = "ExtractFramesActivity"

    private val binding: ActivtiyExtractFramesBinding by lazy {
        ActivtiyExtractFramesBinding.inflate(layoutInflater)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)

        initView()
    }

    private fun initView() {
        binding.tvSelectVideo.setOnClickListener {
            Matisse.from(this)
                .choose(MimeType.ofVideo())
                .countable(true)
                .maxSelectable(1)
                .showSingleMediaType(true)
                .autoHideToolbarOnSingleTap(true)
                .thumbnailScale(0.85f)
                .imageEngine(GlideEngine())
                .showPreview(false)
                .forResult(0)
        }
    }

    private fun getOutDir(): File {
        return File(cacheDir, "_temp_frames")
    }

    private fun runCmd(path: String) {
        lifecycleScope.launch(Dispatchers.Default) {
            FFmpegCmd.release()
            val outDir = getOutDir()
            FileUtil.deleteFile(outDir)
            FileUtil.createDir(outDir)
            val cmd = "ffmpeg -i $path -r 1 -f image2 ${outDir.path}/frame_%03d.jpg"
            val cmdStrings = cmd.split(" ").toTypedArray()
            Log.e(TAG, "runCmd: cmd=$cmd outDir=$outDir")
            val result = FFmpegCmd.exec(cmdStrings) { currentUs, durationUs ->
                Log.e(TAG, "runCmd: $currentUs $durationUs")
            }
            Log.e(TAG, "runCmd: result=$result")
            withContext(Dispatchers.Main) {
                if (result == 0) {
                    loadFrames()
                }
            }
        }

    }

    private fun loadFrames() {
        binding.llFrames.removeAllViews()
        binding.svFrames.scrollTo(0, 0)
        val outDir = getOutDir()
        outDir.listFiles()?.forEach {
            val frameIv = ImageView(this)
            frameIv.scaleType = ImageView.ScaleType.CENTER_CROP
            val lp = LayoutParams(0, 0).apply {
                width = binding.llFrames.height
                height = binding.llFrames.height
            }
            binding.llFrames.addView(frameIv, lp)
            Glide.with(this)
                .load(it.path)
                .skipMemoryCache(true)
                .diskCacheStrategy(DiskCacheStrategy.NONE)
                .addListener(object : RequestListener<Drawable> {
                    override fun onLoadFailed(
                        e: GlideException?,
                        model: Any?,
                        target: Target<Drawable>?,
                        isFirstResource: Boolean
                    ): Boolean {
                        return false
                    }

                    override fun onResourceReady(
                        resource: Drawable?,
                        model: Any?,
                        target: Target<Drawable>?,
                        dataSource: DataSource?,
                        isFirstResource: Boolean
                    ): Boolean {
                        it.delete()
                        return false
                    }

                })
                .into(frameIv)
        }

    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (resultCode != RESULT_OK || requestCode != 0) {
            return
        }
        val uris = Matisse.obtainResult(data)
        if (uris.isEmpty()) {
            return
        }
        lifecycleScope.launch(Dispatchers.Default) {
            val path = UriUtil.queryUriPath(this@ExtractFramesActivity, uris[0])
            launch(Dispatchers.Main) {
                runCmd(path)
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        FFmpegCmd.release()
    }


}