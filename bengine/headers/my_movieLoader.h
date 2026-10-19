#pragma once
#include <iostream>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}
using std::string;

AVFormatContext* formatContext = nullptr; //header



int readFrame(string videoPath) {
	const char* funcVideoPath = videoPath.c_str();
	if (avformat_open_input(&formatContext, funcVideoPath, nullptr, nullptr) < 0) {
		std::cerr << "Could not open the file " << funcVideoPath << std::endl;
		return -1;
	}

	if (avformat_find_stream_info(formatContext, nullptr) < 0) {
		std::cerr << "Could not find stream info. " << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	av_dump_format(formatContext, 0, funcVideoPath, 0);

	avformat_close_input(&formatContext);
	return 0;
}
