#pragma once

#include "ofMain.h"
#include "ofxShaderOnTheFly.h"

namespace entropy
{
	namespace scene
	{
		class PassingNoiseParticle
		{
		protected:
			struct VPoint : public ofVec3f {
				ofVec3f vel;
				float decay;
			};
			vector<VPoint> pts;
			ofVboMesh mesh;
			ofVec3f wind;
			ofxShaderOnTheFly shader;
			ofVec3f fakeLights[5];
		public:
			PassingNoiseParticle();
			~PassingNoiseParticle();
			void setup();
			void update();
			void draw(ofCamera & cam);
		};

	}
}
