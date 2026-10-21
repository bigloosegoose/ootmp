#pragma once
#include <iostream>
#include <string>
#include <glad/glad.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
using std::string, std::endl;

AVFormatContext* formatContext = nullptr;
AVCodecContext* codecContext = nullptr;
AVPacket* packet = nullptr;
AVFrame* frame = nullptr;
AVFrame* rgbFrame = nullptr;
uint8_t* rgbBuffer = nullptr;
SwsContext* swsContext = nullptr;
int videoStreamIndex = -1;
double timeBase = 0.0;



unsigned int movieTexture;
bool ended = true;


int openMovie(string videoPath) {
	const char* funcVideoPath = videoPath.c_str();
	if (avformat_open_input(&formatContext, funcVideoPath, nullptr, nullptr) < 0) {
		std::cerr << "Could not open the file " << funcVideoPath << std::endl;
		return -1;
	}
	//find vid stream and index + codec
	if (avformat_find_stream_info(formatContext, nullptr) < 0) {
		std::cerr << "Could not find stream info. " << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	timeBase = av_q2d(formatContext->streams[videoStreamIndex]->time_base);

	const AVCodec* codec = nullptr;
	for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamIndex = i;
			codec = avcodec_find_decoder(formatContext->streams[i]->codecpar->codec_id);
			break;
		}
	}

	if (videoStreamIndex == -1 || !codec) {
		std::cerr << "no valid codec found!" << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}
	//allocate codec context + parameters
	codecContext = avcodec_alloc_context3(codec);
	if (!codecContext) {
		std::cerr << "Could not allocate codec context!" << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	if (avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamIndex]->codecpar) < 0) {
		std::cerr << "Could not copy codec parameters to context!" << std::endl;
		avcodec_free_context(&codecContext);
		avformat_close_input(&formatContext);
		return -1;
	}
	//open codec
	if (avcodec_open2(codecContext, codec, nullptr) < 0) {
		std::cerr << "Failed to open codec!" << std::endl;
		avcodec_free_context(&codecContext);
		avformat_close_input(&formatContext);
		return -1;
	}

	glBindTexture(GL_TEXTURE_2D, movieTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, codecContext->width, codecContext->height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);


	packet = av_packet_alloc();
	frame = av_frame_alloc();

	int frameCounter = 0;

	swsContext = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, 
												   codecContext->width, codecContext->height, AV_PIX_FMT_RGB24, SWS_BILINEAR,
												   nullptr, nullptr, nullptr);

	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 32);
	rgbBuffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

	rgbFrame = av_frame_alloc();
	av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbBuffer, AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);

	ended = false;
	return 0;
}

bool decodeNextFrame() {
	while (true) {
		int r = avcodec_receive_frame(codecContext, frame);
		if (r == 0) return true;
		if (r != AVERROR(EAGAIN)) return false;

		if (av_read_frame(formatContext, packet) < 0) {
			avcodec_send_packet(codecContext, nullptr); 
			continue;
		}

		if (packet->stream_index == videoStreamIndex) { avcodec_send_packet(codecContext, packet); }
		av_packet_unref(packet);
	}
}

void uploadFrame() {
	sws_scale(swsContext, (uint8_t const* const*)frame->data, frame->linesize, 0, codecContext->height, rgbFrame->data, rgbFrame->linesize);
	}