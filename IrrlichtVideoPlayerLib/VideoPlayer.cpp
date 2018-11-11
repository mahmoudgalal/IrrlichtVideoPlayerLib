/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/
#pragma warning(disable : 4996)

#include "VideoPlayer.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <stdio.h>
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif


// Initalizing these to NULL prevents segfaults!
AVFormatContext   *pFormatCtx = NULL;
AVCodecContext    *pCodecCtxOrig = NULL;
AVCodecContext    *pCodecCtx = NULL;
AVCodec           *pCodec = NULL;
AVFrame           *pFrame = NULL;
AVFrame           *pFrameRGB = NULL;
AVPacket          packet;

struct SwsContext *sws_ctx = NULL;


VideoPlayer::VideoPlayer()
{
	frameTexture = NULL;
}

int VideoPlayer::getFrameRate() {
	return framerate;
}

long VideoPlayer::getVideoDurationInSeconds() {
	return videoDuration;
}
/// Copies data from the decoded Frame to the irrlicht image
void writeVideoTexture(AVFrame *pFrame, IImage* imageRt) {
	
	if (pFrame->data[0] == NULL) {
		return;
	}

	int width = imageRt->getDimension().Width;
	int height = imageRt->getDimension().Height;

	//from big Indian to little indian if needed
	int i, temp;
	uint8_t* frameData = pFrame->data[0];
	//for (i = 0; i < width*height * 3; i += 3)
	//{
		//swap R and B; raw_image[i + 1] is G, so it stays where it is.

		//temp = frameData[i + 0];
		//frameData[i + 0] = frameData[i + 2];
		//frameData[i + 2] = temp;
	//}
	unsigned char* data = (unsigned char*)(imageRt->lock());
	//Decoded format is R8G8B8
	memcpy(data, frameData, width * height * 3);
	imageRt->unlock();
}

ITexture* VideoPlayer::getFrameTexture() {
	driver->removeTexture(frameTexture);
	frameTexture = driver->addTexture("tmp", imageRt);
	return  frameTexture;
}

int VideoPlayer::init(const char* filename, IrrlichtDevice *device, bool scaleToScreenWidth) {
	
	this->device = device;
	driver = device->getVideoDriver();

	//Meta-Data,not used
	AVDictionaryEntry *tag = NULL;
	
	// Register all formats and codecs
	av_register_all();

	// Open video file
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return -1; // Couldn't find stream information

	  // Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, filename, 0);
	//Duration of the video file
	videoDuration = (pFormatCtx->duration / AV_TIME_BASE);
	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	  // Get a pointer to the codec context for the video stream
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}
	// Copy context
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1; // Could not open codec

	  // Allocate video frame
	pFrame = av_frame_alloc();

	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -1;
	//saving decoded width,height data
	frameWidth = pCodecCtx->width;
	frameHeight = pCodecCtx->height;

	int width = scaleToScreenWidth?driver->getScreenSize().Width : pCodecCtx->width;
	int height = scaleToScreenWidth ? getNearestHeight(((float)pCodecCtx->width) / pCodecCtx->height, width): 
		pCodecCtx->height;
	// Determine required buffer size and allocate buffer
	numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, width,
		height);
	buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
		width, height);

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height,
		pCodecCtx->pix_fmt,
		width,
		height,
		AV_PIX_FMT_RGB24,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	//Frame Rate of the file
	framerate = ((float)pCodecCtx->framerate.num) / pCodecCtx->framerate.den;
	//Creating the image holding the decoded video frame data
	imageRt = driver->createImage(ECOLOR_FORMAT::ECF_R8G8B8,
		dimension2du(width, height));
	
	avcodec_flush_buffers(pCodecCtx);
	then = device->getTimer()->getTime();
	return 0;
}

int VideoPlayer::decodeFrame() {
	i = 0;
	while (true) {
		now = device->getTimer()->getTime();
		//Decode next frame only if the current finished. Weak sync method ,to be enhanced later
		if ((now - then) >= (1. / (framerate + 1.5f)) * 1000) {
			if ((i = decodeFrameInternal()) != 0) {
				if (frameTexture)
					driver->removeTexture(frameTexture);				
				continue;
			}
			then = now;			
		}
		break;
	}
	return i;
}

int VideoPlayer::decodeFrameInternal() {
	int ret = 0;
	if (ret = av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame?
			if (frameFinished) {
				//pFormatCtx->duration
				// Convert the image from its native format to RGB
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);

				writeVideoTexture(pFrameRGB,imageRt);
				ret = 0;
			}
		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet);
	}
	return ret;
}


/**
Nearest width preserving aspect
*/
int VideoPlayer::getNearestWidth(float aspect, int height) {
	return height * aspect;
}
/**
Nearest Height preserving aspect
*/
int VideoPlayer::getNearestHeight(float aspect, int width) {
	return width * (1. / aspect);
}

int VideoPlayer::getFrameHeight() {
	return frameHeight;
}

int VideoPlayer::getFrameWidth() {
	return frameWidth;
}

VideoPlayer::~VideoPlayer()
{
	//Free all stuff
	if (imageRt)
		imageRt->drop();
	// Free the RGB image
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame
	av_frame_free(&pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);
}
