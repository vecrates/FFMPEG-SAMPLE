package cn.vecrates.ffmpeg;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;

public class TRadiusLinearLayout extends LinearLayout {

    private float radius = 0;
    private float radiusFactor = 1.f;
    private int bgColor = Color.TRANSPARENT;
    private RectF bgRect = new RectF();
    private Paint bgPaint = new Paint();

    public TRadiusLinearLayout(Context context) {
        this(context, null);
    }

    public TRadiusLinearLayout(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public TRadiusLinearLayout(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        setWillNotDraw(false);

        TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TRadiusLinearLayout);
        bgColor = ta.getColor(R.styleable.TRadiusLinearLayout_t_bg_color, Color.TRANSPARENT);
        radius = ta.getDimension(R.styleable.TRadiusLinearLayout_t_radius, 0f);
        ta.recycle();

        bgPaint.setAntiAlias(true);
        bgPaint.setStyle(Paint.Style.FILL);
        bgPaint.setColor(bgColor);
    }

    public void setTBackground(int color) {
        this.bgColor = color;
        this.bgPaint.setColor(bgColor);
        invalidate();
    }

    public void setTRadius(float radius) {
        this.radius = radius;
        invalidate();
    }

    public void setRadiusFactor(float radiusFactor) {
        this.radiusFactor = radiusFactor;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Log.e("===", "onDraw: " + bgColor);
        if (bgColor != Color.TRANSPARENT) {
            float finalRadius = radius * radiusFactor;
            bgRect.set(0f, 0f, getWidth(), getHeight());
            canvas.drawRoundRect(bgRect, finalRadius, finalRadius, bgPaint);
            bgRect.set(0f, getHeight() * 0.5f, getWidth(), getHeight());
            canvas.drawRect(bgRect, bgPaint);
        }

        super.onDraw(canvas);
    }
}
