#include "pch.h"
#include "ModelManager.h"
#include <Tools/RapidJSON.h>
#include <Core/Engine.h>
#include <fstream> 
#include <iostream> 

using namespace rapidjson;

Stellar::ModelManager::ModelManager()
{
	LoadAllModels();
}

void GoThroughDirectory(std::filesystem::path a_Dir, Value& ModelsArray, Document::AllocatorType& Alloc)
{
	if (!ModelsArray.IsArray())
		return;

	if (a_Dir.extension() == ".gltf")
	{
		Value ModelObject(kObjectType);
		Value FilenameValue;
		Value DirValue;
		
		FilenameValue.SetString(a_Dir.filename().string().c_str(), Alloc);
		DirValue.SetString(a_Dir.string().c_str(), Alloc);

		ModelObject.AddMember("Name", FilenameValue, Alloc);
		ModelObject.AddMember("Path", DirValue, Alloc);
		ModelsArray.PushBack(ModelObject, Alloc);
	}
	else if (std::filesystem::is_directory(a_Dir))
	{
		for (auto DirEntry : std::filesystem::directory_iterator(a_Dir))
		{
			GoThroughDirectory(DirEntry.path(), ModelsArray, Alloc);
		}
	}
}

void Stellar::ModelManager::LoadAllModels()
{
	// Write all models to JSON file
	std::filesystem::path OutputDir = Engine.Resources().GetPath("[Assets]/EditorFiles/Models.json");
	rapidjson::Document WriteDoc;
	WriteDoc.SetObject();
	Value ModelsArray(kArrayType);

	GoThroughDirectory(Engine.Resources().GetPath("[Assets]/Models"), ModelsArray, WriteDoc.GetAllocator());
	WriteDoc.AddMember("Models", ModelsArray, WriteDoc.GetAllocator());

	std::string stringpath = OutputDir.string();
	const char* charpath = stringpath.c_str();
	FILE* fp = NULL;
	auto test = fopen_s(&fp, charpath, "wb");

	char writeBuffer[65536];
	rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	Writer<FileWriteStream> writer(os);
	WriteDoc.Accept(writer);
	fclose(fp);

	// Read all models from the JSON file
	std::ifstream fs;
	fs.open(OutputDir);

	std::string json((std::istreambuf_iterator<char>(fs)),
		std::istreambuf_iterator<char>());

	rapidjson::Document Doc;
	Doc.Parse(json.c_str());

	if (Doc.HasParseError()) {
		std::cerr << "Error parsing JSON: "
			<< Doc.GetParseError() << std::endl;
	}

	for (SizeType i = 0; i < Doc["Models"].Size(); i++)
	{
		auto& ModelEntry = Doc["Models"][i];
		std::filesystem::path FilePath = ModelEntry["Path"].GetString();
		
		if (!FilePath.has_filename())
			std::cerr << "Error Models.json missing filename: "
			<< FilePath << std::endl;

		if (!FilePath.has_parent_path())
			std::cerr << "Error Models.json missing path: "
			<< FilePath << std::endl;

		Model LoadModel(FilePath);
		m_modelMap[ModelEntry["Name"].GetString()] = LoadModel;
	}
}