package cn.vecrates.ffmpeg.render.util;

import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;

import androidx.annotation.NonNull;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MathUtil {

    public static float distance(float x1, float y1, float x2, float y2) {
        float dx = x1 - x2;
        float dy = y1 - y2;
        return (float) Math.sqrt(dx * dx + dy * dy);
    }

    public static float distance(PointF point1, PointF point2) {
        float deltaX = point1.x - point2.x;
        float deltaY = point1.y - point2.y;
        return (float) Math.sqrt(deltaX * deltaX + deltaY * deltaY);
    }

    public static float range(float intensity, float start, float end) {
        return start + (end - start) * intensity;
    }

    public static float range(float intensity, float mid, float start, float end) {
        if (intensity < 0.5f) {
            float range = mid - start;
            intensity = (0.5f - intensity) * 2;
            return mid - range * intensity;
        } else if (intensity > 0.5f) {
            float range = end - mid;
            intensity = (intensity - 0.5f) * 2;
            return mid + range * intensity;
        }
        return mid;
    }

    /**
     * 将src中的强度映射到dest中
     *
     * @param intensity [srcStart,srcEnd]
     * @param srcStart  src最低强度
     * @param srcEnd    src最高强度
     * @param destStart dest最低强度
     * @param destEnd   dest最高强度
     * @return
     */
    public static float mapping(float intensity, float srcStart, float srcEnd, float destStart, float destEnd) {
        intensity = (intensity - srcStart) / (srcEnd - srcStart);
        return destStart + (destEnd - destStart) * intensity;
    }

    public static boolean floatEquals(float val1, float val2) {
        return Math.abs(val1 - val2) < 0.00001f;
    }

    public static boolean floatNotEquals(float val1, float val2) {
        return Math.abs(val1 - val2) > 0.00001f;
    }

    public static float extractNumber(String str) throws Exception {
        // 需要取整数和小数的字符串
        // 控制正则表达式的匹配行为的参数(小数)
        Pattern p = Pattern.compile("(\\d+\\.\\d+)");
        //Matcher类的构造方法也是私有的,不能随意创建,只能通过Pattern.matcher(CharSequence input)方法得到该类的实例.
        Matcher m = p.matcher(str);
        //m.find用来判断该字符串中是否含有与"(\\d+\\.\\d+)"相匹配的子串
        if (m.find()) {
            //如果有相匹配的,则判断是否为null操作
            //group()中的参数：0表示匹配整个正则，1表示匹配第一个括号的正则,2表示匹配第二个正则,在这只有一个括号,即1和0是一样的
            str = m.group(1) == null ? "" : m.group(1);
        } else {
            //如果匹配不到小数，就进行整数匹配
            p = Pattern.compile("(\\d+)");
            m = p.matcher(str);
            if (m.find()) {
                //如果有整数相匹配
                str = m.group(1) == null ? "" : m.group(1);
            } else {
                //如果没有小数和整数相匹配,即字符串中没有整数和小数，就设为空
                str = "";
            }
        }
        return Float.parseFloat(str);
    }

    /****/

    public static Rect getCenterCropRectFrame(int width, int height, float destRatio) {
        RectF rectF = getCenterCropRectFrame(width, height, destRatio, 0);
        return new Rect((int) rectF.left, (int) rectF.top, (int) rectF.right, (int) rectF.bottom);
    }

    public static RectF getCenterCropRectFrame(float width, float height, float destRatio) {
        return getCenterCropRectFrame(width, height, destRatio, 0);
    }

    /**
     * 一条边不变，另一条边变大构造成一个和传入比例形同的矩形
     *
     * @param destRatio w/h
     * @return
     */
    public static RectF getCenterCropRectFrame(float width, float height, float destRatio, float ignoreValue) {
        float ratio = width / height;
        float destWidth = width;
        float destHeight = height;
        //近似相等，认为相等
        if (Math.abs(ratio - destRatio) <= ignoreValue) {
            return new RectF(0, 0, destWidth, destHeight);
        } else if (ratio < destRatio) {
            //比需要的比例细长，高不变宽变大
            destWidth = height * destRatio;
            destHeight = height;
            float left = (destWidth - width) / -2f;
            return new RectF(left, 0, left + destWidth, destHeight);
        } else if (ratio > destRatio) {
            //高变化，宽不变
            destWidth = width;
            destHeight = width / destRatio;
            float top = (destHeight - height) / -2f;
            return new RectF(0, top, destWidth, top + destHeight);
        }
        return new RectF(0, 0, destWidth, destHeight);
    }

    public static Rect getFitCenterRectFrame(int width, int height, float destRatio) {
        RectF rectF = getFitCenterRectFrame(width, height, destRatio, 0);
        return new Rect((int) rectF.left, (int) rectF.top, (int) rectF.right, (int) rectF.bottom);
    }

    public static RectF getFitCenterRectFrame(float width, float height, float destRatio) {
        return getFitCenterRectFrame(width, height, destRatio, 0);
    }

    /**
     * 一条边不变，另一条边缩小构造成一个和传入比例形同的矩形
     *
     * @param width
     * @param height
     * @param destRatio
     * @param ignoreValue 忽略的误差
     * @return
     */
    public static RectF getFitCenterRectFrame(float width, float height, float destRatio, float ignoreValue) {
        float ratio = width * 1.f / height;
        float destWidth = width;
        float destHeight = height;
        //近似相等，认为相等
        if (Math.abs(ratio - destRatio) <= ignoreValue) {
            return new RectF(0, 0, destWidth, destHeight);
        } else if (ratio < destRatio) {
            //比需要的比例细长，宽不变，高缩小
            destWidth = width;
            destHeight = width / destRatio;
            float top = (height - destHeight) / 2f;
            return new RectF(0, top, destWidth, top + destHeight);
        } else if (ratio > destRatio) {
            //比需要的比例矮胖，高不变，宽缩小
            destHeight = height;
            destWidth = destHeight * destRatio;
            float left = (width - destWidth) / 2f;
            return new RectF(left, 0, left + destWidth, destHeight);
        }
        return new RectF(0, 0, destWidth, destHeight);
    }

    public static RectF scaleRectF(RectF rect, float scale) {
        float rectRatio = rect.width() / rect.height();
        float newWidth = rect.width() * scale;
        float newHeight = newWidth / rectRatio;

        float halfX = newWidth * 0.5f;
        float halfY = newHeight * 0.5f;

        float left = rect.centerX() - halfX;
        float top = rect.centerY() - halfY;
        float right = left + newWidth;
        float bottom = top + newHeight;

        return new RectF(left, top, right, bottom);
    }

    public static void setScaleRectF(RectF rect, float scale) {
        float rectRatio = rect.width() / rect.height();
        float newWidth = rect.width() * scale;
        float newHeight = newWidth / rectRatio;

        float halfX = newWidth * 0.5f;
        float halfY = newHeight * 0.5f;

        float left = rect.centerX() - halfX;
        float top = rect.centerY() - halfY;
        float right = left + newWidth;
        float bottom = top + newHeight;

        rect.set(left, top, right, bottom);
    }

    public static Rect scaleRect(Rect rect, float scale, Rect limit) {
        float rectRatio = (float) rect.width() / rect.height();
        float newWidth = rect.width() * scale;
        float newHeight = newWidth / rectRatio;

        float halfX = newWidth * 0.5f;
        float halfY = newHeight * 0.5f;

        int left = (int) (rect.centerX() - halfX);
        int top = (int) (rect.centerY() - halfY);
        int right = (int) (left + newWidth);
        int bottom = (int) (top + newHeight);

        left = Math.max(limit.left, left);
        top = Math.max(limit.top, top);
        right = Math.min(limit.right, right);
        bottom = Math.min(limit.bottom, bottom);

        return new Rect(left, top, right, bottom);
    }

    public static Rect mappingRect(Rect rect, int width, int height) {
        return mappingRect(rect, width, height);
    }

    public static Rect mappingRect(RectF rect, int width, int height) {
        return new Rect((int) (rect.left * width), (int) (rect.top * height),
                (int) (rect.right * width), (int) (rect.bottom * height));
    }

    public static RectF mappingRectF(RectF rect, float width, float height) {
        return new RectF(rect.left * width, rect.top * height,
                rect.right * width, rect.bottom * height);
    }

    /**
     * @param normalizeRect normalized
     * @param dstRect
     * @return
     */
    public static RectF mappingRectF(RectF normalizeRect, RectF dstRect) {
        float width = normalizeRect.width() * dstRect.width();
        float height = normalizeRect.height() * dstRect.height();
        float left = normalizeRect.left * dstRect.width() + dstRect.left;
        float top = normalizeRect.top * dstRect.height() + dstRect.top;
        float right = left + width;
        float bottom = top + height;
        return new RectF(left, top, right, bottom);
    }


    public static void mappingFloatArray(@NonNull float[] array, float start, float end) {
        for (int i = 0; i < array.length; i++) {
            array[i] = start + (end - start) * array[i];
        }
    }


}
