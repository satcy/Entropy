#include "ofApp.h"
#include "H5Cpp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetLogLevel(OF_LOG_NOTICE);
    ofDisableArbTex();
//    ofSetDataPathRoot("../Resources/data/");
    ofBackground(ofColor::black);

    // Set default parameters.
    bGuiVisible = true;
    bMouseOverGui = false;

    stride = 1;

    bNeedsIndices = true;
    bRender3D = true;
    pointSize = 1.5f;
    densityMin = 0.0f;
    densityMax = 0.0015f;
    scale = 1024.0f;

    bNeedsBins = true;
    bCycleBins = false;
    bExportFiles = false;
    binPower = 8;
    binIndex = 125;
    renderIndex = -1;
    pointAdjust = 2.55;
    bBinDebug2D = false;
    bBinDebug3D = true;

    vector<float> posX, posY, posZ;
    loadDataSet("x.h5", posX, 1, true);
    loadDataSet("y.h5", posY, 1, true);
    loadDataSet("z.h5", posZ, 1, true);
    loadDataSet("dx.h5", cellSize, 1, false);
    loadDataSet("density.h5", density, 1, false);

    for (int i = 0; i < posX.size(); ++i) {
        coordRange.add(ofVec3f(posX[i], posY[i], posZ[i]));
        cellSizeRange.add(cellSize[i]);
        densityRange.add(density[i]);
    }

    // Take cell size into account for coord range.
    coordRange.add(coordRange.getMin() - cellSizeRange.getMax());
    coordRange.add(coordRange.getMax() + cellSizeRange.getMax());

    // Find the dimension with the max span, and set all spans to be the same (since we're rendering a cube).
    ofVec3f coordSpan = coordRange.getSpan();
    float maxSpan = MAX(coordSpan.x, MAX(coordSpan.y, coordSpan.z));
    ofVec3f spanOffset(maxSpan * 0.5);
    ofVec3f coordMid = coordRange.getMid();
    coordRange.add(coordMid - spanOffset);
    coordRange.add(coordMid + spanOffset);

    // Set normalization values to remap to [-0.5, 0.5]
    coordSpan = coordRange.getSpan();
    originShift = -0.5 * coordSpan - coordRange.getMin();

    normalizeFactor = MAX(MAX(coordSpan.x, coordSpan.y), coordSpan.z);

    // Upload all data to the VBO.
    vboMesh.getVertices().resize(posX.size());
    for (int i = 0; i < posX.size(); ++i) {
        ofVec3f v(posX[i], posY[i], posZ[i]);
        vboMesh.setVertex(i, v);
    }
    vboMesh.getVbo().setAttributeData(CELLSIZE_ATTRIBUTE, cellSize.data(), 1, cellSize.size(), GL_STATIC_DRAW, 0);
    vboMesh.getVbo().setAttributeData(DENSITY_ATTRIBUTE, density.data(), 1, density.size(), GL_STATIC_DRAW, 0);

    // Load the shaders.
    renderShader.setupShaderFromFile(GL_VERTEX_SHADER, "shaders/render.vert");
    renderShader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shaders/render.frag");
    renderShader.bindAttribute(CELLSIZE_ATTRIBUTE, "cellSize");
    renderShader.bindAttribute(DENSITY_ATTRIBUTE, "density");
    renderShader.bindDefaults();
    renderShader.linkProgram();

    sliceShader.setupShaderFromFile(GL_VERTEX_SHADER, "shaders/slice.vert");
    sliceShader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shaders/slice.frag");
    sliceShader.bindAttribute(CELLSIZE_ATTRIBUTE, "cellSize");
    sliceShader.bindAttribute(DENSITY_ATTRIBUTE, "density");
    sliceShader.bindDefaults();
    sliceShader.linkProgram();
}

