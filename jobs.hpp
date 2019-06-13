#pragma once
#include "sync.hpp"
#include "resources.hpp"
#include <random>
#include <vector>
#include <functional>
#include <thread>
#include <ncurses.h>
#include <chrono>
#include <ctime>

using namespace std::chrono_literals;

struct IJob
{
	const int id;
	const std::string name;
	const int numOfRequiredCores;
	std::vector<std::reference_wrapper<Core>> heldCores;
	std::vector<std::reference_wrapper<File>> files;
	CIPlatform& sync;
	std::mt19937 randEngine{ std::random_device{}() };
	std::uniform_int_distribution<> randNumber{2, 5};

	int getRandNumber();
	bool areFilesFree();
	void waitNicely(int multiplier = 1);
	void lockCores();
	void unlockCores();
	void lockFiles();
	void unlockFiles();

	virtual void execute() = 0;
	virtual void printState(std::string) = 0;

	IJob(int id,
		std::string name,
		int required_resources_num,
		std::vector<std::reference_wrapper<File>> required_files,
		CIPlatform& ci_platform) :
				id(id),
				name(name),
				sync(ci_platform),
				numOfRequiredCores(required_resources_num),
				files(std::move(required_files))
		{
			heldCores.reserve(numOfRequiredCores);
		}

	virtual ~IJob()
	{
		std::lock_guard<std::mutex> sa_counter_mutex(sync.saEndMutex);
		sync.saEndCounter++;
		if (sync.saEndCounter == NUM_JOBS - 1)
		{
			{
				std::lock_guard<std::mutex> sa_end_mutex(sync.ciMutex);
				sync.jobDispatchList.push_front(NUM_JOBS - 1);
			}
			sync.ciCv.notify_one();
		}
	}
};

struct StandardJob : public IJob
{
	void execute() override
	{
		printState("is idle");
		while(sync.run)
		{
			printState("is waiting to be queued");
			{
				std::lock_guard<std::mutex> jobListLock(sync.ciMutex);
				lastExecuted = std::chrono::system_clock::now();
				printDate();
				sync.jobDispatchList.push_back(id);
				sync.printJobList();
			}

			std::unique_lock<std::mutex> criticalSectionLock(sync.ciMutex);
			printState("is waiting for files and resources");
			sync.ciCv.wait(criticalSectionLock, [&]
			{
				return (sync.jobDispatchList.front() == id) &&
						(sync.cpu.numOfFreeCores >= numOfRequiredCores) &&
						(areFilesFree());
			});
			sync.jobDispatchList.pop_front();
			sync.printJobList();
			printState("is acquiring files and resources");
			waitNicely();
			if (isAnyFileNewerThanJob())
			{
				printState("is older than files and will be started again");
				criticalSectionLock.unlock();
				sync.ciCv.notify_all();
				waitNicely();
				continue;
			}
			lockCores();
			lockFiles();
			printState("has acquired files and resources and is executing");

			criticalSectionLock.unlock();
			sync.ciCv.notify_all();

			waitNicely();

			{
				std::lock_guard<std::mutex> resourceLock(sync.ciMutex);
				printState("has finished executing; releasing resources");
				waitNicely();
				unlockFiles();
				unlockCores();
			}
			sync.ciCv.notify_all();

			printState("is waiting to be dispatched again");
			waitNicely();
		}
		printState("END OF WORK");
	}

	bool isAnyFileNewerThanJob();
	void printDate();

	void printState(std::string state) override
	{
		std::scoped_lock<std::mutex> printLock(sync.printMutex);
		auto time = std::chrono::system_clock::to_time_t(lastExecuted);
		char formattedTime[20] = {0};
		std::strftime(formattedTime, sizeof(formattedTime),
					"%Y-%m-%d %H:%M:%S", std::localtime(&time));

		move(id, 0);
		clrtoeol();
		mvprintw(id, 0, "%d. %s", id, name.c_str());
		mvprintw(id, 30, "last run: %s", formattedTime);
		mvprintw(id, 60, "%s", state.c_str());
		refresh();
	}

	std::chrono::system_clock::time_point lastExecuted { std::chrono::system_clock::now() };
	std::thread workingThread;

	StandardJob() = delete;
	StandardJob(int id,
				std::string name,
				int required_resources_num,
				std::vector<std::reference_wrapper<File>> required_files,
				CIPlatform& ci_platform) :
						IJob(id, name, required_resources_num, required_files, ci_platform),
						workingThread(&StandardJob::execute, this)
				{
				}

