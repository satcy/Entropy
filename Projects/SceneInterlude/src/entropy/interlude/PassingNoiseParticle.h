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
			void drawBoxLine(ofVec3f a, ofVec3f b, float bold = 10.0, float dist_multi=1.0);
			struct VPoint : public ofVec3f {
				ofVec3f vel;
				float decay;
				vector<ofVec3f> hist;
				void update(int max_len) {
					hist.push_back(*this);
					while (hist.size() > max_len) {
						hist.erase(hist.begin());
					}
				}

				void drawToMesh(ofVboMesh & m, int step) {
					if (hist.empty()) return;
					if (hist.size() - 1 < step) return;
					for (int i = hist.size() - 1; i >= step; i -= step) {
						m.addVertex(hist[i]);
						m.addVertex(hist[i - step]);
						m.addColor(ofFloatColor(1));
						m.addColor(ofFloatColor(1));
					}
				}
			};
			
			vector<VPoint> pts;
			ofVboMesh mesh;
			ofVboMesh line_mesh;
			ofVec3f wind;
			ofShader shader;
			ofVec3f fakeLights[20];

			ofxShaderOnTheFly sem;
			ofImage img;
		public:
			float point_alpha;
			float box_line_alpha;
			struct {
				int max_len = 5;
				int step = 4;
			} line_params;
			struct {
				float scale = 0.001;
				float speed = 2.653;
			} noise;
		public:
			PassingNoiseParticle();
			~PassingNoiseParticle();
			void setup();
			void update();
			void draw(ofCamera & cam);

			void setWind(ofVec3f v) { wind = v; }
			void reset();
		};

	}
}