//--------------------------------------------------------------
void ofApp::loadDataSet(const string& filename, vector<float>& data, int stride, bool bExponential)
{
    ofxHDF5File h5File;
    h5File.open(filename, true);
    ofLogVerbose() << "File '" << filename << "' has " << h5File.getNumDataSets() << " datasets";

    for (int i = 0; i < h5File.getNumDataSets(); ++i) {
        ofLogVerbose() << "  DataSet " << i << ": " << h5File.getDataSetName(i);
    }
    string dataSetName = h5File.getDataSetName(0);
    ofxHDF5DataSetPtr dataSet = h5File.loadDataSet(dataSetName);

    int count = dataSet->getDimensionSize(0) / stride;
    dataSet->setHyperslab(0, count, stride);
    data.resize(count);

    // Data is 64-bit.
    if (bExponential) {
        // Load it in a temp double array.
        double *rawData = new double[count];
        dataSet->read(rawData);

        // Set the return array from the transformed data.
        for (int i = 0; i < count; ++i) {
            data[i] = powf(10, rawData[i]);
        }

        delete [] rawData;
    }
    else {
        // Read it directly, losing precision.
        dataSet->read(data.data(), H5_DATATYPE_FLOAT);
    }
}

//--------------------------------------------------------------
void ofApp::rebuildIndices()
{
    vboMesh.clearIndices();
    for (int i = 0; i < vboMesh.getNumVertices(); i += stride) {
        vboMesh.addIndex(i);
    }
    bNeedsIndices = false;
}

//--------------------------------------------------------------
void ofApp::rebuildBins()
{
    // Build bins by depth.
    binSizeX = binSizeY = binSizeZ = pow(2, binPower);
    binSliceZ = (coordRange.getSpan().z) / binSizeZ;

    ofLogNotice() << "Bin size is [" << binSizeX << "x" << binSizeY << "x" << binSizeZ << "] with slice " << binSliceZ;

    // Allocate FBO.
    binFbo.allocate(binSizeX, binSizeY);
//    binIndex = 0;
    renderIndex = -1;

    bNeedsBins = false;
}

