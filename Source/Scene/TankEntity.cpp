/*******************************************
	TankEntity.cpp

	Tank entity template and entity classes
********************************************/

// Additional technical notes for the assignment:
// - Each tank has a team number (0 or 1), HP and other instance data - see the end of TankEntity.h
//   You will need to add other instance data suitable for the assignment requirements
// - A function GetTankUID is defined in TankAssignment.cpp and made available here, which returns
//   the UID of the tank on a given team. This can be used to get the enemy tank UID
// - Tanks have three parts: the root, the body and the turret. Each part has its own matrix, which
//   can be accessed with the Matrix function - root: Matrix(), body: Matrix(1), turret: Matrix(2)
//   However, the body and turret matrix are relative to the root's matrix - so to get the actual 
//   world matrix of the body, for example, we must multiply: Matrix(1) * Matrix()
// - Vector facing work similar to the car tag lab will be needed for the turret->enemy facing 
//   requirements for the Patrol and Aim states
// - The CMatrix4x4 function DecomposeAffineEuler allows you to extract the x,y & z rotations
//   of a matrix. This can be used on the *relative* turret matrix to help in rotating it to face
//   forwards in Evade state
// - The CShellEntity class is simply an outline. To support shell firing, you will need to add
//   member data to it and rewrite its constructor & update function. You will also need to update 
//   the CreateShell function in EntityManager.cpp to pass any additional constructor data required
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// - As entities can be destroyed, you must check that entity UIDs refer to existant entities, before
//   using their entity pointers. The return value from EntityManager.GetEntity will be NULL if the
//   entity no longer exists. Use this to avoid trying to target a tank that no longer exists etc.

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
// Will be needed to implement the required tank behaviour in the Update function below
extern TEntityUID GetTankUID( int team );



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// Tank constructor intialises tank-specific data and passes its parameters to the base
// class constructor
CTankEntity::CTankEntity
(
	CTankTemplate* tankTemplate,
	TEntityUID      UID,
	TUInt32         team,
	const string& name /*=""*/,
	const CVector3& position /*= CVector3::kOrigin*/,
	const CVector3& rotation /*= CVector3( 0.0f, 0.0f, 0.0f )*/,
	const CVector3& scale /*= CVector3( 1.0f, 1.0f, 1.0f )*/
) : CEntity(tankTemplate, UID, name, position, rotation, scale)
{
	m_TankTemplate = tankTemplate;

	// Tanks are on teams so they know who the enemy is
	m_Team = team;

	// Initialise other tank data and state
	m_Speed = 0.0f;
	m_HP = m_TankTemplate->GetMaxHP();
	m_State = EState::InActive;
	m_Timer = 0.0f;
	m_AimTimer = 1.0f;

	// Creates the chase cam thats used for this tank
	m_ChaseCam = new CCamera({ Position().x, Position().y + 3.0f, Position().z });
	m_ChaseCam->SetNearFarClip(1.0f, 20000.0f);
}


// Update the tank - controls its behaviour. The shell code just performs some test behaviour, it
// is to be rewritten as one of the assignment requirements
// Return false if the entity is to be destroyed
bool CTankEntity::Update( TFloat32 updateTime )
{

	CMatrix4x4 tankMatrix = Matrix(1) * Matrix(); // Returns the tank matrix
	CVector3 facingVector = tankMatrix.ZAxis(); // Used for getting the facing vector
	facingVector.Normalise();

	m_ChaseCam->Position() = Position() - facingVector * 15.0f + tankMatrix.YAxis() * 3.0f; // Sets the position of the chase cam behind the tank  
	m_ChaseCam->Matrix().FaceTarget(tankMatrix.Position()); // Sets the camera to always face forwards with the tank

	// Fetch any messages
	SMessage msg;
	while (Messenger.FetchMessage( GetUID(), &msg ))
	{
		// Set state variables based on received messages
		switch (msg.type)
		{
			case Msg_Start:
				m_State = EState::Patrol;
				break;
			case Msg_Stop:
				m_State = EState::InActive;
				break;
			case Msg_Inactive:
				m_State = EState::InActive;
				break;
			case Msg_Patrol:
				m_State = EState::Patrol;
				break;
			case Msg_Aim:
				m_State = EState::Aim;
				break;
			case Msg_Evade:
				m_State = EState::Evade;
				break;
			case Msg_Hit:
				Hit();
				break;
			case MSg_FindAmmo:
				m_IsMoving = false;
				m_State = EState::FindAmmo;
				break;
			case Msg_Help:
				m_State = EState::Help;
			case Msg_Death:
				Death(updateTime);
				break;
		}
	}
	// Runs functions and set variables based on the current state
	if (m_State == EState::InActive)
	{
		m_IsMoving = false;
	}
	else if (m_State == EState::Patrol)
	{
		Patrol(updateTime);
	}
	else if (m_State == EState::Aim)
	{
		Aim(updateTime);
	}
	else if (m_State == EState::Evade)
	{
		Evade(updateTime);
	}
	else if (m_State == EState::FindAmmo)
	{
		FindAmmo(updateTime);
	}
	else if (m_State == EState::Help)
	{
		Help(updateTime);
	}

	if (m_HP <= 0) return Death(updateTime); // When the tanks die call the death function

	//// Perform movement...
	//// Move along local Z axis scaled by update time
	if (m_IsMoving)
	{
		Matrix().MoveLocalZ(m_Speed * updateTime); // Always moves the tanks forward if variable is true
	}

	return true; // Don't destroy the entity
}

