#include "ParticleSystem.h"
#include "lb/util/RadixSort.h"
#include "ofxObjLoader.h"

const float Particle::MASSES[NUM_TYPES] = { 500.f, 500.f, 2300.f, 2300.f };
const float Particle::CHARGES[NUM_TYPES] = { -1.f, 1.f, -1.f, 1.f };
const ofFloatColor Particle::COLORS[NUM_TYPES] = { ofFloatColor(0.2f), ofFloatColor(1.f), ofFloatColor(0.2f), ofFloatColor(1.f) };

const float ParticleSystem::MIN_SPEED_SQUARED = 1e-16;

ParticleSystem::ParticleSystem() : 
	m_particlePool(nullptr),
	m_numAttractors(0),
	m_numRepellers(0),
	numParticlesTotal(0)
{
}

ParticleSystem::~ParticleSystem()
{}

void ParticleSystem::init(int _width, int _height, int _depth)
{
	m_halfWidth = _width / 2.0f;
	m_halfHeight = _height / 2.0f;
	m_halfDepth = _depth / 2.0f;

	m_particlePool = new Particle[MAX_PARTICLES];
	for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
	{
		m_positions[i] = new ParticleTboData[MAX_PARTICLES]();
	}

	memset(m_particlePool, 0, sizeof(m_particlePool[0]) * MAX_PARTICLES);
	//memset(m_positions, 0, sizeof(m_positions[0]) * MAX_PARTICLES);
	memset(m_particleBins, 0, sizeof(m_particleBins[0]) * NUM_BINS);
	memset(numParticles, 0, sizeof(numParticles[0]) * Particle::NUM_TYPES);

	numDeadParticles = 0;
	memset(deadParticles, 0, sizeof(deadParticles[0]) * MAX_PARTICLES);

	ofVec3f minBounds(-m_halfWidth, -m_halfHeight, -m_halfDepth);
	ofVec3f maxBounds(m_halfWidth, m_halfHeight, m_halfDepth);
	ofVec3f size = maxBounds - minBounds;

	m_invBoundsScale = 1.0f / (maxBounds - minBounds);
	m_binScale = ofVec3f((float)BIN_DIMS_X, (float)BIN_DIMS_Y, (float)BIN_DIMS_Z) * m_invBoundsScale;

	m_debugBoundsBox.disableNormals();
	m_debugBoundsBox.disableTextures();
	m_debugBoundsBox.setResolution(1);
	m_debugBoundsBox.setWidth(size.x);
	m_debugBoundsBox.setHeight(size.y);
	m_debugBoundsBox.setDepth(size.z);

	photonTex.loadImage("images/flare.png");
	ofxObjLoader::load("models/cube_fillet_1.obj", particleMeshes[Particle::POSITRON]);
	ofxObjLoader::load("models/cube_fillet_1.obj", particleMeshes[Particle::ELECTRON]);
	ofxObjLoader::load("models/pyramid_fillet_1.obj", particleMeshes[Particle::UP_QUARK]);
	ofxObjLoader::load("models/pyramid_fillet_1.obj", particleMeshes[Particle::ANTI_UP_QUARK]);
	for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
	{
		for (auto& v : particleMeshes[i].getVertices()) v *= .4f; // scale mesh
		particleMeshes[i].setUsage(GL_STATIC_DRAW);
	}

	for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
	{
		m_positionTbo[i].allocate();
		m_positionTbo[i].bind(GL_TEXTURE_BUFFER);
		m_positionTbo[i].unbind(GL_TEXTURE_BUFFER);

		m_positionTbo[i].setData(sizeof(m_positions[0][0]) * MAX_PARTICLES, m_positions[i], GL_DYNAMIC_DRAW);
		m_positionTboTex[i].allocateAsBufferTexture(m_positionTbo[i], GL_RGBA32F);
	}
}

void ParticleSystem::shutdown()
{
    delete[] m_particlePool;
	for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
	{
		delete[] m_positions[i];
	}
}

