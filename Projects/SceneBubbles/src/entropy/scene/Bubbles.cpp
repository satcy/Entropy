#include "Bubbles.h"

namespace entropy
{
	namespace scene
	{
		//--------------------------------------------------------------
		Bubbles::Bubbles()
			: Base()
		{}
		
		//--------------------------------------------------------------
		Bubbles::~Bubbles()
		{

		}

		//--------------------------------------------------------------
		void Bubbles::init()
		{
			// Add the pool parameters to the group.
			this->parameters.add(this->pool.parameters);

			// Add the sphere parameters to the group.
			this->parameters.add(this->sphereGeom.parameters);

			this->pool.setDimensions(glm::vec3(128.0f));
			this->pool.setup();

			const auto filePath = this->getAssetsPath("images/Planck-Bubbles-SMICA.tif");
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
				this->sphereTexture.enableMipmap();
				this->sphereTexture.loadData(pixels);
			}
			if (wasUsingArbTex) ofEnableArbTex();
		}

		//--------------------------------------------------------------
		void Bubbles::setup()
		{
			this->pool.reset();
		}

		//--------------------------------------------------------------
		void Bubbles::resizeBack(ofResizeEventArgs & args)
		{
#if defined(COMPUTE_GL_2D) || defined(COMPUTE_CL_2D)
			this->pool.restartSimulation = true;
#endif
		}

		//--------------------------------------------------------------
		void Bubbles::update(double dt)
		{
			this->pool.update();
		}
		
		//--------------------------------------------------------------
		void Bubbles::drawBackWorld()
		{
#if defined(COMPUTE_GL_3D) || defined(COMPUTE_CL_3D)
			ofEnableDepthTest();
			this->drawPool();
			ofEnableDepthTest();
			//ofDisableDepthTest();
#endif
			
			this->sphereTexture.bind();
			{
				this->sphereGeom.draw();
			}
			this->sphereTexture.unbind();
		}

		//--------------------------------------------------------------
		void Bubbles::drawBackOverlay()
		{
#if defined(COMPUTE_GL_2D) || defined(COMPUTE_CL_2D)
			this->drawPool();
#endif
		}

		//--------------------------------------------------------------
		void Bubbles::drawFrontWorld()
		{
#if defined(COMPUTE_GL_3D) || defined(COMPUTE_CL_3D)
			ofEnableDepthTest();
			this->drawPool();
			ofEnableDepthTest();
			//ofDisableDepthTest();
#endif
		}

		//--------------------------------------------------------------
		void Bubbles::gui(ofxPreset::Gui::Settings & settings)
		{
			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow(this->parameters.getName().c_str(), settings, true, nullptr))
			{
				ofxPreset::Gui::AddParameter(this->parameters.tintColor);

				if (ofxPreset::Gui::BeginTree(this->pool.parameters, settings))
				{
					ofxPreset::Gui::AddParameter(this->pool.runSimulation);
					if (ImGui::Button("Reset Simulation"))
					{
						this->pool.resetSimulation = true;
					}

					ofxPreset::Gui::AddParameter(this->pool.dropColor);
					ofxPreset::Gui::AddParameter(this->pool.dropping);
					ofxPreset::Gui::AddParameter(this->pool.dropRate);

					ofxPreset::Gui::AddParameter(this->pool.rippleRate);

					ofxPreset::Gui::AddParameter(this->pool.damping);
					ofxPreset::Gui::AddParameter(this->pool.radius);
					ofxPreset::Gui::AddParameter(this->pool.ringSize);

#if defined(COMPUTE_GL_3D)
					static const vector<string> labels{ "Nearest", "Linear" };
					ofxPreset::Gui::AddRadio(this->pool.filterMode, labels, 2);
					ofxPreset::Gui::AddParameter(this->pool.volumeSize);
#endif

					ofxPreset::Gui::EndTree(settings);
				}

				if (ofxPreset::Gui::BeginTree(this->sphereGeom.parameters, settings))
				{
					ofxPreset::Gui::AddParameter(this->sphereGeom.enabled);
					if (this->sphereGeom.enabled)
					{
						ImGui::SameLine();
						ofxPreset::Gui::AddParameter(this->sphereGeom.autoDraw);
					}
					static const vector<string> labels{ "None", "Back", "Front" };
					ofxPreset::Gui::AddRadio(this->sphereGeom.cullFace, labels, 3);
					ofxPreset::Gui::AddParameter(this->sphereGeom.color);
					ofxPreset::Gui::AddParameter(this->sphereGeom.alpha);
					ofxPreset::Gui::AddParameter(this->sphereGeom.radius);
					ofxPreset::Gui::AddParameter(this->sphereGeom.resolution);
					ofxPreset::Gui::AddParameter(this->sphereGeom.arcHorz);
					ofxPreset::Gui::AddParameter(this->sphereGeom.arcVert);

					ofxPreset::Gui::EndTree(settings);
				}
			}
			ofxPreset::Gui::EndWindow(settings);
		}

		//--------------------------------------------------------------
		void Bubbles::drawPool()
		{
			ofPushStyle();
			{
				ofSetColor(this->parameters.tintColor.get());

				this->pool.draw();
			}
			ofPopStyle();
		}
	}
}