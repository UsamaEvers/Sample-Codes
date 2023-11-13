#include "pch.h"
#include "Model.h"
#include <Core/Engine.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <TinyGLTF/tiny_gltf.h>

Stellar::Model::Model(const std::string& a_PathFileName, const std::string& a_Path, const bool& a_renderAsEntity)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool isglb = a_PathFileName.find(".glb") != std::string::npos;
	bool ret;
	if (isglb)
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, a_PathFileName); // for binary glTF(.glb)
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, a_PathFileName);
	}

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
	}

	LoadVBOs(model);
	LoadTextures(model, a_Path);
	LoadMeshes(model, a_renderAsEntity);

}
Stellar::Model::Model(const std::filesystem::path& a_Path)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	std::string FullFilePath = Engine.Resources().GetPath(a_Path.string());
	bool isglb = FullFilePath.find("glb") != std::string::npos;
	bool ret;

	if (isglb)
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, FullFilePath); // for binary glTF(.glb)
	else
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, FullFilePath);

	if (!warn.empty())
		printf("Warn: %s\n", warn.c_str());
	if (!err.empty())
		printf("Err: %s\n", err.c_str());
	if (!ret)
		printf("Failed to parse glTF\n");

	LoadVBOs(model);
	LoadTextures(model, a_Path.parent_path().string());
	LoadMeshes(model, false);

}

void Stellar::Model::Render()
{
	for (auto mesh : m_meshes)
	{
		glBindVertexArray(mesh.vao);
		if (textures.size() > 0)
		{
			glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, mesh.texture);
		}

		glDrawElements(GL_TRIANGLES, mesh.indicesCount, mesh.IBOType, 0);
	}
	glBindVertexArray(0);

}

void Stellar::Model::LoadNode(const tinygltf::Model& a_Model, const tinygltf::Node& a_Node, glm::vec3 a_Translation)
{
	if (!a_Node.translation.empty())
	{
		a_Translation += vec3(a_Node.translation[0], a_Node.translation[1], a_Node.translation[2]);
	}
	else if (!a_Node.matrix.empty())
	{
		a_Translation += vec3(a_Node.matrix[12], a_Node.matrix[13], a_Node.matrix[14]);
	}
	if (!a_Node.extensions.empty())
	{
		auto value = a_Node.extensions.at("KHR_lights_punctual");
		auto intvalue = value.GetLightType();
		m_Lights[intvalue].pos = a_Translation;
	}
	if (!a_Node.children.empty())
	{
		for (auto gltfChildren : a_Node.children)
		{
			LoadNode(a_Model, a_Model.nodes[gltfChildren], a_Translation);
		}
	}
}

