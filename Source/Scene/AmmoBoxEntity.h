#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{

	class CAmmoBoxTemplate : public CEntityTemplate
	{
	public:

		CAmmoBoxTemplate
		(
			const string& type, const string& name, const string& meshFilename, TFloat32 gravity
		) : CEntityTemplate(type, name, meshFilename)
		{
			m_Gravity = gravity; // Sets the gravity which is the speed the ammo box should fall at
		}

		TFloat32 GetGravity() { return m_Gravity; } // Returns the gravity var

	private:
		TFloat32 m_Gravity; 
	};

	class CAmmoBoxEntity : public CEntity
	{
	public:
		CAmmoBoxEntity
		(
			CAmmoBoxTemplate* ammoTemplate,
			TEntityUID      UID,
			const string& name = "",
			const CVector3& position = CVector3::kOrigin,
			const CVector3& rotation = CVector3(0.0f, 0.0f, 0.0f),
			const CVector3& scale = CVector3(.5f, .5f, .5f)
		);

		virtual bool Update(TFloat32 updateTime);

	private:
		TFloat32 gravity;
	};
}
