package cn.vecrates.ffmpeg.view;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.StaticLayout;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;


public class ExpandableTextView extends RelativeLayout {
    private String TAG = this.getClass().getSimpleName();
    private String TIP_COLLAPSE = "收起";
    private String TIP_EXPAND = "展开";
    private static final String ELLIPSE = "...";
    private static final int ALIGN_RIGHT = 0;//控制按钮在文本右下角，与文本底部基线齐平
    private static final int BOTTOM_START = 1;//控制按钮在文本下方左侧
    private static final int BOTTOM_CENTER = 2;//控制按钮在文本下方中间
    private static final int BOTTOM_END = 3;//控制按钮在文本下方右侧侧
    private TextView mTvContent;//内容
    private float mLineSpaceExtra;//行距
    private TextView mTvExpand;//折叠控件
    protected boolean mIsExpand;//是否折叠的标记
    private CharSequence mOriginText = "";//原始的文本
    private CharSequence mExpandText = "";//展开的文本
    private CharSequence mCollapseText = "";//收起的文本
    private boolean mExpandable = true;//是否支持点击展开
    private int mContentTextSize;
    private int mContentColor;
    private int mTipsColor;
    private int mMaxLines = 3;//折叠后显示的行数，默认为3行
    private int mPosition = ALIGN_RIGHT;
    private Drawable mCollapseDrawable;
    private Drawable mExpandDrawable;
    private boolean performedByUser;
    protected boolean mCancelAnim;
    private int mTextTotalWidth;
    /**
     * 展开折叠监听器
     */
    private OnToggleListener toggleListener;
    protected boolean mMeasured;
    private int mTipMarginTop;
    private int mCollapseHeight;
    private int mExpandHeight;

    public ExpandableTextView(Context context) {
        this(context, null, 0);
    }

    public ExpandableTextView(Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public ExpandableTextView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);

        initView();

