#include "sync.hpp"
#include "jobs.hpp"
#include <thread>
#include <iostream>
#include <ncurses.h>
#include <tuple>
#include <ctime>
#include <chrono>

void startVisualisation()
{
	std::this_thread::sleep_for( std::chrono::milliseconds(50) );

	CIPlatform ci;

	ci.printFiles();
	ci.printCores();
	ci.printJobList();

	std::array<IJob*, NUM_JOBS> jobs
	{
		new StandardJob(0, "Event System Build", 5, {ci.files[3], ci.files[4], ci.files[5], ci.files[6], ci.files[7], ci.files[8]}, ci),
		new StandardJob(1, "Externals Build", 3, {ci.files[12], ci.files[13], ci.files[14]}, ci),
		new StandardJob(2, "Full Build", 9, {ci.files[0], ci.files[1], ci.files[3], ci.files[5], ci.files[7], ci.files[11]}, ci),
		new StandardJob(3, "Unit Tests", 6, {ci.files[0], ci.files[1], ci.files[2], ci.files[9], ci.files[10]}, ci),
		new StandardJob(4, "Externals Adaptation Tests", 4, {ci.files[1], ci.files[9], ci.files[14]}, ci),
		new Commit(5, "Git Commit Puller", 3, {ci.files[0], ci.files[1], ci.files[2], ci.files[3], ci.files[4], ci.files[5]}, ci),
		new Commit(6, "Git Commit Puller", 3, {ci.files[6], ci.files[7], ci.files[8], ci.files[9], ci.files[10], ci.files[11]}, ci),
		new Commit(7, "Svn Commit Puller", 3, {ci.files[12], ci.files[13], ci.files[14]}, ci),
		new StaticAnalysisJob(8, "Static Analysis", CPU_CORES,
			std::vector<std::reference_wrapper<File>>{ci.files.begin(), ci.files.end()},
			ci)
	};

	std::cin.get();
	ci.run = false;

	for (auto job : jobs)
		delete job;
}

void CIPlatform::printCores()
{
	for (int i = 0; i < cpu.cores.size(); ++i)
	{
		std::lock_guard<std::mutex> printLock(printMutex);
		move(NUM_JOBS + NUM_FILES + 2 + i, 0);
		clrtoeol();
		mvprintw(NUM_JOBS + NUM_FILES + 2 + i, 0, "Core %d", i);
		mvprintw(NUM_JOBS + NUM_FILES + 2 + i, 8, "is %s%s",
						cpu.cores[i].isFree ? "free" : "busy with job ",
						cpu.cores[i].isFree ? "" : std::to_string(cpu.cores[i].ownerId).c_str());
	}
}

void CIPlatform::printFiles()
{
	for (auto [i, it] = std::tuple{0, files.begin()}; it != files.end(); ++i, ++it)
	{
		std::lock_guard<std::mutex> printLock(printMutex);
		auto time = std::chrono::system_clock::to_time_t(it->lastChanged);
		char formattedTime[20] = {0};
		std::strftime(formattedTime, sizeof(formattedTime),
						"%Y-%m-%d %H:%M:%S", std::localtime(&time));

		move(NUM_JOBS + 1 + i, 0);
		clrtoeol();
		mvprintw(NUM_JOBS + 1 + i, 0, "%s", it->name.c_str());
		mvprintw(NUM_JOBS + 1 + i, 20, "modified: %d times, last: %s", it->numOfChanges, formattedTime);
		mvprintw(NUM_JOBS + 1 + i, 65, "is %s",
						it->isFree ? "released" : ("held by job " + std::to_string(it->ownerId)).c_str());
	}

}

void CIPlatform::printJobList()
{
	std::lock_guard<std::mutex> printLock(printMutex);
	move(NUM_JOBS + NUM_FILES + CPU_CORES + 3, 0);
	clrtoeol();
	mvprintw(NUM_JOBS + NUM_FILES + CPU_CORES + 3, 0, "Task queue: ");
	for (auto [i, it] = std::tuple{0, jobDispatchList.begin()}; it != jobDispatchList.end(); i+=2, ++it)
		mvprintw(NUM_JOBS + NUM_FILES + CPU_CORES + 3, 12 + i, "%d", *it);
	refresh();
}
