#include "ParseLevel.h"

namespace gen
{

	bool CParseLevel::ParseFile(const string& fileName)
	{
		tinyxml2::XMLDocument xmlDoc;

		tinyxml2::XMLError error = xmlDoc.LoadFile(fileName.c_str());
		if (error != tinyxml2::XML_SUCCESS)  return false;


		tinyxml2::XMLElement* element = xmlDoc.FirstChildElement();
		if (element == nullptr)  return false;

		while (element != nullptr)
		{
			// We found a "Level" tag at the root level, parse it
			string elementName = element->Name();
			if (elementName == "Level")
			{
				ParseLevelElement(element);
			}

			//else if (elementName == "xxx")
			//{
			//   You would add code for other document root level elements here, but our file only has "Level"
			//}

			element = element->NextSiblingElement();
		}

		return true;
	}

	bool CParseLevel::ParseLevelElement(tinyxml2::XMLElement* rootElement)
	{
		tinyxml2::XMLElement* element = rootElement->FirstChildElement();
		while (element != nullptr)
		{
			// Things expected in a "Level" tag
			string elementName = element->Name();
			if (elementName == "Templates")  ParseTemplatesElement(element);
			else if (elementName == "Entities")  ParseEntitiesElement(element);
			else if (elementName == "Patrol") ParsePatrolPointsElement(element);
			// You could add more tags within Level here (not needed for exercise, just saying)

			element = element->NextSiblingElement();
		}

		return true;
	}

	bool CParseLevel::ParseTemplatesElement(tinyxml2::XMLElement* rootElement)
	{
		tinyxml2::XMLElement* element = rootElement->FirstChildElement("EntityTemplate");
		while (element != nullptr)
		{
			// Read the type, name and mesh attributes - these are all required, so fail on error
			const tinyxml2::XMLAttribute* attr = element->FindAttribute("Type");
			if (attr == nullptr)  return false;
			string type = attr->Value();

			attr = element->FindAttribute("Name");
			if (attr == nullptr)  return false;
			string name = attr->Value();

			attr = element->FindAttribute("Mesh");
			if (attr == nullptr)  return false;
			string mesh = attr->Value();


			if (type == "Tank")
			{
				attr = element->FindAttribute("MaxSpeed");
				if (attr == nullptr)  return false;
				float maxSpeed = attr->FloatValue();

				attr = element->FindAttribute("Acceleration");
				if (attr == nullptr)  return false;
				float acceleration = attr->FloatValue();

				attr = element->FindAttribute("TurnSpeed");
				if (attr == nullptr)  return false;
				float turnSpeed = attr->FloatValue();

				attr = element->FindAttribute("TurretTurnSpeed");
				if (attr == nullptr)  return false;
				float turretTurnSpeed = kfPi / attr->FloatValue();

				attr = element->FindAttribute("HP");
				if (attr == nullptr) return false;
				int maxHP = attr->IntValue();

				attr = element->FindAttribute("ShellDamage");
				if (attr == nullptr) return false;
				int shellDamage = attr->IntValue();


				m_EntityManager->CreateTankTemplate(type, name, mesh, maxSpeed, acceleration, turnSpeed, turretTurnSpeed, maxHP, shellDamage);
			}
			else if (type == "AmmoBox")
			{
				attr = element->FindAttribute("Gravity");
				if (attr == nullptr)  return false;
				float gravity = attr->FloatValue();
				m_EntityManager->CreateAmmoBoxTemplate(type, name, mesh, gravity);
			}
			else
			{
				m_EntityManager->CreateTemplate(type, name, mesh);
			}


			// Find next entity template
			element = element->NextSiblingElement("EntityTemplate");
		}

		return true;
	}