void Stellar::Model::LoadMeshes(const tinygltf::Model& a_Model, const bool& a_renderAsEntity)
{
	std::map<std::string, int> attributeLocations;
	attributeLocations["POSITION"] = 0;
	attributeLocations["NORMAL"] = 1;
	attributeLocations["TEXCOORD_0"] = 2;
	attributeLocations["TANGENT"] = 3;

	if (a_Model.lights.size() > 0)
	{
		for (auto gltfLight : a_Model.lights)
		{
			Light light;
			if (gltfLight.color.empty())
				light.col = vec3(1);
			else
				light.col = vec3(gltfLight.color[0], gltfLight.color[1], gltfLight.color[2]);
			
			light.intensity = gltfLight.intensity;
			light.range = gltfLight.range;
			light.type = gltfLight.type;
			m_Lights.push_back(light);
		}
		for (auto gltfscenes : a_Model.scenes)
		{
			for (auto gltfNodes : gltfscenes.nodes)
			{
				LoadNode(a_Model, a_Model.nodes[gltfNodes], vec3(0.0f));
			}
		}
	}
	RemoveLights(); //Disable lights for now, we are adding them manually.

	//loop through all meshes
	for (auto gltfMesh : a_Model.meshes)
	{
		//loop through all primitives
		for (auto primitives : gltfMesh.primitives)
		{
			Mesh returnmesh;
			//check if there are textures (There should always be but this kills mistakes)
			if (textures.size() > 0)
			{
				returnmesh.texture.push_back(textures[a_Model.materials[primitives.material].pbrMetallicRoughness.baseColorTexture.index]);
				if (a_Model.materials[primitives.material].normalTexture.index != -1)
				returnmesh.texture.push_back(textures[a_Model.materials[primitives.material].normalTexture.index]);
			}

			//release crash or burn =D
			glGenVertexArrays(1, &returnmesh.vao);
			glBindVertexArray(returnmesh.vao);

			//loop through all vertex attributes
			for (auto const& attribute : primitives.attributes)
			{
				auto accessors = a_Model.accessors[attribute.second];
				auto bufferview = a_Model.bufferViews[accessors.bufferView];

				glBindBuffer(GL_ARRAY_BUFFER, m_VBO[accessors.bufferView]);
				if (attributeLocations.find(attribute.first) != attributeLocations.end())
				{
					glEnableVertexAttribArray(attributeLocations[attribute.first]);
					glVertexAttribPointer(attributeLocations[attribute.first], accessors.type, accessors.componentType, accessors.normalized, bufferview.byteStride, (void*)(accessors.byteOffset));
				}
			}

			//indices
			{
				auto iAttribute = primitives.indices;
				auto accessors = a_Model.accessors[iAttribute];
				auto bufferview = a_Model.bufferViews[accessors.bufferView];
				auto buffer = a_Model.buffers[bufferview.buffer];

				//Set the indices type = uint || ushort
				returnmesh.IBOType = accessors.componentType;
				//set the amount of indices to be drawn.
				returnmesh.indicesCount = accessors.count;

				glGenBuffers(1, &returnmesh.ibo);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, returnmesh.ibo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferview.byteLength, &buffer.data[bufferview.byteOffset + accessors.byteOffset], GL_STATIC_DRAW);
			}

			m_meshes.push_back(returnmesh);
		}
	}
}

void Stellar::Model::LoadVBOs(const tinygltf::Model& a_Model)
{
	for (auto bufferview : a_Model.bufferViews)
	{
		auto buffer = a_Model.buffers[bufferview.buffer];
		GLuint vbo;
		//Check if the bufferview isn't the indices.  If it is then ignore.
		if (bufferview.target == GL_ARRAY_BUFFER)
		{
			std::vector<float> bufferarray;
			bufferarray.resize(bufferview.byteLength / sizeof(float));
			memcpy(&bufferarray[0], &buffer.data[bufferview.byteOffset], bufferview.byteLength);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, bufferview.byteLength, &buffer.data[bufferview.byteOffset], GL_STATIC_DRAW);
		}

		m_VBO.push_back(vbo);
	}
}

void Stellar::Model::LoadTextures(const tinygltf::Model& a_Model, const std::string& a_filename)
{
	if (a_Model.textures.size() > 0)
	{
		for (auto modelTextures : a_Model.textures)
		{
			auto uri = a_Model.images[modelTextures.source].uri;
			auto sampler = a_Model.samplers[modelTextures.sampler];
			Sampler modelsampler = { (GLuint)sampler.wrapS,
									 (GLuint)sampler.wrapT,
									 (GLuint)sampler.minFilter,
									 (GLuint)sampler.magFilter };

			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, sampler.wrapS, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, sampler.wrapT, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, sampler.minFilter, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, sampler.magFilter, GL_LINEAR);

			std::string filename = Engine.Resources().GetPath(a_filename + uri.c_str());
			int width, height, nrChannels;
			unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);

			if (data)
			{
				GLenum rgbtype = GL_RGB;
				if (nrChannels == 4)
				{
					rgbtype = GL_RGBA;
				}
				glTexImage2D(GL_TEXTURE_2D, 0, rgbtype, width, height, 0, rgbtype, GL_UNSIGNED_BYTE, data);
				glGenerateMipmap(GL_TEXTURE_2D);
			}
			stbi_image_free(data);
			textures.push_back(texture);
		}

		glBindTexture(1, 0);
	}
}

void Stellar::Model::RemoveLights()
{
	for (int i = 0; i < m_Lights.size(); i++)
	{
		if (m_Lights[i].type != "point")
			m_Lights.erase(m_Lights.begin() + i);
	}
}
