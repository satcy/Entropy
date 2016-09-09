#include "PassingNoiseParticle.h"

#ifndef STRINGIFY
#define STRINGIFY(A) #A
#endif // !STRINGIFY


namespace entropy
{
	namespace scene
	{
		PassingNoiseParticle::PassingNoiseParticle()
		{
			

		}


		PassingNoiseParticle::~PassingNoiseParticle()
		{
		}
		void PassingNoiseParticle::setup() {
			wind.x = -2;
			pts.resize(4000);
			mesh.clear();
			mesh.setMode(OF_PRIMITIVE_POINTS);
			mesh.setUsage(GL_DYNAMIC_DRAW);

			for (auto & p : pts) {
				p.x = ofRandom(-1, 1) * 300.0;
				p.y = ofRandom(-1, 1) * 300.0;
				p.z = ofRandom(-1, 1) * 300.0;
				p.vel.x = ofRandom(-1, 1);
				p.vel.y = ofRandom(-1, 1);
				p.vel.z = ofRandom(-1, 1);
				p.decay = ofRandom(0.89, 0.9);
				mesh.addVertex(p);
				mesh.addColor(ofFloatColor(1));
				mesh.addTexCoord(ofDefaultTexCoordType(0));
				
			}

			shader.load("shaders/fakelight");
		}
		void PassingNoiseParticle::update() {
			//return;
			int cnt = 0;
			float mul = 0.001;
			float t = ofGetElapsedTimef() * 0.1;

			float MM = 500;
			for (auto & p : pts) {
				ofVec3f noise;
				noise.x = ofSignedNoise(p.x*mul, p.y*mul, p.z*mul, t + cnt*mul);
				noise.y = ofSignedNoise(p.y*mul, p.z*mul, p.x*mul, t + cnt*mul);
				noise.z = ofSignedNoise(p.z*mul, p.x*mul, p.y*mul, t + cnt*mul);
				p.vel += noise * 1.0;
				p += wind;
				p += p.vel;
				p.vel *= p.decay;

				if (p.x < -MM) p.x += MM * 2.0;
				if (p.x > MM) p.x -= MM * 2.0;
				if (p.y < -MM) p.y += MM * 2.0;
				if (p.y > MM) p.y -= MM * 2.0;
				if (p.z < -MM) p.z += MM * 2.0;
				if (p.z > MM) p.z -= MM * 2.0;
				cnt++;
			}
			for (int i = 0; i < 5; i++) {
				fakeLights[i] = pts[i*100];

			}
			cnt = 0;
			for (auto & p : pts) {
				mesh.setVertex(cnt, p );
				cnt++;
			}
		}

		void PassingNoiseParticle::draw(ofCamera & cam) {
			ofSetColor(255);
			shader.begin();
			shader.setUniformMatrix4f("modelViewMatrix", cam.getModelViewMatrix());
			shader.setUniformMatrix4f("projectionMatrix", cam.getProjectionMatrix());
			shader.setUniform3fv("fakelights[0]", (float*)fakeLights[0].getPtr(), 5);
			ofEnableDepthTest();
			ofEnableAlphaBlending();
			glPointSize(2);
			mesh.draw();
			shader.end();

			ofNoFill();
			ofPushMatrix();
			ofDrawAxis(600);
			//ofDrawSphere(300);
			ofPopMatrix();
		}
	}
}