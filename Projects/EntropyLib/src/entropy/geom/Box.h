#pragma once

#include "ofMain.h"
#include "entropy/render/WireframeFillRenderer.h"

namespace entropy
{
	namespace geom
	{
		class Box
		{
		public:
			Box();
			~Box();

			void clear();
			bool update();

			void draw() const;
			void draw(render::WireframeFillRenderer & renderer);

			const ofVboMesh & getMesh();

			ofParameter<bool> enabled{ "Enabled", true };
			ofParameter<int> cullFace{ "Cull Face", static_cast<int>(CullMode::Back), static_cast<int>(CullMode::None), static_cast<int>(CullMode::Front) };
			ofParameter<ofFloatColor> color{ "Color", ofFloatColor::white };
			ofParameter<float> size{ "Size", 1.0f, 0.0f, 2.0f };
			ofParameter<float> edgeWidth{ "Edge Width", 1.0f, 0.001f, 2.0f };
			ofParameter<int> subdivisions{ "Subdivisions", 1, 1, 10 };

			ofParameterGroup parameters
			{
				"Box",
				enabled,
				cullFace,
				color,
				size,
				edgeWidth,
				subdivisions
			};

		protected:
			enum class CullMode
			{
				None,
				Back,
				Front
			};

			typedef enum
			{
				Front  = 0x000001,
				Back   = 0x000010, 
				Right  = 0x000100,
				Left   = 0x001000,
				Top    = 0x010000,
				Bottom = 0x100000,

				All    = 0x111111
			} Face;

			void rebuildMesh();
			void addEdge(const glm::vec3 & center, const glm::vec3 & dimensions, int faces);

			ofVboMesh mesh;
			bool meshDirty;
			bool colorDirty;

			ofLight light;
			ofMaterial material;

			vector<ofEventListener> paramListeners;
		};
	}
}