	~StandardJob() {workingThread.join();}
};

struct Commit : public IJob
{
	void execute() override
	{
		printState("is idle");
		while(sync.run)
		{
			printState("is waiting to be queued");
			{
				std::lock_guard<std::mutex> jobListLock(sync.ciMutex);
				sync.jobDispatchList.push_back(id);
				sync.printJobList();
			}

			std::unique_lock<std::mutex> criticalSectionLock(sync.ciMutex);
			printState("is waiting for files and resources");
			sync.ciCv.wait(criticalSectionLock, [&]
			{
				return (sync.jobDispatchList.front() == id) &&
						(sync.cpu.numOfFreeCores >= numOfRequiredCores) &&
						(areFilesFree());
			});
			sync.jobDispatchList.pop_front();
			sync.printJobList();
			printState("is acquiring files and resources");
			waitNicely();
			lockCores();
			lockFiles();
			printState("has acquired files and resources and is executing");

			criticalSectionLock.unlock();
			sync.ciCv.notify_all();

			waitNicely();

			{
				std::lock_guard<std::mutex> resourceLock(sync.ciMutex);
				updateFiles();
				printState("has finished executing; releasing resources");
				waitNicely();
				unlockFiles();
				unlockCores();
			}
			sync.ciCv.notify_all();

			printState("is waiting to be dispatched again");
			waitNicely(20);
		}
		printState("END OF WORK");
	}

	void updateFiles();

	void printState(std::string state) override
	{
		std::scoped_lock<std::mutex> printLock(sync.printMutex);
		move(id, 0);
		clrtoeol();
		mvprintw(id, 0, "%d. %s", id, name.c_str());
		mvprintw(id, 30, "%s", state.c_str());
		refresh();
	}

	std::thread workingThread;
	Commit() = delete;
	Commit(int id,
				std::string name,
				int required_resources_num,
				std::vector<std::reference_wrapper<File>> required_files,
				CIPlatform& ci_platform) :
						IJob(id, name, required_resources_num, required_files, ci_platform),
						workingThread(&Commit::execute, this)
				{
				}

	~Commit() {workingThread.join();}
};

struct StaticAnalysisJob : public IJob
{
	void execute() override
	{
		printState("is idle");
		while(sync.run)
		{
			std::unique_lock<std::mutex> criticalSectionLock(sync.ciMutex);
			printState("is waiting to be dispatched");
			sync.ciCv.wait(criticalSectionLock, [&]
			{
				return (sync.jobDispatchList.front() == id) &&
						(sync.cpu.numOfFreeCores >= numOfRequiredCores) &&
						(areFilesFree());
			});
			sync.jobDispatchList.pop_front();
			{
				std::lock_guard<std::mutex> saCounterMutex(sync.saEndMutex);
				if (sync.saEndCounter == NUM_JOBS - 1)
					return;
			}
			sync.printJobList();
			printState("is acquiring files and resources");
			waitNicely();
			lockCores();
			lockFiles();
			printState("has acquired files and resources and is executing");

			criticalSectionLock.unlock();
			sync.ciCv.notify_all();

			waitNicely();

			{
				std::lock_guard<std::mutex> resourceLock(sync.ciMutex);
				updateFiles();
				printState("has finished executing; releasing resources");
				waitNicely();
				unlockFiles();
				unlockCores();
			}
			sync.ciCv.notify_all();
		}
		printState("END OF WORK");
	}

	void updateFiles();

	void printState(std::string state) override
	{
		std::scoped_lock<std::mutex> printLock(sync.printMutex);
		move(id, 0);
		clrtoeol();
		mvprintw(id, 0, "%d. %s", id, name.c_str());
		mvprintw(id, 30, "%s", state.c_str());
		refresh();
	}

	std::thread workingThread;
	StaticAnalysisJob() = delete;
	StaticAnalysisJob(int id,
				std::string name,
				int required_resources_num,
				std::vector<std::reference_wrapper<File>> required_files,
				CIPlatform& ci_platform) :
						IJob(id, name, required_resources_num, required_files, ci_platform),
						workingThread(&StaticAnalysisJob::execute, this)
				{
				}

	~StaticAnalysisJob() {workingThread.join();}
};
