/*
 *  ParticleSystem.cpp
 *
 *  Copyright (c) 2016, Neil Mendoza, http://www.neilmendoza.com
 *  All rights reserved. 
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met: 
 *  
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *  * Neither the name of Neil Mendoza nor the names of its contributors may be used 
 *    to endorse or promote products derived from this software without 
 *    specific prior written permission. 
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE. 
 *
 */
#include "ParticleSystem.h"
#include "ofxObjLoader.h"

namespace nm
{
	const float ParticleSystem::MIN_SPEED_SQUARED = 1;// 1e-16;

	ParticleSystem::ParticleSystem() :
		particles(NULL),
		totalNumParticles(0),
		roughness(.1f),
		deadParticles(NULL),
		numDeadParticles(0),
		newProtons(NULL),
		numNewProtons(0)
	{
		memset(positions, 0, Particle::NUM_TYPES * sizeof(positions[0]));
		memset(numParticles, 0, Particle::NUM_TYPES * sizeof(numParticles[0]));
	}

	ParticleSystem::~ParticleSystem()
	{
		if (particles) delete[] particles;
		if (deadParticles) delete[] deadParticles;
		for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
		{
			if (positions[i]) delete[] positions[i];
		}
	}

	void ParticleSystem::init(const ofVec3f& min, const ofVec3f& max)
	{
		this->min = min;
		this->max = max;

		octree.init(min, max);
		octree.addChildren(true);

		particles = new nm::Particle[MAX_PARTICLES]();
		deadParticles = new unsigned[MAX_PARTICLES];
		newProtons = new ofVec3f[MAX_PARTICLES]();

		//for (unsigned i = 0; i < Particle::NUM_TYPES; ++i) meshes[i] = ofMesh::box(1,1,1,1,1,1);

		ofxObjLoader::load("models/cube.obj", meshes[Particle::POSITRON]);
		ofxObjLoader::load("models/cube.obj", meshes[Particle::ELECTRON]);
		ofxObjLoader::load("models/tetra.obj", meshes[Particle::UP_QUARK]);
		ofxObjLoader::load("models/tetra.obj", meshes[Particle::ANTI_UP_QUARK]);
		for (auto& mesh : meshes) mesh.setUsage(GL_STATIC_DRAW);

		shader.load("shaders/particle");

		// position stuff
		for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
		{
			positions[i] = new ParticleGpuData[MAX_PARTICLES]();
			tbo[i].allocate();
			tbo[i].setData(sizeof(ParticleGpuData) * MAX_PARTICLES, positions, GL_DYNAMIC_DRAW);
			positionsTex[i].allocateAsBufferTexture(tbo[i], GL_RGBA32F);
		}
	}

	void ParticleSystem::addParticle(Particle::Type type, const ofVec3f& position, const ofVec3f& velocity)
	{
		if (totalNumParticles < MAX_PARTICLES)
		{
			//float mass = ofMap(Particle::MASSES[type], 500.f, 2300.f, 0.01f, 0.1f);
			float radius = ofMap(Particle::MASSES[type], 500.f, 2300.f, 5.0f, 16.0f);

			Particle& p = particles[totalNumParticles];
			p.set(position);
			p.setVelocity(velocity);
			p.setType(type);
			p.setCharge(Particle::CHARGES[type]);
			p.setMass(Particle::MASSES[type]);
			p.setRadius(radius);

			totalNumParticles++;
			numParticles[type]++;
		}
		else ofLogError() << "Cannot add more particles";
	}

