#include "Interlude.h"

#include "entropy/Helpers.h"

namespace entropy
{
	namespace scene
	{
		//--------------------------------------------------------------
		Interlude::Interlude()
			: Base()
		{}
		
		//--------------------------------------------------------------
		Interlude::~Interlude()
		{
			this->clear();
		}

		//--------------------------------------------------------------
		void Interlude::clear()
		{

		}

		//--------------------------------------------------------------
		void Interlude::setup()
		{
			part.setup();
		}
		
		//--------------------------------------------------------------
		void Interlude::exit()
		{

		}

		//--------------------------------------------------------------
		void Interlude::resizeBack(ofResizeEventArgs & args)
		{

		}

		//--------------------------------------------------------------
		void Interlude::resizeFront(ofResizeEventArgs & args)
		{

		}

		//--------------------------------------------------------------
		void Interlude::update(double dt)
		{
			part.update();
		}

		//--------------------------------------------------------------
		void Interlude::drawBackBase()
		{

			this->cameras[entropy::render::Layout::Back]->begin();
			part.draw(this->cameras[entropy::render::Layout::Back]->getEasyCam());

			this->cameras[entropy::render::Layout::Back]->end();
		}

		//--------------------------------------------------------------
		void Interlude::drawFrontBase()
		{
			this->cameras[entropy::render::Layout::Front]->begin();
			part.draw(this->cameras[entropy::render::Layout::Front]->getEasyCam() );
			
			this->cameras[entropy::render::Layout::Front]->end();
		}

		//--------------------------------------------------------------
		void Interlude::gui(ofxPreset::Gui::Settings & settings)
		{
			
		}
		
		//--------------------------------------------------------------
		void Interlude::deserialize(const nlohmann::json & json)
		{

		}
	}
}