#pragma once

#include "ofMain.h"
#include "ofxAtem.h"

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	
private:
	ofxAtem::Device atem;
	int currentProgramIndex, currentPreviewIndex;
};
