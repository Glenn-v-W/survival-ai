#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "FSM.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void ProcessEvents(const SDL_Event& e) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	IExamInterface * m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;
	
	void StartEFSM();
	void StartRFSM();
	void StartIFSM();
	void StartMFSM();
	void CleanupFSM();

	///FSM
	//Enemy Handling FSM
	FSM * m_pE_StateMachine = nullptr;
	std::vector<FSMConditionBase*> m_E_Conditions = {};
		static bool Spot1OrMoreEnemies(vector<EnemyInfo> & vEnemiesInFOV);
		static bool NoEnemiesWithinSight(vector<EnemyInfo> & vEnemiesInFOV);
		static bool LetsShoot(IExamInterface* pInterface, vector<EnemyInfo> & vEnemiesInFOV, bool & slot0Item, bool & slot1Item);
	std::vector<FSMDelegateBase*> m_E_Delegates = {};
		static void AddEnemiesToSpotted(IExamInterface* pInterface, vector<EnemyInfo> & vEnemiesInFOV, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float storageTime, int & tagCounter);
		static void Shoot(IExamInterface* pInterface, bool & slot0Item, bool & slot1Item);
	std::vector<FSMState*> m_E_States = {};

	//Run Handling FSM
	FSM * m_pR_StateMachine = nullptr;
	std::vector<FSMConditionBase*> m_R_Conditions = {};
		static bool RobotShouldRun(IExamInterface* pInterface, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float runDistance);
		static bool RobotShouldNotRun(IExamInterface* pInterface, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float runDistance);
	std::vector<FSMDelegateBase*> m_R_Delegates = {};
		static void Run(bool & canRun);
		static void DoNotRun(bool & canRun);
	std::vector<FSMState*> m_R_States = {};

	//Inventory Handling FSM
	FSM * m_pI_StateMachine = nullptr;
	std::vector<FSMConditionBase*> m_I_Conditions = {};
		static bool Spot1OrMoreItems(vector<EntityInfo> & vitemsInFOV);
		static bool NoItemsWithinSight(vector<EntityInfo> & vitemsInFOV);
		static bool CanGrabItem(IExamInterface* pInterface, vector<EntityInfo> & itemsSpotted);
		static bool LetsPickUpWeapon(IExamInterface* pInterface, ItemInfo & grabbedItem, bool & slot0Item, bool & slot1Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & setWeapon0);
		static bool LetsPickUpMedpack(IExamInterface* pInterface, ItemInfo & grabbedItem, bool & slot2Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed);
		static bool LetsPickUpFood(IExamInterface* pInterface, ItemInfo & grabbedItem, bool & slot3Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed);
		static bool LetsDestroyIt(IExamInterface* pInterface, ItemInfo & grabbedItem, bool & slot0Item, bool & slot1Item, bool & slot2Item, bool & slot3Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV);
		static bool GoBack();
		static bool LetsHeal(IExamInterface* pInterface, bool & slot2Item);
		static bool LetsEat(IExamInterface* pInterface, bool & slot3Item);
std::vector<FSMDelegateBase*> m_I_Delegates = {};
		static void AddItemsToSpotted(IExamInterface* pInterface, vector<EntityInfo> & vitemsInFOV, vector<EntityInfo> & itemsSpotted);
		static void GrabItem(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & slot0Item, bool & slot1Item, bool & slot2Item, bool & slot3Item, bool & slot4Item, bool & setWeapon0);
		static void PickUpWeapon(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot0Item, bool & slot1Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & setWeapon0);
		static void PickUpMedpack(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot2Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed);
		static void PickUpFood(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot3Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed);
		static void DestroyIt(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot4Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed);
		static bool Heal(IExamInterface* pInterface, bool & slot2Item);
		static bool Eat(IExamInterface* pInterface, bool & slot3Item);
		std::vector<FSMState*> m_I_States = {};

	//Movement Handling FSM
	FSM * m_pM_StateMachine = nullptr;
	std::vector<FSMConditionBase*> m_M_Conditions = {};
		static bool SpottedAnItem(vector<EntityInfo> & itemsSpotted);
		static bool NoItemsInSight(vector<EntityInfo> & itemsSpotted);
		static bool TimeOut(float & timer, float & deltaTime);
	std::vector<FSMDelegateBase*> m_M_Delegates = {};
	static void SeekCheckpoint(SteeringPlugin_Output & steering, IExamInterface* pInterface, float maxDistance, 
		float minDistance, float enemyAvoidanceToWaypointPercentage, bool & switchDirectionBool, 
		int & lastEnemiesWithinSight, vector<EnemyInfo> & vEnemiesInFOV, vector<std::pair<EnemyInfo, float>> & enemiesSpotted);
	static void SeekItem(SteeringPlugin_Output & steering, IExamInterface* pInterface, float maxDistance, float minDistance, vector<EntityInfo> & itemsSpotted);
	static void StartTimer(float & timer, float timerMax);
	std::vector<FSMState*> m_M_States = {};


	///Variables
	//Enemy
		int m_TagCounter = 1; //Give each Enemy a tag
		vector<std::pair<EnemyInfo, float>> m_EnemiesSpotted; //Stores EnemyInfo + Time since last seen
		float m_StorageTime = 1.5f;
	//Run
		bool m_CanRun = false;
		float m_RunDistance = 15.0f; //If enemy closer than this, RUN!
	//Inventory
		vector<EntityInfo> m_ItemsSpotted;
		ItemInfo m_GrabbedItem;
		bool m_ItemSuccesfullyGrabbed = false;
		bool m_SetWeapon0;
		bool m_Slot0Item = false;
		bool m_Slot1Item = false;
		bool m_Slot2Item = false;
		bool m_Slot3Item = false;
		bool m_Slot4Item = false;
	//Movement
		Elite::Vector2 m_Target = {};
		SteeringPlugin_Output m_Steering;
		float m_DeltaTime;
		float m_Timer = 0;
		float m_TimerMax = 2.5f;
			//Rotating Robot
			int m_LastEnemiesWithinSight = 0;
			bool m_SwitchDirectionBool = false;
			//Enemy Flee Movement
			float m_MinDistance = 1.0f;
			float m_MaxDistance = 5.0f;
			float m_MinEnemyPercentage = 0.1f;
			float m_MaxEnemyPercentage = .5f;
			float m_EnemyAvoidanceToWaypointPercentage = 0.5f;
	//General
		vector<HouseInfo> m_vHousesInFOV;
		vector<EnemyInfo> m_vEnemiesInFOV;
		vector<EntityInfo> m_vItemsInFOV;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}