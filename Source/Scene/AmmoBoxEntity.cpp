#include "AmmoBoxEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{
	extern CEntityManager EntityManager;

	// Messenger class for sending messages to and between entities
	extern CMessenger Messenger;

	CAmmoBoxEntity::CAmmoBoxEntity
	(
		CAmmoBoxTemplate* ammoTemplate, TEntityUID UID, const string& name, const CVector3& position, const CVector3& rotation, const CVector3& scale
	) : CEntity(ammoTemplate, UID, name, position, rotation, scale)
	{
		gravity = ammoTemplate->GetGravity(); // Sets the gravity for the ammo box
	}

	bool CAmmoBoxEntity::Update(TFloat32 updateTime)
	{
		// if not at ground level, fall to ground
		if (Position().y > 2.0f)
		{
			Matrix().MoveLocalY(gravity * updateTime);
		}

		// if collected destroy its self
		SMessage msg;
		while (Messenger.FetchMessage(GetUID(), &msg))
		{
			// Set state variables based on received messages
			switch (msg.type)
			{
			case Msg_CollectedAmmo:
				return false;
				break;
			}
		}

		return true;
	}
}


