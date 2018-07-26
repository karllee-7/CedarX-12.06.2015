#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vdecoder.h"
#include <time.h>
#include <iostream>

using std::cout;
using std::endl;

int main()
{
	cout << "decoder demo start." << endl;
	int ret;
	VideoDecoder *vdecoder = CreateVideoDecoder();
	if(vdecoder == NULL){
		cout << "CreateVideoDecoder error." << endl;
		return -1;
	}
	cout << "CreateVideoDecoder success." << endl;
	VideoStreamInfo vinfo;
	vinfo.eCodecFormat = VIDEO_CODEC_FORMAT_H264;
	vinfo.nWidth = 1920;
	vinfo.nHeight = 1080;
	vinfo.nFrameRate = 30;
	vinfo.nFrameDuration = 33;
	vinfo.nCodecSpecificDataLen = 0;
	vinfo.pCodecSpecificData = NULL;
	
	VConfig vconfig;
	vconfig.eOutputPixelFormat = PIXEL_FORMAT_YUV_MB32_420;
	vconfig.nVbvBufferSize = 1920*1080*4;
	
	ret = InitializeVideoDecoder(vdecoder, &vinfo, &vconfig);
	if(ret){
		cout << "InitializeVideoDecoder error." << endl;
		DestroyVideoDecoder(vdecoder);
		return -1;
	}

	cout << "InitializeVideoDecoder success." << endl;

	DestroyVideoDecoder(vdecoder);
	return 0;
}
