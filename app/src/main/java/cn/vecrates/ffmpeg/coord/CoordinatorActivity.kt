package cn.vecrates.ffmpeg.coord

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import cn.vecrates.ffmpeg.databinding.ActivityCoordinatorBinding

class CoordinatorActivity : AppCompatActivity() {

    private val binding: ActivityCoordinatorBinding by lazy {
        ActivityCoordinatorBinding.inflate(layoutInflater)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)

        binding.layoutAppBar.addOnOffsetChangedListener { appBarLayout, _ ->
            val maxOffset: Int = binding.viewTabTop.height - binding.viewTabTop.minimumHeight
            val barTop = Math.abs(appBarLayout.getTop())
            val factor = 1f - barTop.toFloat() / maxOffset
            binding.llTabBar.setRadiusFactor(factor)
        };
    }

}