#include "jobs.hpp"

static constexpr int FILE_MODIFICATION_THRESHOLD = 5;

int IJob::getRandNumber()
{
	return randNumber(randEngine);
}

bool IJob::areFilesFree()
{
	for (const File& file : files)
		if (!file.isFree)
			return false;

	return true;
}

void IJob::lockCores()
{
	for (Core& core : sync.cpu.cores)
	{
		if (core.isFree)
		{
			core.isFree = false;
			core.ownerId = id;
			heldCores.push_back(core);
		}
		if (heldCores.size() == numOfRequiredCores)
		{
			sync.printCores();
			return;
		}
	}
}

void IJob::unlockCores()
{
	for (Core& core : heldCores)
	{
		core.isFree = true;
		core.ownerId = -1;
	}

	sync.printCores();
	heldCores.clear();
}

void IJob::lockFiles()
{
	for (File& file : files)
	{
		file.isFree = false;
		file.ownerId = id;
	}
	sync.printFiles();
}

void IJob::unlockFiles()
{
	for (File& file : files)
	{
		file.isFree = true;
		file.ownerId = -1;
	}
	sync.printFiles();
}

void IJob::waitNicely(int multiplier)
{
	auto wait_time = getRandNumber() * multiplier;
	auto start_time = std::chrono::high_resolution_clock::now();
	{
		std::lock_guard<std::mutex> print_lock(sync.printMutex);
		move(id, 110);
		clrtoeol();
	}
	while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count() < wait_time)
	{
		{
			std::lock_guard<std::mutex> print_lock(sync.printMutex);
			mvprintw(id, 110, "%3d%%", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count() / (wait_time * 10));
			refresh();
		}
		std::this_thread::sleep_for( 100ms );
	}
	{
		std::lock_guard<std::mutex> print_lock(sync.printMutex);
		move(id, 110);
		clrtoeol();
	}
}


bool StandardJob::isAnyFileNewerThanJob()
{
	for (const File& file : files)
		if (file.lastChanged > lastExecuted)
			return true;

	return false;
}

void StandardJob::printDate()
{
	std::scoped_lock<std::mutex> printLock(sync.printMutex);
	auto time = std::chrono::system_clock::to_time_t(lastExecuted);
	char formattedTime[20] = {0};
	std::strftime(formattedTime, sizeof(formattedTime),
				"%Y-%m-%d %H:%M:%S", std::localtime(&time));
	mvprintw(id, 30, "last run: %s", formattedTime);
}


void Commit::updateFiles()
{
	bool is_sa_needed{ false };
	for (File& file : files)
	{
		file.lastChanged = std::chrono::system_clock::now();
		file.numOfChanges++;
		if ((file.numOfChanges >= 5) &&
			(!sync.isStaticAnalysisDispatched))
		{
			sync.isStaticAnalysisDispatched = true;
			sync.jobDispatchList.push_front(NUM_JOBS - 1);
			sync.printJobList();
		}
	}
	sync.printFiles();
}


void StaticAnalysisJob::updateFiles()
{
	for (File& file : files)
		file.numOfChanges = 0;
	sync.printFiles();
	sync.isStaticAnalysisDispatched = false;
}
