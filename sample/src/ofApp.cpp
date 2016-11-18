#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(WIDTH, HEIGHT);
	ofSetWindowPosition((ofGetScreenWidth() - ofGetWidth()) / 2, (ofGetScreenHeight() - ofGetHeight()) / 2);
	ofDisableArbTex();
	ofSetFrameRate(60);
	//ofSetVerticalSync(true);


	{
		ofLog(OF_LOG_NOTICE, "application start with resolution: %u x %u", ofGetWidth(), ofGetHeight());

	}
	
	// osvr
	{
		osvr = OpenSourceVirtualReality::create(osvr_identifier);
		osvr->addInterface(osvr_interface_head);
	}

	// allocate fbo
    {
        ofFbo::Settings s;
        s.width = FBO_WIDTH;
		s.height = FBO_HEIGHT;
        s.useDepth = true;
		s.colorFormats = { GL_RGBA8 };
        
        fbo.allocate(s);
    }
    
	// setup gui
    {
		gui.setup();
		gui.setName("gui");

		ofParameterGroup g_settings;
		g_settings.setName("settings");
		g_settings.add(time_step.set("time_step", 1.0f / 120.0f, 0.0f, 1.0f / 30.0f));
		time_step.setSerializable(false);
		g_settings.add(elapsed_time.set("elapsed_time", ofGetElapsedTimef()));
		elapsed_time.setSerializable(false);
		g_settings.add(time_value.set("time_value", 0, 0, 1));
		time_value.setSerializable(false);
		g_settings.add(g_threshold.set("threshold", 0.5f, 0, 1));
		gui.add(g_settings);

		gui.loadFromFile(gui_filename);
    }
	
	reset();
}

//--------------------------------------------------------------
void ofApp::update(){
	ofSetWindowTitle("ofxOSVR sample: " + ofToString(ofGetFrameRate(), 1));

	ofVec3f translation;
	ofQuaternion rotation;
	osvr->getInterfacePose(osvr_interface_head, translation, rotation);
	ofMatrix4x4 model_matrix;
	model_matrix.setTranslation(translation);
	model_matrix.setRotate(rotation);

	
	

	// update params
	updateParameters();
	
	// update main fbo
	{
		fbo.begin();
		auto viewport = ofGetCurrentViewport();
		ofClear(0);

		auto viewers = osvr->getViewers();
		
		for (auto& vi : viewers)
		{
			for (auto& eye : vi.second.eyes)
			{
				auto& this_eye = eye.second;
				for (auto& sf : this_eye.surfaces)
				{
					auto& this_surface = sf.second;
					
					ofPushView();
					ofViewport(this_surface.viewport);
					
					ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
					ofPushMatrix(); // projection matrix

					ofMatrix4x4 proj;
					ofLoadIdentityMatrix();
					ofScale(1.0f, -1.0f, 1.0f);
					ofMultMatrix(this_surface.projection_matrix);
					
					{
						ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
						ofPushMatrix(); // modelview matrix
						ofLoadMatrix(this_eye.modelview_matrix);

						//auto trans = this_eye.modelview_matrix.getInverse().getTranslation();
						//printf("eye trans %f, %f, %f\n", trans.x, trans.y, trans.z);
						//printf("viewport: %f, %f, %f, %f\n", this_surface.viewport.x, this_surface.viewport.y, this_surface.viewport.width, this_surface.viewport.height);

						// draw something
						int num_box = 20;
						float da = TWO_PI / num_box;
						float r = 1.0f * g_threshold;
						for (int i = 0; i < num_box; i++)
						{
							float x = cos(da * i) * r;
							float y = sin(da * i) * r;
							ofDrawBox(x, 0.0f, y, 0.1f);
						}

						ofPopMatrix(); // modelview matrix
					}
					ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
					ofPopMatrix(); // projection matrix

					ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);

					ofPopView();
				}
				
			}
		}


		fbo.end();
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofClear(0);
	auto viewport = ofGetCurrentViewport();
	
	{
		auto rect = getCenteredRect(fbo.getWidth(), fbo.getHeight(), viewport.width, viewport.height, false);
		auto& tex = fbo.getTexture();
		tex.bind();
		drawRectangle(rect);
		tex.unbind();
	}
	
	// draw debug things
	if (is_debug_visible)
	{
		gui.draw();
	}
}

//--------------------------------------------------------------
void ofApp::exit() {
	ofLog(OF_LOG_NOTICE, "application exit");

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){	
	switch (key)
	{
	case OF_KEY_F1:
		is_debug_visible = !is_debug_visible;
		break;
	case OF_KEY_F5:
		reset();
		break;
	case OF_KEY_F11:
		toggleFullscreen(WIDTH, HEIGHT);
		break;
	case 's':
		gui.saveToFile(gui_filename);
		break;
	case 'l':
		gui.loadFromFile(gui_filename);
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

///////////////////////////////////////////////////////////////

void ofApp::reset()
{
	ofLog(OF_LOG_NOTICE, "%s reset", ofGetTimestampString("%H:%M:%S").c_str());

}

void ofApp::updateParameters()
{
	float current_time = ofGetElapsedTimef();
	time_step = ofClamp(current_time - elapsed_time, time_step.getMin(), time_step.getMax());
	elapsed_time = current_time;
	int interval = 100000;
	float value = ofGetElapsedTimeMillis() % interval / float(interval);
	time_value = sin(value * TWO_PI) * 0.5f + 0.5f; // continuous sin value
}

