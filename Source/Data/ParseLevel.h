#pragma once

#include "TinyXML2/tinyxml2.h"
#include "Defines.h"
#include "CVector3.h"
#include "EntityManager.h"

namespace gen
{

	class CParseLevel
	{
	public:
		CParseLevel(CEntityManager* entityManager) : m_EntityManager(entityManager)
		{
		}

		bool ParseFile(const string& fileName);

	private:

		bool ParseLevelElement(tinyxml2::XMLElement* rootElement);
		bool ParseTemplatesElement(tinyxml2::XMLElement* rootElement);
		bool ParseEntitiesElement(tinyxml2::XMLElement* rootElement);
		bool ParsePatrolPointsElement(tinyxml2::XMLElement* rootElement);

		CVector3 GetVector3FromElement(tinyxml2::XMLElement* rootElement);

		CEntityManager* m_EntityManager;
	};
}