void CTankEntity::Patrol(float frameTime)
{
	if (!m_IsMoving) // when not moving set the target and set vmoving to true
	{		
		m_Target = m_PtTwo;
		m_IsMoving = true;
	}
	else
	{
		Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * frameTime); // Rotates the turrent when when partrolling

		CMatrix4x4 turretMatrix = Matrix(2) * Matrix(); // Gets the turrents matrix

		CVector3 facingVector = turretMatrix.ZAxis(); 
		facingVector.Normalise();
		CVector3 endPos = Position() + facingVector * 30.0f; // Raycast for the tank from the turrents facing vector

		// Loops through all the entities that have the tank template
		CEntity* entity;
		EntityManager.BeginEnumEntities("", "", "Tank");
		while (entity = EntityManager.EnumEntity())
		{
			CTankEntity* TEntity = static_cast<CTankEntity*>(entity); // Casts the entity to the tank object to get access to its functions
			if (TEntity != nullptr && m_Team != TEntity->GetTeam()) // Checks if the object is a tank and checks if its an opponents tank
			{
				if (Distance(endPos, TEntity->Position()) < 25.0f) // Checks if the enemy tank is within the raycast
				{
					// Sends an aim message to its self
					SMessage msg;
					msg.type = Msg_Aim; 
					msg.from = GetUID();

					m_EnemyTarget = TEntity->Position(); // Sets the position that the enemy is at

					Messenger.SendMessageA(GetUID(), msg);
				}
			}
		}
		EntityManager.EndEnumEntities();

		// Normalise the Z and X axis
		Matrix().ZAxis().Normalise();
		Matrix().XAxis().Normalise();
		CVector3 target = m_Target - Position(); // Sets the target used for rotating 
		target.Normalise(); 

		float productX = Dot(Matrix().XAxis(), target); // gets the dot product of x used for checking if it needs to turn left or right
		float productZ = Dot(Matrix().ZAxis(), target); // Product used for getting the angle
		float angle = acos(productZ); // getting the angle used for turning speed 

		float turnSpeed = ToRadians(m_TankTemplate->GetTurnSpeed());
		if (productX > 0)
		{
			// Turn Right
			Matrix().RotateY(Min(turnSpeed, angle));
		}
		else
		{
			// Turn Left
			Matrix().RotateY(-Min(turnSpeed, angle));
		}

		if (Distance(Position(), m_Target) > 2.0f) // Increases the speed if not in range
		{
			m_Speed += m_TankTemplate->GetAcceleration() * frameTime;
			if (m_Speed > m_TankTemplate->GetMaxSpeed())
			{
				m_Speed = m_TankTemplate->GetMaxSpeed();
			}
		}
		else
		{
			// Stop speeding if the tank is in range
			m_Speed -= m_TankTemplate->GetAcceleration() * frameTime; 
			if (m_Speed < 0.0f)
			{
				// Selects the next waypoints
				if (m_Target == m_PtOne)
				{
					m_PtTwo = SelectWaypoint(); 
					m_Target = m_PtTwo;
				}
				else if (m_Target == m_PtTwo)
				{
					m_PtOne = SelectWaypoint();
					m_Target = m_PtOne;
				}
			}
		}
	}
}

