#include "Base.h"

#include "entropy/Helpers.h"

namespace entropy
{
	namespace scene
	{
		//--------------------------------------------------------------
		Base::Base()
		{}

		//--------------------------------------------------------------
		Base::~Base()
		{}

		//--------------------------------------------------------------
		void Base::setup_()
		{
			// Set data path root for scene.
			ofSetDataPathRoot(this->getDataPath());
			
			// Setup default cameras.
			this->resetCamera(render::Layout::Back);
			this->resetCamera(render::Layout::Front);

			auto & parameters = this->getParameters();
			
			// Add geom::Box and render::PostEffects parameters to the ofParameterGroup so 
			// that they are taken into account for ofxTimeline mappings and serialization.
			this->boxes[render::Layout::Back].parameters.setName("Box Back");
			this->boxes[render::Layout::Front].parameters.setName("Box Front");
			parameters.add(this->boxes[render::Layout::Back].parameters);
			parameters.add(this->boxes[render::Layout::Front].parameters);

			this->postEffects[render::Layout::Back].setName("Post Effects Back");
			this->postEffects[render::Layout::Front].setName("Post Effects Front");
			parameters.add(this->postEffects[render::Layout::Back]);
			parameters.add(this->postEffects[render::Layout::Front]);

			// Setup child Scene.
			this->setup();

			// List presets.
			this->populatePresets();

			// Setup timeline.
			static string timelineDataPath;
			if (timelineDataPath.empty())
			{
				timelineDataPath = ofFilePath::addTrailingSlash(GetSharedDataPath());
				timelineDataPath.append(ofFilePath::addTrailingSlash("ofxTimeline"));
			}
			this->timeline.setup(timelineDataPath);
			this->timeline.setLoopType(OF_LOOP_NONE);
			this->timeline.setFrameRate(30.0f);
			this->timeline.setDurationInSeconds(600);
			this->timeline.setAutosave(false);
			this->timeline.setPageName(this->getParameters().base.getName());

			for (auto & it : this->cameras)
			{
				const auto trackName = ((it.first == render::Layout::Back) ? "Camera Back" : "Camera Front");
				const auto trackIdentifier = this->getParameters().base.getName() + "_" + trackName;
				auto track = new ofxTLCameraTrack();
				track->setCamera(it.second);
				track->setXMLFileName(this->timeline.nameToXMLName(trackIdentifier));
				this->timeline.addTrack(trackIdentifier, track);
				track->setDisplayName(trackName);
				track->lockCameraToTrack = false;
				this->cameraTracks[it.first] = track;
			}
			
			// List mappings.
			this->populateMappings(parameters);

			// Set base parameter listeners.
			this->parameterListeners.push_back(parameters.base.backCamera.relativeYAxis.newListener([this](bool & value)
			{
				this->cameras[render::Layout::Back].setRelativeYAxis(value);
			}));
			this->parameterListeners.push_back(parameters.base.backCamera.fov.newListener([this](float & value)
			{
				this->cameras[render::Layout::Back].setFov(value);
			}));
			this->parameterListeners.push_back(parameters.base.backCamera.nearClip.newListener([this](float & value)
			{
				this->cameras[render::Layout::Back].setNearClip(value);
			}));
			this->parameterListeners.push_back(parameters.base.backCamera.farClip.newListener([this](float & value)
			{
				this->cameras[render::Layout::Back].setFarClip(value);
			}));
			this->parameterListeners.push_back(parameters.base.frontCamera.relativeYAxis.newListener([this](bool & value)
			{
				this->cameras[render::Layout::Front].setRelativeYAxis(value);
			}));
			this->parameterListeners.push_back(parameters.base.frontCamera.attachToBack.newListener([this](bool & value)
			{
				if (value)
				{
					this->cameras[render::Layout::Front].setParent(this->cameras[render::Layout::Back], true);
				}
				else
				{
					this->cameras[render::Layout::Front].clearParent(true);
				}
			}));
			this->parameterListeners.push_back(parameters.base.frontCamera.fov.newListener([this](float & value)
			{
				this->cameras[render::Layout::Front].setFov(value);
			}));
			this->parameterListeners.push_back(parameters.base.frontCamera.nearClip.newListener([this](float & value)
			{
				this->cameras[render::Layout::Front].setNearClip(value);
			}));
			this->parameterListeners.push_back(parameters.base.frontCamera.farClip.newListener([this](float & value)
			{
				this->cameras[render::Layout::Front].setFarClip(value);
			}));

			// Load default preset.
			this->currPreset.clear();
			this->loadPreset(kPresetDefaultName);
		}

