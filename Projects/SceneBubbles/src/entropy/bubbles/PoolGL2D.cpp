#include "PoolGL2D.h"

#ifdef COMPUTE_GL_2D

namespace entropy
{
	namespace bubbles
	{
		//--------------------------------------------------------------
		PoolGL2D::PoolGL2D()
			: PoolBase()
		{
			// Update parameter group.
			this->parameters.setName("Pool GL 2D");
		}
		
		//--------------------------------------------------------------
		void PoolGL2D::init()
		{
			PoolBase::init();

			// Load the shader.
			this->shader.load("shaders/passthru.vert", "shaders/ripple.frag");
		}

		//--------------------------------------------------------------
		void PoolGL2D::resize()
		{
			// Allocate the textures and buffers.
			for (int i = 0; i < 3; ++i) {
				this->textures[i].setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
				this->textures[i].allocate(this->dimensions.x, this->dimensions.y, GL_RGBA16F);

				this->fbos[i].allocate();
				this->fbos[i].attachTexture(this->textures[i], 0);
				this->fbos[i].begin();
				{
					ofClear(0, 0);
				}
				this->fbos[i].end();
				this->fbos[i].checkStatus();
			}

			// Build a mesh to render a quad.
			this->mesh.clear();
			this->mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);

			this->mesh.addVertex(glm::vec3(0, 0, 0));
			this->mesh.addVertex(glm::vec3(this->dimensions.x, 0, 0));
			this->mesh.addVertex(glm::vec3(this->dimensions.x, this->dimensions.y, 0));
			this->mesh.addVertex(glm::vec3(0, this->dimensions.y, 0));

			this->mesh.addTexCoord(glm::vec2(0, 0));
			this->mesh.addTexCoord(glm::vec2(this->dimensions.x, 0));
			this->mesh.addTexCoord(glm::vec2(this->dimensions.x, this->dimensions.y));
			this->mesh.addTexCoord(glm::vec2(0, this->dimensions.y));
		}

		//--------------------------------------------------------------
		void PoolGL2D::reset()
		{
			PoolBase::reset();

			// Clear the textures and buffers.
			for (int i = 0; i < 3; ++i)
			{
				this->fbos[i].begin();
				{
					ofClear(0, 0);
				}
				this->fbos[i].end();
			}
		}

		//--------------------------------------------------------------
		void PoolGL2D::addDrop()
		{
			this->fbos[this->prevIdx].begin();
			{
				ofDisableAlphaBlending();

				ofPushMatrix();
				{
					ofScale(1.0, -1.0, 1.0);
					ofTranslate(0.0, this->dimensions.y * -1.0, 0.0);

					ofPushStyle();
					{
						ofSetColor((ofRandomuf() < 0.5 ? this->dropColor1.get() : this->dropColor2.get()));
						ofNoFill();

						ofDrawCircle(ofRandom(0.0f, this->dimensions.x), ofRandom(0.0f, this->dimensions.y), this->radius);
					}
					ofPopStyle();
				}
				ofPopMatrix();
			}
			this->fbos[this->prevIdx].end();
		}

		//--------------------------------------------------------------
		void PoolGL2D::stepRipple()
		{
			this->fbos[this->tempIdx].begin();
			{
				this->shader.begin();
				this->shader.setUniform1f("uDamping", this->damping / 10.0f + 0.9f);  // 0.9 - 1.0 range
				this->shader.setUniformTexture("uPrevBuffer", this->textures[this->prevIdx], 1);
				this->shader.setUniformTexture("uCurrBuffer", this->textures[this->currIdx], 2);
				{
					this->mesh.draw();
				}
				this->shader.end();
			}
			this->fbos[this->tempIdx].end();
		}

		//--------------------------------------------------------------
		void PoolGL2D::copyResult()
		{
			this->fbos[this->currIdx].begin();
			{
				this->textures[this->tempIdx].bind();
				{
					this->mesh.draw();
				}
				this->textures[this->tempIdx].unbind();
			}
			this->fbos[this->currIdx].end();
		}

		//--------------------------------------------------------------
		void PoolGL2D::mixFrames(float pct)
		{
			this->fbos[this->currIdx].begin();
			{
				ofClear(0, 0);
				//ofEnableAlphaBlending();

				ofSetColor(255, 255 * (1.0f - pct));
				this->textures[this->prevIdx].bind();
				{
					this->mesh.draw();
				}
				this->textures[this->prevIdx].unbind();

				ofSetColor(255, 255 * pct);
				this->textures[this->tempIdx].bind();
				{
					this->mesh.draw();
				}
				this->textures[this->tempIdx].unbind();
			}
			this->fbos[this->currIdx].end();
		}

		//--------------------------------------------------------------
		void PoolGL2D::draw()
		{
			ofPushStyle();
			{
				ofEnableAlphaBlending();
				ofSetColor(255, this->alpha * 255);

				this->getTexture().draw(0, 0);
			}
			ofPopStyle();
		}

		//--------------------------------------------------------------
		const ofTexture & PoolGL2D::getTexture() const
		{
			return this->textures[this->prevIdx];
		}
	}
}

#endif // COMPUTE_GL_2D