void CTankEntity::Aim(float frameTime)
{
	m_Speed = 0.0f; // Sets the movement to speed to not move
	Matrix(2).FaceTarget(m_EnemyTarget); // Faces the target
	if (m_ShellsAmmo > 0) // If tank has shells then start aim timer
	{
		if (m_AimTimer < 0.0f)
		{
			if (!m_Fired) // Shots the shell when the timer has ran out
			{
				EntityManager.CreateShell("Shell Type 1", m_EnemyTarget, this, "", { Position().x, 1.8f, Position().z });
				m_ShellsShot++;
				m_ShellsAmmo--;
				m_AimTimer = 1.0f; // Resets the timer
				m_Fired = true;
				SMessage msg;
				msg.type = Msg_Evade;
				msg.from = SystemUID;

				Messenger.SendMessageA(GetUID(), msg);
			}
		}
		else
		{
			m_AimTimer -= frameTime;
		}
	}
	else
	{
		// If ran out of ammo then find ammo
		SMessage msg;
		msg.type = MSg_FindAmmo;
		msg.from = SystemUID;
		Messenger.SendMessageA(GetUID(), msg);
	}
}

void CTankEntity::Evade(float frameTime)
{
	m_Fired = false;
	SetRandomTarget(); // Sets random target
	m_EvadeStart = true;
	if (m_IsMoving) // If tank is moving face the target
	{
		Matrix().FaceTarget(m_Target); // Tank face target
		Matrix(2).FaceTarget(m_Target); // Turrent face target
	}

	if (m_ShellsAmmo <= 0) // if ran out of shells then find ammo
	{
		SMessage msg;
		msg.type = MSg_FindAmmo;
		msg.from = SystemUID;
		Messenger.SendMessageA(GetUID(), msg);
	}

	Matrix().ZAxis().Normalise();
	Matrix().XAxis().Normalise();
	CVector3 target = m_Target - Position();
	target.Normalise();

	float productX = Dot(Matrix().XAxis(), target);
	float productZ = Dot(Matrix().ZAxis(), target);
	float angle = acos(productZ);

	float turnSpeed = ToRadians(m_TankTemplate->GetTurnSpeed());
	if (productX > 0)
	{
		// Turn Right
		Matrix().RotateY(Min(turnSpeed, angle));
	}
	else
	{
		Matrix().RotateY(-Min(turnSpeed, angle));
	}

	if (Distance(Position(), m_Target) > 2.0f)
	{
		m_Speed += m_TankTemplate->GetAcceleration() * frameTime;
		if (m_Speed > m_TankTemplate->GetMaxSpeed())
		{
			m_Speed = m_TankTemplate->GetMaxSpeed();
		}
	}
	else
	{
		// Sets the variables and states
		m_IsMoving = false;
		m_EvadeStart = false;
		m_State = EState::Patrol;

		m_Speed -= m_TankTemplate->GetAcceleration() * frameTime;
		if (m_Speed < 0.0f)
		{
			if (m_Target == m_PtOne)
			{
				m_PtTwo = SelectWaypoint();
				m_Target = m_PtTwo;
			}
			else if (m_Target == m_PtTwo)
			{
				m_PtOne = SelectWaypoint();
				m_Target = m_PtOne;
			}
		}
	}

}

void CTankEntity::Hit()
{
	// If hit then take damage and send a help msg to all team tanks
	m_HP -= m_TankTemplate->GetShellDamage();
	SMessage msg;
	msg.type = Msg_Help;
	msg.from = SystemUID;

	CEntity* entityLoop;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entityLoop = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entityLoop);
		if (TEntity != nullptr && TEntity->GetTeam() == m_Team)
		{
			Messenger.SendMessageA(TEntity->GetUID(), msg);
		}
	}
	EntityManager.EndEnumEntities();

	
}

