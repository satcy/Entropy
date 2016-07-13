/*
 *  ParticleSystem.h
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
#pragma once

#include "ofMain.h"
#include "Octree.h"
#include "Particle.h"
#include "ParticleEvents.h"

namespace nm
{
    struct ParticleGpuData
    {
        glm::mat4 transform;
    };

	struct Light
	{
		glm::vec3 position;
		ofFloatColor color;
		float intensity;
		float radius;
	};

    class ParticleSystem
    {
    public:
        static const unsigned MAX_PARTICLES = 5000;
        static const unsigned NUM_LIGHTS = 2;
		static const float MIN_SPEED_SQUARED;
        
        ParticleSystem();
        ~ParticleSystem();
        
        void init(const glm::vec3& min, const glm::vec3& max);
        
        void addParticle(Particle::Type type, const glm::vec3& position, const glm::vec3& velocity);
        
        void update();
        
        void draw();

		void drawWalls();

        // lighting, should be private but for
        // GUI adding simplicity they're public
        Light lights[NUM_LIGHTS];
        float roughness;

		void serialize(nlohmann::json & json);
		void deserialize(const nlohmann::json & json);
        
    private:
        Octree<Particle> octree;
        nm::Particle* particles;
        tbb::atomic<unsigned> numParticles[Particle::NUM_TYPES];
		tbb::atomic<unsigned> totalNumParticles;
		unsigned* deadParticles;
		tbb::atomic<unsigned> numDeadParticles;
        ofVboMesh meshes[Particle::NUM_TYPES];
        ofShader particleShader;
        glm::vec3 min, max, dims;
		ofShader wallShader;
        
        // position stuff
        ofBufferObject tbo[Particle::NUM_TYPES];
		ParticleGpuData* positions[Particle::NUM_TYPES];
        ofTexture positionsTex[Particle::NUM_TYPES];

		glm::vec3* newPhotons;
		tbb::atomic<unsigned> numNewPhotons;
    };
}