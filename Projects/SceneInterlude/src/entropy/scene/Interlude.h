#pragma once

#include "entropy/geom/Stripes.h"
#include "entropy/render/Layout.h"
#include "entropy/scene/Base.h"
#include "entropy/interlude/PassingNoiseParticle.h"
namespace entropy
{
	namespace scene
	{
		class Interlude
			: public Base
		{
		public:
			string getName() const override
			{
				return "entropy::scene::Interlude";
			}

			Interlude();
			~Interlude();

			void init() override;
			void clear() override;

			void setup() override;
			void exit() override;

			void resizeBack(ofResizeEventArgs & args) override;
			void resizeFront(ofResizeEventArgs & args) override;

			void update(double dt) override;

			void drawBackWorld() override;
			void drawFrontWorld() override;

			void gui(ofxPreset::Gui::Settings & settings) override;

			void serialize(nlohmann::json & json) override;
			void deserialize(const nlohmann::json & json) override;

			static const int MAX_NUM_STRIPES = 8;
		protected:
			PassingNoiseParticle part;
		protected:
			std::shared_ptr<geom::Stripes> addStripes(render::Layout layout);
			void removeStripes(render::Layout layout);

			std::map<render::Layout, std::vector<std::shared_ptr<geom::Stripes>>> stripes;
			std::map<render::Layout, bool[MAX_NUM_STRIPES]> openGuis;  // Don't use vector<bool> because they're weird: http://en.cppreference.com/w/cpp/container/vector_bool

			virtual ofParameterGroup & getParameters() override
			{
				return this->parameters;
			}

			struct : ofParameterGroup
			{
				struct : ofParameterGroup
				{
					ofParameter<float> backAlpha{ "Back Alpha", 1.0f, 0.0f, 1.0f };
					ofParameter<float> frontAlpha{ "Front Alpha", 1.0f, 0.0f, 1.0f };

					PARAM_DECLARE("Stripes", backAlpha, frontAlpha);
				} stripes;

				struct : ofParameterGroup {
					ofParameter<float> wind_x{ "wind_x", -11.0f, -30.0f, 30.0f };
					ofParameter<float> wind_y{ "wind_y", 0.0f, -30.0f, 30.0f };
					ofParameter<float> wind_z{ "wind_z", 8.6f, -30.0f, 30.0f };

					ofParameter<float> noise_scale{ "noise_scale", 0.001f, 0.001f, 0.01f };
					ofParameter<float> noise_speed{ "noise_speed", 2.562f, 0.1f, 10.0f };

					ofParameter<int> line_params_max_len{ "line_max_len", 5, 0, 100.0 };
					ofParameter<int> line_params_step{ "line_step", 4, 1, 10.0 };

					ofParameter<float> point_alpha{ "point_alpha", 1.0f, 0.0f, 1.0f };
					ofParameter<float> box_line_alpha{ "box_line_alpha", 1.0f, 0.0f, 1.0f };

					ofParameter<bool> reset_particle{ "reset", false };

					PARAM_DECLARE("particles", wind_x, wind_y, wind_z, noise_scale, line_params_max_len, line_params_step,
						point_alpha, box_line_alpha, reset_particle);
				} particles;

				PARAM_DECLARE("Interlude", 
					stripes, particles);
			} parameters;
		
		};
	}
}
