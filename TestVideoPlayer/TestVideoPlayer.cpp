/****************************************
	Copyright 2018  Mahmoud Galal
	   mahmoudgalal57@yahoo.com
****************************************/

#pragma warning(disable : 4996)

#include <iostream>
#include "VideoPlayer.h"
#include <irrlicht.h>
using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif


IVideoDriver* driver;
IGUIStaticText* textNode;

/// returns file duration with ealpsed time string 
stringw getformatedDuration(long duration,long elapsed) {
	stringw text = L"Duration :";
	text += (duration) / 3600;
	text += L":";
	text += ((duration % 3600) / 60);
	text += L":";
	text += ((duration % 3600) % 60);
	if (elapsed >= 0) {
		text += L"\nElapsed : ";
		text += (elapsed) / 3600;
		text += L":";
		text += (((elapsed) % 3600) / 60);
		text += L":";
		text += (((elapsed) % 3600) % 60);
	}
	return text;
}

int main()
{
	IrrlichtDevice *device =
		createDevice(video::EDT_DIRECT3D9, dimension2d<u32>(640, 480), 24,
			false, false, false, 0);

	if (!device)
		return 1;

	driver = device->getVideoDriver();
	ISceneManager* smgr = device->getSceneManager();
	IGUIEnvironment* guienv = device->getGUIEnvironment();

	smgr->addCameraSceneNode(0, vector3df(0, 30, -40), vector3df(0, 5, 0));

	IMeshSceneNode* cube = smgr->addCubeSceneNode(40, NULL, -1, vector3df(40, -40, 40));
	cube->setMaterialType(E_MATERIAL_TYPE::EMT_SOLID);
	cube->setMaterialFlag(E_MATERIAL_FLAG::EMF_LIGHTING, false);

	textNode = guienv->addStaticText(L"Duration:", recti(10, 10, 100, 50), true);
	textNode->enableOverrideColor(true);
	textNode->setOverrideColor(SColor(255, 0, 0, 200));  

	u32 then = device->getTimer()->getTime();
	u32 firstStartTime = device->getTimer()->getTime();

	bool fileFinished = false;
	/// Create a video player and initialize it
	VideoPlayer vp;
	vp.init("big_buck_bunny.ogv", device);

	while (device->run())
	{
		const u32 now = device->getTimer()->getTime();
		if(!fileFinished)
			vp.decodeFrame();
		//detect if file plyback finished, a weak method to be enhanced later. 
		if (((now - firstStartTime) / 1000) > vp.getVideoDurationInSeconds())
			fileFinished = true;
		//Fetch latest frame Texture
		ITexture* videoFrame = vp.getFrameTexture();
		cube->setMaterialTexture(0, videoFrame);
		
		stringw text = L"Duration :";		

		textNode->setText(getformatedDuration(vp.getVideoDurationInSeconds(), 
			fileFinished?-1:(now - firstStartTime) / 1000).c_str());
		driver->beginScene(true, true, SColor(255, 10, 10, 10));
		//Start Drawing the Video Frame
		{
			if (videoFrame) {
				int swidth = driver->getScreenSize().Width;
				int sheight = driver->getScreenSize().Height;
				int videoHeight = videoFrame->getSize().Height;
				int videoWidth = videoFrame->getSize().Width;
				if (!fileFinished)
					driver->draw2DImage(videoFrame, vector2di((swidth- videoWidth)/2, 
					(sheight - videoHeight) / 2));
			}
			vector3df rot = cube->getRotation();
			rot.Y += 0.2;
			rot.Z += 0.2;
			rot.X += 0.25;
			cube->setRotation(rot);

			smgr->drawAll();
			guienv->drawAll();
		}		
		driver->endScene();

		stringw s = L"VideoPlayerLib - Irrlicht Engine Demo ,FPS:";
		s32 fps = driver->getFPS();
		s += fps;
		device->setWindowCaption(s.c_str());	
	}	

	device->drop();
	return 0;
}
