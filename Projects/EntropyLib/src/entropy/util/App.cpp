#include "App.h"

#include "ofxPreset.h"
#include "entropy/Helpers.h"

namespace entropy
{
	namespace util
	{
		//--------------------------------------------------------------
		App_::App_()
		{
			// Instantiate attributes.
			this->canvasBack = make_shared<entropy::render::Canvas>("Canvas Back");
			this->canvasFront = make_shared<entropy::render::Canvas>("Canvas Front");
			this->sceneManager = make_shared<entropy::scene::Manager>();

			// Setup gui.
			this->controlScreenParameters.setName("Control Screen");
			this->backScreenParameters.setName("Back Screen");
			this->frontScreenParameters.setName("Front Screen");

			this->imGui.setup();
			this->overlayVisible = true;
			
			// Register events listeners.
			ofAddListener(ofEvents().update, this, &App_::onUpdate);
			ofAddListener(ofEvents().draw, this, &App_::onDraw);
			
			ofAddListener(ofEvents().keyPressed, this, &App_::onKeyPressed);
			ofAddListener(ofEvents().keyReleased, this, &App_::onKeyReleased);

			ofAddListener(ofEvents().mouseMoved, this, &App_::onMouseMoved);
			ofAddListener(ofEvents().mousePressed, this, &App_::onMousePressed);
			ofAddListener(ofEvents().mouseDragged, this, &App_::onMouseDragged);
			ofAddListener(ofEvents().mouseReleased, this, &App_::onMouseReleased);

			ofAddListener(this->canvasBack->resizeEvent, this, &App_::onCanvasBackResized);
			ofAddListener(this->canvasFront->resizeEvent, this, &App_::onCanvasFrontResized);
			ofAddListener(ofEvents().windowResized, this, &App_::onWindowResized);

			// Load initial settings, if any.
			this->loadSettings();
		}

		//--------------------------------------------------------------
		App_::~App_()
		{
			// Unregister event listeners.
			ofRemoveListener(ofEvents().update, this, &App_::onUpdate);
			ofRemoveListener(ofEvents().draw, this, &App_::onDraw);

			ofRemoveListener(ofEvents().keyPressed, this, &App_::onKeyPressed);
			ofRemoveListener(ofEvents().keyReleased, this, &App_::onKeyReleased);

			ofRemoveListener(ofEvents().mouseMoved, this, &App_::onMouseMoved);
			ofRemoveListener(ofEvents().mousePressed, this, &App_::onMousePressed);
			ofRemoveListener(ofEvents().mouseDragged, this, &App_::onMouseDragged);
			ofRemoveListener(ofEvents().mouseReleased, this, &App_::onMouseReleased);

			ofRemoveListener(this->canvasBack->resizeEvent, this, &App_::onCanvasBackResized);
			ofRemoveListener(this->canvasFront->resizeEvent, this, &App_::onCanvasFrontResized);
			ofRemoveListener(ofEvents().windowResized, this, &App_::onWindowResized);

			this->imGui.close();

			// Reset pointers.
			this->canvasBack.reset();
			this->canvasFront.reset();
			this->sceneManager.reset();
		}

		//--------------------------------------------------------------
		const string & App_::getDataPath()
		{
			static string dataPath;
			if (dataPath.empty())
			{
				dataPath = GetSharedDataPath();
				dataPath = ofFilePath::addTrailingSlash(dataPath.append("entropy"));
				dataPath = ofFilePath::addTrailingSlash(dataPath.append("util"));
				dataPath = ofFilePath::addTrailingSlash(dataPath.append("App"));
			}
			return dataPath;
		}

		//--------------------------------------------------------------
		const string & App_::getSettingsFilePath()
		{
			static string filePath;
			if (filePath.empty())
			{
				filePath = this->getDataPath();
				filePath.append("settings.json");
			}
			return filePath;
		}