void ParticleSystem::addParticle(Particle::Type type, const ofVec3f& _pos, const ofVec3f& _vel, float _mass, float _radius, float charge)
{
	if (numParticlesTotal < MAX_PARTICLES)
	{
		Particle& p = m_particlePool[numParticlesTotal];
		p.position = _pos;
		p.velocity = _vel;
		p.mass = _mass;
		p.radius = _radius;
		p.type = type;
		p.charge = charge;
		p.alive = true;
		p.createPhoton = false;

		++(numParticles[type]);
		++numParticlesTotal;
	}
	else ofLogError() << "Cannot add particle, MAX_PARTICLES already reached.";
}

void ParticleSystem::addAttractor( const ofVec3f& _pos, float _strength )
{
    Attractor& att = m_attractors[ m_numAttractors ];
    att.position = _pos;
    att.strength = _strength;
    ++m_numAttractors;
}

void ParticleSystem::addRepeller( const ofVec3f& _pos, float _strength )
{
    Repeller& rep = m_repellers[ m_numRepellers ];
    rep.position = _pos;
    rep.strength = _strength;
    ++m_numRepellers;
}

void ParticleSystem::removeParticle(unsigned idx)
{
	if (idx < numParticlesTotal)
	{
		m_particlePool[idx].alive = false;
		deadParticles[numDeadParticles++] = idx;
	}
	else ofLogError() << "Attempt to remove particle that wasn't alive";
}

void ParticleSystem::sortParticlesByBin()
{
    // sort the keys and values - this will give us the particle indices sorted by bin id 
    lb::RadixSort16<uint16_t>( m_particleSortKeys, m_tempParticleSortKeys, m_particleIndices, m_tempParticleIndices, numParticlesTotal);
}

void ParticleSystem::update()
{
    memset( m_particleIndices, 0, sizeof( m_particleIndices[ 0 ] ) * MAX_PARTICLES );
    memset( m_particleSortKeys, 0, sizeof( m_particleSortKeys[ 0 ] ) * MAX_PARTICLES );
    memset( m_tempParticleSortKeys, 0, sizeof( m_tempParticleSortKeys[ 0 ] ) * MAX_PARTICLES );
    memset( m_tempParticleIndices, 0, sizeof( m_tempParticleIndices[ 0 ] ) * MAX_PARTICLES );
    memset( m_particleBins, 0, sizeof( m_particleBins[ 0 ] ) * NUM_BINS);

	unsigned idxByType[Particle::NUM_TYPES];
	memset(idxByType, 0, sizeof(idxByType[0]) * Particle::NUM_TYPES);
#ifdef TARGET_OSX
    dispatch_apply(m_numParticles, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(size_t idx) {
#else
#pragma omp parallel for
    for ( int idx = 0; idx < numParticlesTotal; ++idx ) {
#endif
        Particle& p = m_particlePool[ idx ];
        p.position += p.velocity;
        if (p.velocity.lengthSquared() > MIN_SPEED_SQUARED) p.velocity *= 0.99f;

        // check walls
        if ( p.position.x > m_halfWidth )
        {
            p.position.x -= m_halfWidth * 2;
        }
        else if ( p.position.x < -m_halfWidth )
        {
            p.position.x += m_halfWidth * 2;
        }

        if ( p.position.y > m_halfHeight)
        {
            p.position.y -= m_halfHeight * 2;
        }
        else if ( p.position.y < -m_halfHeight )
        {
            p.position.y += m_halfHeight * 2;
        }

        if ( p.position.z > m_halfDepth )
        {
            p.position.z -= m_halfDepth * 2;
        }
        else if ( p.position.z < -m_halfDepth )
        {
            p.position.z += m_halfDepth * 2;
        }

        const uint16_t px = (uint16_t)floorf( ( p.position.x + m_halfWidth ) * m_binScale.x );
        const uint16_t py = (uint16_t)floorf( ( p.position.y + m_halfHeight ) * m_binScale.y );
        const uint16_t pz = (uint16_t)floorf( ( p.position.z + m_halfDepth ) * m_binScale.z );
        const uint16_t binId = binIdFromXYZ( px, py, pz );

        m_particleIndices[ idx ] = idx;
        m_particleSortKeys[ idx ] = binId;
#pragma omp atomic
        ++m_particleBins[ binId ].particleCount;

		m_positions[p.type][idxByType[p.type]++].transform =
			ofMatrix4x4::newLookAtMatrix(ofVec3f(0.0f, 0.0f, 0.0f), p.velocity, ofVec3f(0.0f, 1.0f, 0.0f)) *
			ofMatrix4x4::newScaleMatrix(ofVec3f(p.radius, p.radius, p.radius)) *
			ofMatrix4x4::newTranslationMatrix(p.position);

		//  ofLogNotice() << "bin " << (uint16_t)binId << " ... " << px << ", " << py << ", " << pz << endl;

#ifdef TARGET_OSX
    });
#else
    }
#endif

	for (unsigned i = 0; i < Particle::NUM_TYPES; ++i)
	{
		m_positionTbo[i].updateData(0, sizeof(ParticleTboData) * numParticles[i], m_positions[i]);
	}

    //m_positionTbo.updateData( 0, sizeof( m_positions[ 0 ] ) * m_numParticles, m_positions );
}

