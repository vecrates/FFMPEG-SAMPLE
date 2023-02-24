package cn.vecrates.ffmpeg.render;

import cn.vecrates.ffmpeg.render.pass.DisplayPass;
import cn.vecrates.ffmpeg.render.pass.FormatPass;

public class VideoDrawerProxy extends AbsVideoDrawerProxy {

    private FormatPass formatPass;
    private DisplayPass displayPass;

    @Override
    protected void initPasses() {
        formatPass = new FormatPass(passContext);
        addPass(formatPass);
        displayPass = new DisplayPass(passContext);
        addPass(displayPass);
    }

    @Override
    public FormatPass getFormatPass() {
        return formatPass;
    }

    @Override
    public DisplayPass getDisplayPass() {
        return displayPass;
    }

    @Override
    void updateYUV(byte[] y, byte[] u, byte[] v) {
        if (formatPass != null) {
            formatPass.updateYUV(y, u, v);
        }
    }

}