//--------------------------------------------------------------
void ofApp::imGui()
{
    static const int kGuiMargin = 10;

    gui.begin();
    {
        ImGui::SetNextWindowPos(ofVec2f(kGuiMargin, kGuiMargin), ImGuiSetCond_Appearing);
        ImGui::SetNextWindowSize(ofVec2f(380, 380), ImGuiSetCond_Appearing);
        if (ImGui::Begin("VAPOR", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%.1f FPS (%.3f ms/frame)", ofGetFrameRate(), 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("%lu Indices / %lu Vertices", vboMesh.getNumIndices(), vboMesh.getNumVertices());

            if (ImGui::CollapsingHeader("Data", nullptr, true, true)) {
                if (ImGui::SliderInt("Stride", &stride, 1, 128)) {
                    bNeedsIndices = true;
                }
                if (ImGui::DragFloatRange2("Density Range", &densityMin, &densityMax, 0.0001f, 0.0f, 1.0f, "Min: %.4f%%", "Max: %.4f%%")) {
                    renderIndex = -1;
                }
            }

            if (ImGui::CollapsingHeader("3D", nullptr, true, true)) {
                ImGui::Checkbox("Render", &bRender3D);
                ImGui::SliderFloat("Scale", &scale, 1.0f, 2048.0f);
                ImGui::SliderFloat("Point Size", &pointSize, 0.1f, 64.0f);
            }

            if (ImGui::CollapsingHeader("Export", nullptr, true, true)) {
                if (ImGui::SliderInt("Bin Power", &binPower, 0, 10)) {
                    bNeedsBins = true;
                }
                ImGui::SliderInt("Bin Index", &binIndex, 0, binSizeZ - 1);
                ImGui::Checkbox("Cycle Bins", &bCycleBins);
                if (ImGui::Checkbox("Export Files", &bExportFiles)) {
                    binIndex = 0;
                    renderIndex = -1;
                    bCycleBins = true;
                }
                if (ImGui::SliderFloat("Point Adjust", &pointAdjust, 0.0f, 5.0f)) {
                    renderIndex = -1;
                }
                ImGui::Checkbox("Debug 3D", &bBinDebug3D);
                if (ImGui::TreeNode("Debug 2D")) {
                    bBinDebug2D = true;
                    ImGui::Image((ImTextureID)(uintptr_t)binFbo.getTexture().getTextureData().textureID, ofVec2f(binSizeX, binSizeY));
                    ImGui::TreePop();
                }
                else {
                    bBinDebug2D = false;
                }
            }
        }
        ImGui::End();

        ofVec2f windowPos = ImGui::GetWindowPos();
        ofVec2f windowSize = ImGui::GetWindowSize();
        ofRectangle windowBounds(windowPos, windowSize.x, windowSize.y);
        bMouseOverGui = windowBounds.inside(ofGetMouseX(), ofGetMouseY());
    }
    gui.end();
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (bNeedsIndices) {
        rebuildIndices();
    }
    if (bNeedsBins) {
        rebuildBins();
    }

    ofEnablePointSprites();

    if ((bBinDebug2D || bExportFiles) && binIndex != renderIndex) {
        float minDepth = coordRange.getMin().z + binIndex * binSliceZ;
        float maxDepth = minDepth + binSliceZ;

        ofLogNotice() << "Rendering bin " << binIndex << " to texture.";
        binFbo.begin();
        {
            ofClear(0, 0);
            ofSetColor(ofColor::white);
            glPointSize(1.0);

            sliceShader.begin();
            sliceShader.setUniform1f("pointAdjust", pointAdjust);
            sliceShader.setUniform3f("minCoord", coordRange.getMin());
            sliceShader.setUniform3f("maxCoord", coordRange.getMax());
            sliceShader.setUniform1f("binSizeX", binSizeX);
            sliceShader.setUniform1f("binSizeY", binSizeY);
            sliceShader.setUniform1f("minDepth", minDepth);
            sliceShader.setUniform1f("maxDepth", maxDepth);
//                sliceShader.setUniform1f("minDensity", densityRange.getMin());
//                sliceShader.setUniform1f("maxDensity", densityRange.getMax());
            sliceShader.setUniform1f("minDensity", densityMin * densityRange.getSpan());
            sliceShader.setUniform1f("maxDensity", densityMax * densityRange.getSpan());
            {
                vboMesh.getVbo().drawElements(GL_POINTS, vboMesh.getNumIndices());
            }
            sliceShader.end();
        }
        binFbo.end();

        renderIndex = binIndex;

        if (bExportFiles) {
            ofLogVerbose() << "Saving texture " << binIndex << " to disk.";
            binFbo.readToPixels(binPixels);
            ofSaveImage(binPixels, "vapor/texture_" + ofToString(binIndex, 4, '0') + ".png");
        }
    }

    if (bMouseOverGui) {
        cam.disableMouseInput();
    }
    else {
        cam.enableMouseInput();
    }
    bMouseOverGui = false;
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (bRender3D) {
        ofSetColor(ofColor::white);
        glPointSize(1.0);

        cam.setNearClip(0);
        cam.setFarClip(FLT_MAX);
        cam.begin();
        {
            ofPushMatrix();

            ofScale(scale / normalizeFactor, scale / normalizeFactor, scale / normalizeFactor);
            ofTranslate(originShift);
            {
                renderShader.begin();
                renderShader.setUniform1f("pointSize", pointSize / cellSizeRange.getMin());
                renderShader.setUniform1f("densityMin", densityMin * densityRange.getSpan());
                renderShader.setUniform1f("densityMax", densityMax * densityRange.getSpan());
                if (bBinDebug3D) {
                    renderShader.setUniform1f("debugMin", coordRange.getMin().z + binIndex * binSliceZ);
                    renderShader.setUniform1f("debugMax", coordRange.getMin().z + (binIndex + 1) * binSliceZ);
                }
                else {
                    renderShader.setUniform1f("debugMin", FLT_MAX);
                    renderShader.setUniform1f("debugMax", FLT_MIN);
                }
                {
                    vboMesh.getVbo().drawElements(GL_POINTS, vboMesh.getNumIndices());
                }
                renderShader.end();
            }
            ofPopMatrix();

            ofDrawAxis(20);
        }
        cam.end();
    }

    if (bCycleBins) {
        binIndex = (binIndex + 1) % binSizeZ;

        if (bExportFiles && binIndex == 0) {
            bExportFiles = false;
            bCycleBins = false;
        }
    }

    if (bGuiVisible) {
        imGui();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
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
