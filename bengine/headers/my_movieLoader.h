#pragma once
#include <iostream>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
using std::string, std::endl;

AVFormatContext* formatContext = nullptr; //header



int readFrames(string videoPath) {
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

	int videoStreamIndex = -1;
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
	AVCodecContext* codecContext = avcodec_alloc_context3(codec);
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

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	int frameCounter = 0;

	//read loop
	while (av_read_frame(formatContext, packet) >= 0) {
		if (packet->stream_index == videoStreamIndex) {
			int response = avcodec_send_packet(codecContext, packet);

			if (response < 0) {
				std::cerr << "Error sending packet to decoder!" << std::endl;
				break;
			}

			while (response >= 0) {
				response = avcodec_receive_frame(codecContext, frame);
				if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
					break;
				}
				else if (response < 0) {
					std::cerr << "Error receving frame from decoder!" << endl;
					goto cleanup;
				}
			}

		}
		//after decoding frame
		frameCounter++;
		//cout << "Frame " << frameCounter << av_get_picture_type_char(frame->pict_type) << endl;
	
		av_packet_unref(packet);
	}

	cleanup:
	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codecContext);
	avformat_close_input(&formatContext);
	
	return 0;
}