void CTankEntity::FindAmmo(float frameTime)
{
	// Finds the nearest ammo
	CEntity* entity;
	CEntity* entityLoop;
	CVector3 nearestAmmoPos = CVector3(Random(-30.0f, 30.0f), Position().y, Random(-30.0f, 30.0f));
	EntityManager.BeginEnumEntities("", "", "AmmoBox");
	float nearest = 20.0f; 
	while (entityLoop = EntityManager.EnumEntity())
	{
		CAmmoBoxEntity* AEntity = static_cast<CAmmoBoxEntity*>(entityLoop);
		if (AEntity != nullptr)
		{
			if (Distance(Position(), AEntity->Position()) < nearest)
			{
				nearestAmmoPos = AEntity->Position();
				entity = entityLoop;
			}
		}
	}
	EntityManager.EndEnumEntities();

	if (!m_IsMoving)
	{
		m_NearestAmmoTarget = nearestAmmoPos; // Sets the target to the ammo to the nearest ammo
		m_IsMoving = true;
	}
	else
	{
		Matrix(2).RotateLocalY(m_TankTemplate->GetTurretTurnSpeed() * frameTime);

		Matrix().ZAxis().Normalise();
		Matrix().XAxis().Normalise();
		CVector3 target = m_NearestAmmoTarget - Position();
		target.Normalise();

		float productX = Dot(Matrix().XAxis(), target);
		float productZ = Dot(Matrix().ZAxis(), target);
		float angle = acos(productZ);

		float turnSpeed = ToRadians(m_TankTemplate->GetTurnSpeed());
		if (productX > 0)
		{
			// Turn Right
			Matrix().RotateY(Min(turnSpeed, angle));
		}
		else
		{
			Matrix().RotateY(-Min(turnSpeed, angle));
		}

		if (Distance(Position(), m_NearestAmmoTarget) > 2.0f)
		{
			m_Speed += m_TankTemplate->GetAcceleration() * frameTime;
			if (m_Speed > m_TankTemplate->GetMaxSpeed())
			{
				m_Speed = m_TankTemplate->GetMaxSpeed();
			}
		}
		else
		{
			m_Speed -= m_TankTemplate->GetAcceleration() * frameTime;
			if (m_Speed < 0.0f)
			{
				if (entity != nullptr)
				{
					// If in range of ammo box 
					CAmmoBoxEntity* AEntity = static_cast<CAmmoBoxEntity*>(entity);
					if (AEntity != nullptr)
					{
						m_ShellsAmmo = 10; // Set ammo
						entity = nullptr; // resets entity
						nearestAmmoPos = CVector3(Random(-30.0f, 30.0f), Position().y, Random(-30.0f, 30.0f)); // resets ammo 
						// Sends message to the ammo box letting it know to destroy its self
						SMessage msg1;
						msg1.type = Msg_CollectedAmmo;
						msg1.from = GetUID();
						Messenger.SendMessageA(AEntity->GetUID(), msg1);
						
					}
				}
			}
			// GO back to patrol state
				SMessage msg;
				msg.type = Msg_Patrol;
				msg.from = SystemUID;
				Messenger.SendMessageA(GetUID(), msg);
		}
	}
	
}

void CTankEntity::Help(float frameTime)
{
	m_Speed = 0.0f;
	if (m_HelpTimer < 0.0f) 
	{
		// Go back to patrol when timer runs out
		SMessage msg;
		msg.type = Msg_Patrol;
		msg.from = SystemUID;
		Messenger.SendMessageA(GetUID(), msg);
	}
	else
	{
		m_HelpTimer -= frameTime;

		CEntity* entityLoop;
		CVector3 NearestTankPos = CVector3(Random(-30.0f, 30.0f), Position().y, Random(-30.0f, 30.0f));
		EntityManager.BeginEnumEntities("", "", "Tank");
		float nearest = 20.0f;
		while (entityLoop = EntityManager.EnumEntity())
		{
			CTankEntity* AEntity = static_cast<CTankEntity*>(entityLoop);
			if (AEntity != nullptr)
			{
				// if found reset timer and go to aim state starting ememy target
				if (Distance(Position(), AEntity->Position()) < nearest)
				{
					m_EnemyTarget = entityLoop->Position();
					m_HelpTimer = m_HelpTimerMax;
					SMessage msg;
					msg.type = Msg_Aim;
					msg.from = SystemUID;
					Messenger.SendMessageA(GetUID(), msg);
				}
			}
		}
		EntityManager.EndEnumEntities();

		
	}
}

void CTankEntity::SetRandomTarget()
{
	if (!m_EvadeStart)
	{
		m_Target = CVector3(Random(-40.0f, 40.0f), Position().y, Random(-40.0f, 40.0f)); 
	}
}

bool CTankEntity::Death(float frameTime)
{
	if (m_DeathTimer < 0.0f)
	{
		// When timer is up set the score and remove the tank
		if (m_Team == 0)
		{
			EntityManager.TeamTwoScore();
		}
		else if (m_Team == 1)
		{
			EntityManager.TeamOneScore();
		}
		return false;
	}
	else
	{
		// Destroy the tank
		m_DeathTimer -= frameTime;
		Matrix(2).MoveLocalY(m_DestroyedSpeed * frameTime);
		Matrix(2).RotateLocalX(m_DeathTurretSpeed * frameTime);
		Matrix(2).RotateLocalY(m_DeathTurretSpeed * frameTime);

		Matrix().RotateLocalY(m_DeathTankSpeed * frameTime);
	}
	return true;
}

CVector3 CTankEntity::SelectWaypoint()
{
	return EntityManager.GetPatrolPoints(m_Team).PatrolPoints; // Gets a patrol point from the entity manager
}


} // namespace gen
