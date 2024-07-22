package cn.vecrates.ffmpeg.util

import cn.vecrates.ffmpeg.App

fun Int.dp(): Int {
    val desity = App.context.resources.displayMetrics.density
    return (this * desity).toInt()
}

fun Float.dp(): Float {
    val desity = App.context.resources.displayMetrics.density
    return this * desity
}