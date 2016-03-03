#include "CMBApp.h"

namespace entropy
{
    //--------------------------------------------------------------
    void CMBApp::setup()
    {
        ofSetLogLevel(OF_LOG_VERBOSE);

        tintColor = ofColor::white;
        dropColor = ofColor::white;

        bDropOnPress = false;
        bDropUnderMouse = false;
        dropRate = 1;

        damping = 0.995f;
        radius = 10.0f;
        ringSize = 0.5f;

        bRestart = true;
        bGuiVisible = true;

        openCL.setupFromOpenGL();

        openCL.loadProgramFromFile("cl/ripples.cl");
        dropKernel = openCL.loadKernel("drop2D");
        ripplesKernel = openCL.loadKernel("ripples2D");
        copyKernel = openCL.loadKernel("copy2D");

        shader.load("shaders/ripples");
    }

    //--------------------------------------------------------------
    void CMBApp::restart()
    {
        activeIndex = 0;

        dimensions.x = ofGetWidth();
        dimensions.y = ofGetHeight();

        // Allocate the FBOs.
        ofFbo::Settings fboSettings;
        fboSettings.width = dimensions.x;
        fboSettings.height = dimensions.y;
        fboSettings.internalformat = GL_RGBA32F;

        for (int i = 0; i < 2; ++i) {
//            fbos[i].allocate(fboSettings);
//            fbos[i].begin();
//            {
//                ofClear(0, 0);
//            }
//            fbos[i].end();

//            clImages[i].initFromTexture(fbos[i].getTexture());
            clImages[i].initWithTexture(dimensions.x, dimensions.y, GL_RGBA32F);
        }
        clImageTmp.initWithTexture(dimensions.x, dimensions.y, GL_RGBA32F);

        // Build a mesh to render a quad.
//        mesh.clear();
//        mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
//        mesh.addVertex(ofVec3f(0, 0));
//        mesh.addVertex(ofVec3f(width, 0));
//        mesh.addVertex(ofVec3f(width, height));
//        mesh.addVertex(ofVec3f(0, height));
//        mesh.addTexCoord(ofVec2f(0, 0));
//        mesh.addTexCoord(ofVec2f(width, 0));
//        mesh.addTexCoord(ofVec2f(width, height));
//        mesh.addTexCoord(ofVec2f(0, height));

        bRestart = false;
    }

    //--------------------------------------------------------------
    void CMBApp::update()
    {
        if (bRestart || dimensions.x != ofGetWidth() || dimensions.y != ofGetHeight()) {
            restart();
        }

        // Add new drops.
//        ofPushStyle();
//        ofPushMatrix();

        int srcIdx = (activeIndex + 1) % 2;
        int dstIdx = activeIndex;

//        fbos[srcIdx].begin();
//        {
//            bool bMousePressed = ofGetMousePressed() && !bMouseOverGui;
//            if ((bDropOnPress && bMousePressed) || (!bDropOnPress && ofGetFrameNum() % dropRate == 0)) {
//                ofSetColor(dropColor);
//                ofNoFill();
//                if (bDropUnderMouse) {
//                    ofDrawCircle(ofGetMouseX(), ofGetMouseY(), radius);
//                }
//                else {
//                    ofDrawCircle(ofRandomWidth(), ofRandomHeight(), radius);
//                }
//            }
//        }
//        fbos[srcIdx].end();

//        ofPopMatrix();
//        ofPopStyle();

        // Add new drops.
        bool bMousePressed = ofGetMousePressed() && !bMouseOverGui;
        if ((bDropOnPress && bMousePressed) || (!bDropOnPress && ofGetFrameNum() % dropRate == 0)) {
            dropKernel->setArg(0, clImages[srcIdx]);
            dropKernel->setArg(1, bDropUnderMouse? ofVec2f(ofGetMouseX(), ofGetMouseY()) : ofVec2f(ofRandomWidth(), ofRandomHeight()));
            dropKernel->setArg(2, radius);
            dropKernel->setArg(3, ringSize);
            dropKernel->setArg(4, dropColor);
            dropKernel->run2D(dimensions.x, dimensions.y);
        }


        // Layer the drops.
//        fbos[dstIdx].begin();
//        shader.begin();
//        shader.setUniformTexture("uPrevBuffer", fbos[dstIdx], 1);
//        shader.setUniformTexture("uCurrBuffer", fbos[srcIdx], 2);
//        shader.setUniform1f("uDamping", damping / 10.0f + 0.9f);  // 0.9 - 1.0 range
//        {
//            mesh.draw();
//        }
//        shader.end();
//        fbos[dstIdx].end();

        // Layer the drops.
        ripplesKernel->setArg(0, clImages[srcIdx]);
        ripplesKernel->setArg(1, clImages[dstIdx]);
        ripplesKernel->setArg(2, clImageTmp);
        ripplesKernel->setArg(3, damping / 10.0f + 0.9f);  // 0.9 - 1.0 range
        ripplesKernel->run2D(dimensions.x, dimensions.y);

        // Copy temp image to dest (necessary since we can't read_write in OpenCL 1.2)
        copyKernel->setArg(0, clImageTmp);
        copyKernel->setArg(1, clImages[dstIdx]);
        copyKernel->run2D(dimensions.x, dimensions.y);
    }