void ParticleSystem::step( float _dt )
{
    for ( uint32_t idx = 0; idx < numParticlesTotal; ++idx )
    {
        Particle& p = m_particlePool[ idx ];

        ofVec3f acc( 0.0f, 0.0f, 0.0f );
        for ( int attIdx = 0; attIdx < m_numAttractors; ++attIdx )
        {
            const Attractor& att = m_attractors[ attIdx ];
            const float eps = 1e-9f; // minimum dist for nbody interaction

            ofVec3f delta = att.position - p.position;
            float distSqr = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z + eps;
            float invDist = 1.0f / sqrtf( distSqr );
            float invDist3 = invDist * invDist * invDist;
            float f = att.strength * invDist3;

            acc += f * delta;
        }

        for ( int repIdx = 0; repIdx < m_numRepellers; ++repIdx )
        {
            const Repeller& rep = m_repellers[ repIdx ];
            const float eps = 1e-9f; // minimum dist for nbody interaction

            ofVec3f delta = p.position - rep.position;
            float distSqr = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z + eps;
            float invDist = 1.0f / sqrtf( distSqr );
            float invDist3 = invDist * invDist * invDist;
            float f = rep.strength * invDist3;

            acc += f * delta;
        }

        p.velocity += acc / p.mass;
    }

    sortParticlesByBin();

    uint16_t offset = 0;
    for ( uint16_t binIdx = 0; binIdx < NUM_BINS; ++binIdx )
    {
        ParticleBin& bin = m_particleBins[ binIdx ];
        if ( bin.particleCount > 0 )
        {
            bin.offset = offset;
            offset += bin.particleCount;
        }
    }

    // neighbor +/- {range} bins
    const int range = 2;

    // loop through all bins
    for ( int binZ = 0; binZ < BIN_DIMS_Z; ++binZ )
    {
        const uint16_t z0 = ( binZ - range ) < 0 ? 0 : binZ - range;
        const uint16_t z1 = ( binZ + range ) > ( BIN_DIMS_Z - 1 ) ? BIN_DIMS_Z - 1 : binZ + range;

        for ( int binY = 0; binY < BIN_DIMS_Y; ++binY )
        {
            const uint16_t y0 = ( binY - range ) < 0 ? 0 : binY - range;
            const uint16_t y1 = ( binY + range ) > ( BIN_DIMS_Y - 1 ) ? BIN_DIMS_Y - 1 : binY + range;

            for ( int binX = 0; binX < BIN_DIMS_X; ++binX )
            {
                // current bin
                const uint16_t binIdx = binIdFromXYZ( binX, binY, binZ );

                const uint16_t x0 = ( binX - range ) < 0 ? 0 : binX - range;
                const uint16_t x1 = ( binX + range ) > ( BIN_DIMS_X - 1 ) ? BIN_DIMS_X - 1 : binX + range;
                //   ofLogNotice() << "bin " << binX << ", " << binY << ", " << binZ << " min: " << x0 << ", " << y0 << ", " << z0 << " .. max: " << x1 << ", " << y1 << ", " << z1;

                const ParticleBin& bin = m_particleBins[ binIdx ];

                // loop through all particles in bin
#ifdef TARGET_OSX
                dispatch_apply(bin.particleCount, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^(size_t idx) {
#else
#pragma omp parallel for
                for ( int idx = 0; idx < bin.particleCount; ++idx )
				{
#endif
                    uint16_t particleIdx = m_particleIndices[ bin.offset + idx ];
					Particle& p = m_particlePool[particleIdx];

					if (p.alive)
					{
						uint16_t neighbourParticleIdx = 0;                    
						ofVec3f acc( 0.0f, 0.0f, 0.0f );

						// loop through neighbor bins
						for (int nz = z0; nz < z1; ++nz)
						{
							for (int ny = y0; ny < y1; ++ny)
							{
								for (int nx = x0; nx < x1; ++nx)
								{
									const ParticleBin& nbin = m_particleBins[binIdFromXYZ(nx, ny, nz)];
									for (uint16_t neighborIdx = 0; neighborIdx < nbin.particleCount; ++neighborIdx)
									{
										// n-body euler integration
										neighbourParticleIdx = m_particleIndices[nbin.offset + neighborIdx];
										if (particleIdx != neighbourParticleIdx)
										{
											Particle& neighborP = m_particlePool[neighbourParticleIdx];
											ofVec3f dir = neighborP.position - p.position;
											float distSqr = dir.lengthSquared();
											float dist = sqrtf(distSqr);
											if (dist < 2.f)
											{
												p.alive = false;
												neighborP.alive = false;
												p.createPhoton = true;

												// fix me: should be critical section
												deadParticles[numDeadParticles] = particleIdx;
#pragma omp atomic
												numDeadParticles++;

												deadParticles[numDeadParticles] = particleIdx;
#pragma omp atomic
												numDeadParticles++;

											}
											else
											{
												ofVec3f dirNormalised = dir / dist;
												// same charge force goes towards other particle
												//if (-p.charge * neighborP.charge > 0.f) cout << "attracting" << endl;
												acc += -80.f * dirNormalised * p.charge * neighborP.charge * p.mass * neighborP.mass / distSqr;
											}
										}
									}
								}
							}
						}
						p.velocity += _dt * acc / p.mass;
					}
#ifdef TARGET_OSX
                });
#else
                }
#endif			
            }
        }
    }

	// remove dead particles by swapping them to the end of the array
	for (unsigned i = 0; i < numDeadParticles; ++i)
	{
		if (m_particlePool[deadParticles[i]].createPhoton)
		{
			Photon photon;
			photon.vel = ofVec3f(ofRandomf(), ofRandomf(), ofRandomf()).normalize().scale(50.f);
			photon.pos = m_particlePool[deadParticles[i]].position;
			photons.push_back(photon);
		}
		numParticlesTotal--;
		numParticles[(unsigned)m_particlePool[deadParticles[i]].type]--;
		m_particlePool[deadParticles[i]] = m_particlePool[numParticlesTotal];
	}
	numDeadParticles = 0;
}

