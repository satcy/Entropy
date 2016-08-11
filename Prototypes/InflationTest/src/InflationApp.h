#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "NoiseField.h"
#include "GPUMarchingCubes.h"
#include "ofxTextureRecorder.h"
#include "Constants.h"

namespace entropy
{
    class InflationApp : public ofBaseApp
    {
    public:
        void setup();
        void update();
        void draw();

        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);

        ofEasyCam camera;
        double now;

        GPUMarchingCubes gpuMarchingCubes;

        ofParameter<float> scale{"scale", 1, 1, 100};
        ofParameter<float> threshold{"threshold", 0.345, 0.0, 1.0};
        ofParameter<bool> inflation{"inflation", false};
        ofParameter<bool> flipNormals{"flip normals", false};
        ofParameterGroup marchingCubesParameters{
            "marching cubes",
            gpuMarchingCubes.resolution,
            scale,
            threshold,
            inflation,
        };
        ofxPanel panelMarchingCubes{marchingCubesParameters, "marching-cubes.json"};

        // Noise Field
		NoiseField noiseField;
        ofxPanel panelNoiseField{noiseField.parameters, "noise-field.json"};


        // Render
        ofParameter<bool> debug{"debug", false};
        ofParameter<bool> drawGrid{"draw grid", true};
        ofParameter<bool> simulationRunning{"simulation running", true};
        ofParameter<bool> record{"record",false};
        ofParameter<bool> additiveBlending{"additive blending",false};
        ofParameter<bool> bloom{"bloom",true};

        ofParameter<float> brightThres{"bright thresh.",1,0.5f,3};
        ofParameter<float> sigma{"sigma",0.9,0.5f,18};
        ofParameter<int> blurPasses{"blur passes", 1, 1, 5};
        ofParameter<float> contrast{"contrast",1,0.5f,1.5f};
        ofParameter<float> brightness{"brightness",0,-1.f,1.f};
        ofParameter<int> tonemapType{"tonemap",0,0,5};
        ofParameterGroup bloomParameters{
            "bloom parameters",
            brightThres,
            sigma,
            blurPasses,
            contrast,
            brightness,
            tonemapType,
        };

        ofParameterGroup paramsRender{
            "render",
            debug,
            drawGrid,
            gpuMarchingCubes.wireframe,
            gpuMarchingCubes.shadeNormals,
            simulationRunning,
            record,
            additiveBlending,
            bloom,
            bloomParameters,
        };
        ofxPanel panelRender{paramsRender, "render.json"};


        // GUI
        bool guiVisible = true;
		uint64_t timeToSetIso;
		uint64_t timeToUpdate;


        // Postpo effects and recording
        ofFbo fboscene;
        ofFbo fbobright;
        ofFbo fbo2;
        ofFbo finalFbo;
        ofShader shaderBright;
        ofShader blurV;
        ofShader blurH;
        ofShader tonemap;

        ofxTextureRecorder saverThread;
    };
}
