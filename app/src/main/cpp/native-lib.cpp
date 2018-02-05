#include <jni.h>
#include <string>

#ifdef ANDROID

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(>_<)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(>_<) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <android/log.h>
}

char *progress;

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}


JNIEXPORT jstring JNICALL
Java_com_example_lee_ffmpegdemo_MainActivity_progress(JNIEnv *env, jobject instance) {

    // TODO
    return env->NewStringUTF(progress);

}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_lee_ffmpegdemo_MainActivity_test(JNIEnv *env, jobject instance,
                                                  jstring filepath_) {
    const char *input_str = NULL;
    AVFormatContext *formatContext;
    // TODO

//    sprintf(input_str, "%s", env->GetStringUTFChars(filepath_, NULL));

    input_str = env->GetStringUTFChars(filepath_, JNI_FALSE);
    LOGE("input:\t%s", input_str);

    av_register_all();

    formatContext = avformat_alloc_context();

    int i = avformat_open_input(&formatContext, input_str, NULL, NULL);
    if (i == 0) {
        if (formatContext->duration != NULL) {
            LOGE("duration = %lld", formatContext->duration);
        }

        if (formatContext->bit_rate != NULL) {
            LOGE("bit_rate = %lld", formatContext->bit_rate);
        }

        if (formatContext->max_analyze_duration != NULL) {
            LOGE("max_analyze_duration = %lld", formatContext->max_analyze_duration);
        }

        if (formatContext->iformat->name != NULL) {
            LOGE("iformat name= %s", formatContext->iformat->name);
        }
        if (formatContext->metadata != NULL) {

            AVDictionaryEntry *dict = NULL;

            while (dict = av_dict_get(formatContext->metadata, "", dict, AV_DICT_IGNORE_SUFFIX)) {
                LOGE("key = %s \t value = %s", dict->key, dict->value);
            }
        }


        char *filename = formatContext->filename;
        avformat_free_context(formatContext);
        return env->NewStringUTF(filename);
    } else {


        char error[1024] = "";
        av_strerror(i, error, 1024);
        LOGE("%s", error);
        return env->NewStringUTF(error);

    }
}

//    LOGE("duration=%jd bit_rate=%jd max_analyze_duration=%jd", formatContext->duration,
//         formatContext->bit_rate, formatContext->max_analyze_duration);



extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_lee_ffmpegdemo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    char info[40000] = {0};
    avfilter_register_all();
    AVFilter *f_temp = (AVFilter *) avfilter_next(NULL);
    while (f_temp != NULL) {
        sprintf(info, "%s[%10s]\n", info, f_temp->name);
        f_temp = f_temp->next;
    }
    //LOGE("%s", info);


    return env->NewStringUTF(info);
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_example_lee_ffmpegdemo_MainActivity_Transfrom(JNIEnv *env, jobject instance,
                                                       jstring filepath_, jstring output_jstr) {
    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    uint8_t *out_buffer;
    AVPacket *packet;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    FILE *fp_yuv;
    int frame_cnt;
    clock_t time_start, time_finish;
    double time_duration = 0.0;

    char input_str[500] = {0};
    char output_str[500] = {0};
    char info[1000] = {0};
    sprintf(input_str, "%s", env->GetStringUTFChars(filepath_, NULL));
    sprintf(output_str, "%s", env->GetStringUTFChars(output_jstr, NULL));

    LOGE("input_str %s output_str %s", input_str, output_str);
    //FFmpeg av_log() callback
    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0) {

        if (pFormatCtx->duration == NULL) {
            LOGE("Couldn't open input stream.\n");
            return -1;
        }

    }

    LOGE("bit_rate %jd filename %s", pFormatCtx->bit_rate, pFormatCtx->filename);
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("Couldn't find stream information.\n");
        return -1;
    }
    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    if (videoindex == -1) {
        LOGE("Couldn't find a video stream.\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[videoindex]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("Couldn't find Codec.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Couldn't open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    out_buffer = (unsigned char *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
                         AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);


    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, NULL, NULL, NULL);


    sprintf(info, "[Input     ]%s\n", input_str);
    sprintf(info, "%s[Output    ]%s\n", info, output_str);
    sprintf(info, "%s[Format    ]%s\n", info, pFormatCtx->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n", info, pCodecCtx->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx->width, pCodecCtx->height);


    fp_yuv = fopen(output_str, "wb+");
    if (fp_yuv == NULL) {
        printf("Cannot open output file.\n");
        return -1;
    }

    frame_cnt = 0;
    time_start = clock();

    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoindex) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                LOGE("Decode Error.\n");
                return -1;
            }
            if (got_picture) {
                sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize,
                          0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
                //Output info
                char pictype_str[10] = {0};
                switch (pFrame->pict_type) {
                    case AV_PICTURE_TYPE_I:
                        sprintf(pictype_str, "I");
                        break;
                    case AV_PICTURE_TYPE_P:
                        sprintf(pictype_str, "P");
                        break;
                    case AV_PICTURE_TYPE_B:
                        sprintf(pictype_str, "B");
                        break;
                    default:
                        sprintf(pictype_str, "Other");
                        break;
                }
                LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
                frame_cnt++;
            }
        }
        av_free_packet(packet);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0,
                  pCodecCtx->height,
                  pFrameYUV->data, pFrameYUV->linesize);
        int y_size = pCodecCtx->width * pCodecCtx->height;
        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
        //Output info
        char pictype_str[10] = {0};
        switch (pFrame->pict_type) {
            case AV_PICTURE_TYPE_I:
                sprintf(pictype_str, "I");
                break;
            case AV_PICTURE_TYPE_P:
                sprintf(pictype_str, "P");
                break;
            case AV_PICTURE_TYPE_B:
                sprintf(pictype_str, "B");
                break;
            default:
                sprintf(pictype_str, "Other");
                break;
        }
        LOGI("Frame Index: %5d. Type:%s", frame_cnt, pictype_str);
        frame_cnt++;
    }
    time_finish = clock();
    time_duration = (double) (time_finish - time_start);

    sprintf(info, "%s[Time      ]%fms\n", info, time_duration);
    sprintf(info, "%s[Count     ]%d\n", info, frame_cnt);

    sws_freeContext(img_convert_ctx);

    fclose(fp_yuv);

    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_lee_ffmpegdemo_MainActivity_CPrintMessage(JNIEnv *env, jclass type) {

    // TODO
    jclass main_act = env->FindClass("com/example/lee/ffmpegdemo/MainActivity");

    if (main_act == NULL) {
        return;
    }


    jmethodID print = env->GetStaticMethodID(main_act, "printMessage", "(Ljava/lang/String;)V");

    if (print == NULL) {
        return;
    }
    jstring str = env->NewStringUTF("lueluelue");

    jfieldID name = env->GetStaticFieldID(main_act, "name", "Ljava/lang/String;");
    if (name != NULL) {
        jstring newName = env->NewStringUTF("yingyingying");

        env->SetStaticObjectField(main_act, name, newName);
    }


    if (env->ExceptionCheck()) {

        env->ExceptionDescribe();
        LOGE("call method");

    }


    env->CallStaticVoidMethod(main_act, print, str);

    env->DeleteLocalRef(main_act);
//    env->DeleteLocalRef(print);
    env->DeleteLocalRef(str);
//    env->DeleteLocalRef(newName);
}