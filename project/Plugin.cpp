#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include <iostream>

using namespace std;

void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);
	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Frankenboid";
	info.Student_FirstName = "Glenn";
	info.Student_LastName = "van Waesberghe";
	info.Student_Class = "2DAE1";

	//Start finite state machines:
		//Enemy Handling
	StartEFSM();
		//Run Handling
	StartRFSM();
		//Inventory Handling
	StartIFSM();
		//Movement Handling
	StartMFSM();
}

void Plugin::DllInit()
{
	//Can be used to figure out the source of a Memory Leak
	//Possible undefined behavior, you'll have to trace the source manually 
	//if you can't get the origin through _CrtSetBreakAlloc(0) [See CallStack]
	//_CrtSetBreakAlloc(0);

	//Called when the plugin is loaded
}

void Plugin::DllShutdown()
{
	//Called when the plugin gets unloaded
	CleanupFSM();
}

void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
							//params.LevelFile = "LevelTwo.gppl";
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
	params.OverrideDifficulty = false; //Override Difficulty?
	params.Difficulty = 1.f; //Difficulty Override: 0 > 1 (Overshoot is possible, >1)
}

void Plugin::ProcessEvents(const SDL_Event& e)
{
	////Demo Event Code
	////In the end your AI should be able to walk around without external input
	//switch (e.type)
	//{
	//case SDL_MOUSEBUTTONUP:
	//{
	//	if (e.button.button == SDL_BUTTON_LEFT)
	//	{
	//		int x, y;
	//		SDL_GetMouseState(&x, &y);
	//		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(x), static_cast<float>(y));
	//		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	//	}
	//	break;
	//}
	//case SDL_KEYDOWN:
	//{
	//	if (e.key.keysym.sym == SDLK_SPACE)
	//	{
	//		m_CanRun = true;
	//	}
	//	else if (e.key.keysym.sym == SDLK_LEFT)
	//	{
	//		m_AngSpeed -= Elite::ToRadians(10);
	//	}
	//	else if (e.key.keysym.sym == SDLK_RIGHT)
	//	{
	//		m_AngSpeed += Elite::ToRadians(10);
	//	}
	//	else if (e.key.keysym.sym == SDLK_g)
	//	{
	//		m_GrabItem = true;
	//	}
	//	else if (e.key.keysym.sym == SDLK_u)
	//	{
	//		m_UseItem = true;
	//	}
	//	else if (e.key.keysym.sym == SDLK_r)
	//	{
	//		m_RemoveItem = true;
	//	}
	//	else if (e.key.keysym.sym == SDLK_d)
	//	{
	//		m_DropItem = true;
	//	}
	//	break;
	//}
	//case SDL_KEYUP:
	//{
	//	if (e.key.keysym.sym == SDLK_SPACE)
	//	{
	//		m_CanRun = false;
	//	}
	//	break;
	//}
	//}
}

SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	m_DeltaTime = dt;

	m_ItemSuccesfullyGrabbed = false;
	//Update FOV vectors
	m_vHousesInFOV = GetHousesInFOV();
	m_vEnemiesInFOV.clear();
	m_vItemsInFOV.clear();
	vector<EntityInfo> vEntitiesInFOV = GetEntitiesInFOV();
	for (size_t i = 0; i < vEntitiesInFOV.size(); i++)
	{
		if (vEntitiesInFOV[i].Type == eEntityType::ENEMY)
		{
			EnemyInfo enemyInfo;
			m_pInterface->Enemy_GetInfo(vEntitiesInFOV[i], enemyInfo);
			m_vEnemiesInFOV.push_back(enemyInfo);
		}
		else if (vEntitiesInFOV[i].Type == eEntityType::ITEM)
		{
			m_vItemsInFOV.push_back(vEntitiesInFOV[i]);
		}
	}

	m_pE_StateMachine->Update();

	m_pR_StateMachine->Update();

	m_pI_StateMachine->Update();
	
	for (auto & it : m_EnemiesSpotted)
	{
		it.second -= dt;
	}

	//Remove Enemies from m_EnemiesSpotted vector if enough time has elapsed
	if (m_EnemiesSpotted.size() > 0)
	{
		if (m_EnemiesSpotted[0].second <= 0)
		{
			m_EnemiesSpotted.erase(m_EnemiesSpotted.begin());
		}
	}

	m_pM_StateMachine->Update();
	m_Steering.RunMode = m_CanRun;

	return m_Steering;
}

