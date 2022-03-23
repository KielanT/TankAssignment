/*******************************************
	TankAssignment.cpp

	Shell scene and game functions
********************************************/

#include <sstream>
#include <string>
using namespace std;

#include <d3d10.h>
#include <d3dx10.h>

#include "Defines.h"
#include "CVector3.h"
#include "Camera.h"
#include "Light.h"
#include "EntityManager.h"
#include "Messenger.h"
#include "TankAssignment.h"
#include "ParseLevel.h"

namespace gen
{

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Control speed
const float CameraRotSpeed = 2.0f;
float CameraMoveSpeed = 80.0f;

// Amount of time to pass before calculating new average update time
const float UpdateTimePeriod = 1.0f;



//-----------------------------------------------------------------------------
// Global system variables
//-----------------------------------------------------------------------------

// Get reference to global DirectX variables from another source file
extern ID3D10Device*           g_pd3dDevice;
extern IDXGISwapChain*         SwapChain;
extern ID3D10DepthStencilView* DepthStencilView;
extern ID3D10RenderTargetView* BackBufferRenderTarget;
extern ID3DX10Font*            OSDFont;

// Actual viewport dimensions (fullscreen or windowed)
extern TUInt32 ViewportWidth;
extern TUInt32 ViewportHeight;

// Current mouse position
extern TUInt32 MouseX;
extern TUInt32 MouseY;

// Messenger class for sending messages to and between entities
extern CMessenger Messenger;


//-----------------------------------------------------------------------------
// Global game/scene variables
//-----------------------------------------------------------------------------

// Entity manager
CEntityManager EntityManager;
CParseLevel LevelParser(&EntityManager);

// Tank UIDs
TEntityUID TankA;
TEntityUID TankB;

// Other scene elements
const int NumLights = 2;
CLight*  Lights[NumLights];
SColourRGBA AmbientLight;
CCamera* MainCamera;
CCamera* LoopCamera;

// Sum of recent update times and number of times in the sum - used to calculate
// average over a given time period
float SumUpdateTimes = 0.0f;
int NumUpdateTimes = 0;
float AverageUpdateTime = -1.0f; // Invalid value at first

bool ShowText = false;
bool gameOver = false;

const float ammoMaxSpawnTime = 30.0f;
const float ammoMinSpawnTime = 20.0f;
float ammoSpawnTimer = ammoMaxSpawnTime;

int teamOneScore = 0;
int teamTwoScore = 0;

string winningTeam = "";

vector<CTankEntity*> TankArray;
int tankCounter = 0;

CTankEntity* NearestEntity = 0;
CTankEntity* SelectedEntity = 0;
//-----------------------------------------------------------------------------
// Scene management
//-----------------------------------------------------------------------------

// Creates the scene geometry
bool SceneSetup()
{
	//////////////////////////////////////////////
	// Prepare render methods
	InitialiseMethods();
	InitInput();

	srand(time(NULL));
	//////////////////////////////////////////
	// Create scenery templates and entities
	LevelParser.ParseFile("Scene.xml");

	for (int tree = 0; tree < 100; ++tree)
	{
		
		// Some random trees
		EntityManager.CreateEntity( "Tree", "Tree",
			                        CVector3(Random(-200.0f, 30.0f), 0.0f, Random(40.0f, 150.0f)),
			                        CVector3(0.0f, Random(0.0f, 2.0f * kfPi), 0.0f) );
	}
	

	/////////////////////////////
	// Camera / light setup

	// Set camera position and clip planes
	MainCamera = new CCamera(CVector3(0.0f, 30.0f, -100.0f), CVector3(ToRadians(15.0f), 0, 0));
	MainCamera->SetNearFarClip(1.0f, 20000.0f);

	LoopCamera = MainCamera;
	LoopCamera->SetNearFarClip(1.0f, 20000.0f);

	// Sunlight and light in building
	Lights[0] = new CLight(CVector3(-5000.0f, 4000.0f, -10000.0f), SColourRGBA(1.0f, 0.9f, 0.6f), 15000.0f);
	Lights[1] = new CLight(CVector3(6.0f, 7.5f, 40.0f), SColourRGBA(1.0f, 0.0f, 0.0f), 1.0f);

	// Ambient light level
	AmbientLight = SColourRGBA(0.6f, 0.6f, 0.6f, 1.0f);

	return true;
}


// Release everything in the scene
void SceneShutdown()
{
	// Release render methods
	ReleaseMethods();

	// Release lights
	for (int light = NumLights - 1; light >= 0; --light)
	{
		delete Lights[light];
	}

	// Release camera
	delete MainCamera;

	TankArray.clear();
	// Destroy all entities
	EntityManager.DestroyAllEntities();
	EntityManager.DestroyAllTemplates();
}


//-----------------------------------------------------------------------------
// Game Helper functions
//-----------------------------------------------------------------------------

// Get UID of tank A (team 0) or B (team 1)
TEntityUID GetTankUID(int team)
{
	return (team == 0) ? TankA : TankB;
}


//-----------------------------------------------------------------------------
// Game loop functions
//-----------------------------------------------------------------------------

// Draw one frame of the scene
void RenderScene( float updateTime )
{
	// Setup the viewport - defines which part of the back-buffer we will render to (usually all of it)
	D3D10_VIEWPORT vp;
	vp.Width  = ViewportWidth;
	vp.Height = ViewportHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDevice->RSSetViewports( 1, &vp );

	// Select the back buffer and depth buffer to use for rendering
	g_pd3dDevice->OMSetRenderTargets( 1, &BackBufferRenderTarget, DepthStencilView );
	
	// Clear previous frame from back buffer and depth buffer
	g_pd3dDevice->ClearRenderTargetView( BackBufferRenderTarget, &AmbientLight.r );
	g_pd3dDevice->ClearDepthStencilView( DepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// Update camera aspect ratio based on viewport size - for better results when changing window size
	MainCamera->SetAspect( static_cast<TFloat32>(ViewportWidth) / ViewportHeight );

	// Set camera and light data in shaders
	MainCamera->CalculateMatrices();
	SetCamera(MainCamera);
	SetAmbientLight(AmbientLight);
	SetLights(&Lights[0]);

	// Render entities and draw on-screen text
	EntityManager.RenderAllEntities();
	RenderSceneText( updateTime );

    // Present the backbuffer contents to the display
	SwapChain->Present( 0, 0 );
}


// Render a single text string at the given position in the given colour, may optionally centre it
void RenderText( const string& text, int X, int Y, float r, float g, float b, bool centre = false )
{
	RECT rect;
	if (!centre)
	{
		SetRect( &rect, X, Y, 0, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
	else
	{
		SetRect( &rect, X - 100, Y, X + 100, 0 );
		OSDFont->DrawText( NULL, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DXCOLOR( r, g, b, 1.0f ) );
	}
}

// Render on-screen text each frame
void RenderSceneText( float updateTime )
{
	// Accumulate update times to calculate the average over a given period
	SumUpdateTimes += updateTime;
	++NumUpdateTimes;
	if (SumUpdateTimes >= UpdateTimePeriod)
	{
		AverageUpdateTime = SumUpdateTimes / NumUpdateTimes;
		SumUpdateTimes = 0.0f;
		NumUpdateTimes = 0;
	}

	// Write FPS text string
	stringstream outText;
	if (AverageUpdateTime >= 0.0f)
	{
		outText << "Frame Time: " << AverageUpdateTime * 1000.0f << "ms" << endl << "FPS:" << 1.0f / AverageUpdateTime;
		RenderText( outText.str(), 2, 2, 0.0f, 0.0f, 0.0f );
		RenderText( outText.str(), 0, 0, 1.0f, 1.0f, 0.0f );
		outText.str("");
	}

	outText << "Team One Score:  " << EntityManager.GetTeamOneScore() << "\nTeam Two Score: " << EntityManager.GetTeamTwoScore();
	RenderText(outText.str(), 500, 10, 0.0f, 0.0f, 0.0f);
	RenderText(outText.str(), 498, 8, 1.0f, 1.0f, 0.0f);
	outText.str("");
	
	if (gameOver)
	{
		outText << "Team " << winningTeam << " Wins!";
		RenderText(outText.str(), 500, 500, 0.0f, 0.0f, 0.0f);
		RenderText(outText.str(), 498, 498, 1.0f, 1.0f, 0.0f);
		outText.str("");
	}
	
	CEntity* entity;
	int x, y;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);

		if (TEntity != nullptr)
		{

			if (MainCamera->PixelFromWorldPt(TEntity->Position(), ViewportWidth, ViewportHeight, &x, &y))
			{
				
				outText << TEntity->GetName();

				if (ShowText)
				{
					outText << "\n" << TEntity->GetState()		<< "\nHP: " << TEntity->GetHealth() 
							<< "\nShot: " << TEntity->GetShellsShot() << "\nAmmo: " << TEntity->GetShellsAmmo();

				}
				if (TEntity == NearestEntity)
				{
					RenderText(outText.str(), x, y, 0.5f, 0.5f, 0.0f, true);
					RenderText(outText.str(), x - 2, y - 2, 1.0f, 0.0f, 0.0f, true);
					outText.str("");
				}
				else
				{
					RenderText(outText.str(), x, y, 0.0f, 0.0f, 0.0f, true);
					RenderText(outText.str(), x - 2, y - 2, 1.0f, 1.0f, 0.0f, true);
					outText.str("");
				}
			}
		}
	}
	EntityManager.EndEnumEntities();

	EntityManager.BeginEnumEntities("", "", "AmmoBox");
	while (entity = EntityManager.EnumEntity())
	{
		CAmmoBoxEntity* AEntity = static_cast<CAmmoBoxEntity*>(entity);
		if (AEntity != nullptr && MainCamera->PixelFromWorldPt(AEntity->Position(), ViewportWidth, ViewportHeight, &x, &y))
		{
			outText << "Ammo Box";
			RenderText(outText.str(), x, y - 40, 0.0f, 0.0f, 0.0f, true);
			RenderText(outText.str(), x - 2, y - 42, 1.0f, 1.0f, 0.0f, true);
			outText.str("");
		}
	}
	EntityManager.EndEnumEntities();

	NearestEntity = 0;
	TInt32 X, Y;
	float nearestDistance = 50.0f;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);
		if (TEntity != nullptr)
		{

			//if (MainCamera->PixelFromWorldPt(&entityPixel, entity->Position(), ViewportWidth, ViewportHeight))
			if (MainCamera->PixelFromWorldPt(entity->Position(), ViewportWidth, ViewportHeight, &X, &Y))
			{
				CVector2 MousePixel = { (float)MouseX, (float)MouseY };
				CVector2 entityPixel = { (float)X,(float)Y };

				float pixelDistance = Distance(MousePixel, entityPixel);
				if (pixelDistance < nearestDistance)
				{
					NearestEntity = TEntity;
					nearestDistance = pixelDistance;
				}
			}
		}
	}
	EntityManager.EndEnumEntities();

}


