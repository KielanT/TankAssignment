/*******************************************
	ShellEntity.cpp

	Shell entity class
********************************************/

#include "ShellEntity.h"
#include "TankEntity.h"
#include "EntityManager.h"
#include "Messenger.h"

namespace gen
{

// Reference to entity manager from TankAssignment.cpp, allows look up of entities by name, UID etc.
// Can then access other entity's data. See the CEntityManager.h file for functions. Example:
//    CVector3 targetPos = EntityManager.GetEntity( targetUID )->GetMatrix().Position();
extern CEntityManager EntityManager;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;

// Helper function made available from TankAssignment.cpp - gets UID of tank A (team 0) or B (team 1).
// Will be needed to implement the required shell behaviour in the Update function below
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Shell Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Shell constructor intialises shell-specific data and passes its parameters to the base
// class constructor
CShellEntity::CShellEntity
(
	CEntityTemplate* entityTemplate,
	TEntityUID       UID,
	CVector3 target,
	CTankEntity* ParentEntity,
	const string&    name /*=""*/,
	const CVector3&  position /*= CVector3::kOrigin*/, 
	const CVector3&  rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3&  scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
	
) : CEntity( entityTemplate, UID, name, position, rotation, scale )
{
	// Initialise any shell data you add
	m_Target = target; // Sets the target of the shell to move to
	m_Timer = 3.0f; // Lifetime timer
	m_ParentEntity = ParentEntity; // Sets the parent entity

	if (m_ParentEntity != nullptr)
	{
		m_Team = m_ParentEntity->GetTeam(); // Sets the team of the shell
	}
}


// Update the shell - controls its behaviour. The shell code is empty, it needs to be written as
// one of the assignment requirements
// Return false if the entity is to be destroyed
bool CShellEntity::Update( TFloat32 updateTime )
{
	Matrix().MoveLocalZ(10 * updateTime); // Moves the shell
	Matrix().FaceTarget(m_Target, Matrix().YAxis()); // Sets the shell to face direction

	// Loops through checking the nearest tank and causing damage if in range
	CEntity* entity;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);
		if (TEntity != nullptr && m_Team != TEntity->GetTeam())
		{
			if (Distance(Position(), TEntity->Position()) < 5.0f)
			{
				// Tells the tank its been hit
				SMessage msg;
				msg.type = Msg_Hit;
				msg.from = GetUID();

				Messenger.SendMessageA(TEntity->GetUID(), msg);
				return false;
			}
		}
	}
	EntityManager.EndEnumEntities();

	// When timer runs out destroys itself
	if (m_Timer < 0.0f)
	{
		return false;
	}
	else
	{
		m_Timer -= updateTime;
	}

	return true; // Placeholder
}


} // namespace gen
