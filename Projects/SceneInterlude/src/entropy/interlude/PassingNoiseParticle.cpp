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
		void PassingNoiseParticle::reset() {

			float MM = 1000;
			for (auto & p : pts) {
				p.hist.clear();
				p.x = ofRandom(1, 1.5) * MM;
				p.y = ofRandom(-1, 1) * MM;
				p.z = ofRandom(-1, 1) * MM;
			}
		}
		void PassingNoiseParticle::setup() {
			wind.x = -20;
			pts.resize(4000);
			mesh.clear();
			mesh.setMode(OF_PRIMITIVE_POINTS);
			mesh.setUsage(GL_DYNAMIC_DRAW);

			line_mesh.clear();
			line_mesh.setMode(OF_PRIMITIVE_LINES);
			line_mesh.setUsage(GL_DYNAMIC_DRAW);

			for (auto & p : pts) {
				p.x = ofRandom(-1, 1) * 300.0;
				p.y = ofRandom(-1, 1) * 300.0;
				p.z = ofRandom(-1, 1) * 300.0;
				p.vel.x = ofRandom(-1, 1);
				p.vel.y = ofRandom(-1, 1);
				p.vel.z = ofRandom(-1, 1);
				p.decay = ofRandom(0.89, 0.93);
				mesh.addVertex(p);
				mesh.addColor(ofFloatColor(1));
				mesh.addTexCoord(ofDefaultTexCoordType(0));
				
			}

			shader.load("shaders/fakelight");

			sem.load("shaders/sem");
			img.loadImage("images/IMG_2297.JPG");
			img.mirror(true, true);
		}

		void PassingNoiseParticle::drawBoxLine(ofVec3f a, ofVec3f b, float bold, float dist_multi) {
			float dist = a.distance(b);
			if (dist > 400) return;
			ofVec3f c = (b - a).getNormalized();

			ofQuaternion q;
			q.makeRotate(ofVec3f(1, 0, 0), c);
			ofPushMatrix();
			ofTranslate(a);
			ofPushMatrix();
			ofMultMatrix(ofMatrix4x4(q));
			ofDrawBox(dist*dist_multi / 2.0, 0.0, 0.0, dist*dist_multi, bold, bold);
			ofPopMatrix();
			ofPopMatrix();
		}
		void PassingNoiseParticle::update() {
			//return;
			int cnt = 0;
			float mul = noise.scale;
			float randomize = 0.001;
			float speed = noise.speed;
			float t = ofGetElapsedTimef() * 0.1;

			line_mesh.clear();

			float MM = 1000;
			for (auto & p : pts) {
				ofVec3f noise;
				noise.x = ofSignedNoise(p.x*mul, p.y*mul, p.z*mul, t + cnt*randomize);
				noise.y = ofSignedNoise(p.y*mul, p.z*mul, p.x*mul, t + cnt*randomize);
				noise.z = ofSignedNoise(p.z*mul, p.x*mul, p.y*mul, t + cnt*randomize);
				p.vel += noise * speed;
				p += wind;
				p += p.vel;
				p.vel *= p.decay;

				bool wrapped = false;
				if (p.x < -MM*1.5) {
					p.x += MM * 3.0; wrapped = true;
				}
				if (p.x > MM*1.5) {p.x -= MM * 3.0; wrapped = true;
			}
				if (p.y < -MM) { p.y += MM * 2.0; wrapped = true;
				}
				if (p.y > MM) { p.y -= MM * 2.0; wrapped = true;
				}
				if (p.z < -MM) { p.z += MM * 2.0; wrapped = true;
				}
				if (p.z > MM) { p.z -= MM * 2.0; wrapped = true;
				}
				if (wrapped) {
					p.hist.clear();
				}
				p.update(line_params.max_len);
				p.drawToMesh(line_mesh, line_params.step);

				cnt++;
			}
			for (int i = 0; i < 20; i++) {
				fakeLights[i] = pts[i*100];

			}
			cnt = 0;
			for (auto & p : pts) {
				mesh.setVertex(cnt, p );
				cnt++;
			}
		}

		void PassingNoiseParticle::draw(ofCamera & cam) {
			ofPushStyle();
			ofSetColor(255);
			shader.begin();
			shader.setUniformMatrix4f("modelViewMatrix", cam.getModelViewMatrix());
			shader.setUniformMatrix4f("projectionMatrix", cam.getProjectionMatrix());
			shader.setUniform3fv("fakelights[0]", (float*)fakeLights[0].getPtr(), 20);
			shader.setUniform1f("alpha", point_alpha);
			ofEnableDepthTest();
			ofEnableAlphaBlending();
			glPointSize(2);
			mesh.draw();
			line_mesh.draw();
			shader.end();

			sem.begin();
			sem.setUniformMatrix4f("modelViewMatrix", cam.getModelViewMatrix());
			sem.setUniformMatrix4f("projectionMatrix", cam.getProjectionMatrix());
			sem.setUniformTexture("tMatCap", img, 1);
			sem.setUniform1f("alpha", box_line_alpha);
			ofEnableDepthTest();
			ofEnableAlphaBlending();
			for (int i = 0; i < pts.size(); i += 10) {
				drawBoxLine(pts[i], pts[(i + 10) % pts.size()], 100);
			}
			sem.end();

			ofNoFill();
			ofPushMatrix();
			ofDrawAxis(600);
			//ofDrawSphere(300);
			ofPopMatrix();
			ofPopStyle();
		}
	}
}