	void ParticleSystem::update()
	{
		numDeadParticles = 0;
		numNewProtons = 0;
		octree.clear();
		octree.addPoints(particles, totalNumParticles);
		octree.updateCenterOfCharge();

		float dt = ofGetLastFrameTime();
		tbb::atomic<unsigned> typeIndices[Particle::NUM_TYPES];
		for (unsigned i = 0; i < Particle::NUM_TYPES; ++i) typeIndices[i] = 0;
		tbb::parallel_for(tbb::blocked_range<size_t>(0, totalNumParticles),
			[&](const tbb::blocked_range<size_t>& r) {
			for (size_t i = r.begin(); i != r.end(); ++i)
			{
				// zero particle forces
				particles[i].zeroForce();

				// sum all forces acting on particle
				Particle* annihiliate = octree.sumForces(particles[i]);

				if (annihiliate)
				{
					// close enough to another particle to annihilate it
					unsigned deadIdx = numDeadParticles.fetch_and_increment();
					deadParticles[deadIdx] = i;

					// make the particle with the lower address in memory of
					// the pair be the one that is responsible for producing
					// the proton
					if (&particles[i] < annihiliate)
					{
						unsigned newProtonIdx = numNewProtons.fetch_and_increment();
						newProtons[newProtonIdx] = particles[i];
					}
				}
				else
				{
					// add velocity (TODO: improved Euler integration)
					particles[i].setVelocity(particles[i].getVelocity() + particles[i].getForce() * dt / particles[i].getMass());

					// damp velocity
					if (particles[i].getVelocity().lengthSquared() > MIN_SPEED_SQUARED) particles[i].setVelocity(.998f * particles[i].getVelocity());

					// add position (TODO: improved Euler integration)
					particles[i] += particles[i].getVelocity() * dt;

					// check whether particle is out of bounds
					for (unsigned j = 0; j < 3; ++j)
					{
						// add a little bit so things don't get stuck teleporting on the edges
						if (particles[i][j] > max[j]) particles[i][j] = min[j] + 10.f;
						if (particles[i][j] < min[j]) particles[i][j] = max[j] - 10.f;
					}
					unsigned idx = typeIndices[particles[i].getType()].fetch_and_increment();
					positions[particles[i].getType()][idx].transform =
						ofMatrix4x4::newLookAtMatrix(ofVec3f(0.0f, 0.0f, 0.0f), particles[i].getVelocity(), ofVec3f(0.0f, 1.0f, 0.0f)) *
						ofMatrix4x4::newScaleMatrix(ofVec3f(particles[i].getRadius(), particles[i].getRadius(), particles[i].getRadius())) *
						ofMatrix4x4::newTranslationMatrix(particles[i]);
				}
			}
		});

		// start deleting particles at the end first so we don't swap out a particle
		// that is actually dead and would be swapped out later in the iteration
		std::sort(deadParticles, deadParticles + numDeadParticles, std::greater<float>());
	
		if (numDeadParticles)
		{
			// kill all the particles
			for (unsigned i = 0; i < numDeadParticles; ++i)
			{
				unsigned deadIdx = deadParticles[i];
				unsigned endIdx = totalNumParticles.fetch_and_decrement() - 1;
				numParticles[(unsigned)particles[deadIdx].getType()].fetch_and_decrement();
				// replace dead particle with one from the end of the array
				if (endIdx >= 0 && deadIdx < totalNumParticles) particles[deadIdx] = particles[endIdx];
			}
		}

		// add new protons
		protonTest.addVertices(newProtons, numNewProtons);
		for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
		{
			tbo[i].updateData(0, sizeof(ParticleGpuData) * numParticles[i], positions[i]);
		}
	}

	void ParticleSystem::draw()
	{
		glPushAttrib(GL_ENABLE_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		shader.begin();
		{
			shader.setUniform1i("numLights", NUM_LIGHTS);
			shader.setUniformMatrix4f("viewMatrix", ofGetCurrentViewMatrix());
			shader.setUniform1f("roughness", roughness);

			for (int i = 0; i < NUM_LIGHTS; i++)
			{
				string index = ofToString(i);
				shader.setUniform3f("lights[" + index + "].position", lights[i].position * ofGetCurrentViewMatrix());
				shader.setUniform4f("lights[" + index + "].color", lights[i].color);
				shader.setUniform1f("lights[" + index + "].intensity", lights[i].intensity);
				shader.setUniform1f("lights[" + index + "].radius", lights[i].radius);
			}

			for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
			{
				shader.setUniform3f("particleColor", Particle::COLORS[i].r, Particle::COLORS[i].g, Particle::COLORS[i].b);
				shader.setUniformTexture("uOffsetTex", positionsTex[i], 0);
				if (numParticles[i]) meshes[i].drawInstanced(OF_MESH_FILL, numParticles[i]);
			}
		}
		shader.end();
		glPopAttrib();
	}

	void ParticleSystem::drawProtonTest()
	{
		ofPushStyle();
		ofSetColor(255);
		protonTest.drawVertices();
		ofPopStyle();
	}
}