		//--------------------------------------------------------------
		bool App_::loadSettings()
		{
			auto filePath = this->getSettingsFilePath();
			auto file = ofFile(filePath, ofFile::ReadOnly);
			if (!file.exists())
			{
				ofLogWarning(__FUNCTION__) << "File not found at path " << filePath;
				return false;
			}

			nlohmann::json json;
			file >> json;

			ofxPreset::Serializer::Deserialize(json, this->controlScreenParameters);
			ofxPreset::Serializer::Deserialize(json, this->backScreenParameters);
			ofxPreset::Serializer::Deserialize(json, this->frontScreenParameters);

			return true;
		}

		//--------------------------------------------------------------
		bool App_::saveSettings()
		{
			nlohmann::json json;
			ofxPreset::Serializer::Serialize(json, this->controlScreenParameters);
			ofxPreset::Serializer::Serialize(json, this->backScreenParameters);
			ofxPreset::Serializer::Serialize(json, this->frontScreenParameters);

			auto filePath = this->getSettingsFilePath();
			auto file = ofFile(filePath, ofFile::WriteOnly);
			file << json.dump(4);

			return true;
		}

		//--------------------------------------------------------------
		shared_ptr<entropy::render::Canvas> App_::getCanvasBack() const
		{
			return this->canvasBack;
		}

		//--------------------------------------------------------------
		shared_ptr<entropy::render::Canvas> App_::getCanvasFront() const
		{
			return this->canvasFront;
		}

		//--------------------------------------------------------------
		shared_ptr<entropy::scene::Manager> App_::getSceneManager() const
		{
			return this->sceneManager;
		}

		//--------------------------------------------------------------
		const ofRectangle & App_::getBoundsControl() const
		{
			return this->boundsControl;
		}

		//--------------------------------------------------------------
		const ofRectangle & App_::getBoundsBack() const
		{
			return this->boundsBack;
		}

		//--------------------------------------------------------------
		const ofRectangle & App_::getBoundsFront() const
		{
			return this->viewportFront;
		}

		//--------------------------------------------------------------
		bool App_::isMouseOverGui() const
		{
			return this->guiSettings.mouseOverGui;
		}
		
		//--------------------------------------------------------------
		bool App_::isOverlayVisible() const
		{
			return this->overlayVisible;
		}

		//--------------------------------------------------------------
		void App_::onUpdate(ofEventArgs & args)
		{
			if (this->overlayVisible || ofGetWindowMode() == OF_WINDOW)
			{
				ofShowCursor();
			}
			else
			{
				ofHideCursor();
			}

			this->canvasBack->update();
			this->canvasFront->update();

			auto dt = ofGetLastFrameTime();
			this->sceneManager->update(dt);
		}
		
		//--------------------------------------------------------------
		void App_::onDraw(ofEventArgs & args)
		{
			// Back screen.
			{
				// Draw the content.
				this->canvasBack->beginDraw();
				{
					this->sceneManager->drawSceneBack();
				}
				this->canvasBack->endDraw();

				// Post-process the content if necessary.
				const auto postProcessing = this->sceneManager->postProcess(this->canvasBack->getDrawTexture(), this->canvasBack->getPostFbo());

				// Render the scene.
				this->canvasBack->render(postProcessing);
			}

			// Front screen.
			{
				// Draw the content.
				this->canvasFront->beginDraw();
				{
					this->sceneManager->drawSceneFront();
				}
				this->canvasFront->endDraw();

				// Post-process the content if necessary.
				const auto postProcessing = this->sceneManager->postProcess(this->canvasFront->getDrawTexture(), this->canvasFront->getPostFbo());

				// Render the scene.
				this->canvasFront->render(postProcessing);
			}

			if (!this->overlayVisible) return;

			// Draw the gui overlay.
			this->imGui.begin();
			{
				this->guiSettings.mouseOverGui = this->canvasBack->isEditing() || this->canvasFront->isEditing();
				this->guiSettings.windowPos = ofVec2f(kGuiMargin, kGuiMargin);
				this->guiSettings.windowSize = ofVec2f::zero();

				if (ofxPreset::Gui::BeginWindow("App", this->guiSettings))
				{
					ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ofGetFrameRate());

					ofxPreset::Gui::AddGroup(this->controlScreenParameters, this->guiSettings);
					ofxPreset::Gui::AddGroup(this->backScreenParameters, this->guiSettings);
					ofxPreset::Gui::AddGroup(this->frontScreenParameters, this->guiSettings);

					if (ImGui::Button("Apply and Save"))
					{
						this->applyConfiguration();
						this->saveSettings();
					}
					ImGui::SameLine();
					if (ImGui::Button("Restore"))
					{
						this->loadSettings();
					}
				}
				ofxPreset::Gui::EndWindow(this->guiSettings);

				this->canvasBack->drawGui(this->guiSettings);
				this->canvasFront->drawGui(this->guiSettings);
				this->sceneManager->drawGui(this->guiSettings);
			}
			this->imGui.end();

