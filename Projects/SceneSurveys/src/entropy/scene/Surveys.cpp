#include "Surveys.h"

#include "entropy/surveys/GaussianMapTexture.h"

namespace entropy
{
	namespace scene
	{
		//--------------------------------------------------------------
		Surveys::Surveys()
			: Base()
		{}
		
		//--------------------------------------------------------------
		Surveys::~Surveys()
		{
			this->clear();
		}

		//--------------------------------------------------------------
		void Surveys::init()
		{
			// Load the data.
			this->dataSetBoss.setup("BOSS", this->getAssetsPath("particles/boss_fragment-batch-%iof10.hdf5"), 0, 1, "PartType6");
			this->dataSetDes.setup("DES", this->getAssetsPath("particles/des_fragment-batch-%iof20.hdf5"), 0, 2, "PartType6");
			this->dataSetVizir.setup("ViziR", this->getAssetsPath("particles/Hipparchos-Tycho-stars-fromViziR.hdf5"), 0, 1, "PartType4");

			// Set ofParameterGroup names.
			this->parameters.setName("Surveys");
			this->backParameters.setName("Back");
			this->frontParameters.setName("Front");

			// Add extra parameters to the group (for serialization and timeline mappings).
			this->parameters.add(this->backParameters);
			this->parameters.add(this->frontParameters);
			this->parameters.add(this->dataSetBoss.parameters);
			this->parameters.add(this->dataSetDes.parameters);
			this->parameters.add(this->dataSetVizir.parameters);

			// Build the galaxy quad.
			this->galaxyQuad.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
			this->galaxyQuad.addVertex(glm::vec3(-1.0f, -1.0f, 0.0f));
			this->galaxyQuad.addVertex(glm::vec3(-1.0f,  1.0f, 0.0f));
			this->galaxyQuad.addVertex(glm::vec3( 1.0f, -1.0f, 0.0f));
			this->galaxyQuad.addVertex(glm::vec3( 1.0f,  1.0f, 0.0f));
			this->galaxyQuad.addTexCoord(glm::vec2(0.0f, 1.0f));
			this->galaxyQuad.addTexCoord(glm::vec2(0.0f, 0.0f));
			this->galaxyQuad.addTexCoord(glm::vec2(1.0f, 1.0f));
			this->galaxyQuad.addTexCoord(glm::vec2(1.0f, 0.0f));

			// Build the texture.
			//entropy::survey::CreateGaussianMapTexture(texture, 32, GL_TEXTURE_2D);
			const auto filePath = this->getAssetsPath("images/spiral-galaxy.jpg");
			//const auto filePath = this->getAssetsPath("images/Gaia_star_density_image_log.png");
			ofPixels pixels;
			ofLoadImage(pixels, filePath);
			if (!pixels.isAllocated())
			{
				ofLogError(__FUNCTION__) << "Could not load file at path " << filePath;
			}

			bool wasUsingArbTex = ofGetUsingArbTex();
			ofDisableArbTex();
			{
				this->texture.enableMipmap();
				this->texture.loadData(pixels);
			}
			if (wasUsingArbTex) ofEnableArbTex();

			// Load the shader.
			this->spriteShader.load("shaders/sprite");
		}

		//--------------------------------------------------------------
		void Surveys::clear()
		{
			// Clear the data.
			this->dataSetBoss.clear();
			this->dataSetDes.clear();
			this->dataSetVizir.clear();

			// Clear the texture.
			texture.clear();
		}

		//--------------------------------------------------------------
		void Surveys::setup()
		{}
		
		//--------------------------------------------------------------
		void Surveys::exit()
		{}
		
		//--------------------------------------------------------------
		void Surveys::drawBackWorld()
		{
			this->drawDataSet(this->backParameters);
		}

		//--------------------------------------------------------------
		void Surveys::drawFrontWorld()
		{
			this->drawDataSet(this->frontParameters);
		}

		//--------------------------------------------------------------
		void Surveys::drawDataSet(LayoutParameters & parameters)
		{
			// Draw the galaxy in the center.
			ofPushMatrix();
			{
				ofScale(this->parameters.galaxy.scale);
				ofRotateX(this->parameters.galaxy.orientation.get().x);
				ofRotateY(this->parameters.galaxy.orientation.get().y);
				ofRotateZ(this->parameters.galaxy.orientation.get().z);

				ofPushStyle();
				{
					ofSetColor(255, this->parameters.galaxy.alpha * 255);

					this->texture.bind();
					this->galaxyQuad.draw();
					this->texture.unbind();
				}
				ofPopStyle();
			}
			ofPopMatrix();

			ofPushMatrix();
			ofScale(parameters.scale);
			{
				ofEnableBlendMode(OF_BLENDMODE_ADD);
				ofDisableDepthTest();

				this->spriteShader.begin();
				this->spriteShader.setUniformTexture("uTex0", texture, 1);
				this->spriteShader.setUniform1f("uPointSize", parameters.pointSize);
				ofEnablePointSprites();
				{
					static const auto kLatitudeMin = -HALF_PI;
					static const auto kLatitudeMax = HALF_PI;
					static const auto kLongitudeMin = 0;
					static const auto kLongitudeMax = TWO_PI;

					if (parameters.renderBoss)
					{
						this->spriteShader.setUniform1f("uMinLatitude", ofMap(this->dataSetBoss.parameters.minLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMaxLatitude", ofMap(this->dataSetBoss.parameters.maxLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMinLongitude", ofMap(this->dataSetBoss.parameters.minLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->spriteShader.setUniform1f("uMaxLongitude", ofMap(this->dataSetBoss.parameters.maxLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->dataSetBoss.draw();
					}
					if (parameters.renderDes)
					{
						this->spriteShader.setUniform1f("uMinLatitude", ofMap(this->dataSetDes.parameters.minLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMaxLatitude", ofMap(this->dataSetDes.parameters.maxLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMinLongitude", ofMap(this->dataSetDes.parameters.minLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->spriteShader.setUniform1f("uMaxLongitude", ofMap(this->dataSetDes.parameters.maxLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->dataSetDes.draw();
					}
					if (parameters.renderVizir)
					{
						this->spriteShader.setUniform1f("uMinLatitude", ofMap(this->dataSetVizir.parameters.minLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMaxLatitude", ofMap(this->dataSetVizir.parameters.maxLatitude, 0.0f, 1.0f, kLatitudeMin, kLatitudeMax));
						this->spriteShader.setUniform1f("uMinLongitude", ofMap(this->dataSetVizir.parameters.minLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->spriteShader.setUniform1f("uMaxLongitude", ofMap(this->dataSetVizir.parameters.maxLongitude, 0.0f, 1.0f, kLongitudeMin, kLongitudeMax));
						this->dataSetVizir.draw();
					}
				}
				ofDisablePointSprites();
				this->spriteShader.end();
				
				ofEnableBlendMode(OF_BLENDMODE_ALPHA);
			}
			ofPopMatrix();
		}

		//--------------------------------------------------------------
		void Surveys::gui(ofxPreset::Gui::Settings & settings)
		{
			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow(this->parameters.getName().c_str(), settings, true, nullptr))
			{
				ofxPreset::Gui::AddGroup(this->parameters.galaxy, settings);
				
				this->dataSetBoss.gui(settings);
				this->dataSetDes.gui(settings);
				this->dataSetVizir.gui(settings);

				ofxPreset::Gui::AddGroup(this->backParameters, settings);
				ofxPreset::Gui::AddGroup(this->frontParameters, settings);
			}
			ofxPreset::Gui::EndWindow(settings);
		}
	}
}