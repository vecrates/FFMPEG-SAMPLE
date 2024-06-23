package cn.vecrates.ffmpeg.hwcodec

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.text.TextUtils
import android.util.Log
import android.view.Surface

open class VideoDecoder {

    companion object {
        const val TAG = "VideoDecoder"
        const val TIMEOUT_US = 1000000
        const val TRY_AGAIN_TIME = 100
        const val NEED_SEEK = 1
        const val TO_EOS = 2
        const val SHOULD_SHOW = 3
        const val GO_ON = 4
    }


    private var mediaExtractor: MediaExtractor? = null
    private var mediaFormat: MediaFormat? = null
    private var mediaCodec: MediaCodec? = null
    private var surface: Surface? = null
    private var isOutputEos = false

    private var videoWidth: Int = 0
    private var videoHeight: Int = 0
    private var videoDuration: Long = 0
    private var frameIntervalUs: Long = (1000000 / 30f).toLong()

    private var curDecodeTime = 0
    private var destSeekTime = 0

    private fun init(path: String): Boolean {
        mediaExtractor = MediaExtractor()
        mediaExtractor!!.setDataSource(path)
        val videoTrackIndex = findVideoTrackIndex(mediaExtractor!!)
        if (videoTrackIndex < 0) {
            return false
        }
        mediaFormat = mediaExtractor!!.getTrackFormat(videoTrackIndex)
        mediaExtractor!!.selectTrack(videoTrackIndex)
        if (mediaFormat == null) {
            return false;
        }

        if (!mediaFormat!!.containsKey(MediaFormat.KEY_DURATION)) {
            return false
        }
        videoDuration = mediaFormat!!.getLong(MediaFormat.KEY_DURATION)

        if (!mediaFormat!!.containsKey(MediaFormat.KEY_WIDTH)) {
            return false
        }
        videoWidth = mediaFormat!!.getInteger(MediaFormat.KEY_WIDTH)

        if (!mediaFormat!!.containsKey(MediaFormat.KEY_HEIGHT)) {
            return false
        }
        videoHeight = mediaFormat!!.getInteger(MediaFormat.KEY_HEIGHT)

        return true
    }

    fun startCodec(): Boolean {
        val mimeType = mediaFormat!!.getString(MediaFormat.KEY_MIME)
        if (TextUtils.isEmpty(mimeType)) {
            release()
            return false
        }

        mediaCodec = MediaCodec.createDecoderByType(mimeType!!)
        if (mediaCodec == null) {
            release()
            Log.e(TAG, "mediaCodec create failed")
            return false;
        }

        try {
            mediaCodec!!.configure(mediaFormat, surface, null, 0)
            mediaCodec!!.start()
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }

        return true
    }

    private fun findVideoTrackIndex(mediaExtractor: MediaExtractor): Int {
        for (i in 0..mediaExtractor.trackCount) {
            val mediaFormat = mediaExtractor.getTrackFormat(i)
            val mime: String? = mediaFormat.getString(MediaFormat.KEY_MIME)
            if (mime != null && mime.startsWith("video")) {
                return i
            }
        }
        return -1
    }

    fun release() {
        mediaCodec?.stop()
        mediaCodec?.release()
        mediaExtractor?.release()
        mediaCodec = null
        mediaExtractor = null
        mediaFormat = null
    }

}