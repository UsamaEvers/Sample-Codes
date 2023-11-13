#pragma once
#include "Model.h"

namespace Stellar
{
	class ModelManager
	{
	public:
		ModelManager();
		~ModelManager() {}

		Model* GetModel(std::string a_FileName) { return &m_modelMap[a_FileName]; }
	private:

		void LoadAllModels();
		std::map<std::string, Model> m_modelMap;
	};
};