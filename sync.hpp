#pragma once
#include <mutex>
#include <array>
#include <list>
#include <condition_variable>
#include "resources.hpp"

static constexpr int CPU_CORES = 12;
static constexpr int NUM_FILES = 15;
static constexpr int NUM_JOBS = 9;

struct CIPlatform
{
	std::mutex ciMutex;
	std::condition_variable ciCv;
	bool isStaticAnalysisDispatched{ false };
	std::list<int> jobDispatchList;

	bool run{ true };
	std::mutex printMutex;
	CPU<CPU_CORES> cpu;

	std::mutex saEndMutex;
	int saEndCounter{ 0 };

	std::array<File, NUM_FILES> files
	{{
		{"Main.cpp"},
		{"Config.cpp"},
		{"Config.hpp"},
		{"ThreadSync.cpp"},
		{"ThreadSync.hpp"},
		{"EventDispatcher.cpp"},
		{"EventDispatcher.hpp"},
		{"MsgSender.cpp"},
		{"MsgSender.hpp"},
		{"UnitTests.cpp"},
		{"UnitTests.hpp"},
		{"EnumTypes.hpp"},
		{"ExtDef.hpp"},
		{"ExtInstrincts.cc"},
		{"ExtMakefile"},
	}};

	void printCores();
	void printFiles();
	void printJobList();
};

void startVisualisation();