		//--------------------------------------------------------------
		void Base::exit_()
		{
			this->exit();

			// Save default preset.
			this->savePreset(kPresetDefaultName);

			// Clear camera tracks.
			for (auto & it : this->cameraTracks)
			{
				this->timeline.removeTrack(it.second);
				delete it.second;
			}
			this->cameraTracks.clear();

			// Clear Pop-ups.
			while (!this->popUps.empty())
			{
				this->removePopUp();
			}

			// Clear parameter listeners.
			this->parameterListeners.clear();
		}

		//--------------------------------------------------------------
		void Base::resize_(render::Layout layout, ofResizeEventArgs & args)
		{
			this->cameras[layout].setAspectRatio(args.width / static_cast<float>(args.height));

			if (layout == render::Layout::Back)
			{
				this->resizeBack(args);
			}
			else
			{
				this->resizeFront(args);
			}
		}

		//--------------------------------------------------------------
		void Base::update_(double dt)
		{
			if (GetApp()->isMouseOverGui() || !this->getParameters().base.backCamera.mouseControl)
			{
				this->cameras[render::Layout::Back].disableMouseInput();
			}
			else
			{
				this->cameras[render::Layout::Back].enableMouseInput();
			}
			if (GetApp()->isMouseOverGui() || !this->getParameters().base.frontCamera.mouseControl)
			{
				this->cameras[render::Layout::Front].disableMouseInput();
			}
			else
			{
				this->cameras[render::Layout::Front].enableMouseInput();
			}

			for (auto & it : this->mappings)
			{
				it.second->update();
			}

			for (auto popUp : this->popUps)
			{
				popUp->update_(dt);
			}

			for (auto & it : this->boxes)
			{
				it.second.update();
			}

			this->update(dt);
		}

		//--------------------------------------------------------------
		void Base::drawBase_(render::Layout layout)
		{
			if (layout == render::Layout::Back)
			{
				ofBackground(this->getParameters().base.background.get());
				this->drawBackBase();
			}
			else
			{
				ofClear(0, 255);
				this->drawFrontBase();
			}
		}

		//--------------------------------------------------------------
		void Base::drawWorld_(render::Layout layout)
		{
			auto & parameters = this->getParameters();
			
			//if (layout == render::Layout::Back)
			//{
			//	this->cameras[layout].setNearClip(parameters.base.camera.backClipNear);
			//	this->cameras[layout].setFarClip(parameters.base.camera.backClipFar);
			//}
			//else
			//{
			//	this->cameras[layout].setNearClip(parameters.base.camera.frontClipNear);
			//	this->cameras[layout].setFarClip(parameters.base.camera.frontClipFar);
			//}
			this->cameras[layout].begin(GetCanvasViewport(layout));
			ofEnableDepthTest();
			{
				this->boxes[layout].draw();

				if (layout == render::Layout::Back)
				{
					this->drawBackWorld();
				}
				else
				{
					this->drawFrontWorld();
				//	this->boxes[layout].draw();
				}
			}
			ofDisableDepthTest();
			this->cameras[layout].end();
		}

		//--------------------------------------------------------------
		void Base::drawOverlay_(render::Layout layout)
		{
			if (layout == render::Layout::Back)
			{
				this->drawBackOverlay();
			}
			else
			{
				this->drawFrontOverlay();
			}

			for (auto popUp : this->popUps)
			{
				if (popUp->getLayout() == layout)
				{
					popUp->draw_();
				}
			}
		}