    //--------------------------------------------------------------
    void CMBApp::imGui()
    {
        static const int kGuiMargin = 10;

        gui.begin();
        {
            ofVec2f windowPos(kGuiMargin, kGuiMargin);
            ofVec2f windowSize = ofVec2f::zero();

            ImGui::SetNextWindowPos(windowPos, ImGuiSetCond_Appearing);
            ImGui::SetNextWindowSize(ofVec2f(380, 94), ImGuiSetCond_Appearing);
            if (ImGui::Begin("CMB", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%.1f FPS (%.3f ms/frame)", ofGetFrameRate(), 1000.0f / ImGui::GetIO().Framerate);

                ImGui::Checkbox("Restart", &bRestart);

                ImGui::ColorEdit3("Tint Color", &tintColor[0]);
                ImGui::ColorEdit3("Drop Color", &dropColor[0]);

                ImGui::Checkbox("Drop On Press", &bDropOnPress);
                ImGui::Checkbox("Drop Under Mouse", &bDropUnderMouse);

                ImGui::SliderInt("Drop Rate", &dropRate, 1, 60);
                ImGui::SliderFloat("Damping", &damping, 0.0f, 1.0f);
                ImGui::SliderFloat("Radius", &radius, 1.0f, 50.0f);
                ImGui::SliderFloat("Ring Size", &ringSize, 0.0f, 5.0f);

                windowSize.set(ImGui::GetWindowSize());
                ImGui::End();
            }

            ofRectangle windowBounds(windowPos, windowSize.x, windowSize.y);
            bMouseOverGui = windowBounds.inside(ofGetMouseX(), ofGetMouseY());
        }
        gui.end();
    }

    //--------------------------------------------------------------
    void CMBApp::draw()
    {
        ofSetColor(255);

        //openCL.finish();

        ofPushStyle();
        ofSetColor(tintColor);
        ofEnableAlphaBlending();
//        fbos[activeIndex].draw(0, 0);
        clImages[activeIndex].getTexture().draw(0, 0);
        ofPopStyle();

        activeIndex = 1 - activeIndex;

        if (bGuiVisible) {
            imGui();
        }
    }

    //--------------------------------------------------------------
    void CMBApp::keyPressed(int key)
    {
        switch (key) {
            case '`':
                bGuiVisible ^= 1;
                break;

            case OF_KEY_TAB:
                ofToggleFullscreen();
                break;

            default:
                break;
        }
    }

    //--------------------------------------------------------------
    void CMBApp::keyReleased(int key){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseMoved(int x, int y ){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseDragged(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mousePressed(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseReleased(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseEntered(int x, int y){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseExited(int x, int y){

    }

    //--------------------------------------------------------------
    void CMBApp::windowResized(int w, int h)
    {
        bRestart = true;
    }

    //--------------------------------------------------------------
    void CMBApp::gotMessage(ofMessage msg){
        
    }
    
    //--------------------------------------------------------------
    void CMBApp::dragEvent(ofDragInfo dragInfo){ 
        
    }
}
