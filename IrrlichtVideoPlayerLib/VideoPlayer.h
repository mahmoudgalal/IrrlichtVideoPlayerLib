/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/

#pragma once

#include <irrlicht.h>

using namespace irr;
using namespace video;
using namespace core;

class VideoPlayer
{
public:
	VideoPlayer();
	/**   
	 Initialize the Libav video decoding .
	 <param name = "filename"> Video file name, relative or absolute, local or remote(http) </param>
	 <param name = "device"> Irrlicht device</param>
	 <param name = "scaleToScreenWidth"> true to scale the video frame to the device screen width,
	 false will decode to the native width and height of the video</param>
	 
	 <returns>Returns zero in success.</returns>
	*/
	int init(const char* filename  , IrrlichtDevice *device,bool scaleToScreenWidth = true);
	/** 
	 Decodes a single frame of the video file.should be called after init succeed
	 <returns>Returns zero in success.</returns>
	*/
	int decodeFrame();
	/** 
	returns the native width of the decoded video file. could be called after init succeed
	*/
	int getFrameWidth();
	/**
	returns the native Height of the decoded video file. Should be called after init succeed
	*/
	int getFrameHeight();
	/**
	returns the latest decoded frame as irrlicht ITexture
	*/
	ITexture* getFrameTexture();
	/**
	returns the video FPS
	*/
	int getFrameRate();
	/**
	returns the video length in seconds
	*/
	long getVideoDurationInSeconds();
	~VideoPlayer();

private:
	float framerate = 0.0f;
	int   frameFinished;
	int   numBytes ;
	unsigned char * buffer = NULL;
	long videoDuration;
	int                i, videoStream;
	IVideoDriver* driver;
	IrrlichtDevice *device;
	IImage* imageRt;
	ITexture* frameTexture;
	u32 now, then;
	int frameWidth, frameHeight;

	int getNearestWidth(float aspect, int height);
	int getNearestHeight(float aspect, int width);
	int decodeFrameInternal();
	
};