void ParticleSystem::debugDrawWorldBounds()
{
    ofDisableDepthTest();
    glDisable( GL_CULL_FACE );
    m_debugBoundsBox.drawWireframe();
    ofEnableDepthTest();
}

void ParticleSystem::debugDrawParticles(Particle::Type type)
{
	glPushAttrib(GL_ENABLE_BIT);
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

	// check we have particles of this type as OF draws at least one primitive even if 
	// primCount is set to zero!!
	if (numParticles[type]) particleMeshes[type].drawInstanced( OF_MESH_FILL, numParticles[type]);
	glPopAttrib();
}

void ParticleSystem::hackyUpdatePhotons()
{
	const float killDist = 2000.f * 2000.f;
	const float dt = ofGetLastFrameTime();
	for (auto& it = photons.begin(); it != photons.end();)
	{
		if (it->pos.lengthSquared() > killDist) it = photons.erase(it);
		else
		{
			it->pos += it->vel;
			++it;
		}
	}
}

void ParticleSystem::drawPhotons()
{
	ofPushStyle();
	glPushAttrib(GL_ENABLE_BIT);
	ofSetRectMode(OF_RECTMODE_CENTER);
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	for (auto& photon : photons)
	{
		photonTex.draw(photon.pos, 40.f, 40.f);
	}
	glDepthMask(GL_TRUE);
	glPopAttrib();
	ofPopStyle();
}