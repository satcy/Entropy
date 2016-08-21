#pragma once

#include "ofParameter.h"
#include "ofxPreset.h"

namespace entropy
{
	namespace render
	{
		struct PostParameters
			: ofParameterGroup
		{
			struct : ofParameterGroup
			{
				ofParameter<bool> enabled{ "Enabled", true };
				ofParameter<int> numPasses{ "Num Passes", 1, 1, 10 };
				ofParameter<float> brightnessThreshold{ "Brightness Threshold", 1.0f, 0.01f, 3.0f };
				ofParameter<float> sigma{ "Sigma", 0.9f, 0.5f, 18.0f };
				ofParameter<bool> debugBlur{ "Debug blur", false };

				PARAM_DECLARE("Bloom", enabled, numPasses, brightnessThreshold, sigma, debugBlur);
			} bloom;

			struct : ofParameterGroup
			{
				ofParameter<float> exposure{ "Exposure", 4.0f, 0.1f, 10.0f };
				ofParameter<float> gamma{ "Gamma", 2.2f, 0.01f, 10.0f };
				ofParameter<int> tonemapping{ "Tonemapping", 6, 0, 6 };
				ofParameter<float> contrast{ "Contrast", 1.0f, 0.5f, 1.5f };
				ofParameter<float> brightness{ "Brightness", 0.0f, -1.0f, 1.0f };

				PARAM_DECLARE("Color", exposure, gamma, tonemapping, contrast, brightness);
			} color;

			PARAM_DECLARE("Post Effects", bloom, color);
		};
		
		class PostEffects
		{
		public:
			PostEffects();
			~PostEffects();

			void process(const ofTexture & srcTexture, ofFbo & dstFbo, PostParameters & parameters);

			void resize(int width, int height);

			const string & getDataPath();
			string getShaderPath(const string & shaderFile = "");

		protected:
			void updateSize();

			ofFbo fboTemp[2];
			ofFbo::Settings fboSettings;

			ofShader brightnessThresholdShader;
			ofShader blurHorzShader;
			ofShader blurVertShader;
			ofShader colorCorrectShader;

			GLuint defaultVao;
			ofVboMesh fullQuad;
		};
	}
}