			// Draw the non-gui overlay.
			this->sceneManager->drawOverlay(this->guiSettings);
		}

		//--------------------------------------------------------------
		void App_::applyConfiguration()
		{
			// Calculate the bounds per screen.
			ofRectangle totalBounds;
			if (this->controlScreenParameters.enabled)
			{
				this->boundsControl = ofRectangle(totalBounds.getMaxX(), totalBounds.getMinY(), this->controlScreenParameters.screenWidth * this->controlScreenParameters.numCols, this->controlScreenParameters.screenHeight * this->controlScreenParameters.numRows);
				totalBounds.growToInclude(this->boundsControl);
			}
			if (this->backScreenParameters.enabled)
			{
				this->boundsBack = ofRectangle(totalBounds.getMaxX(), totalBounds.getMinY(), this->backScreenParameters.screenWidth * this->backScreenParameters.numCols, this->backScreenParameters.screenHeight * this->backScreenParameters.numRows);
				totalBounds.growToInclude(this->boundsBack);
			}
			if (this->frontScreenParameters.enabled)
			{
				this->viewportFront = ofRectangle(totalBounds.getMaxX(), totalBounds.getMinY(), this->frontScreenParameters.screenWidth * this->frontScreenParameters.numCols, this->frontScreenParameters.screenHeight * this->frontScreenParameters.numRows);
				totalBounds.growToInclude(this->viewportFront);
			}

			// Resize the window.
			ofSetWindowShape(totalBounds.width, totalBounds.height);
		}
		
		//--------------------------------------------------------------
		void App_::onKeyPressed(ofKeyEventArgs & args)
		{
			if (args.keycode == GLFW_KEY_F && (ofGetKeyPressed(OF_KEY_CONTROL) || ofGetKeyPressed(OF_KEY_COMMAND)))
			{
				ofToggleFullscreen();
				return;
			}
			if (args.key == '`')
			{
				this->overlayVisible ^= 1;
			}
			if (this->canvasBack->keyPressed(args))
			{
				return;
			}
			if (this->canvasFront->keyPressed(args))
			{
				return;
			}
			if (this->sceneManager->keyPressed(args))
			{
				return;
			}
		}

		//--------------------------------------------------------------
		void App_::onKeyReleased(ofKeyEventArgs & args)
		{

		}

		//--------------------------------------------------------------
		void App_::onMouseMoved(ofMouseEventArgs & args)
		{
			//this->canvas->cursorMoved(args);
		}

		//--------------------------------------------------------------
		void App_::onMousePressed(ofMouseEventArgs & args)
		{
			//this->canvas->cursorDown(args);
		}

		//--------------------------------------------------------------
		void App_::onMouseDragged(ofMouseEventArgs & args)
		{
			//this->canvas->cursorDragged(args);
		}

		//--------------------------------------------------------------
		void App_::onMouseReleased(ofMouseEventArgs & args)
		{

		}

		//--------------------------------------------------------------
		void App_::onCanvasBackResized(ofResizeEventArgs & args)
		{
			this->sceneManager->canvasResized(args);
		}

		//--------------------------------------------------------------
		void App_::onCanvasFrontResized(ofResizeEventArgs & args)
		{
			this->sceneManager->canvasResized(args);
		}

		//--------------------------------------------------------------
		void App_::onWindowResized(ofResizeEventArgs & args)
		{
			//this->canvas->windowResized(args);
		}
	}
}