        //设置折叠展开标识控件的位置
        updateExpandArrowAndPosition(mPosition);
    }

    private void initView() {
        mTvContent = new TextView(getContext());
        LayoutParams lp = new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        addView(mTvContent, lp);
        mTvContent.setId(View.generateViewId());

        mTvExpand = new TextView(getContext());
        lp = new LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        addView(mTvExpand, lp);
        mTvExpand.setId(View.generateViewId());
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        //如果不支持展开折叠，那么layout截取事件并响应事件
        return !mExpandable;
    }

    private int sp2px(Context context, float spValue) {
        final float fontScale = context.getResources().getDisplayMetrics().scaledDensity;
        return (int) (spValue * fontScale + 0.5f);
    }

    private void updateExpandArrowAndPosition(int position) {
        LayoutParams params = new LayoutParams(mTvExpand.getLayoutParams());
        switch (position) {
            case BOTTOM_START:
                params.topMargin = mTipMarginTop;
                params.addRule(RelativeLayout.BELOW, mTvContent.getId());
                params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, RelativeLayout.TRUE);
                break;
            case BOTTOM_CENTER:
                params.topMargin = mTipMarginTop;
                params.addRule(RelativeLayout.BELOW, mTvContent.getId());
                params.addRule(RelativeLayout.CENTER_HORIZONTAL, RelativeLayout.TRUE);
                break;
            case BOTTOM_END:
                params.topMargin = mTipMarginTop;
                params.addRule(RelativeLayout.BELOW, mTvContent.getId());
                params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
                break;
            case ALIGN_RIGHT:
                params.addRule(RelativeLayout.ALIGN_BOTTOM, mTvContent.getId());
                params.addRule(RelativeLayout.ALIGN_PARENT_RIGHT, RelativeLayout.TRUE);
                break;
            default:
                params.addRule(RelativeLayout.BELOW, mTvContent.getId());
                break;
        }
        mTvExpand.setLayoutParams(params);
    }

    private void initText(final CharSequence text) {
        //根据指定的折叠行数获取折叠文本
        mOriginText = text;
        mTvContent.setText(mOriginText);
        if (!mMeasured) {
            if (getViewTreeObserver() != null) {
                getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {

                    @Override
                    public boolean onPreDraw() {
                        getViewTreeObserver().removeOnPreDrawListener(this);
                        //获取控件尺寸
                        if (getWidth() != 0) {
                            mTextTotalWidth = getWidth() - getPaddingLeft() - getPaddingRight();
                            Log.d(TAG, "控件宽度：" + mTextTotalWidth);
                            mMeasured = true;
                            toggleText();
                        }
                        return true;
                    }
                });
            }
        }
    }

    public void setText(CharSequence text) {
        //if (TextUtils.isEmpty(text)) return;
        mIsExpand = !mIsExpand;
        performedByUser = false;
        initText(text);
    }

    /**
     * 指定显示文本,并指定赋值后展开还是折叠的状态
     *
     * @param text  显示文本
     * @param close true代表默认收起,false代表默认展开
     */
    public void setText(CharSequence text, boolean close) {
        mIsExpand = !close;
        performedByUser = true;
        initText(text);
    }

    public void setExpandable(boolean expandable) {
        this.mExpandable = expandable;
    }

    /**
     * 设置展开提示语，在setText方法之前调用
     *
     * @param label 展开提示语，如“展开”
     */
    public void setExpandLabel(String label) {
        if (!TextUtils.isEmpty(label)) {
            TIP_EXPAND = label;
        }
    }

    /**
     * 设置折叠提示语，在setText方法之前调用
     *
     * @param label 折叠提示语，如“收起”
     */
    public void seCollapseLabel(String label) {
        if (!TextUtils.isEmpty(label)) {
            TIP_COLLAPSE = label;
        }
    }

    /**
     * 设置展开提示图标，在setText方法之前调用
     *
     * @param drawable 展开图标
     */
    public void setExpandDrawable(Drawable drawable) {
        if (drawable != null) {
            mExpandDrawable = drawable;
            mExpandDrawable.setBounds(0, 0, mContentTextSize, mContentTextSize);
        }
    }

    /**
     * 设置折叠提示图标，在setText方法之前调用
     *
     * @param drawable 折叠图标
     */
    public void setCollapseDrawable(Drawable drawable) {
        if (drawable != null) {
            mCollapseDrawable = drawable;
            mCollapseDrawable.setBounds(0, 0, mContentTextSize, mContentTextSize);
        }
    }

    public void setExpandableTextViewClick(View.OnClickListener click) {
        if (mTvContent != null) {
            mTvContent.setOnClickListener(click);
        }
    }

    public void setExpandableTextViewLongClick(OnLongClickListener longClick) {
        if (mTvContent != null) {
            mTvContent.setOnLongClickListener(longClick);
        }
    }

    public CharSequence getText() {
        return mOriginText;
    }

    /**
     * 展开或收起文本
     */
    public void toggleText() {
        //修改展开折叠标志
        mIsExpand = !mIsExpand;
        boolean canCollapse = formatText(mOriginText);

        if (!canCollapse) {
            //说明无需折叠
            mTvExpand.setVisibility(GONE);
            mTvContent.setText(mOriginText);
            return;
        } else {
            mTvExpand.setVisibility(VISIBLE);
        }
        if (!performedByUser || !mExpandable) {
            mTvContent.setText(mCollapseText);
            mIsExpand = false;
            if (mExpandDrawable != null) {
                mTvExpand.setCompoundDrawablesWithIntrinsicBounds(null, null, mExpandDrawable, null);
            }
            mTvExpand.setText(TIP_EXPAND);
            return;
        }
        if (mIsExpand) {
            mTvContent.setMaxLines(Integer.MAX_VALUE);
            mTvContent.setText(mExpandText);
            if (!mCancelAnim) {
                //展开动画
                ValueAnimator anim = ValueAnimator.ofInt(mCollapseHeight, mExpandHeight).setDuration(200);
                anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        LayoutParams params = (LayoutParams) mTvContent.getLayoutParams();
                        int h = (Integer) animation.getAnimatedValue();
                        params.height = h;
                        if (h >= mExpandHeight) {
                            params.height = LayoutParams.WRAP_CONTENT;
                        }
                        mTvContent.setLayoutParams(params);
                    }

                });
                anim.start();
            }

            if (mCollapseDrawable != null) {
                mTvExpand.setCompoundDrawablesWithIntrinsicBounds(null, null, mCollapseDrawable, null);
            } else {
                mTvExpand.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
            }
            mTvExpand.setText(TIP_COLLAPSE);
            if (toggleListener != null) {
                toggleListener.onToggle(true);
            }
        } else {
            mTvContent.setMaxLines(mMaxLines);
            mTvContent.setText(mCollapseText);
            if (!mCancelAnim) {
                //收起动画
                ValueAnimator anim = ValueAnimator.ofInt(mExpandHeight, mCollapseHeight).setDuration(200);
                anim.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        LayoutParams params = (LayoutParams) mTvContent.getLayoutParams();
                        int h = (Integer) animation.getAnimatedValue();
                        params.height = h;
                        if (h <= mCollapseHeight) {
                            params.height = LayoutParams.WRAP_CONTENT;
                        }
                        mTvContent.setLayoutParams(params);
                    }
                });
                anim.start();
            }

            if (mExpandDrawable != null) {
                mTvExpand.setCompoundDrawablesWithIntrinsicBounds(null, null, mExpandDrawable, null);
            } else {
                mTvExpand.setCompoundDrawablesWithIntrinsicBounds(null, null, null, null);
            }
            mTvExpand.setText(TIP_EXPAND);
            if (toggleListener != null) {
                toggleListener.onToggle(false);
            }
        }
        invalidate();
    }

    /**
     * 格式化折叠展开时的文本
     *
     * @return 是否超出给定行数
     */
    public boolean formatText(CharSequence text) {
        if (text == null) text = "";
        TextPaint paint = mTvContent.getPaint();
        StaticLayout staticLayout = new StaticLayout(text, paint, mTextTotalWidth, Layout.Alignment.ALIGN_NORMAL, 1.0f, mLineSpaceExtra, false);
        int lineCount = staticLayout.getLineCount();
        if (lineCount <= mMaxLines) {
            // 不足最大行数，直接设置文本
            //少于最小展示行数，不再展示更多相关布局
            mTvExpand.setVisibility(View.GONE);
            mCollapseText = text;
            mExpandText = text;
            return false;
        } else {
            // 超出最大行数
            mTvExpand.setVisibility(View.VISIBLE);
            //#######1.获取折叠后的文本#######
            int lastLineStartIndex = staticLayout.getLineStart(mMaxLines - 1);
            int lastLineEndIndex = staticLayout.getLineEnd(mMaxLines - 1);
            // 防止越界
            if (lastLineStartIndex < 0) {
                lastLineStartIndex = 0;
            }
            if (lastLineEndIndex > text.length()) {
                lastLineEndIndex = text.length();
            }
            if (lastLineStartIndex > lastLineEndIndex) {
                lastLineStartIndex = lastLineEndIndex;
            }
            CharSequence lastLineText = text.subSequence(lastLineStartIndex, lastLineEndIndex);
            float lastLineWidth = 0f;
            if (lastLineText != null) {
                lastLineWidth = paint.measureText(lastLineText, 0, lastLineText.length());
            }
            // 计算后缀的宽度
            int imgWidth = 0;
            if (mExpandDrawable != null) {
                imgWidth = mExpandDrawable.getIntrinsicWidth();
            }
            int expandedTextWidth = 0;
            //这里使用空格是为了确保最终拼接的长度不会超过整行宽度
            if (mPosition == ALIGN_RIGHT) {
                expandedTextWidth = (int) paint.measureText(ELLIPSE + "  " + TIP_EXPAND) + imgWidth;
            } else {
                expandedTextWidth = (int) paint.measureText(ELLIPSE);
            }
            // 如果大于屏幕宽度则需要减去部分字符
            if (lastLineWidth + expandedTextWidth >= mTextTotalWidth) {
                int cutCount = paint.breakText(mOriginText, lastLineStartIndex, lastLineEndIndex, false, expandedTextWidth, null);
                lastLineEndIndex -= cutCount;
            }
            StringBuilder appd = new StringBuilder(ELLIPSE);
            //再测量一下,有可能放置不下
            lastLineEndIndex = ensureLastLineEndIndex(paint, lastLineStartIndex, lastLineEndIndex, imgWidth, appd);
            // 因设置的文本可能是带有样式的文本，如SpannableStringBuilder，所以根据计算的字符数从原始文本中截取
            SpannableStringBuilder spannable = new SpannableStringBuilder();
            // 截取文本，还是因为原始文本的样式原因不能直接使用paragraphs中的文本
            CharSequence ellipsizeText = mOriginText.subSequence(0, lastLineEndIndex);
            spannable.append(ellipsizeText);
            spannable.append(ELLIPSE);
            Log.d("截取后的字符串:", spannable.toString());
            mCollapseText = spannable;
            mCollapseHeight = new StaticLayout(mCollapseText, paint, mTextTotalWidth, Layout.Alignment.ALIGN_NORMAL, 1, mLineSpaceExtra, false).getHeight();

            //#######2.获取展开后的字符串#######
            mExpandText = text;
            if (mPosition == ALIGN_RIGHT) {
                lastLineStartIndex = staticLayout.getLineStart(lineCount - 1);
                lastLineEndIndex = staticLayout.getLineEnd(lineCount - 1);
                // 防止越界
                if (lastLineStartIndex < 0) {
                    lastLineStartIndex = 0;
                }
                if (lastLineEndIndex > text.length()) {
                    lastLineEndIndex = text.length();
                }
                if (lastLineStartIndex > lastLineEndIndex) {
                    lastLineStartIndex = lastLineEndIndex;
                }
                lastLineText = text.subSequence(lastLineStartIndex, lastLineEndIndex);
                lastLineWidth = 0f;
                if (lastLineText != null) {
                    lastLineWidth = paint.measureText(lastLineText, 0, lastLineText.length());
                }
                String space = "  ";
                // 计算后缀的宽度
                expandedTextWidth = (int) paint.measureText(space + TIP_COLLAPSE) + 1;
                imgWidth = 0;
                if (mCollapseDrawable != null) {
                    imgWidth = mCollapseDrawable.getIntrinsicWidth();
                }
                expandedTextWidth += imgWidth;
                // 如果大于屏幕宽度则需要换行
                if (lastLineWidth + expandedTextWidth > mTextTotalWidth) {
                    SpannableStringBuilder stringBuilder = new SpannableStringBuilder(text);
                    mExpandText = stringBuilder.append("\n");
                }
            }
            mExpandHeight = new StaticLayout(mExpandText, paint, mTextTotalWidth, Layout.Alignment.ALIGN_NORMAL, 1, mLineSpaceExtra, false).getHeight();
            return true;
        }

    }

    protected int ensureLastLineEndIndex(TextPaint paint, int lastLineStartIndex, int lastLineEndIndex, int imgWidth, StringBuilder appd) {
        float lastLineWidth;
        String originStr = mOriginText.toString();
        lastLineWidth = paint.measureText(originStr.substring(lastLineStartIndex, lastLineEndIndex) + "  " + ELLIPSE + "  " + TIP_EXPAND) + imgWidth;
        if (lastLineWidth > mTextTotalWidth) {
            //再减掉一个字
            lastLineEndIndex--;
            //添加点占位
            int spaceWidth = (int) paint.measureText(".");
            int spaceCount = (int) ((mTextTotalWidth - paint.measureText(originStr.substring(lastLineStartIndex, lastLineEndIndex) + "  " + ELLIPSE + "  " + TIP_EXPAND) - imgWidth) / spaceWidth);
            for (int i = 0; i < spaceCount; i++) {
                appd.append(".");
            }
        }
        return lastLineEndIndex;
    }

    public void setToggleListener(OnToggleListener listener) {
        this.toggleListener = listener;
    }

    public void setOnClickExpandListener(OnClickListener listener) {
        this.mTvExpand.setOnClickListener(listener);
    }

    public interface OnToggleListener {
        void onToggle(boolean expanded);
    }
}