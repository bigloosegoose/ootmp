#pragma once
#include <iostream>
#include <string>
#include <glad/glad.h>
#include <soloud/soloud.h>
#include <soloud/soloud_wav.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
using std::string, std::endl;

SoLoud::Wav movieAudio;
unsigned int audioHandle = 0;
bool hasAudio = false;

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


int loadAudio(string path);

int openMovie(string videoPath) {
	videoStreamIndex = -1;
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
	timeBase = av_q2d(formatContext->streams[videoStreamIndex]->time_base);

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

	loadAudio(videoPath);
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
	glBindTexture(GL_TEXTURE_2D, movieTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, codecContext->width, codecContext->height,
		GL_RGB, GL_UNSIGNED_BYTE, rgbFrame->data[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
	}

void closeMovie() {
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codecContext);
	avformat_close_input(&formatContext);
	av_freep(&rgbBuffer);
	av_frame_free(&rgbFrame);
	sws_freeContext(swsContext);
	//ended = true;
}

int loadAudio(string path) {
	AVFormatContext* afmt = nullptr;
	avformat_open_input(&afmt, path.c_str(), nullptr, nullptr);
	const AVCodec* acodec = nullptr;
	int aIdx = av_find_best_stream(afmt, AVMEDIA_TYPE_AUDIO, -1, -1, &acodec, 0);

	if (aIdx < 0) {
		avformat_close_input(&afmt);
		return -1; 
	}

	AVCodecContext* acc = avcodec_alloc_context3(acodec);
	avcodec_parameters_to_context(acc, afmt->streams[aIdx]->codecpar);
	avcodec_open2(acc, acodec, nullptr);

	SwrContext* swr = nullptr;
	AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
	swr_alloc_set_opts2(&swr, &stereo, AV_SAMPLE_FMT_FLTP, acc->sample_rate,
		&acc->ch_layout, acc->sample_fmt, acc->sample_rate, 0, nullptr);
	swr_init(swr);

	std::vector<float> left, right;
	AVPacket* ap = av_packet_alloc();
	AVFrame* af = av_frame_alloc();
	while (av_read_frame(afmt, ap) >= 0) {
		if (ap->stream_index == aIdx && avcodec_send_packet(acc, ap) == 0) {
			while (avcodec_receive_frame(acc, af) == 0) {
				size_t old = left.size();
				left.resize(old + af->nb_samples);
				right.resize(old + af->nb_samples);
				uint8_t* out[2] = { (uint8_t*)&left[old], (uint8_t*)&right[old] };
				int n = swr_convert(swr, out, af->nb_samples,
					(const uint8_t**)af->extended_data, af->nb_samples);
				if (n < 0) n = 0;
				left.resize(old + n);
				right.resize(old + n);
			}
		}
		av_packet_unref(ap);
	}

	left.insert(left.end(), right.begin(), right.end());
	movieAudio.loadRawWave(left.data(), left.size() / 2, (float)acc->sample_rate, 2, true);
	hasAudio = true;
}
