#include <jni.h>
#include <string>

#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

// 因为ffmpeg是纯C代码，要在cpp中使用则需要使用 extern "C"
extern "C" {
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}


//顶点着色器glsl的宏
// 第二个#号的意思是自动链接字符串，而不用增加引号，参考ijkplayer的写法

#define GET_STR(x) #x

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"FFRender",__VA_ARGS__)

static const char *vertexShader = GET_STR(

        attribute vec4 aPosition; //顶点坐标，在外部获取传递进来

        attribute vec2 aTexCoord; //材质（纹理）顶点坐标

        varying vec2 vTexCoord;   //输出的材质（纹理）坐标，给片元着色器使用
        void main() {
            //纹理坐标转换，以左上角为原点的纹理坐标转换成以左下角为原点的纹理坐标，
            // 比如以左上角为原点的（0，0）对应以左下角为原点的纹理坐标的（0，1）
            vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
            gl_Position = aPosition;
        }
);

//片元着色器,软解码和部分x86硬解码解码得出来的格式是YUV420p

static const char *fragYUV420P = GET_STR(

        precision mediump float;    //精度

        varying vec2 vTexCoord;     //顶点着色器传递的坐标，相同名字opengl会自动关联

        uniform sampler2D yTexture; //输入的材质（不透明灰度，单像素）

        uniform sampler2D uTexture;

        uniform sampler2D vTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r; // y分量
            // 因为UV的默认值是127，所以我们这里要减去0.5（OpenGLES的Shader中会把内存中0～255的整数数值换算为0.0～1.0的浮点数值）
            yuv.g = texture2D(uTexture, vTexCoord).r - 0.5; // u分量
            yuv.b = texture2D(vTexture, vTexCoord).r - 0.5; // v分量
            // yuv转换成rgb，两种方法，一种是RGB按照特定换算公式单独转换
            // 另外一种是使用矩阵转换
            rgb = mat3(1.0, 1.0, 1.0,
                       0.0, -0.39465, 2.03211,
                       1.13983, -0.58060, 0.0) * yuv;
            //输出像素颜色
            gl_FragColor = vec4(rgb, 1.0);
        }
);

GLint InitShader(const char *code, GLint type) {
    //创建shader
    GLint sh = glCreateShader(type);
    if (sh == 0) {
        LOGE("glCreateShader %d failed!", type);
        return 0;
    }
    //加载shader
    glShaderSource(sh,
                   1,    //shader数量
                   &code, //shader代码
                   0);   //代码长度
    //编译shader
    glCompileShader(sh);

    //获取编译情况
    GLint status;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        LOGE("glCompileShader failed!");
        return 0;
    }
    LOGE("glCompileShader success!");
    return sh;
}


/**
 * 将数据转换成double类型的一个方法
 * @param r
 * @return
 */
static double r2d(AVRational r) {
    return r.num == 0 || r.den == 0 ? 0 : (double) r.num / (double) r.den;
}