// Update the scene between rendering
void UpdateScene(float updateTime)
{
	// Call all entity update functions
	EntityManager.UpdateAllEntities(updateTime);
	SpawnAmmoBox(updateTime);

	if (KeyHit(Key_0))
	{
		ShowText = !ShowText;
	}

	if (KeyHit(Key_1))
	{
		SMessage msg;
		msg.type = Msg_Start;
		msg.from = SystemUID;

		CEntity* entity;
		EntityManager.BeginEnumEntities("", "", "Tank");
		while (entity = EntityManager.EnumEntity())
		{
			CTankEntity* TEntity = static_cast<CTankEntity*>(entity);

			if (TEntity != nullptr)
			{
				Messenger.SendMessageA(TEntity->GetUID(), msg);
			}
		}
	}

	if (KeyHit(Key_2))
	{
		SetTanksInactive();
	}

	CEntity* entity;
	EntityManager.BeginEnumEntities("", "", "Tank");
	TankArray.clear();
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);

		if (TEntity != nullptr)
		{
			TankArray.push_back(TEntity);
		}
	}

	if (KeyHit(Key_Numpad7))
	{
		if (tankCounter <= 0.0f)
		{
			tankCounter = TankArray.size() - 1;
		}
		else
		{
			tankCounter--;
		}
		MainCamera = TankArray[tankCounter]->GetCamera();
	}
	if (KeyHit(Key_Numpad9))
	{
		if (tankCounter >= TankArray.size() - 1)
		{
			tankCounter = 0;
		}
		else
		{
			tankCounter++;
			
		}
		MainCamera = TankArray[tankCounter]->GetCamera();
	}
	if (KeyHit(Key_Numpad8))
	{
		MainCamera = LoopCamera;
	}

	if (EntityManager.GetTeamOneScore() >= 3.0f)
	{
		SetTanksInactive();
		winningTeam = "One";
		gameOver = true;
		DestroyLoserTanks(1);
	}
	else if (EntityManager.GetTeamTwoScore() >= 3.0f)
	{
		SetTanksInactive();
		winningTeam = "Two";
		gameOver = false;
		DestroyLoserTanks(0);
	}

	

	if (KeyHit(Mouse_LButton) && NearestEntity != nullptr)
	{
		SMessage msg;
		msg.type = Msg_Evade;
		msg.from = SystemUID;
		Messenger.SendMessageA(NearestEntity->GetUID(), msg);
	}
	
	if (KeyHit(Mouse_RButton))
	{
		
		if (SelectedEntity != nullptr && SelectedEntity->IsSelected())
		{
			TInt32 X, Y;
			CVector3 mousePoint = MainCamera->WorldPtFromPixel(X, Y, ViewportWidth, ViewportHeight);

			CVector3 rayDirection = mousePoint - MainCamera->Position();

			SelectedEntity->SetTarget({ mousePoint.x, SelectedEntity->Position().y, mousePoint.z});
			SelectedEntity->SetSelected(false);
		}
		if(NearestEntity != nullptr)
		{
			SelectedEntity = NearestEntity;
			SelectedEntity->SetSelected(true);
		}
		
	}

	// Set camera speeds
		// Key F1 used for full screen toggle
		if (KeyHit(Key_F2)) CameraMoveSpeed = 5.0f;
	if (KeyHit(Key_F3)) CameraMoveSpeed = 40.0f;

	// Move the camera
	MainCamera->Control(Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D,
		CameraMoveSpeed * updateTime, CameraRotSpeed * updateTime);
}

