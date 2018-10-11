#pragma once
#pragma region MISC
struct SteeringPlugin_Output
{
	Elite::Vector2 LinearVelocity = { 0.f,0.f };
	float AngularVelocity = 0.f;
	bool AutoOrientate = true;
	bool RunMode = false;
};

struct PluginInfo
{
	string BotName = "Minion";
	string Student_FirstName = "Not Set";
	string Student_LastName = "Not Set";
	string Student_Class = "Not Set";
};

struct GameDebugParams //Debuggin Purposes only (Ignored during release build)
{
	GameDebugParams(bool spawnEnemies = true, int enemyCount = 20, bool godMode = false, bool autoFollowCam = false)
	{
		SpawnEnemies = spawnEnemies;
		GodMode = godMode;
		AutoFollowCam = autoFollowCam;
		EnemyCount = enemyCount;
	}

	bool SpawnEnemies = true; //Spawn enemies?
	int EnemyCount = 20; //Amount of enemies?
	int ItemCount = 40; //Amount of items?
	bool GodMode = false; //Use GodMode? (Invincible)
	bool IgnoreEnergy = false; //Ignore energy depletion
	bool AutoFollowCam = false; //Auto follow the player
	bool RenderUI = false; //Render Player UI (Parameters)
	bool AutoGrabClosestItem = false; //Auto Grab closest item (Item_Grab)
	string LevelFile = "LevelOne.gppl"; //Level to load?
	int Seed = 1234; //Seed for random generator
	bool OverrideDifficulty = false; //Override difficulty?
	float Difficulty = 1.f; //0 > ... (Easy > Harder) [1.0 is max, but overshoot is possible]
};
#pragma endregion

//Helpers
//*******
union CheapVariant
{
	CheapVariant() {}

	//INT
	int iVal;
	CheapVariant(int val) { iVal = val; }
	operator int() const { return iVal; }

	//UINT
	UINT uiVal;
	CheapVariant(UINT val) { uiVal = val; }
	operator UINT() const { return uiVal; }

	//FLOAT
	float fVal;
	CheapVariant(float val) { fVal = val; }
	operator float() const { return fVal; }

	//BOOL
	bool bVal;
	CheapVariant(bool val) { bVal = val; }
	operator bool() const { return bVal; }
};

#pragma region ENTITIES
//Enumerations
//************
enum class eEntityType
{
	ITEM,
	ENEMY,

	//@END
	_LAST = ENEMY
};

enum class eItemType
{
	PISTOL,
	MEDKIT,
	FOOD,
	GARBAGE,
	RANDOM_DROP, //Internal Only
	RANDOM_DROP_WITH_CHANCE, //Internal Only

	//@END
	_LAST = GARBAGE
};

enum class eEnemyType
{
	DEFAULT,
	ZOMBIE_NORMAL,
	ZOMBIE_RUNNER,
	ZOMBIE_HEAVY,
	RANDOM_ENEMY, //Internal Only
	//...

	//@END
	_LAST = ZOMBIE_HEAVY
};


//Structures
//**********
struct StatisticsInfo
{
	int Score; //Current Score
	float Difficulty; //Current difficulty (0 > 1 > ... / Easy > Normal > Hard)
	float TimeSurvived; //Total time survived
	float KillCountdown; //Make sure to make a kill before this timer runs out

	int NumEnemiesKilled; //Amount of enemies killed
	int NumEnemiesHit; //Amount of enemy hits
	int NumItemsPickUp; //Amount of items picked up
	int NumMissedShots; //Shots missed after firing
	int NumIgnoredCalls; //Interface calles that failed for some reason (check the console)
	int NumChkpntsReached; //Amount of checkpoints reached

};

struct HouseInfo
{
	Elite::Vector2 Center;
	Elite::Vector2 Size;
};

struct EnemyInfo
{
	eEnemyType Type;
	Elite::Vector2 Location;

	int EnemyHash = 0;
	float Size;
	int Health;

	int Tag = 0;
};

struct ItemInfo
{
	eItemType Type;
	Elite::Vector2 Location;

	int ItemHash = 0;
};

struct EntityInfo
{
	eEntityType Type;
	Elite::Vector2 Location;

	int EntityHash = 0;
};

struct WorldInfo
{
	Elite::Vector2 Center;
	Elite::Vector2 Dimensions;
};

struct AgentInfo
{
	float Stamina;
	float Health;
	float Energy;
	bool RunMode;
	bool IsInHouse;
	bool Bitten;
	bool Death;

	float FOV_Angle;
	float FOV_Range;

	Elite::Vector2 LinearVelocity;
	float AngularVelocity;
	float CurrentLinearSpeed;
	Elite::Vector2 Position;
	float Orientation;
	float MaxLinearSpeed;
	float MaxAngularSpeed;
	float GrabRange;
	float AgentSize;
};
#pragma endregion