extern "C"
JNIEXPORT void JNICALL
Java_cn_vecrates_ffmpeg_render_FFRender_playVideoByOpenGL(JNIEnv *env, jobject thiz, jstring video_path,
                                                   jobject surface) {


    const char *path = env->GetStringUTFChars(video_path, 0);

    AVFormatContext *fmt_ctx;
// 初始化格式化上下文
    fmt_ctx = avformat_alloc_context();

// 使用ffmpeg打开文件
    int re = avformat_open_input(&fmt_ctx, path, nullptr, nullptr);
    if (re != 0) {
        LOGE("打开文件失败：%s", av_err2str(re));
        return;
    }

//探测流索引
    re = avformat_find_stream_info(fmt_ctx, nullptr);

    if (re < 0) {
        LOGE("索引探测失败：%s", av_err2str(re));
        return;
    }

//寻找视频流索引
    int v_idx = av_find_best_stream(
            fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

    if (v_idx == -1) {
        LOGE("获取视频流索引失败");
        return;
    }
//解码器参数
    AVCodecParameters *c_par;
//解码器上下文
    AVCodecContext *cc_ctx;
//声明一个解码器
    const AVCodec *codec;

    c_par = fmt_ctx->streams[v_idx]->codecpar;

//通过id查找解码器
    codec = avcodec_find_decoder(c_par->codec_id);

    if (!codec) {

        LOGE("查找解码器失败");
        return;
    }

//用参数c_par实例化编解码器上下文，，并打开编解码器
    cc_ctx = avcodec_alloc_context3(codec);

// 关联解码器上下文
    re = avcodec_parameters_to_context(cc_ctx, c_par);

    if (re < 0) {
        LOGE("解码器上下文关联失败:%s", av_err2str(re));
        return;
    }

//打开解码器
    re = avcodec_open2(cc_ctx, codec, nullptr);

    if (re != 0) {
        LOGE("打开解码器失败:%s", av_err2str(re));
        return;
    }

// 获取视频的宽高,也可以通过解码器获取
    AVStream *as = fmt_ctx->streams[v_idx];
    int width = as->codecpar->width;
    int height = as->codecpar->height;

    LOGE("width:%d", width);
    LOGE("height:%d", height);

//数据包
    AVPacket *pkt;
//数据帧
    AVFrame *frame;

//初始化
    pkt = av_packet_alloc();
    frame = av_frame_alloc();

//1 获取原始窗口
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);


///EGL
//1 EGL display创建和初始化
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed!");
        return;
    }
    if (EGL_TRUE != eglInitialize(display, 0, 0)) {
        LOGE("eglInitialize failed!");
        return;
    }
//2 surface
//2-1 surface窗口配置
//输出配置
    EGLConfig config;
    EGLint configNum;
    EGLint configSpec[] = {
        EGL_NONE
    };
    if (EGL_TRUE != eglChooseConfig(display, configSpec, &config, 1, &configNum)) {
        LOGE("eglChooseConfig failed!");
        return;
    }
//创建surface
    EGLSurface winsurface = eglCreateWindowSurface(display, config, nwin, 0);
    if (winsurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed!");
        return;
    }

//3 context 创建关联的上下文
    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed!");
        return;
    }
    if (EGL_TRUE != eglMakeCurrent(display, winsurface, winsurface, context)) {
        LOGE("eglMakeCurrent failed!");
        return;
    }

    LOGE("EGL Init Success!");

//顶点和片元shader初始化
//顶点shader初始化
    GLint vsh = InitShader(vertexShader, GL_VERTEX_SHADER);
//片元yuv420 shader初始化
    GLint fsh = InitShader(fragYUV420P, GL_FRAGMENT_SHADER);


//创建渲染程序
    GLint program = glCreateProgram();
    if (program == 0) {
        LOGE("glCreateProgram failed!");
        return;
    }
//渲染程序中加入着色器代码
    glAttachShader(program, vsh);
    glAttachShader(program, fsh);

//链接程序
    glLinkProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        LOGE("glLinkProgram failed!");
        return;
    }
    glUseProgram(program);
    LOGE("glLinkProgram success!");



//加入三维顶点数据 两个三角形组成正方形
    static float vers[] = {
            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
    };
    GLuint apos = (GLuint) glGetAttribLocation(program, "aPosition");
    glEnableVertexAttribArray(apos);
//传递顶点
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 12, vers);

//加入材质坐标数据
    static float txts[] = {
            1.0f, 0.0f, //右下
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0, 1.0
    };
    GLuint atex = (GLuint) glGetAttribLocation(program, "aTexCoord");
    glEnableVertexAttribArray(atex);
    glVertexAttribPointer(atex, 2, GL_FLOAT, GL_FALSE, 8, txts);

//材质纹理初始化
//设置纹理层
    glUniform1i(glGetUniformLocation(program, "yTexture"), 0); //对于纹理第1层
    glUniform1i(glGetUniformLocation(program, "uTexture"), 1); //对于纹理第2层
    glUniform1i(glGetUniformLocation(program, "vTexture"), 2); //对于纹理第3层