void SpawnAmmoBox(float updateTime)
{
	if (ammoSpawnTimer < 0.0f)
	{
		EntityManager.CreateAmmoBox("AmmoBox", "AmmoBox", CVector3(Random(-30.0f, 30.0f), 30.0f, Random(-30.0f, 30.0f)));
		ammoSpawnTimer = Random(ammoMinSpawnTime, ammoMaxSpawnTime);
	}
	else
	{
		ammoSpawnTimer -= updateTime;
	}
}

void SetTanksInactive()
{
	SMessage msg;
	msg.type = Msg_Inactive;
	msg.from = SystemUID;

	CEntity* entity;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);

		if (TEntity != nullptr)
		{
			Messenger.SendMessageA(TEntity->GetUID(), msg);
		}
	}
	EntityManager.EndEnumEntities();
}

void DestroyLoserTanks(int team)
{
	SMessage msg;
	msg.type = Msg_Death;
	msg.from = SystemUID;

	CEntity* entity;
	EntityManager.BeginEnumEntities("", "", "Tank");
	while (entity = EntityManager.EnumEntity())
	{
		CTankEntity* TEntity = static_cast<CTankEntity*>(entity);

		if (TEntity != nullptr && TEntity->GetTeam() == team)
		{
			Messenger.SendMessageA(TEntity->GetUID(), msg);
		}
	}
}

} // namespace gen