	bool CParseLevel::ParseEntitiesElement(tinyxml2::XMLElement* rootElement)
	{
		tinyxml2::XMLElement* element = rootElement->FirstChildElement("Entity");
		while (element != nullptr)
		{
			// Read the type and name attributes - these are required, so fail on error
			const tinyxml2::XMLAttribute* attr = element->FindAttribute("Type");
			if (attr == nullptr)  return false;
			string type = attr->Value();

			attr = element->FindAttribute("Name");
			if (attr == nullptr)  return false;
			string name = attr->Value();


			// Next check for optional child elements such as position or scale, but set default values for all in case they are not provided
			CVector3 pos{ 0, 0, 0 };
			CVector3 rot{ 0, 0, 0 };
			CVector3 scale{ 1, 1, 1 };

			tinyxml2::XMLElement* child = element->FirstChildElement("Position");
			if (child != nullptr)  pos = GetVector3FromElement(child); // Helper method will read the X,Y,Z attributes into a CVector3

			child = element->FirstChildElement("Rotation");
			if (child != nullptr)  rot = GetVector3FromElement(child);
			rot.x = ToRadians(rot.x);
			rot.y = ToRadians(rot.y);
			rot.z = ToRadians(rot.z);

			child = element->FirstChildElement("Scale");
			if (child != nullptr)  scale = GetVector3FromElement(child);


			string templateType = m_EntityManager->GetTemplate(type)->GetType();
			if (templateType == "Ammo")
				m_EntityManager->CreateAmmoBox(type, name, pos, rot, scale);
			else
				m_EntityManager->CreateEntity(type, name, pos, rot, scale);

			// Next ordinary entity
			element = element->NextSiblingElement("Entity");
		}



		//--------------------
		// Teams of entities

		// Now find collections of entities in a teams tag. This is almost the same loop as above but
		// with a bit of extra code to collect the team info

		// Find team elements
		tinyxml2::XMLElement* teamElement = rootElement->FirstChildElement("Team");
		while (teamElement != nullptr)
		{
			// Read the team and colour attributes - these are all required, so fail on error
			const tinyxml2::XMLAttribute* attr = teamElement->FindAttribute("Name");
			if (attr == nullptr)  return false;
			int team = attr->IntValue();


			// For each entity in the team
			tinyxml2::XMLElement* element = teamElement->FirstChildElement("Entity");
			while (element != nullptr)
			{
				// Read the type and name attributes - these are all required, so fail on error
				attr = element->FindAttribute("Type");
				if (attr == nullptr)  return false;
				string type = attr->Value();

				attr = element->FindAttribute("Name");
				if (attr == nullptr)  return false;
				string name = attr->Value();



				// Next check for optional child elements such as position or scale, but set default values for all in case they are not provided
				CVector3 pos{ 0, 0, 0 };
				CVector3 rot{ 0, 0, 0 };
				CVector3 scale{ 1, 1, 1 };

				tinyxml2::XMLElement* child = element->FirstChildElement("Position");
				if (child != nullptr)  pos = GetVector3FromElement(child);

				child = element->FirstChildElement("Rotation");
				if (child != nullptr)  rot = GetVector3FromElement(child);
				rot.x = ToRadians(rot.x);
				rot.y = ToRadians(rot.y);
				rot.z = ToRadians(rot.z);

				child = element->FirstChildElement("Scale");
				if (child != nullptr) scale = GetVector3FromElement(child);





				// All data collected, create the entity, will allow any type of entity on a team
				string templateType = m_EntityManager->GetTemplate(type)->GetType();
				if (templateType == "Tank")
					m_EntityManager->CreateTank(type, team, name, pos, rot, scale);
				else
					m_EntityManager->CreateEntity(type, name, pos, rot, scale);

				// Next entity in this team
				element = element->NextSiblingElement("Entity");
			}

			// Next team
			teamElement = teamElement->NextSiblingElement("Team");

		}

		return true;
	}

	bool CParseLevel::ParsePatrolPointsElement(tinyxml2::XMLElement* rootElement)
	{
		// Find team elements
		tinyxml2::XMLElement* teamElement = rootElement->FirstChildElement("Team");
		while (teamElement != nullptr)
		{
			// Read the team and colour attributes - these are all required, so fail on error
			const tinyxml2::XMLAttribute* attr = teamElement->FindAttribute("Name");
			if (attr == nullptr)  return false;
			int team = attr->IntValue();


			// For each entity in the team
			tinyxml2::XMLElement* element = teamElement->FirstChildElement("Point");
			while (element != nullptr)
			{
				// Read the type and name attributes - these are all required, so fail on error
				attr = element->FindAttribute("X");
				if (attr == nullptr)  return false;
				float X = attr->FloatValue();


				attr = element->FindAttribute("Z");
				if (attr == nullptr)  return false;
				float Z = attr->FloatValue();

				CEntityManager::SPatrolPoints point;
				point.teamNum = team;
				point.PatrolPoints = { X, 0.5f, Z };

				m_EntityManager->PushPatrolPoints(point);

				// Next entity in this team
				element = element->NextSiblingElement("Point");
			}

			// Next team
			teamElement = teamElement->NextSiblingElement("Team");

		}

		return true;
	}

	CVector3 CParseLevel::GetVector3FromElement(tinyxml2::XMLElement* rootElement)
	{
		CVector3 vector{ 0, 0, 0 };

		const tinyxml2::XMLAttribute* attr = rootElement->FindAttribute("X");
		if (attr != nullptr)  vector.x = attr->FloatValue();

		attr = rootElement->FindAttribute("Y");
		if (attr != nullptr)  vector.y = attr->FloatValue();

		attr = rootElement->FindAttribute("Z");
		if (attr != nullptr)  vector.z = attr->FloatValue();

		// We also support a "Randomise" tag within any CVector3 type tag, it's another vector3 that randomises the first
		tinyxml2::XMLElement* child = rootElement->FirstChildElement("Randomise");
		if (child != nullptr)
		{
			float random = 0;

			attr = child->FindAttribute("X");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.x += Random(-random, random);

			attr = child->FindAttribute("Y");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.y += Random(-random, random);

			attr = child->FindAttribute("Z");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.z += Random(-random, random);
		}


		return vector;
	}
}