		//--------------------------------------------------------------
		void Base::gui_(ofxPreset::Gui::Settings & settings)
		{
			auto & parameters = this->getParameters();

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow("Presets", settings))
			{
				if (ImGui::Button("Save"))
				{
					this->savePreset(this->currPreset);
				}
				ImGui::SameLine();
				if (ImGui::Button("Save As..."))
				{
					auto name = ofSystemTextBoxDialog("Enter a name for the preset", "");
					if (!name.empty())
					{
						this->savePreset(name);
					}
				}

				ImGui::ListBoxHeader("Load", 3);
				for (auto & name : this->presets)
				{
					if (ImGui::Selectable(name.c_str(), (name == this->currPreset)))
					{
						this->loadPreset(name);
					}
				}
				ImGui::ListBoxFooter();
			}
			ofxPreset::Gui::EndWindow(settings);

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow("Pop-Ups", settings))
			{
				ImGui::ListBoxHeader("List", 3);
				for (auto i = 0; i < this->popUps.size(); ++i)
				{
					auto name = "Pop-Up " + ofToString(i);
					ImGui::Checkbox(name.c_str(), &this->popUps[i]->editing);
				}
				ImGui::ListBoxFooter();

				if (ImGui::Button("Add Pop-Up..."))
				{
					ImGui::OpenPopup("Pop-Ups");
					ImGui::SameLine();
				}
				if (ImGui::BeginPopup("Pop-Ups"))
				{
					static vector<string> popUpNames{ "Image", "Video" };
					for (auto i = 0; i < popUpNames.size(); ++i)
					{
						if (ImGui::Selectable(popUpNames[i].c_str()))
						{
							if (i == 0)
							{
								this->addPopUp(popup::Type::Image);
							}
							else if (i == 1)
							{
								this->addPopUp(popup::Type::Video);
							}
						}
					}
					ImGui::EndPopup();
				}

				if (!this->popUps.empty())
				{
					ImGui::SameLine();
					if (ImGui::Button("Remove Pop-Up"))
					{
						this->removePopUp();
					}
				}
			}
			ofxPreset::Gui::EndWindow(settings);

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow("Mappings", settings))
			{
				for (auto & it : this->mappings)
				{
					auto mapping = it.second;
					if (ofxPreset::Gui::AddParameter(mapping->animated))
					{
						if (mapping->animated)
						{
							mapping->addTrack(this->timeline);
						}
						else
						{
							mapping->removeTrack(this->timeline);
						}
					}
				}
			}
			ofxPreset::Gui::EndWindow(settings);

			// Pop-up gui windows.
			{
				auto popUpSettings = ofxPreset::Gui::Settings();
				popUpSettings.windowPos.x += (settings.windowSize.x + kGuiMargin) * 2.0f;
				popUpSettings.windowPos.y = 0.0f;
				for (auto i = 0; i < this->popUps.size(); ++i)
				{
					this->popUps[i]->gui_(popUpSettings);
				}
				settings.mouseOverGui |= popUpSettings.mouseOverGui;
			}

			// Add a gui window for the Base parameters.
			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow(parameters.base.getName(), settings))
			{
				ofxPreset::Gui::AddParameter(parameters.base.background);
				
				if (ofxPreset::Gui::BeginTree(parameters.base.backCamera, settings))
				{
					if (ofxPreset::Gui::AddParameter(parameters.base.backCamera.mouseControl))
					{
						if (parameters.base.backCamera.mouseControl)
						{
							parameters.base.frontCamera.mouseControl = false;
						}
					}
					ofxPreset::Gui::AddParameter(parameters.base.backCamera.relativeYAxis);
					ofxPreset::Gui::AddParameter(parameters.base.backCamera.fov);
					ofxPreset::Gui::AddRange("Clipping", parameters.base.backCamera.nearClip, parameters.base.backCamera.farClip);

					if (ImGui::Button("Reset"))
					{
						this->resetCamera(render::Layout::Back);
					}

					ofxPreset::Gui::EndTree(settings);
				}

				if (ofxPreset::Gui::BeginTree(parameters.base.frontCamera, settings))
				{
					if (ofxPreset::Gui::AddParameter(parameters.base.frontCamera.mouseControl))
					{
						if (parameters.base.frontCamera.mouseControl)
						{
							parameters.base.backCamera.mouseControl = false;
						}
					}
					ofxPreset::Gui::AddParameter(parameters.base.frontCamera.relativeYAxis);
					ofxPreset::Gui::AddParameter(parameters.base.frontCamera.attachToBack);
					ofxPreset::Gui::AddParameter(parameters.base.frontCamera.fov);
					ofxPreset::Gui::AddRange("Clipping", parameters.base.frontCamera.nearClip, parameters.base.frontCamera.farClip);

					if (ImGui::Button("Reset"))
					{
						this->resetCamera(render::Layout::Front);
					}

					ofxPreset::Gui::EndTree(settings);
				}

				for (auto & it : this->boxes)
				{
					if (ImGui::CollapsingHeader(it.second.parameters.getName().c_str(), nullptr, true, true))
					{
						ofxPreset::Gui::AddParameter(it.second.enabled);
						static const vector<string> labels{ "None", "Back", "Front" };
						ofxPreset::Gui::AddRadio(it.second.cullFace, labels, 3);
						ofxPreset::Gui::AddParameter(it.second.color);
						ofxPreset::Gui::AddParameter(it.second.alpha);
						ofxPreset::Gui::AddParameter(it.second.size);
						ofxPreset::Gui::AddParameter(it.second.edgeRatio);
						ofxPreset::Gui::AddParameter(it.second.subdivisions);
					}
				}
			}
			ofxPreset::Gui::EndWindow(settings);

			// Add gui windows for the Post Effects parameters.
			for (auto & it : this->postEffects)
			{
				auto & postParameters = it.second;
				ofxPreset::Gui::SetNextWindow(settings);
				if (ofxPreset::Gui::BeginWindow(postParameters.getName().c_str(), settings))
				{
					ofxPreset::Gui::AddGroup(postParameters.bloom, settings);

					if (ImGui::CollapsingHeader(postParameters.color.getName().c_str(), nullptr, true, true))
					{
						ofxPreset::Gui::AddParameter(postParameters.color.exposure);
						ofxPreset::Gui::AddParameter(postParameters.color.gamma);
						static vector<string> labels =
						{
							"None",
							"Gamma Only",
							"Reinhard",
							"Reinhard Lum",
							"Filmic",
							"ACES",
							"Uncharted 2"
						};
						ofxPreset::Gui::AddRadio(postParameters.color.tonemapping, labels, 3);
						ofxPreset::Gui::AddParameter(postParameters.color.brightness);
                        ofxPreset::Gui::AddParameter(postParameters.color.contrast);
					}

                    ofxPreset::Gui::AddGroup(postParameters.vignette, settings);
				}
				ofxPreset::Gui::EndWindow(settings);
			}

			// Let the child class handle its child parameters.
			this->gui(settings);
		}

		//--------------------------------------------------------------
		void Base::serialize_(nlohmann::json & json)
		{
			ofxPreset::Serializer::Serialize(json, this->getParameters());
			ofxPreset::Serializer::Serialize(json, this->cameras[render::Layout::Back], "Camera Back");
			ofxPreset::Serializer::Serialize(json, this->cameras[render::Layout::Front], "Camera Front");
			
			this->serialize(json);

			auto & jsonMappings = json["Mappings"];
			for (auto & it : this->mappings)
			{
				ofxPreset::Serializer::Serialize(jsonMappings, it.second->animated);
			}

			auto & jsonPopUps = json["Pop-Ups"];
			for (auto popUp : this->popUps)
			{
				nlohmann::json jsonPopUp;
				popUp->serialize_(jsonPopUp);
				jsonPopUps.push_back(jsonPopUp);
			}
		}

		//--------------------------------------------------------------
		void Base::deserialize_(const nlohmann::json & json)
		{
			// Deserialize cameras first so that front attachment doesn't interfere with position.
			if (json.count("Camera Back"))
			{
				ofxPreset::Serializer::Deserialize(json, this->cameras[render::Layout::Back], "Camera Back");
			}
			if (json.count("Camera Front"))
			{
				cout << "Deserializing camera" << endl;
				ofxPreset::Serializer::Deserialize(json, this->cameras[render::Layout::Front], "Camera Front");
			}
			ofxPreset::Serializer::Deserialize(json, this->getParameters());
			
			// Clear previous Pop-ups.
			while (!this->popUps.empty())
			{
				this->removePopUp();
			}

			// Add new Pop-ups.
			if (json.count("Pop-Ups"))
			{
				for (auto & jsonPopUp : json["Pop-Ups"])
				{
					int typeAsInt = jsonPopUp["type"];
					popup::Type type = static_cast<popup::Type>(typeAsInt);

					auto popUp = this->addPopUp(type);
					if (popUp)
					{
						popUp->deserialize_(jsonPopUp);
					}
				}
			}

			this->deserialize(json);

			for (auto & it : this->mappings)
			{
				it.second->animated.set(false);
			}
			if (json.count("Mappings"))
			{
				auto & jsonGroup = json["Mappings"];
				for (auto & it : this->mappings)
				{
					ofxPreset::Serializer::Deserialize(jsonGroup, it.second->animated);
				}
			}
			this->refreshMappings();
		}

		//--------------------------------------------------------------
		void Base::drawTimeline(ofxPreset::Gui::Settings & settings)
		{
			// Disable mouse events if it's already been captured.
			if (settings.mouseOverGui)
			{
				this->timeline.disableEvents();
			}
			else
			{
				this->timeline.enableEvents();
			}

			this->timeline.setWidth(settings.screenBounds.getWidth());
			this->timeline.setOffset(glm::vec2(settings.screenBounds.getMinY(), settings.screenBounds.getMaxY() - this->timeline.getHeight()));
			this->timeline.draw();
			settings.mouseOverGui |= this->timeline.getDrawRect().inside(ofGetMouseX(), ofGetMouseY());
		}

		//--------------------------------------------------------------
		int Base::getCurrentTimelineFrame()
		{
			return this->timeline.getCurrentFrame();
		}

		//--------------------------------------------------------------
		string Base::getAssetsPath(const string & file)
		{
			if (this->assetsPath.empty())
			{
				auto tokens = ofSplitString(this->getName(), "::", true, true);
				auto assetsPath = GetSharedAssetsPath();
				for (auto & component : tokens)
				{
					assetsPath = ofFilePath::addTrailingSlash(assetsPath.append(component));
				}
				this->assetsPath = assetsPath;
			}
			if (file.empty())
			{
				return this->assetsPath;
			}

			auto filePath = this->assetsPath;
			filePath.append(file);
			return filePath;
		}

		//--------------------------------------------------------------
		string Base::getDataPath(const string & file)
		{
			if (this->dataPath.empty())
			{
				auto tokens = ofSplitString(this->getName(), "::", true, true);
				auto dataPath = GetSharedDataPath();
				for (auto & component : tokens)
				{
					dataPath = ofFilePath::addTrailingSlash(dataPath.append(component));
				}
				this->dataPath = dataPath;
			}
			if (file.empty()) 
			{
				return this->dataPath;
			}

			auto filePath = this->dataPath;
			filePath.append(file);
			return filePath;
		}

		//--------------------------------------------------------------
		string Base::getPresetPath(const string & preset)
		{
			auto presetPath = ofFilePath::addTrailingSlash(this->getDataPath("presets"));
			if (!preset.empty())
			{
				presetPath.append(ofFilePath::addTrailingSlash(preset));
			}
			return presetPath;
		}

		//--------------------------------------------------------------
		string Base::getCurrentPresetPath(const string & file)
		{
			auto currentPresetPath = this->getPresetPath(this->currPreset);
			if (file.empty())
			{
				return currentPresetPath;
			}

			currentPresetPath.append(file);
			return currentPresetPath;
		}

		//--------------------------------------------------------------
		const vector<string> & Base::getPresets() const
		{
			return this->presets;
		}

		//--------------------------------------------------------------
		const string & Base::getCurrentPresetName() const
		{
			return this->currPreset;
		}

		//--------------------------------------------------------------
		bool Base::loadPreset(const string & presetName)
		{
			const auto presetPath = this->getPresetPath(presetName);
			auto presetFile = ofFile(presetPath);
			if (!presetFile.exists())
			{
				ofLogWarning("Base::loadPreset") << "File not found at path " << presetPath;
				return false;
			}

			auto paramsPath = presetPath;
			paramsPath.append("parameters.json");
			auto paramsFile = ofFile(paramsPath);
			if (paramsFile.exists())
			{
				nlohmann::json json;
				paramsFile >> json;

				this->deserialize_(json);
			}

			this->timeline.loadTracksFromFolder(presetPath);

			this->currPreset = presetName;

			this->presetLoadedEvent.notify(this->currPreset);

			return true;
		}

		//--------------------------------------------------------------
		bool Base::savePreset(const string & presetName)
		{
			const auto presetPath = this->getPresetPath(presetName);

			auto paramsPath = presetPath;
			paramsPath.append("parameters.json");
			auto paramsFile = ofFile(paramsPath, ofFile::WriteOnly);
			nlohmann::json json;
			this->serialize_(json);
			paramsFile << json.dump(4);

			this->timeline.saveTracksToFolder(presetPath);

			this->populatePresets();

			return true;
		}

		//--------------------------------------------------------------
		void Base::populatePresets()
		{
			auto presetsPath = this->getPresetPath();
			auto presetsDir = ofDirectory(presetsPath);
			presetsDir.listDir();
			presetsDir.sort();

			this->presets.clear();
			for (auto i = 0; i < presetsDir.size(); ++i)
			{
				if (presetsDir.getFile(i).isDirectory())
				{
					this->presets.push_back(presetsDir.getName(i));
				}
			}
		}

		//--------------------------------------------------------------
		void Base::populateMappings(const ofParameterGroup & group, string name)
		{
			for (const auto & parameter : group)
			{
				// Group.
				auto parameterGroup = dynamic_pointer_cast<ofParameterGroup>(parameter);
				if (parameterGroup)
				{
					// Recurse through contents.
					this->populateMappings(*parameterGroup, name);
					continue;
				}

				// Parameter.
				{
					auto parameterFloat = dynamic_pointer_cast<ofParameter<float>>(parameter);
					if (parameterFloat)
					{
						auto mapping = make_shared<util::Mapping<float, ofxTLCurves>>();
						mapping->setup(parameterFloat);
						this->mappings.emplace(mapping->getName(), mapping);
						continue;
					}

					auto parameterInt = dynamic_pointer_cast<ofParameter<int>>(parameter);
					if (parameterInt)
					{
						auto mapping = make_shared<util::Mapping<int, ofxTLCurves>>();
						mapping->setup(parameterInt);
						this->mappings.emplace(mapping->getName(), mapping);
						continue;
					}

					auto parameterBool = dynamic_pointer_cast<ofParameter<bool>>(parameter);
					if (parameterBool)
					{
						auto mapping = make_shared<util::Mapping<bool, ofxTLSwitches>>();
						mapping->setup(parameterBool);
						this->mappings.emplace(mapping->getName(), mapping);
						continue;
					}

					auto parameterColor = dynamic_pointer_cast<ofParameter<ofFloatColor>>(parameter);
					if (parameterColor)
					{
						auto mapping = make_shared<util::Mapping<ofFloatColor, ofxTLColorTrack>>();
						mapping->setup(parameterColor);
						this->mappings.emplace(mapping->getName(), mapping);
						continue;
					}
				}
			}
		}

		//--------------------------------------------------------------
		void Base::refreshMappings()
		{
			for (auto & it : this->mappings)
			{
				auto mapping = it.second;
				if (mapping->animated) 
				{
					mapping->addTrack(this->timeline);
				}
				else
				{
					mapping->removeTrack(this->timeline);
				}
			}
		}

		//--------------------------------------------------------------
		void Base::resetCamera(render::Layout layout)
		{
			auto & parameters = this->getParameters();
			
			//this->cameras[layout].setupPerspective(false, 60.0f, 0.1f, 100000.0f);
			//this->cameras[layout].setVFlip(false);
			//this->cameras[layout].setNearClip(0.1f);
			//this->cameras[layout].setFarClip(100000.0f);
			//this->cameras[layout].setFov(60.0f);
			this->cameras[layout].setAspectRatio(GetCanvasWidth(layout) / GetCanvasHeight(layout));
			
			if (layout == render::Layout::Front)
			{
				this->cameras[render::Layout::Front].clearParent();
			}
			
			this->cameras[layout].reset();

			if (layout == render::Layout::Front && parameters.base.frontCamera.attachToBack)
			{
				this->cameras[render::Layout::Front].setParent(this->cameras[render::Layout::Back], true);
			}
		}

		//--------------------------------------------------------------
		shared_ptr<popup::Base> Base::addPopUp(popup::Type type)
		{
			shared_ptr<popup::Base> popUp;
			if (type == popup::Type::Image)
			{
				popUp = make_shared<popup::Image>();
			}
			else if (type == popup::Type::Video)
			{
				popUp = make_shared<popup::Video>();
			}
			else
			{
				ofLogError(__FUNCTION__) << "Unrecognized pop-up type " << static_cast<int>(type);
				return nullptr;
			}

			auto idx = this->popUps.size();
			popUp->setup_(idx);
			popUp->addTrack(this->timeline);
			this->popUps.push_back(popUp);

			return popUp;
		}

		//--------------------------------------------------------------
		void Base::removePopUp()
		{
			auto popUp = this->popUps.back();
			popUp->removeTrack(this->timeline);
			popUp->exit_();
			this->popUps.pop_back();
		}

		//--------------------------------------------------------------
		ofEasyCam & Base::getCameraBack()
		{
			return this->cameras[render::Layout::Back];
		}
		
		//--------------------------------------------------------------
		ofEasyCam & Base::getCameraFront()
		{
			return this->cameras[render::Layout::Front];
		}

		//--------------------------------------------------------------
		void Base::setCameraControlArea(render::Layout layout, const ofRectangle & controlArea)
		{
			this->cameras[layout].setControlArea(controlArea);
		}

		//--------------------------------------------------------------
		void Base::setCameraLocked(bool cameraLocked)
		{
			for (auto & it : this->cameraTracks)
			{
				it.second->lockCameraToTrack = cameraLocked;
			}
		}

		//--------------------------------------------------------------
		void Base::toggleCameraLocked()
		{
			for (auto & it : this->cameraTracks)
			{
				it.second->lockCameraToTrack ^= 1;
			}
		}

		//--------------------------------------------------------------
		bool Base::isCameraLocked() const
		{
			return this->cameraTracks.at(render::Layout::Back)->lockCameraToTrack;
		}

		//--------------------------------------------------------------
		void Base::addCameraKeyframe(render::Layout layout)
		{
			this->cameraTracks[layout]->addKeyframe();
        }

        //--------------------------------------------------------------
        render::PostParameters & Base::getPostParameters(render::Layout layout)
        {
            return this->postEffects[layout];
        }

		//--------------------------------------------------------------
		void Base::beginExport()
		{
			this->timeline.setFrameBased(true);
			for (auto & it : this->cameraTracks)
			{
				it.second->lockCameraToTrack = true;
			}
			this->timeline.setCurrentTimeToInPoint();
			this->timeline.play();
		}

		//--------------------------------------------------------------
		void Base::endExport()
		{
			this->timeline.stop();
			this->timeline.setFrameBased(false);
			for (auto & it : this->cameraTracks)
			{
				it.second->lockCameraToTrack = false;
			}
		}
	}
}
