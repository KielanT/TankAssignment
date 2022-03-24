/*******************************************
	TankEntity.h

	Tank entity template and entity classes
********************************************/

#pragma once

#include <string>
using namespace std;

#include "Defines.h"
#include "CVector3.h"
#include "Entity.h"

namespace gen
{

/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Template Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank template inherits the type, name and mesh from the base template and adds further
// tank specifications
class CTankTemplate : public CEntityTemplate
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank entity template constructor sets up the tank specifications - speed, acceleration and
	// turn speed and passes the other parameters to construct the base class
	CTankTemplate
	(
		const string& type, const string& name, const string& meshFilename,
		TFloat32 maxSpeed, TFloat32 acceleration, TFloat32 turnSpeed,
		TFloat32 turretTurnSpeed, TUInt32 maxHP, TUInt32 shellDamage
	) : CEntityTemplate( type, name, meshFilename )
	{
		// Set tank template values
		m_MaxSpeed = maxSpeed;
		m_Acceleration = acceleration;
		m_TurnSpeed = turnSpeed;
		m_TurretTurnSpeed = turretTurnSpeed;
		m_MaxHP = maxHP;
		m_ShellDamage = shellDamage;
	}

	// No destructor needed (base class one will do)


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	//	Getters

	TFloat32 GetMaxSpeed()
	{
		return m_MaxSpeed;
	}

	TFloat32 GetAcceleration()
	{
		return m_Acceleration;
	}

	TFloat32 GetTurnSpeed()
	{
		return m_TurnSpeed;
	}

	TFloat32 GetTurretTurnSpeed()
	{
		return m_TurretTurnSpeed;
	}

	TInt32 GetMaxHP()
	{
		return m_MaxHP;
	}

	TInt32 GetShellDamage()
	{
		return m_ShellDamage;
	}


/////////////////////////////////////
//	Private interface
private:

	// Common statistics for this tank type (template)
	TFloat32 m_MaxSpeed;        // Maximum speed for this kind of tank
	TFloat32 m_Acceleration;    // Acceleration  -"-
	TFloat32 m_TurnSpeed;       // Turn speed    -"-
	TFloat32 m_TurretTurnSpeed; // Turret turn speed    -"-

	TUInt32  m_MaxHP;           // Maximum (initial) HP for this kind of tank
	TUInt32  m_ShellDamage;     // HP damage caused by shells from this kind of tank
};



/*-----------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------
	Tank Entity Class
-------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------*/

// A tank entity inherits the ID/positioning/rendering support of the base entity class
// and adds instance and state data. It overrides the update function to perform the tank
// entity behaviour
// The shell code performs very limited behaviour to be rewritten as one of the assignment
// requirements. You may wish to alter other parts of the class to suit your game additions
// E.g extra member variables, constructor parameters, getters etc.
class CTankEntity : public CEntity
{
/////////////////////////////////////
//	Constructors/Destructors
public:
	// Tank constructor intialises tank-specific data and passes its parameters to the base
	// class constructor
	CTankEntity
	(
		CTankTemplate*  tankTemplate,
		TEntityUID      UID,
		TUInt32         team,
		const string&   name = "",
		const CVector3& position = CVector3::kOrigin, 
		const CVector3& rotation = CVector3( 0.0f, 0.0f, 0.0f ),
		const CVector3& scale = CVector3( 1.0f, 1.0f, 1.0f )
	);

	// No destructor needed


/////////////////////////////////////
//	Public interface
public:

	/////////////////////////////////////
	// Getters

	TFloat32 GetSpeed()
	{
		return m_Speed;
	}


	/////////////////////////////////////
	// Update

	// Update the tank - performs tank message processing and behaviour
	// Return false if the entity is to be destroyed
	// Keep as a virtual function in case of further derivation
	virtual bool Update( TFloat32 updateTime );
	
	// Return State the tank is currently in
	std::string GetState()
	{
		switch (m_State)
		{
		case EState::InActive:
			return "InActive";
			break;
		case EState::Patrol:
			return "Patrol";
			break;
		case EState::Aim:
			return "Aim";
			break;
		case EState::Evade:
			return "Evade";
			break;
		case EState::FindAmmo:
			return "Search";
			break;
		case EState::Help:
			return "Help";
			break;
		default:
			return "Unknown State";
			break;
		}
	}

	// Returns the team
	TUInt32 GetTeam() { return m_Team; }

	// Returns the health
	TFloat32 GetHealth() { return m_HP; }

	// Returns the ammount of shells shot
	TInt32 GetShellsShot() { return m_ShellsShot; }

	// Returns the ammount of ammo
	TInt32 GetShellsAmmo() { return m_ShellsAmmo; }

	// Returns the chase camera
	CCamera* GetCamera() { return m_ChaseCam; }

	// Sets the target to change the target point of where the tank is going to go
	void SetTarget(CVector3 target) { m_Target = target; }

	// Used for picking to test if the tank has been selected
	bool IsSelected() { return m_IsSelected; }
	void SetSelected(bool selected) { m_IsSelected = selected; }

/////////////////////////////////////
//	Private interface
private:

	/////////////////////////////////////
	// Functions

	void Patrol(float frameTime);
	void Aim(float frameTime);
	void Evade(float frameTime);
	void Hit();
	void FindAmmo(float frameTime);
	void Help(float frameTime);

	void SetRandomTarget();

	bool Death(float frameTime);

	// Used to select waypoints that are called in from xml
	CVector3 SelectWaypoint(); 

	/////////////////////////////////////
	// Types

	// States available for a tank - placeholders for shell code
	enum class EState
	{
		InActive,
		Patrol,
		Aim,
		Evade,
		FindAmmo,
		Help
	};


	/////////////////////////////////////
	// Data

	// The template holding common data for all tank entities
	CTankTemplate* m_TankTemplate;

	const TFloat32 m_HelpTimerMax = 3.0f;

	// Tank data
	TUInt32  m_Team;  // Team number for tank (to know who the enemy is)
	TUInt32  m_ShellsShot = 0;  
	TUInt32  m_ShellsAmmo = 10;  
	TFloat32 m_Speed; // Current speed (in facing direction)
	TFloat32  m_HP;    // Current hit points for the tank

	// Tank state
	EState   m_State; // Current state
	TFloat32 m_Timer; // A timer used in the example update function   
	TFloat32 m_AimTimer;
	TFloat32 m_DeathTimer = 2.0f;
	TFloat32 m_DestroyedSpeed = 2.0f;
	TFloat32 m_DeathTurretSpeed = 100.0f;
	TFloat32 m_DeathTankSpeed = 50.0f;
	TFloat32 m_HelpTimer = m_HelpTimerMax;

	bool m_IsMoving = false;
	bool m_EvadeStart = false;
	bool m_Fired = false;
	bool m_IsSelected = false;

	CVector3 m_PtOne = { Random(-30, 30), Position().y, Random(-30, 30) };
	CVector3 m_PtTwo = { Random(-30, 30), Position().y, Random(-30, 30) };
	CVector3 m_Target = m_PtOne;
	CVector3 m_EnemyTarget;

	CVector3 m_NearestAmmoTarget;

	bool test = false;

	CCamera* m_ChaseCam;
};


} // namespace gen