void Plugin::Render(float dt) const
{
	auto checkpointLocation = m_pInterface->World_GetCheckpointLocation();
	auto nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);
	m_pInterface->Draw_SolidCircle(nextTargetPos, 2.5f, { 0,0 }, { 0, 1, 0 });

	for (size_t i = 0; i < m_EnemiesSpotted.size(); i++)
	{
		m_pInterface->Draw_SolidCircle(m_EnemiesSpotted[i].first.Location, 1.0f, { 0,0 }, { 0, 0, 1 });
	}

	for (size_t i = 0; i < m_ItemsSpotted.size(); i++)
	{
		m_pInterface->Draw_SolidCircle(m_ItemsSpotted[i].Location, 1.0f, { 0,0 }, { 1, 0, 0 });
	}
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}
vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::StartEFSM()
{
	///Enemy Handling FSM
	//---------------------------------------------
	//Define transition conditions - type: FSMConditionBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMConditionBase* conditionSpottingEnemies = new FSMCondition<vector<EnemyInfo> &>(
		{ FSMDelegateContainer<bool, vector<EnemyInfo> &>(Spot1OrMoreEnemies, m_vEnemiesInFOV) });
	m_E_Conditions.push_back(conditionSpottingEnemies);

	FSMConditionBase* conditionSpottingNoEnemies = new FSMCondition<vector<EnemyInfo> &>(
		{ FSMDelegateContainer<bool, vector<EnemyInfo> &>(NoEnemiesWithinSight, m_vEnemiesInFOV) });
	m_E_Conditions.push_back(conditionSpottingNoEnemies);

	FSMConditionBase* conditionLetsShoot = new FSMCondition<IExamInterface*, vector<EnemyInfo> &, bool &, bool &>(
		{ FSMDelegateContainer<bool, IExamInterface*, vector<EnemyInfo> &, bool &, bool &>(LetsShoot, m_pInterface, m_vEnemiesInFOV, m_Slot0Item, m_Slot1Item) });
	m_E_Conditions.push_back(conditionLetsShoot);

	FSMConditionBase* conditionGoBack = new FSMCondition<>({ FSMDelegateContainer<bool>(GoBack) });
	m_E_Conditions.push_back(conditionGoBack);

	//---------------------------------------------
	//Define actions - type: FSMDelegateBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMDelegateBase* addEnemyLocationsAction = new FSMDelegate<IExamInterface*, vector<EnemyInfo> &, vector<std::pair<EnemyInfo, float>> &, float, int &>(
		{ FSMDelegateContainer<void, IExamInterface*,  vector<EnemyInfo> &,vector<std::pair<EnemyInfo, float>> &, float, int &>(AddEnemiesToSpotted, m_pInterface, m_vEnemiesInFOV, m_EnemiesSpotted, m_StorageTime, m_TagCounter) });
	m_E_Delegates.push_back(addEnemyLocationsAction);

	FSMDelegateBase* shootAction = new FSMDelegate<IExamInterface*, bool &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, bool &, bool &>(Shoot, m_pInterface, m_Slot0Item, m_Slot1Item) });
	m_E_Delegates.push_back(shootAction);

	//---------------------------------------------
	//Define states - type: FSMState*
	//Use FSMDelegateContainer to bind parameters!
	FSMState* neutralState = new FSMState();
	m_E_States.push_back(neutralState);

	FSMState* addingEnemiesState = new FSMState();
	m_E_States.push_back(addingEnemiesState);

	FSMState* shootState = new FSMState();
	m_E_States.push_back(shootState);

	//---------------------------------------------
	//BUILD STATES by assigning actions, transitions (type: FSMTransition) and conditions
	//search
	neutralState->SetTransitions(
		{
			FSMTransition({ conditionSpottingEnemies },{ }, addingEnemiesState)
		});

	addingEnemiesState->SetActions({ addEnemyLocationsAction });
	addingEnemiesState->SetTransitions(
		{
			FSMTransition({ conditionSpottingNoEnemies },{}, neutralState),
			FSMTransition({ conditionLetsShoot },{ shootAction }, shootState)
		});

	shootState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	//---------------------------------------------
	//Create state machine
	m_pE_StateMachine = new FSM(m_E_States, neutralState);

	//Boot FSM
	m_pE_StateMachine->Start();
}
void Plugin::StartRFSM()
{
	///Run Handling FSM
	//---------------------------------------------
	//Define transition conditions - type: FSMConditionBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMConditionBase* conditionshouldRun = new FSMCondition<IExamInterface*, vector<std::pair<EnemyInfo, float>> &, float>(
		{ FSMDelegateContainer<bool, IExamInterface*, vector<std::pair<EnemyInfo, float>> &, float>(RobotShouldRun, m_pInterface, m_EnemiesSpotted, m_RunDistance) });
	m_R_Conditions.push_back(conditionshouldRun);

	FSMConditionBase* conditionshouldNotRun = new FSMCondition<IExamInterface*, vector<std::pair<EnemyInfo, float>> &, float>(
		{ FSMDelegateContainer<bool, IExamInterface*, vector<std::pair<EnemyInfo, float>> &, float>(RobotShouldNotRun, m_pInterface, m_EnemiesSpotted, m_RunDistance) });
	m_R_Conditions.push_back(conditionshouldNotRun);

	//---------------------------------------------
	//Define actions - type: FSMDelegateBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMDelegateBase* runAction = new FSMDelegate<bool &>(
		{ FSMDelegateContainer<void, bool &>(Run, m_CanRun) });
	m_R_Delegates.push_back(runAction);

	FSMDelegateBase* doNotRunAction = new FSMDelegate<bool &>(
		{ FSMDelegateContainer<void, bool &>(DoNotRun, m_CanRun) });
	m_R_Delegates.push_back(doNotRunAction);

	//---------------------------------------------
	//Define states - type: FSMState*
	//Use FSMDelegateContainer to bind parameters!
	FSMState* neutralState = new FSMState();
	m_R_States.push_back(neutralState);

	FSMState* runState = new FSMState();
	m_R_States.push_back(runState);

	//---------------------------------------------
	//BUILD STATES by assigning actions, transitions (type: FSMTransition) and conditions
	//search
	neutralState->SetTransitions(
		{
			FSMTransition({ conditionshouldRun },{}, runState)
		});

	runState->SetEntryActions({ runAction });
	runState->SetExitActions({ doNotRunAction });
	runState->SetTransitions(
		{
			FSMTransition({ conditionshouldNotRun },{}, neutralState)
		});

	//---------------------------------------------
	//Create state machine
	m_pR_StateMachine = new FSM(m_R_States, neutralState);

	//Boot FSM
	m_pR_StateMachine->Start();
}
void Plugin::StartIFSM()
{
	///Inventory Handling FSM
	//---------------------------------------------
	//Define transition conditions - type: FSMConditionBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMConditionBase* conditionSpottingItems = new FSMCondition<vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, vector<EntityInfo> &>(Spot1OrMoreItems, m_vItemsInFOV) });
	m_I_Conditions.push_back(conditionSpottingItems);

	FSMConditionBase* conditionSpottingNoItems = new FSMCondition<vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, vector<EntityInfo> &>(NoItemsWithinSight, m_vItemsInFOV) });
	m_I_Conditions.push_back(conditionSpottingNoItems);

	FSMConditionBase* conditionCanGrabItem = new FSMCondition<IExamInterface*, vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, IExamInterface*, vector<EntityInfo> &>(CanGrabItem, m_pInterface, m_ItemsSpotted) });
	m_I_Conditions.push_back(conditionCanGrabItem);

	FSMConditionBase* conditionLetsPickUpWeapon = new FSMCondition<IExamInterface*, ItemInfo &, bool &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &, bool &>(
		{ FSMDelegateContainer<bool, IExamInterface*, ItemInfo &, bool &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &, bool &>(
			LetsPickUpWeapon, m_pInterface, m_GrabbedItem, m_Slot0Item, m_Slot1Item, m_ItemsSpotted, m_vItemsInFOV, m_ItemSuccesfullyGrabbed, m_SetWeapon0) });
	m_I_Conditions.push_back(conditionLetsPickUpWeapon);

	FSMConditionBase* conditionLetsPickUpMedpack = new FSMCondition<IExamInterface*, ItemInfo &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &>(
		{ FSMDelegateContainer<bool, IExamInterface*, ItemInfo &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &>(
			LetsPickUpMedpack, m_pInterface, m_GrabbedItem, m_Slot2Item, m_ItemsSpotted, m_vItemsInFOV, m_ItemSuccesfullyGrabbed) });
	m_I_Conditions.push_back(conditionLetsPickUpMedpack);

	FSMConditionBase* conditionLetsPickUpFood = new FSMCondition<IExamInterface*, ItemInfo &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &>(
		{ FSMDelegateContainer<bool, IExamInterface*, ItemInfo &, bool &, vector<EntityInfo> &, vector<EntityInfo> &, bool &>(
			LetsPickUpFood, m_pInterface, m_GrabbedItem, m_Slot3Item, m_ItemsSpotted, m_vItemsInFOV, m_ItemSuccesfullyGrabbed) });
	m_I_Conditions.push_back(conditionLetsPickUpFood);

	FSMConditionBase* conditionLetsDestroyIt = new FSMCondition<IExamInterface*, ItemInfo &, bool &, bool &, bool &, bool &, vector<EntityInfo> &, vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, IExamInterface*, ItemInfo &, bool &, bool &, bool &, bool &, vector<EntityInfo> &, vector<EntityInfo> &>(
			LetsDestroyIt, m_pInterface, m_GrabbedItem, m_Slot0Item, m_Slot1Item, m_Slot2Item, m_Slot3Item, m_ItemsSpotted, m_vItemsInFOV) });
	m_I_Conditions.push_back(conditionLetsDestroyIt);

	FSMConditionBase* conditionGoBack = new FSMCondition<>({ FSMDelegateContainer<bool>(GoBack) });
	m_I_Conditions.push_back(conditionGoBack);

	FSMConditionBase* conditionLetsHeal = new FSMCondition<IExamInterface*, bool &>({ FSMDelegateContainer<bool, IExamInterface*, bool &>(LetsHeal, m_pInterface, m_Slot2Item) });
	m_I_Conditions.push_back(conditionLetsHeal);

	FSMConditionBase* conditionLetsEat = new FSMCondition<IExamInterface*, bool &>({ FSMDelegateContainer<bool, IExamInterface*, bool &>(LetsEat, m_pInterface, m_Slot3Item) });
	m_I_Conditions.push_back(conditionLetsEat);

	//---------------------------------------------
	//Define actions - type: FSMDelegateBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMDelegateBase* addItemLocationsAction = new FSMDelegate<IExamInterface*, vector<EntityInfo> &, vector<EntityInfo> &>(
		{ FSMDelegateContainer<void, IExamInterface*,  vector<EntityInfo> &,vector<EntityInfo> &>(AddItemsToSpotted, m_pInterface, m_vItemsInFOV, m_ItemsSpotted) });
	m_I_Delegates.push_back(addItemLocationsAction);

	FSMDelegateBase* grabItemAction = new FSMDelegate<IExamInterface*, ItemInfo &, vector<EntityInfo> &, vector<EntityInfo> &, bool &, bool &, bool &, bool &, bool &, bool &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, ItemInfo &, vector<EntityInfo> &, vector<EntityInfo> &, bool &, bool &, bool &, bool &, bool &, bool &, bool &>(GrabItem, m_pInterface, m_GrabbedItem, m_ItemsSpotted, m_vItemsInFOV, m_ItemSuccesfullyGrabbed, m_Slot0Item, m_Slot1Item, m_Slot2Item, m_Slot3Item, m_Slot4Item, m_SetWeapon0) });
	m_I_Delegates.push_back(grabItemAction);

	FSMDelegateBase* pickUpWeaponAction = new FSMDelegate<IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, bool &, vector<EntityInfo> &, bool &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, bool &, vector<EntityInfo> &, bool &, bool &>(PickUpWeapon, m_pInterface, m_GrabbedItem, m_ItemsSpotted, m_Slot0Item, m_Slot1Item, m_vItemsInFOV, m_ItemSuccesfullyGrabbed, m_SetWeapon0) });
	m_I_Delegates.push_back(pickUpWeaponAction);

	FSMDelegateBase* pickUpMedpackAction = new FSMDelegate<IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(PickUpMedpack, m_pInterface, m_GrabbedItem, m_ItemsSpotted, m_Slot2Item, m_vItemsInFOV, m_ItemSuccesfullyGrabbed) });
	m_I_Delegates.push_back(pickUpMedpackAction);

	FSMDelegateBase* pickUpFoodAction = new FSMDelegate<IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(PickUpFood, m_pInterface, m_GrabbedItem, m_ItemsSpotted, m_Slot3Item, m_vItemsInFOV, m_ItemSuccesfullyGrabbed) });
	m_I_Delegates.push_back(pickUpFoodAction);

	FSMDelegateBase* destroyItAction = new FSMDelegate<IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, ItemInfo &, vector<EntityInfo> &, bool &, vector<EntityInfo> &, bool &>(DestroyIt, m_pInterface, m_GrabbedItem, m_ItemsSpotted, m_Slot4Item, m_vItemsInFOV, m_ItemSuccesfullyGrabbed) });
	m_I_Delegates.push_back(destroyItAction);

	FSMDelegateBase* healAction = new FSMDelegate<IExamInterface*, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, bool &>(Heal, m_pInterface, m_Slot2Item) });
	m_I_Delegates.push_back(healAction);

	FSMDelegateBase* eatAction = new FSMDelegate<IExamInterface*, bool &>(
		{ FSMDelegateContainer<void, IExamInterface*, bool &>(Eat, m_pInterface, m_Slot3Item) });
	m_I_Delegates.push_back(eatAction);

	//---------------------------------------------
	//Define states - type: FSMState*
	//Use FSMDelegateContainer to bind parameters!
	FSMState* neutralState = new FSMState();
	m_I_States.push_back(neutralState);

	FSMState* addingItemsState = new FSMState();
	m_I_States.push_back(addingItemsState);

	FSMState* grabItemState = new FSMState();
	m_I_States.push_back(grabItemState);

	FSMState* pickUpWeaponState = new FSMState();
	m_I_States.push_back(pickUpWeaponState);

	FSMState* pickUpMedpackState = new FSMState();
	m_I_States.push_back(pickUpMedpackState);

	FSMState* pickUpFoodState = new FSMState();
	m_I_States.push_back(pickUpFoodState);

	FSMState* destroyItState = new FSMState();
	m_I_States.push_back(destroyItState);

	FSMState* healState = new FSMState();
	m_I_States.push_back(healState);

	FSMState* eatState = new FSMState();
	m_I_States.push_back(eatState);

	//---------------------------------------------
	//BUILD STATES by assigning actions, transitions (type: FSMTransition) and conditions
	//search
	neutralState->SetTransitions(
		{
			FSMTransition({ conditionSpottingItems },{}, addingItemsState),
			FSMTransition({ conditionLetsHeal },{}, healState),
			FSMTransition({ conditionLetsEat },{}, eatState)
		});

	addingItemsState->SetActions({ addItemLocationsAction });
	addingItemsState->SetTransitions(
		{
			FSMTransition({ conditionSpottingNoItems },{}, neutralState),
			FSMTransition({ conditionCanGrabItem },{}, grabItemState)
		});

	grabItemState->SetEntryActions({grabItemAction});
	grabItemState->SetTransitions(
		{
			//FSMTransition({ conditionLetsPickUpWeapon },{ pickUpWeaponAction }, pickUpWeaponState),
			//FSMTransition({ conditionLetsPickUpMedpack },{ pickUpMedpackAction }, pickUpMedpackState),
			//FSMTransition({ conditionLetsPickUpFood },{ pickUpFoodAction }, pickUpFoodState),
			//FSMTransition({ conditionLetsDestroyIt },{ destroyItAction }, destroyItState)
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	//pickUpWeaponState->SetEntryActions({ pickUpWeaponAction });
	pickUpWeaponState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	//pickUpMedpackState->SetEntryActions({ pickUpMedpackAction });
	pickUpMedpackState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	//pickUpFoodState->SetEntryActions({ pickUpFoodAction });
	pickUpFoodState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	//destroyItState->SetEntryActions({ destroyItAction });
	destroyItState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});

	healState->SetEntryActions({ healAction });
	healState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});


	eatState->SetEntryActions({ eatAction });
	eatState->SetTransitions(
		{
			FSMTransition({ conditionGoBack },{}, neutralState)
		});


	//---------------------------------------------
	//Create state machine
	m_pI_StateMachine = new FSM(m_I_States, neutralState);

	//Boot FSM
	m_pI_StateMachine->Start();
}
void Plugin::StartMFSM()
{
	///Movement Handling FSM
	//---------------------------------------------
	//Define transition conditions - type: FSMConditionBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMConditionBase* conditionSpottedItem = new FSMCondition<vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, vector<EntityInfo> &>(SpottedAnItem, m_ItemsSpotted) });
	m_M_Conditions.push_back(conditionSpottedItem);

	FSMConditionBase* conditionSpottingNoItems = new FSMCondition<vector<EntityInfo> &>(
		{ FSMDelegateContainer<bool, vector<EntityInfo> &>(NoItemsInSight, m_ItemsSpotted) });
	m_M_Conditions.push_back(conditionSpottingNoItems);

	FSMConditionBase* conditionTimeOut = new FSMCondition<float &, float &>(
		{ FSMDelegateContainer<bool, float &, float &>(TimeOut, m_Timer, m_DeltaTime) });
	m_M_Conditions.push_back(conditionTimeOut);

	//---------------------------------------------
	//Define actions - type: FSMDelegateBase*
	//Use FSMDelegateContainer to bind parameters!
	FSMDelegateBase* seekCheckpointAction = new FSMDelegate
		<SteeringPlugin_Output &, IExamInterface*, float, float, float, bool &, int &, vector<EnemyInfo> &, vector<std::pair<EnemyInfo, float>> &>(
		{ FSMDelegateContainer<void, SteeringPlugin_Output &, IExamInterface*, float, float, float, bool &, 
			int &, vector<EnemyInfo> &, vector<std::pair<EnemyInfo, float>> &>(SeekCheckpoint, m_Steering, m_pInterface, m_MaxDistance, m_MinDistance,
				m_EnemyAvoidanceToWaypointPercentage, m_SwitchDirectionBool, m_LastEnemiesWithinSight, m_vEnemiesInFOV, m_EnemiesSpotted) });
	m_M_Delegates.push_back(seekCheckpointAction);
	
	FSMDelegateBase* seekItemAction = new FSMDelegate
		<SteeringPlugin_Output &, IExamInterface*, float, float, vector<EntityInfo> &>(
			{ FSMDelegateContainer<void, SteeringPlugin_Output &, IExamInterface*, float, float, vector<EntityInfo> &>(
				SeekItem, m_Steering, m_pInterface, m_MaxDistance, m_MinDistance, m_ItemsSpotted) });
	m_M_Delegates.push_back(seekItemAction);

	FSMDelegateBase* startTimerAction = new FSMDelegate <float &, float>({ FSMDelegateContainer<void, float &, float>(
				StartTimer, m_Timer, m_TimerMax) });
	m_M_Delegates.push_back(startTimerAction);

	//---------------------------------------------
	//Define states - type: FSMState*
	//Use FSMDelegateContainer to bind parameters!
	FSMState* seekCheckpointState = new FSMState();
	m_M_States.push_back(seekCheckpointState);

	FSMState* seekItemState = new FSMState();
	m_M_States.push_back(seekItemState);

	//---------------------------------------------
	//BUILD STATES by assigning actions, transitions (type: FSMTransition) and conditions
	//search
	seekCheckpointState->SetActions({ seekCheckpointAction });
	seekCheckpointState->SetTransitions(
		{
			FSMTransition({ conditionSpottedItem },{startTimerAction}, seekItemState)
		});

	seekItemState->SetActions({ seekItemAction });
	seekItemState->SetTransitions(
		{
			FSMTransition({ conditionSpottingNoItems },{}, seekCheckpointState),
			FSMTransition({ conditionTimeOut },{}, seekCheckpointState)
		});

	//---------------------------------------------
	//Create state machine
	m_pM_StateMachine = new FSM(m_M_States, seekCheckpointState);

	//Boot FSM
	m_pM_StateMachine->Start();
}
void Plugin::CleanupFSM()
{
	//Cleanup
	//Enemy
	SAFE_DELETE(m_pE_StateMachine);

	for (int i = 0; i < static_cast<int>(m_E_Conditions.size()); ++i)
		SAFE_DELETE(m_E_Conditions[i]);
	m_E_Conditions.clear();

	for (int i = 0; i < static_cast<int>(m_E_Delegates.size()); ++i)
		SAFE_DELETE(m_E_Delegates[i]);
	m_E_Delegates.clear();

	for (int i = 0; i < static_cast<int>(m_E_States.size()); ++i)
		SAFE_DELETE(m_E_States[i]);
	m_E_States.clear();

	//Run
	SAFE_DELETE(m_pR_StateMachine);

	for (int i = 0; i < static_cast<int>(m_R_Conditions.size()); ++i)
		SAFE_DELETE(m_R_Conditions[i]);
	m_R_Conditions.clear();

	for (int i = 0; i < static_cast<int>(m_R_Delegates.size()); ++i)
		SAFE_DELETE(m_R_Delegates[i]);
	m_R_Delegates.clear();

	for (int i = 0; i < static_cast<int>(m_R_States.size()); ++i)
		SAFE_DELETE(m_R_States[i]);
	m_R_States.clear();

	//Inventory
	SAFE_DELETE(m_pI_StateMachine);

	for (int i = 0; i < static_cast<int>(m_I_Conditions.size()); ++i)
		SAFE_DELETE(m_I_Conditions[i]);
	m_I_Conditions.clear();

	for (int i = 0; i < static_cast<int>(m_I_Delegates.size()); ++i)
		SAFE_DELETE(m_I_Delegates[i]);
	m_I_Delegates.clear();

	for (int i = 0; i < static_cast<int>(m_I_States.size()); ++i)
		SAFE_DELETE(m_I_States[i]);
	m_I_States.clear();

	//Movement
	SAFE_DELETE(m_pM_StateMachine);

	for (int i = 0; i < static_cast<int>(m_M_Conditions.size()); ++i)
		SAFE_DELETE(m_M_Conditions[i]);
	m_M_Conditions.clear();

	for (int i = 0; i < static_cast<int>(m_M_Delegates.size()); ++i)
		SAFE_DELETE(m_M_Delegates[i]);
	m_M_Delegates.clear();

	for (int i = 0; i < static_cast<int>(m_M_States.size()); ++i)
		SAFE_DELETE(m_M_States[i]);
	m_M_States.clear();
}


