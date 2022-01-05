//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_DECODER_H
#define VP8_FULL_DUPLEX_CAPN_DECODER_H

#include <memory>
#include <opencv4/opencv2/opencv.hpp>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "../ThreadSafeDeque.h"

class Decoder {
public:
    Decoder(int& width, int& height) {
        codec_context_.reset(new vpx_codec_ctx_t());
        decoder_config_.reset(new vpx_codec_dec_cfg());
        decoder_config_->threads = 2;
        decoder_config_->w = static_cast<unsigned int>(width);
        decoder_config_->h = static_cast<unsigned int>(height);

        long long ret = vpx_codec_dec_init(codec_context_.get(), vpx_codec_vp8_dx(), decoder_config_.get(), 0);

    }

    void decode(const uint8_t* buffer, uint64_t size) {
        assert(codec_context_);
        const vpx_codec_err_t err = vpx_codec_decode(codec_context_.get(), buffer, size, nullptr, 0);
        if (err != VPX_CODEC_OK) {
            //STREAM_LOG_ERROR("Failed to decode frame, VPX error code:%d", err);
            return;
        }
        vpx_codec_iter_t iter = nullptr;
        vpx_image_t* image = nullptr;
        while ((image = vpx_codec_get_frame(codec_context_.get(), &iter)) != nullptr) {
            assert(VPX_IMG_FMT_I420 == image->fmt);

            cv::Mat i420;
            i420.create(cv::Size(image->d_w, image->d_h * 3 / 2), CV_8U);
            unsigned char* dest = i420.data;
            for (int plane = 0; plane < 3; ++plane) {
                const unsigned char *buf = image->planes[plane];
                const int stride = image->stride[plane];
                const int w = (plane ? (image->d_w + 1) >> 1 : image->d_w)
                              * ((image->fmt & VPX_IMG_FMT_HIGHBITDEPTH) ? 2 : 1);
                const int h = plane ? (image->d_h + 1) >> 1 : image->d_h;

                for (int y = 0; y < h; ++y) {
                    memcpy(dest, buf, w);
                    buf += stride;
                    dest += w;
                }
            }
            vpx_img_free(image);

            cv::Mat bgr;
            //cv::cvtColor(i420, bgr, cv::COLOR_YUV2BGR_I420);
            cv::cvtColor(i420, bgr, cv::COLOR_YUV2RGB_I420);
            //frame_queue_.push_back(bgr);
            //delegate_->onFrameDecoded(bgr);
            show_frame(bgr);
        }
    }

    void show_frame(cv::Mat& frame) {
        cv::imshow("Video", frame);
        cv::waitKey(1);
    }


    void display() {
        if (!frame_queue_.empty()) {
            cv::Mat decoded_frame;
            frame_queue_.pop_front_waiting(decoded_frame);
            cv::imshow("Video", decoded_frame);
            cv::waitKey(1);
        }
    }

private:
    ThreadSafeDeque<cv::Mat> frame_queue_{};
    //std::deque<cv::Mat> frame_queue_{};
    std::shared_ptr<vpx_codec_ctx_t> codec_context_;
    std::shared_ptr<vpx_codec_dec_cfg_t> decoder_config_;
};

#endif //VP8_FULL_DUPLEX_CAPN_DECODER_H
