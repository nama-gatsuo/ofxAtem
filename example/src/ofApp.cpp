#include "ofApp.h"


void ofApp::setup() {
	
	ofSetFrameRate(30);
	
	// Find IP address of your device from Atem Software Controll etc.
	atem.connect("192.168.10.240");
	atem.printInfo();
	
	currentProgramIndex = atem.getProgramIndex();
	currentPreviewIndex = atem.getPreviewIndex();
}

void ofApp::update(){
	
	currentProgramIndex = atem.getProgramIndex();
	currentPreviewIndex = atem.getPreviewIndex();
}

void ofApp::draw(){
	
	ofDrawBitmapStringHighlight(atem.getProductName(), 10, 20);
	ofDrawBitmapString("switch input:\tarrow-up, arrow-down\nswitch target:\tarrow-left, arrow-right\n", 10, 60);

	std::string s = "";
	
	for (const auto& input : atem.getInputMap()) {
		
		if (input->index == currentProgramIndex) {
			s += "[program]";
		} else {
			s += "         ";
		}

		if (input->index == currentPreviewIndex) {
			s += "[preview]";
		} else {
			s += "         ";
		}

		s += " ---- ";
		s += input->longName + " - ";
		s += input->portType + "\n";
		
	}

	ofDrawBitmapString(s, 10, 120);
	
	
}

void ofApp::exit() {
	atem.disconnect();
}

void ofApp::keyPressed(int key){
	
	static int currentTarget = 0;

	if (key == OF_KEY_DOWN) {
		
		if (currentTarget == 0) {
			currentProgramIndex++;
			currentProgramIndex %= atem.getInputMap().size();
			atem.setProgramByIndex(currentProgramIndex);
		} else {
			currentPreviewIndex++;
			currentPreviewIndex %= atem.getInputMap().size();
			atem.setPreviewByIndex(currentPreviewIndex);
		}

	} else if (key == OF_KEY_UP) {

		if (currentTarget == 0) {
			currentProgramIndex += atem.getInputMap().size() - 1;
			currentProgramIndex %= atem.getInputMap().size();
			atem.setProgramByIndex(currentProgramIndex);
		} else {
			currentPreviewIndex += atem.getInputMap().size() - 1;
			currentPreviewIndex %= atem.getInputMap().size();
			atem.setPreviewByIndex(currentPreviewIndex);
		}

	} else if (key == OF_KEY_RIGHT || key == OF_KEY_LEFT) {
		currentTarget = 1 - currentTarget;
	}

}


