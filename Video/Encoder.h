//
// Created by corey on 1/3/22.
//

#ifndef VP8_FULL_DUPLEX_CAPN_ENCODER_H
#define VP8_FULL_DUPLEX_CAPN_ENCODER_H

#include <opencv4/opencv2/opencv.hpp>
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>
#include <cstdio>
#include <memory>
#include <chrono>


struct VideoPacket {
    std::unique_ptr<uint8_t[]> data;
    std::uint64_t size{};
};

class Encoder {
public:

    Encoder(int width, int height, int fps) {
        //VP9E_SET_LOSSLESS
        //assert(!codec_context_);
        vpx_codec_err_t ret;
        //quality_ = FigureCQLevel(config.quality);
        if (!encoder_config_) {
            encoder_config_.reset(new vpx_codec_enc_cfg());
            ret = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), encoder_config_.get(), 0);
            if (ret) {
                //STREAM_LOG_ERROR("Failed to get default encoder configuration. Error No.: %d", ret);
                std::cout << "Failed to get default encoder configuration" << std::endl;
            }
        }
        encoder_config_->rc_end_usage = VPX_Q;
        // encoder_config_->rc_target_bitrate = config.target_bitrate;
        encoder_config_->g_timebase.num = 1;
        encoder_config_->g_timebase.den = fps;
        encoder_config_->rc_min_quantizer = quality_;
        encoder_config_->rc_max_quantizer = quality_;
        // Keyframe configurations
        //keyframe_forced_interval_ = config.keyframe_forced_interval;

        if (codec_context_) {
            ret = vpx_codec_enc_config_set(codec_context_.get(), encoder_config_.get());
            if (ret) {
                //STREAM_LOG_ERROR("Failed to update codec configuration. Error No.:%d", ret);
                std::cout << "Failed to update codec configuration. Error" << std::endl;
            }
            ret = vpx_codec_control_(codec_context_.get(), VP8E_SET_CQ_LEVEL, quality_);
            if (ret) {
                //STREAM_LOG_ERROR("Failed to set CQ_LEVEL. Error No.:%d", ret);
                std::cout << "Failed to set CQ_LEVEL" << std::endl;
            }
        }

        assert(!codec_context_);

        encoder_config_->g_w = width;
        encoder_config_->g_h = height;
        codec_context_.reset(new vpx_codec_ctx_t());
        ret = vpx_codec_enc_init(codec_context_.get(), vpx_codec_vp8_cx(), encoder_config_.get(), 0);
        if (ret) {
            std::cout << "Failed to initialize VPX encoder" << std::endl;
            //STREAM_LOG_ERROR("Failed to initialize VPX encoder. Error No.:%d", ret);
            //return false;
        }
        ret = vpx_codec_control_(codec_context_.get(), VP8E_SET_CQ_LEVEL, quality_);
        //return true;

    }

    std::shared_ptr<VideoPacket> encode(cv::Mat& mat) {

        std::shared_ptr<VideoPacket> packet;

        // Convert image to i420 color space used by vpx
        cv::Mat i420;
        cv::cvtColor(mat, i420, cv::COLOR_RGB2YUV_I420);

        vpx_image_t image;

        if (!vpx_img_wrap(&image, VPX_IMG_FMT_I420, encoder_config_->g_w,
                          encoder_config_->g_h, 1, i420.data)) {
            //STREAM_LOG_ERROR("Failed to wrap cv::Mat into vpx image.");
            return packet;
        }

        int flags = 0;
        //if (frame_count_ % keyframe_forced_interval_ == 0)
        //    flags |= VPX_EFLAG_FORCE_KF;

        vpx_codec_iter_t iter = nullptr;
        const vpx_codec_cx_pkt_t *pkt = nullptr;

        const vpx_codec_err_t ret = vpx_codec_encode(codec_context_.get(), &image,
                                                     0, 1, flags, VPX_DL_REALTIME);

        if (ret != VPX_CODEC_OK) {
            return packet;
        }

        packet = std::make_shared<VideoPacket>();
        packet->data = std::make_unique<uint8_t[]>((encoder_config_->g_h * 1.5) * encoder_config_->g_w);

        while ((pkt = vpx_codec_get_cx_data(codec_context_.get(), &iter))) {
            if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
                //*size = pkt->data.frame.sz;
                // might be the total size
                packet->size += pkt->data.frame.sz;

                std::memcpy(packet->data.get(), pkt->data.frame.buf, pkt->data.frame.sz);

                bool key_frame = pkt->data.frame.flags & VPX_FRAME_IS_KEY;
                //printf(key_frame ? "K" : ".");
            }
        }
        //-std::cout << packet->size << std::endl;
        return packet;
    }

private:
    std::shared_ptr<vpx_codec_ctx_t> codec_context_;
    std::shared_ptr<vpx_codec_enc_cfg_t> encoder_config_;
    uint64_t frame_count_{};
    std::chrono::high_resolution_clock::time_point start_time_;
    uint32_t keyframe_forced_interval_{};
    uint32_t quality_{};
};

#endif //VP8_FULL_DUPLEX_CAPN_ENCODER_H
