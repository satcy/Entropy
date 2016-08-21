#include "DataSet.h"

#include "ofxHDF5.h"

#include "entropy/Helpers.h"

namespace entropy
{
	namespace survey
	{
		//--------------------------------------------------------------
		DataSet::DataSet()
		{
			this->clear();
		}
		
		//--------------------------------------------------------------
		DataSet::~DataSet()
		{
			this->clear();
		}

		//--------------------------------------------------------------
		void DataSet::setup(const std::string & name, const std::string & format, size_t startIdx, size_t count)
		{
			this->parameters.setName(name);

			this->clear();

			// Load the data one fragment at a time.
			for (int i = 0; i < count; ++i)
			{
				char filename[256];
				sprintf(filename, format.c_str(), (i + startIdx + 1));
				const auto filePath = GetCurrentSceneAssetsPath(filename);
				this->loadFragment(filePath, this->points);
			}

			// Sort the data points by radius.
			std::sort(this->points.begin(), this->points.end(), [](const glm::vec4 & a, const glm::vec4 & b) -> bool
			{
				return (a.z < b.z);
			});

			// Upload everything to the vbo.
			this->vbo.setVertexData((float *)(this->points.data()), 4, this->points.size(), GL_STATIC_DRAW, sizeof(glm::vec4));
		}
		
		//--------------------------------------------------------------
		void DataSet::clear()
		{
			this->points.clear();

			this->vbo.clear();
			this->vboDirty = false;
		}

		//--------------------------------------------------------------
		size_t DataSet::loadFragment(const string & filePath, vector<glm::vec4> & points)
		{
			static const int stride = 1;

			ofxHDF5File h5File;
			h5File.open(filePath, true);
			ofxHDF5GroupPtr h5Group = h5File.loadGroup("PartType6");

			// Load the coordinate data.
			auto coordsDataSet = h5Group->loadDataSet("Coordinates");
			int coordsCount = coordsDataSet->getDimensionSize(0) / stride;
			coordsDataSet->setHyperslab(0, coordsCount, stride);

			vector<glm::vec3> coordsData(coordsCount);
			coordsDataSet->read(coordsData.data());

			// Load the mass data.
			auto massesDataSet = h5Group->loadDataSet("Masses");
			int massesCount = massesDataSet->getDimensionSize(0) / stride;
			massesDataSet->setHyperslab(0, massesCount, stride);

			vector<float> massesData(massesCount);
			massesDataSet->read(massesData.data());

			// Combine the data into a single struct.
			// x == longitude
			// y == latitude
			// z == radius
			// w == mass
			for (int i = 0; i < coordsData.size(); ++i)
			{
				points.push_back(glm::vec4(ofDegToRad(coordsData[i].x), ofDegToRad(coordsData[i].y), coordsData[i].z, massesData[i]));
			}

			return coordsData.size();

			//// Sort the data points by radius.
			//std::sort(points.begin(), points.end(), [](const glm::vec4 & a, const glm::vec4 & b) -> bool
			//{
			//	return (a.z < b.z);
			//});

			//// Convert the position data to Cartesian coordinates and store all data in the passed vectors.
			//int startIdx = vertices.size();
			//vertices.resize(startIdx + points.size());
			//masses.resize(startIdx + massesData.size());
			//for (int i = 0; i < points.size(); ++i)
			//{
			//	int idx = startIdx + i;
			//	vertices[idx].x = points[i].radius * cos(ofDegToRad(points[i].latitude)) * cos(ofDegToRad(points[i].longitude));
			//	vertices[idx].y = points[i].radius * cos(ofDegToRad(points[i].latitude)) * sin(ofDegToRad(points[i].longitude));
			//	vertices[idx].z = points[i].radius * sin(ofDegToRad(points[i].latitude));
			//	masses[idx] = massesData[i];
			//}

			//return points.size();
		}

		//--------------------------------------------------------------
		void DataSet::update()
		{
			if (this->vboDirty)
			{
				this->vboDirty = false;
			}
		}
		
		//--------------------------------------------------------------
		void DataSet::draw()
		{
			ofSetColor(this->parameters.color.get());

			int startIdx = ofMap(this->parameters.minDistance, 0.0f, 1.0f, 0, this->points.size(), true);
			int endIdx = ofMap(this->parameters.maxDistance, 0.0f, 1.0f, 0, this->points.size(), true);

			this->vbo.draw(GL_POINTS, startIdx, endIdx - startIdx + 1);
		}

		//--------------------------------------------------------------
		void DataSet::gui(ofxPreset::Gui::Settings & settings)
		{
			if (ImGui::CollapsingHeader(this->parameters.getName().c_str(), nullptr, true, true))
			{
				ofxPreset::Gui::AddRange("Distance", this->parameters.minDistance, this->parameters.maxDistance);
			}
		}

		//--------------------------------------------------------------
		void DataSet::serialize(nlohmann::json & json)
		{
			ofxPreset::Serializer::Serialize(json, this->parameters);
		}

		//--------------------------------------------------------------
		void DataSet::deserialize(const nlohmann::json & json)
		{
			ofxPreset::Serializer::Deserialize(json, this->parameters);
		}
	}
}