//创建opengl纹理
    GLuint texts[3] = {0};
//创建三个纹理
    glGenTextures(3, texts);

//设置纹理属性
    glBindTexture(GL_TEXTURE_2D, texts[0]);
//缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,           //细节基本 0默认
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图
                 width, height, //拉升到全屏
                 0,             //边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图 要与上面一致
                 GL_UNSIGNED_BYTE, //像素的数据类型
                 NULL                    //纹理的数据
    );

//设置纹理属性
    glBindTexture(GL_TEXTURE_2D, texts[1]);
//缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,           //细节基本 0默认
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图
                 width / 2, height / 2, //拉升到全屏
                 0,             //边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图 要与上面一致
                 GL_UNSIGNED_BYTE, //像素的数据类型
                 NULL                    //纹理的数据
    );

//设置纹理属性
    glBindTexture(GL_TEXTURE_2D, texts[2]);
//缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//设置纹理的格式和大小
    glTexImage2D(GL_TEXTURE_2D,
                 0,           //细节基本 0默认
                 GL_LUMINANCE,//gpu内部格式 亮度，灰度图
                 width / 2, height / 2, //拉升到全屏
                 0,             //边框
                 GL_LUMINANCE,//数据的像素格式 亮度，灰度图 要与上面一致
                 GL_UNSIGNED_BYTE, //像素的数据类型
                 NULL                    //纹理的数据
    );



    //纹理的修改和显示
    unsigned char *buf[3] = {0};
    buf[0] = new unsigned char[width * height];
    buf[1] = new unsigned char[width * height / 4];
    buf[2] = new unsigned char[width * height / 4];


    while (av_read_frame(fmt_ctx, pkt) >= 0) {//持续读帧
// 只解码视频流
        if (pkt->stream_index == v_idx) {

//发送数据包到解码器
            avcodec_send_packet(cc_ctx, pkt);

//清理
            av_packet_unref(pkt);

//这里为什么要使用一个for循环呢？
// 因为avcodec_send_packet和avcodec_receive_frame并不是一对一的关系的
//一个avcodec_send_packet可能会出发多个avcodec_receive_frame
            for (;;) {
// 接受解码的数据
                re = avcodec_receive_frame(cc_ctx, frame);
                if (re != 0) {
                    break;
                } else {

// 解码得到YUV数据

// 数据Y
                    buf[0] = frame->data[0];

                    memcpy(buf[0], frame->data[0], width * height);
// 数据U
                    memcpy(buf[1], frame->data[1], width * height / 4);

// 数据V
                    memcpy(buf[2], frame->data[2], width * height / 4);

//激活第1层纹理,绑定到创建的opengl纹理
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texts[0]);
//替换纹理内容
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE,
                                    GL_UNSIGNED_BYTE, buf[0]);


//激活第2层纹理,绑定到创建的opengl纹理
                    glActiveTexture(GL_TEXTURE0 + 1);
                    glBindTexture(GL_TEXTURE_2D, texts[1]);
//替换纹理内容
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                                    GL_UNSIGNED_BYTE, buf[1]);


//激活第2层纹理,绑定到创建的opengl纹理
                    glActiveTexture(GL_TEXTURE0 + 2);
                    glBindTexture(GL_TEXTURE_2D, texts[2]);
//替换纹理内容
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                                    GL_UNSIGNED_BYTE, buf[2]);

//三维绘制
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//窗口显示
                    eglSwapBuffers(display, winsurface);

                }
            }

        }
    }
//关闭环境
    avcodec_free_context(&cc_ctx);
// 释放资源
    av_frame_free(&frame);
    av_packet_free(&pkt);

    avformat_free_context(fmt_ctx);

    LOGE("播放完毕");

    env->ReleaseStringUTFChars(video_path, path);

}