#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "rknn_api.h"

#define MODEL_PATH "/root/focus/model_640_fp16.rknn"
#define MODEL_WIDTH 640
#define MODEL_HEIGHT 640
#define CAPTURE_WIDTH 1280
#define CAPTURE_HEIGHT 720

static volatile int keep_running = 1;
void handle_sigterm(int sig) { keep_running = 0; }

void get_timestamp(char *buf, size_t len) {
    time_t now = time(NULL);
    struct tm *tm_utc = gmtime(&now);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", tm_utc);
}

void resize_rgb(unsigned char *src, int src_w, int src_h,
                unsigned char *dst, int dst_w, int dst_h) {
    float x_ratio = (float)src_w / dst_w;
    float y_ratio = (float)src_h / dst_h;
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            int x1 = (int)src_x;
            int y1 = (int)src_y;
            int x2 = (x1 + 1 < src_w) ? x1 + 1 : x1;
            int y2 = (y1 + 1 < src_h) ? y1 + 1 : y1;
            float dx = src_x - x1;
            float dy = src_y - y1;
            for (int c = 0; c < 3; c++) {
                float p1 = src[(y1 * src_w + x1) * 3 + c];
                float p2 = src[(y1 * src_w + x2) * 3 + c];
                float p3 = src[(y2 * src_w + x1) * 3 + c];
                float p4 = src[(y2 * src_w + x2) * 3 + c];
                float val = p1 * (1-dx) * (1-dy) + p2 * dx * (1-dy) +
                           p3 * (1-dx) * dy + p4 * dx * dy;
                dst[(y * dst_w + x) * 3 + c] = (unsigned char)val;
            }
        }
    }
}

int main(int argc, char **argv) {
    signal(SIGTERM, handle_sigterm);
    signal(SIGINT, handle_sigterm);

    char *rtsp_url = getenv("RTSP_URL");
    if (!rtsp_url)
        rtsp_url = "rtsp://admin:admin123@192.168.1.21:554/stream1";

    rknn_context ctx = 0;
    if (rknn_init(&ctx, MODEL_PATH, 0, 0, NULL) != 0) return 1;

    FILE *out = fopen("/root/focus/result.txt", "a");
    if (!out) { rknn_destroy(ctx); return 1; }

    unsigned char *rgb_buf = malloc(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3);
    unsigned char *resized_buf = malloc(MODEL_WIDTH * MODEL_HEIGHT * 3);
    float *input_buf = malloc(MODEL_WIDTH * MODEL_HEIGHT * 3 * sizeof(float));

    char ffmpeg_cmd[512];
    snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
             "ffmpeg -rtsp_transport tcp -i %s -vf fps=5 -f rawvideo -pix_fmt rgb24 -s %dx%d - 2>/dev/null",
             rtsp_url, CAPTURE_WIDTH, CAPTURE_HEIGHT);

    FILE *ffmpeg = popen(ffmpeg_cmd, "r");
    if (!ffmpeg) {
        free(rgb_buf); free(resized_buf); free(input_buf);
        fclose(out); rknn_destroy(ctx);
        return 1;
    }

    size_t frame_size = CAPTURE_WIDTH * CAPTURE_HEIGHT * 3;

    while (keep_running) {
        size_t n = fread(rgb_buf, 1, frame_size, ffmpeg);
        if (n != frame_size) break;

        resize_rgb(rgb_buf, CAPTURE_WIDTH, CAPTURE_HEIGHT, resized_buf, MODEL_WIDTH, MODEL_HEIGHT);

        for (int i = 0; i < MODEL_WIDTH * MODEL_HEIGHT * 3; i++) {
            input_buf[i] = (float)resized_buf[i] / 255.0f;
        }

        rknn_input inputs[1];
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_FLOAT32;
        inputs[0].size = MODEL_WIDTH * MODEL_HEIGHT * 3 * sizeof(float);
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].buf = input_buf;

        if (rknn_inputs_set(ctx, 1, inputs) == RKNN_SUCC) {
            if (rknn_run(ctx, NULL) == RKNN_SUCC) {
                rknn_output outputs[1];
                memset(outputs, 0, sizeof(outputs));
                outputs[0].want_float = 1;
                outputs[0].is_prealloc = 0;

                if (rknn_outputs_get(ctx, 1, outputs, NULL) == RKNN_SUCC) {
                    float *out_data = (float *)outputs[0].buf;
                    float blur_score = out_data[0];
                    float sharp_score = out_data[1];

                    char timestamp[32];
                    get_timestamp(timestamp, sizeof(timestamp));
                    fprintf(out, "{\"timestamp\":\"%s\",\"sharp\":%.6f,\"blur\":%.6f}\n",
                            timestamp, sharp_score, blur_score);
                    fflush(out);

                    rknn_outputs_release(ctx, 1, outputs);
                }
            }
        }
    }

    pclose(ffmpeg);
    free(rgb_buf);
    free(resized_buf);
    free(input_buf);
    fclose(out);
    rknn_destroy(ctx);
    return 0;
}
