#pragma once
#include "glad/glad.h"
#include "Ecs/EngineComponents.h"
#include "Ecs/System.h"
#include "Ecs/Entity.h"
#include <GLFW/glfw3.h>
#include <vector>
#include "Graphics/Texture.h"
#include <filesystem> 
#define GLFW_INCLUDE_NONE

namespace tinygltf
{
	struct Model;
	struct Node;
}

namespace Stellar
{
	struct Mesh
	{
		GLuint vao;
		GLuint vbo;
		GLuint ibo;
		std::vector<GLuint> texture;
		unsigned int indicesCount;
		GLenum IBOType;
		//Set to true if you are trying to instance/combine and are rendering multiple meshes as one model
		//Set to false if you are trying to instance/combine and are rendering only one mesh as a model.
		bool renderAsEntity;
	};

	struct Light
	{
		Light(glm::vec3 a_pos, glm::vec3 a_col, float a_intensity, float a_range, std::string a_type) :
			pos(a_pos), col(a_col), intensity(a_intensity), range(a_range), type(a_type) {}
		Light()
		{
			pos = glm::vec3(0);
			col = glm::vec3(0);
			intensity = 0.f;
			range = 1.f;
			type = "";
		}
		vec3  pos;
		vec3  col;
		float intensity;
		float range;
		std::string type;

	};

	class Model
	{
	public:
		// Name of the GLTF file + extension 
		// e.g., Axolotl.gltf
		Model(const std::filesystem::path& a_Path);
	
		void							Render();
		std::vector<Mesh>				GetMeshes()  const { return m_Meshes; }
		std::vector<Light>				GetLights() const { return m_Lights; }
		Mesh							GetMesh(GLuint a_Index)   const { return m_Meshes[a_Index]; }
	private:
		void							LoadNode(const tinygltf::Model&, const tinygltf::Node&, vec3 a_translation);
		void							LoadMeshes(const tinygltf::Model&, const bool& a_OnlyModel);
		void							LoadVBOs(const tinygltf::Model&);
		void							LoadTextures(const tinygltf::Model&, const std::string&);
		void							RemoveLights();


		std::vector<GLuint>				m_VBO;
		std::vector<Mesh>				m_Meshes;
		std::vector<GLuint>				m_Textures;
		std::vector<Light>				m_Lights;
		Texture							m_Texture;
	};
};
#endif