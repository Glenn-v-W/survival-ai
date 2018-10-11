/*=============================================================================*/
// Copyright 2017-2018 Elite Engine
// Authors: Matthieu Delaere, Thomas Goussaert
/*=============================================================================
CoreDefines.h: Contains some of the most used defines in the engine.
=============================================================================*/
#ifndef ELITE_CORE_DEFINES
#define ELITE_CORE_DEFINES
namespace Elite
{
	// === API's ===
#define USE_BOX2D

	/* --- Platforms --- */
#define PLATFORM_WINDOWS 0
#define PLATFORM_PS4 1

#ifdef _WIN32
#define PLATFORM_ID PLATFORM_WINDOWS
#elif _ORBIS
#define PLATFORM_ID PLATFORM_PS4
#endif

	//=== Suppressors ===
#define UNREFERENCED_PARAMETER(x) (x)

	//=== Management ===
#define SAFE_DELETE(p) if (p) { delete (p); (p) = nullptr; }

	//=== Exception ===
	struct Elite_Exception
	{
		explicit Elite_Exception(const std::string& msg) :_msg(msg) {}
		const std::string _msg;
	};

	//=== Aligment ===
#ifdef _WIN32
	#define ALIGN_8  __declspec(align(8))
	#define ALIGN_16 __declspec(align(16))
	#define ALIGN_32 __declspec(align(32))
	#define ALIGN_64 __declspec(align(64))
#elif
	#define ELITE_ALIGN_8 
	#define ELITE_ALIGN_16
	#define ELITE_ALIGN_32
	#define ELITE_ALIGN_64
#endif
}
#endif