//Enemy Handling FSM
//Conditions
bool Plugin::Spot1OrMoreEnemies(vector<EnemyInfo> & vEnemiesInFOV)
{
	if (vEnemiesInFOV.size() > 0)
	{
		return true;
	}
	return false;
}
bool Plugin::NoEnemiesWithinSight(vector<EnemyInfo> & vEnemiesInFOV)
{
	if (vEnemiesInFOV.size() == 0)
	{
		return true;
	}
	return false;
}
bool Plugin::LetsShoot(IExamInterface * pInterface, vector<EnemyInfo> & vEnemiesInFOV, bool & slot0Item, bool & slot1Item)
{
	if (slot1Item || slot0Item)
	{
		auto agentInfo = pInterface->Agent_GetInfo();
		if (vEnemiesInFOV.size() > 0)
		{
			Elite::Vector2 facingDir = Elite::Vector2(cos(agentInfo.Orientation - b2_pi / 2.f), sin(agentInfo.Orientation - b2_pi / 2.f));
			facingDir.Normalize();

			for (size_t i = 0; i < vEnemiesInFOV.size(); i++)
			{
				//Check if facing in correct direction

				Elite::Vector2 enemyDir = Elite::Vector2(vEnemiesInFOV[i].Location - agentInfo.Position);
				enemyDir.Normalize();

				float dot = facingDir.x * enemyDir.x + facingDir.y * enemyDir.y;
				float det = facingDir.x * enemyDir.y - facingDir.y * enemyDir.x;
				float angle = atan2(det, dot);

				if (angle < 0)
				{
					angle *= -1;
				}

				if (angle < 0.05f) // Insert something here
				{
					if (slot1Item)
					{
						ItemInfo gun1;
						pInterface->Inventory_GetItem(1, gun1);
						float range = pInterface->Item_GetMetadata(gun1, "range");
						float distance = vEnemiesInFOV[i].Location.Distance(agentInfo.Position);
						//Check if close enough
						if (distance < range)
						{
							return true;
						}
					}
					if (slot0Item)
					{
						ItemInfo gun0;
						pInterface->Inventory_GetItem(0, gun0);
						float range = pInterface->Item_GetMetadata(gun0, "range");
						float distance = vEnemiesInFOV[i].Location.Distance(agentInfo.Position);
						//Check if close enough
						if (distance < range)
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}
//Actions
void Plugin::AddEnemiesToSpotted(IExamInterface* pInterface, vector<EnemyInfo> & vEnemiesInFOV, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float storageTime, int & tagCounter)
{
	//Save all spotted enemies into the m_EnemiesSpotted vector
	for (size_t i = 0; i < vEnemiesInFOV.size(); i++)
	{
		int tag = vEnemiesInFOV[i].Tag;

		if (tag == 0)
		{
			//Handle Tags
				//Actually Global
				pInterface->Enemy_SetTag(vEnemiesInFOV[i], tagCounter);
				//Local in vector
				vEnemiesInFOV[i].Tag = tagCounter;
				//Increment Counter
				tagCounter++;

			enemiesSpotted.push_back(std::pair<EnemyInfo, float>(vEnemiesInFOV[i], storageTime));
		}
		else
		{
			bool foundEnemyInVector = false;
			for (size_t j = 0; j < enemiesSpotted.size(); j++)
			{
				if (enemiesSpotted[j].first.Tag == tag)
				{
					enemiesSpotted[j].second = storageTime;
					enemiesSpotted[j].first.Location = vEnemiesInFOV[i].Location;
					foundEnemyInVector = true;
				}
			}
			if (!foundEnemyInVector)
			{
				enemiesSpotted.push_back(std::pair<EnemyInfo, float>(vEnemiesInFOV[i], storageTime));
			}
		}

	}
}
void Plugin::Shoot(IExamInterface * pInterface, bool & slot0Item, bool & slot1Item)
{
	if (slot0Item)
	{
		pInterface->Inventory_UseItem(0);
		ItemInfo itemInfo;
		pInterface->Inventory_GetItem(0, itemInfo);
		int ammo = pInterface->Item_GetMetadata(itemInfo, "ammo");
		if (ammo == 0)
		{
			pInterface->Inventory_RemoveItem(0);
			slot0Item = false;
		}
	}
	else
	{
		pInterface->Inventory_UseItem(1);
		ItemInfo itemInfo;
		pInterface->Inventory_GetItem(1, itemInfo);
		int ammo = pInterface->Item_GetMetadata(itemInfo, "ammo");
		if (ammo == 0)
		{
			pInterface->Inventory_RemoveItem(1);
			slot1Item = false;
		}
	}
}


//Run Handling FSM
//Conditions
bool Plugin::RobotShouldRun(IExamInterface * pInterface, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float runDistance)
{
	auto agentInfo = pInterface->Agent_GetInfo();
	if (agentInfo.Stamina > 0)
	{
		for (size_t i = 0; i < enemiesSpotted.size(); i++)
		{
			float distance = enemiesSpotted[i].first.Location.Distance(agentInfo.Position);
			if (distance < runDistance)
			{
				return true;
			}
		}
		if (agentInfo.Bitten)
		{
			return true;
		}
	}
	return false;
}
bool Plugin::RobotShouldNotRun(IExamInterface * pInterface, vector<std::pair<EnemyInfo, float>> & enemiesSpotted, float runDistance)
{
	auto agentInfo = pInterface->Agent_GetInfo();
	if (agentInfo.Stamina > 0)
	{
		for (size_t i = 0; i < enemiesSpotted.size(); i++)
		{
			if (enemiesSpotted[i].first.Location.Distance(agentInfo.Position) < runDistance)
			{
				return false;
			}
		}
		if (agentInfo.Bitten)
		{
			return false;
		}
	}
	return true;
}
//Actions
void Plugin::Run(bool & canRun)
{
	canRun = true;
}
void Plugin::DoNotRun(bool & canRun)
{
	canRun = false;
}


//Inventory Handling FSM
//Conditions
bool Plugin::Spot1OrMoreItems(vector<EntityInfo> & vitemsInFOV)
{
	if (vitemsInFOV.size() > 0)
	{
		return true;
	}
	return false;
}
bool Plugin::NoItemsWithinSight(vector<EntityInfo> & vitemsInFOV)
{
	if (vitemsInFOV.size() == 0)
	{
		return true;
	}
	return false;
}
bool Plugin::CanGrabItem(IExamInterface* pInterface, vector<EntityInfo> & itemsSpotted)
{
	if (itemsSpotted.size() > 0)
	{
		AgentInfo agentInfo = pInterface->Agent_GetInfo();
		if (itemsSpotted[0].Location.Distance(agentInfo.Position) < 2.5f)
		{
			return true;
		}
	}
	return false;
}
bool Plugin::LetsPickUpWeapon(IExamInterface * pInterface, ItemInfo & grabbedItem, bool & slot0Item, bool & slot1Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & setWeapon0)
{
	if (grabbedItem.Type == eItemType::PISTOL)
	{
		if (!slot0Item || !slot1Item)
		{
			return true;
		}
		if (slot0Item)
		{
			ItemInfo oldItem;
			if (vItemsInFOV.size() == 0)
			{
				return false;
			}
			int attempts = 0;
			while (!itemSuccesfullyGrabbed)
			{
				for (size_t i = 0; i < vItemsInFOV.size(); i++)
				{
					if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
					{
						itemSuccesfullyGrabbed = pInterface->Item_Grab(itemsSpotted[0], grabbedItem);
					}
				}
				if (attempts == 5)
				{
					return false;
				}
				attempts++;
			}
			pInterface->Inventory_GetItem(0, oldItem);
			int oldAmmo = pInterface->Item_GetMetadata(oldItem, "ammo");
			int oldDps = pInterface->Item_GetMetadata(oldItem, "dps");

			int ammo = pInterface->Item_GetMetadata(grabbedItem, "ammo");
			int dps = pInterface->Item_GetMetadata(grabbedItem, "dps");

			if (ammo * dps > oldAmmo * oldDps)
			{
				setWeapon0 = true;
				return true;
			}
		}
		else if (slot1Item)
		{
			ItemInfo oldItem;
			pInterface->Inventory_GetItem(1, oldItem);
			int oldAmmo = pInterface->Item_GetMetadata(oldItem, "ammo");
			int oldDps = pInterface->Item_GetMetadata(oldItem, "dps");

			int ammo = pInterface->Item_GetMetadata(grabbedItem, "ammo");
			int dps = pInterface->Item_GetMetadata(grabbedItem, "dps");

			if (ammo * dps > oldAmmo * oldDps)
			{
				setWeapon0 = false;
				return true;
			}
		}
	}
	return false;
}
bool Plugin::LetsPickUpMedpack(IExamInterface * pInterface, ItemInfo & grabbedItem, bool & slot2Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed)
{
	if (grabbedItem.Type == eItemType::MEDKIT)
	{
		if (!slot2Item)
		{
			return true;
		}
		else
		{
			ItemInfo oldItem;
			if (vItemsInFOV.size() == 0)
			{
				return false;
			}
			int attempts = 0;
			while (!itemSuccesfullyGrabbed)
			{
				for (size_t i = 0; i < vItemsInFOV.size(); i++)
				{
					if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
					{
						itemSuccesfullyGrabbed = pInterface->Item_Grab(itemsSpotted[0], grabbedItem);
					}
				}
				if (attempts == 5)
				{
					return false;
				}
				attempts++;
			}
			pInterface->Inventory_GetItem(2, oldItem);
			int oldHealth = pInterface->Item_GetMetadata(oldItem, "health");

			int health = pInterface->Item_GetMetadata(grabbedItem, "health");
			if (health > oldHealth)
			{
				return true;
			}
		}
	}
	return false;
}
bool Plugin::LetsPickUpFood(IExamInterface * pInterface, ItemInfo & grabbedItem, bool & slot3Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed)
{
	if (grabbedItem.Type == eItemType::FOOD)
	{
		if (!slot3Item)
		{
			return true;
		}
		else
		{
			ItemInfo oldItem;
			if (vItemsInFOV.size() == 0)
			{
				return false;
			}
			int attempts = 0;
			while (!itemSuccesfullyGrabbed)
			{
				for (size_t i = 0; i < vItemsInFOV.size(); i++)
				{
					if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
					{
						itemSuccesfullyGrabbed = pInterface->Item_Grab(itemsSpotted[0], grabbedItem);
					}
				}
				if (attempts == 5)
				{
					return false;
				}
				attempts++;
			}
			pInterface->Inventory_GetItem(3, oldItem);
			int oldEnergy = pInterface->Item_GetMetadata(oldItem, "energy");

			int energy = pInterface->Item_GetMetadata(grabbedItem, "energy");
			if (energy > oldEnergy)
			{
				return true;
			}
		}
	}
	return false;
}
bool Plugin::LetsDestroyIt(IExamInterface * pInterface, ItemInfo & grabbedItem, bool & slot0Item, bool & slot1Item, bool & slot2Item, bool & slot3Item, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV)
{
	return true;
}
bool Plugin::GoBack()
{
	return true;
}
bool Plugin::LetsHeal(IExamInterface * pInterface, bool & slot2Item)
{
	if (!slot2Item)
	{
		return false;
	}
	ItemInfo itemInfo;
	pInterface->Inventory_GetItem(2, itemInfo);

	AgentInfo agentInfo = pInterface->Agent_GetInfo();
	float maxHealth = 10.0f;
	float currentHealth = agentInfo.Health;
	int healthGain = pInterface->Item_GetMetadata(itemInfo, "health");

	if (currentHealth + healthGain < maxHealth)
	{
		return true;
	}
	return false;
}
bool Plugin::LetsEat(IExamInterface * pInterface, bool & slot3Item)
{
	if (!slot3Item)
	{
		return false;
	}
	ItemInfo itemInfo;
	pInterface->Inventory_GetItem(3, itemInfo);

	AgentInfo agentInfo = pInterface->Agent_GetInfo();
	float maxEnergy = 10.0f;
	float currentEnergy = agentInfo.Energy;
	float energyGain = pInterface->Item_GetMetadata(itemInfo, "energy");

	if (currentEnergy + energyGain < maxEnergy)
	{
		return true;
	}
	return false;
}
//Actions
void Plugin::AddItemsToSpotted(IExamInterface* pInterface, vector<EntityInfo> & vitemsInFOV, vector<EntityInfo> & itemsSpotted)
{
	//Save all spotted enemies into the m_EnemiesSpotted vector
	for (size_t i = 0; i < vitemsInFOV.size(); i++)
	{
		Elite::Vector2 location = vitemsInFOV[i].Location;

		//If not in vector already
		bool alreadyInVector = false;
		for (size_t j = 0; j < itemsSpotted.size(); j++)
		{
			if (itemsSpotted[j].Location == location)
			{
				itemsSpotted[j] = vitemsInFOV[i];
				alreadyInVector = true;
			}
		}
		if (!alreadyInVector)
		{
			itemsSpotted.push_back(vitemsInFOV[i]);
		}
	}
}
void Plugin::GrabItem(IExamInterface* pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & slot0Item, bool & slot1Item, bool & slot2Item, bool & slot3Item, bool & slot4Item, bool & setWeapon0)
{
	///CODE FOR PICKING UP AN ITEM -> MOVE TO ITEM FSM
	if (!itemSuccesfullyGrabbed)
	{
		for (size_t i = 0; i < vItemsInFOV.size(); i++)
		{
			if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
			{
				itemSuccesfullyGrabbed = pInterface->Item_Grab(vItemsInFOV[i], grabbedItem);
			}
		}
	}
	if (LetsPickUpWeapon(pInterface, grabbedItem, slot0Item, slot1Item, itemsSpotted, vItemsInFOV, itemSuccesfullyGrabbed, setWeapon0))
	{
		PickUpWeapon(pInterface, grabbedItem, itemsSpotted, slot0Item, slot1Item, vItemsInFOV, itemSuccesfullyGrabbed, setWeapon0);
	}
	else if (LetsPickUpMedpack(pInterface, grabbedItem, slot2Item, itemsSpotted, vItemsInFOV, itemSuccesfullyGrabbed))
	{
		PickUpMedpack(pInterface, grabbedItem, itemsSpotted, slot2Item, vItemsInFOV, itemSuccesfullyGrabbed);
	}
	else if (LetsPickUpFood(pInterface, grabbedItem, slot3Item, itemsSpotted, vItemsInFOV, itemSuccesfullyGrabbed))
	{
		PickUpFood(pInterface, grabbedItem, itemsSpotted, slot3Item, vItemsInFOV, itemSuccesfullyGrabbed);
	}
	else
	{
		DestroyIt(pInterface, grabbedItem, itemsSpotted, slot4Item, vItemsInFOV, itemSuccesfullyGrabbed);
	}
}
void Plugin::PickUpWeapon(IExamInterface * pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot0Item, bool & slot1Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed, bool & setWeapon0)
{
	if (!itemSuccesfullyGrabbed)
	{
		for (size_t i = 0; i < vItemsInFOV.size(); i++)
		{
			if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
			{
				pInterface->Item_Grab(vItemsInFOV[i], grabbedItem);
			}
		}
	}

	if (!slot0Item && !slot1Item)
	{
		slot0Item = true;
		pInterface->Inventory_AddItem(0, grabbedItem);
		itemsSpotted.erase(itemsSpotted.begin());
		itemSuccesfullyGrabbed = false;
		return;
	}
	else
	{
		if (!slot0Item && slot1Item)
		{
			slot0Item = true;
			pInterface->Inventory_AddItem(0, grabbedItem);
			itemsSpotted.erase(itemsSpotted.begin());
			itemSuccesfullyGrabbed = false;
			return;
		}
		else if(slot0Item && !slot1Item)
		{
			slot1Item = true;
			pInterface->Inventory_AddItem(1, grabbedItem);
			itemsSpotted.erase(itemsSpotted.begin());
			itemSuccesfullyGrabbed = false;
			return;
		}
		else if (setWeapon0)
		{
			if (slot0Item)
			{
				pInterface->Inventory_RemoveItem(0);
			}
			slot0Item = true;
			pInterface->Inventory_AddItem(0, grabbedItem);
			itemsSpotted.erase(itemsSpotted.begin());
			itemSuccesfullyGrabbed = false;
			return;
		}
		else
		{
			if (slot1Item)
			{
				pInterface->Inventory_RemoveItem(0);
			}
			slot1Item = true;
			pInterface->Inventory_AddItem(1, grabbedItem);
			itemsSpotted.erase(itemsSpotted.begin());
			itemSuccesfullyGrabbed = false;
			return;
		}
	}
}
void Plugin::PickUpMedpack(IExamInterface * pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot2Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed)
{
	if (!itemSuccesfullyGrabbed)
	{
		for (size_t i = 0; i < vItemsInFOV.size(); i++)
		{
			if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
			{
				pInterface->Item_Grab(vItemsInFOV[i], grabbedItem);
			}
		}
	}
	if (slot2Item)
	{
		pInterface->Inventory_UseItem(2);
		pInterface->Inventory_RemoveItem(2);
	}
	bool success = pInterface->Inventory_AddItem(2, grabbedItem);
	slot2Item = success;
	itemsSpotted.erase(itemsSpotted.begin());
	itemSuccesfullyGrabbed = false;
}
void Plugin::PickUpFood(IExamInterface * pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot3Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed)
{
	if (!itemSuccesfullyGrabbed)
	{
		for (size_t i = 0; i < vItemsInFOV.size(); i++)
		{
			if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
			{
				pInterface->Item_Grab(vItemsInFOV[i], grabbedItem);
			}
		}
	}
	if (slot3Item)
	{
		pInterface->Inventory_UseItem(3);
		pInterface->Inventory_RemoveItem(3);
	}
	bool success = pInterface->Inventory_AddItem(3, grabbedItem);
	slot3Item = success;
	itemsSpotted.erase(itemsSpotted.begin());
	itemSuccesfullyGrabbed = false;
}
void Plugin::DestroyIt(IExamInterface * pInterface, ItemInfo & grabbedItem, vector<EntityInfo> & itemsSpotted, bool & slot4Item, vector<EntityInfo> & vItemsInFOV, bool & itemSuccesfullyGrabbed)
{
	if (!itemSuccesfullyGrabbed)
	{
		for (size_t i = 0; i < vItemsInFOV.size(); i++)
		{
			if (vItemsInFOV[i].Location == itemsSpotted[0].Location)
			{
				pInterface->Item_Grab(vItemsInFOV[i], grabbedItem);
			}
		}
	}
	if (slot4Item)
	{
		ItemInfo itemInfo;
		pInterface->Inventory_GetItem(4, itemInfo);
		switch (itemInfo.Type)
		{
		case eItemType::FOOD:
			pInterface->Inventory_UseItem(4);
			break;
		case eItemType::MEDKIT:
			pInterface->Inventory_UseItem(4);
			break;
		default:
			break;
		}
		pInterface->Inventory_RemoveItem(4);
	}
	pInterface->Inventory_AddItem(4, grabbedItem);
	pInterface->Inventory_RemoveItem(4);
	itemsSpotted.erase(itemsSpotted.begin());
	itemSuccesfullyGrabbed = false;
}
bool Plugin::Heal(IExamInterface * pInterface, bool & slot2Item)
{
	pInterface->Inventory_UseItem(2);
	pInterface->Inventory_RemoveItem(2);
	slot2Item = false;
	return false;
}
bool Plugin::Eat(IExamInterface * pInterface, bool & slot3Item)
{
	pInterface->Inventory_UseItem(3);
	pInterface->Inventory_RemoveItem(3);
	slot3Item = false;
	return false;
}


//Movement Handling FSM
//Conditions
bool Plugin::SpottedAnItem(vector<EntityInfo> & itemsSpotted)
{
	return (itemsSpotted.size() > 0);
}
bool Plugin::NoItemsInSight(vector<EntityInfo> & itemsSpotted)
{
	return (itemsSpotted.size() == 0);
}
bool Plugin::TimeOut(float & timer, float & deltaTime)
{
	timer -= deltaTime;
	if (timer <= 0)
	{
		return true;
	}
	return false;
}
//Action
void Plugin::SeekCheckpoint(SteeringPlugin_Output & steering, IExamInterface* pInterface, float maxDistance,
	float minDistance, float enemyAvoidanceToWaypointPercentage, bool & switchDirectionBool,
	int & lastEnemiesWithinSight, vector<EnemyInfo> & vEnemiesInFOV, vector<std::pair<EnemyInfo, float>> & enemiesSpotted)
{
	steering = SteeringPlugin_Output();

	AgentInfo agentInfo = pInterface->Agent_GetInfo();

	Elite::Vector2 checkpointLocation = pInterface->World_GetCheckpointLocation();
	Elite::Vector2 nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(checkpointLocation);


	float minToMaxDistanceScale = 1 / (maxDistance - minDistance);
	vector<Elite::Vector2> desiredDirections;

	desiredDirections.push_back(nextTargetPos - agentInfo.Position);

	//Enemy Flee Influence
	vector<float> m_EnemyDistances;
	vector<float> m_EnemyPriorities;

	//Calculate the distance between all positions in the m_EnemiesSpotted vector and the player
	for (size_t i = 0; i < enemiesSpotted.size(); i++)
	{
		m_EnemyDistances.push_back(enemiesSpotted[i].first.Location.Distance(agentInfo.Position));
	}

	//Calculate Priority(/Weight) of each location
	for (size_t i = 0; i < m_EnemyDistances.size(); i++)
	{
		float temp = m_EnemyDistances[i] - minDistance;
		temp *= minToMaxDistanceScale;
		m_EnemyPriorities.push_back(temp);
	}

	//Calculate flee directions and apply priority(/weight)
	for (size_t i = 0; i < m_EnemyPriorities.size(); i++)
	{
		Elite::Vector2 temp = Elite::Vector2(enemiesSpotted[i].first.Location - agentInfo.Position);
		temp *= -m_EnemyPriorities[i];
		desiredDirections.push_back(temp);
	}

	//Add these together and normalize
	Elite::Vector2 desiredDirection;
	for (size_t i = 0; i < desiredDirections.size(); i++)
	{
		desiredDirection += desiredDirections[i];
	}
	desiredDirection.Normalize();

	//Apply flee-to-seek percentage
	desiredDirection *= enemyAvoidanceToWaypointPercentage;

	//Add seeking checkpoint
	Elite::Vector2 goalDir = (nextTargetPos - agentInfo.Position);
	goalDir.Normalize();
	goalDir *= (1 - enemyAvoidanceToWaypointPercentage);
	desiredDirection += goalDir;

	desiredDirection.Normalize();

	steering.LinearVelocity = desiredDirection;
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

														 //Rotation Logic
	float angSpeed = -float(E_PI_2);

	if (switchDirectionBool)
	{
		angSpeed *= -1.0f;
	}

	if (size_t(lastEnemiesWithinSight) > vEnemiesInFOV.size())
	{
		switchDirectionBool = !switchDirectionBool;
	}
	lastEnemiesWithinSight = vEnemiesInFOV.size();


	steering.AngularVelocity = angSpeed;
	steering.AutoOrientate = false;
}
void Plugin::SeekItem(SteeringPlugin_Output & steering, IExamInterface* pInterface, float maxDistance, float minDistance, vector<EntityInfo> & itemsSpotted)
{
	steering = SteeringPlugin_Output();
	
	auto agentInfo = pInterface->Agent_GetInfo();

	auto nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(itemsSpotted[0].Location);
	
	//Simple Seek Behaviour (towards Target)
	steering.LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed
	
	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrientate = true; //Setting AutoOrientate to TRue overrides the AngularVelocity
	
	steering.RunMode = false;
}
void Plugin::StartTimer(float & timer, float timerMax)
{
	timer = timerMax;
}
