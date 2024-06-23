package cn.vecrates.ffmpeg

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.annotation.SuppressLint
import android.graphics.RectF
import android.os.Bundle
import android.provider.Settings.Global
import android.util.Log
import android.view.MotionEvent
import android.view.View
import android.view.View.GONE
import android.view.View.OnTouchListener
import androidx.appcompat.app.AppCompatActivity
import androidx.constraintlayout.widget.ConstraintLayout
import androidx.media3.common.MediaItem
import androidx.media3.common.Player
import androidx.media3.common.Player.REPEAT_MODE_ALL
import androidx.media3.common.VideoSize
import androidx.media3.exoplayer.ExoPlayer
import cn.vecrates.ffmpeg.databinding.ActivityPlayerBinding

class PlayerActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "PlayerActivity"
    }

    private lateinit var binding: ActivityPlayerBinding

    private var full: Boolean = false
    private val originRectF: RectF = RectF()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)

        initVideo()
    }

    private fun initVideo() {
        val player = ExoPlayer.Builder(this).build()
        binding.viewPlayer.useController = false
        binding.viewPlayer.player = player
        binding.viewPlayer.player?.addListener(object : Player.Listener {

            override fun onVideoSizeChanged(videoSize: VideoSize) {
                super.onVideoSizeChanged(videoSize)
                layoutVideo(videoSize.width, videoSize.height)
            }

            override fun onPlaybackStateChanged(playbackState: Int) {
                super.onPlaybackStateChanged(playbackState)
                Log.e(TAG, "onPlaybackStateChanged: " + playbackState)
                if (playbackState == Player.STATE_READY) {
                    player.play()
                }
            }

        })

        player.run {
            val mediaItem =
                MediaItem.fromUri("https://mp4.vjshi.com/2024-04-27/f9b27c3c17504566a9a1681e1ae2bb7e.mp4")
            setMediaItem(mediaItem)
            repeatMode = REPEAT_MODE_ALL
            prepare()
        }

        binding.viewPlayer.setOnTouchListener(object : OnTouchListener {
            @SuppressLint("ClickableViewAccessibility")
            override fun onTouch(p0: View, p1: MotionEvent): Boolean {
                if (p1.action != MotionEvent.ACTION_DOWN) {
                    return false
                }

                full = !full
                switchFull(full)

                return false
            }
        })

    }

    private fun layoutVideo(width: Int, height: Int) {
        val usableWidth = binding.root.width
        val usableHeight = binding.root.height * 0.5f
        val usableRatio = usableWidth.toFloat() / usableHeight
        val videoRatio = width.toFloat() / height
        val lp = binding.viewPlayer.layoutParams as ConstraintLayout.LayoutParams
        if (videoRatio < usableRatio) {
            lp.height = usableHeight.toInt()
            lp.width = (lp.height * videoRatio).toInt()
        } else {
            lp.width = usableWidth
            lp.height = (lp.width / videoRatio).toInt()
        }
        lp.marginStart = Math.max(0f, (usableWidth - lp.width) * 0.5f).toInt()
        lp.topMargin = Math.max(0f, (usableHeight - lp.height) * 0.5f).toInt() + 100
        binding.viewPlayer.layoutParams = lp
    }

    private fun switchFull(toFull: Boolean) {
        if (binding.viewPlayer.player == null) {
            return
        }
        val usableWidth = binding.root.width
        val usableHeight = binding.root.height * 0.8f
        val baseMarginTop = (binding.root.height - usableHeight) * 0.5f
        val usableRatio = usableWidth.toFloat() / usableHeight
        val videoSize = binding.viewPlayer.player!!.videoSize
        val videoRatio = videoSize.width.toFloat() / videoSize.height

        val srcRectF = RectF(
            binding.viewPlayer.left.toFloat(),
            binding.viewPlayer.top.toFloat(),
            binding.viewPlayer.right.toFloat(),
            binding.viewPlayer.bottom.toFloat()
        )
        if (originRectF.width() <= 0f) {
            originRectF.set(srcRectF)
        }

        val destRectF = RectF(originRectF)
        if (toFull) {
            var destWidth = usableWidth
            var destHeight = destWidth / videoRatio
            if (videoRatio < usableRatio) {
                destHeight = usableHeight
                destWidth = (destHeight * videoRatio).toInt()
            }
            val destLeft = (usableWidth - destWidth) / 2f
            val destTop = (usableHeight - destHeight) / 2f + baseMarginTop
            val destRight = destLeft + destWidth
            val destBottom = destTop + destHeight
            destRectF.set(destLeft, destTop, destRight, destBottom)
        }

        val bgFactor = if (toFull) 0f else 1f

        val animator = ValueAnimator.ofFloat(0f, 1f)
        animator.duration = 1000
        animator.addUpdateListener {
            val value = it.animatedValue as Float
            val curCenterX = (destRectF.centerX() - srcRectF.centerX()) * value + srcRectF.centerX()
            val curCenterY = (destRectF.centerY() - srcRectF.centerY()) * value + srcRectF.centerY()
            val curWidth = (destRectF.width() - srcRectF.width()) * value + srcRectF.width()
            val curHeight = (destRectF.height() - srcRectF.height()) * value + srcRectF.height()
            val curMarginStart = (curCenterX - curWidth * 0.5f).toInt()
            val curMarginTop = (curCenterY - curHeight * 0.5f).toInt()

            val lp = binding.viewPlayer.layoutParams as ConstraintLayout.LayoutParams
            lp.width = curWidth.toInt()
            lp.height = curHeight.toInt()
            lp.marginStart = curMarginStart
            lp.topMargin = curMarginTop
            binding.viewPlayer.layoutParams = lp

            binding.viewPlayerBg.alpha = Math.abs(value - bgFactor)
            Log.e(TAG, "switchFull: ${binding.viewPlayerBg.alpha}")
            if (binding.viewPlayerBg.alpha <= 0f) {
                binding.viewPlayerBg.visibility = GONE
            } else {
                binding.viewPlayerBg.visibility = View.VISIBLE
            }
        }
        animator.addListener(object : AnimatorListenerAdapter() {
            override fun onAnimationEnd(animation: Animator) {
                super.onAnimationEnd(animation)
                animator.removeAllUpdateListeners()
                animator.removeAllListeners()
            }
        })
        animator.start()

    }

    override fun onDestroy() {
        super.onDestroy()
        binding.viewPlayer.player?.run {
            stop()
            release()
        }
        binding.viewPlayer.player = null
    }


}