#include "pch.h"
#include "HeartbeatTimer.h"
#include "GlobalKill.h"
#include <winternl.h>
#include "../SessionLauncherIdentity.h"

HeartbeatTimer::HeartbeatTimer(std::weak_ptr<SharedMemoryInternal> shm, std::weak_ptr<SettingsStateAndEvents> set)
	: sharedMemWeak(shm), settingsWeak(set)
{
    const DWORD launcherPID = SessionLauncherIdentity::findProcessId();
    if (!launcherPID)
        throw HCMInitException(std::format("Could not uniquely find Session.exe launcher, error {}", GetLastError()));
    // Allows a non-elevated game to monitor an elevated launcher.
    HandlePtr h(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, TRUE, launcherPID));
    if (!h)
        throw HCMInitException(std::format("Could not open Session.exe with PROCESS_QUERY_LIMITED_INFORMATION, error {}", GetLastError()));


	HCMExternalHandle = std::move(h);

	_thd = std::thread([this]()
		{
			static int waitCount = 0;
			while (!GlobalKill::isKillSet())
			{
				waitCount = (waitCount + 1) % 100;
				if (waitCount != 0)
				{
					// process inject command queue
					try
					{
						lockOrThrow(settingsWeak, settings);
						lockOrThrow(sharedMemWeak, sharedMem);

						if (sharedMem->getAndClearInjectQueue())
						{
							PLOG_DEBUG << "Inject command recieved, firing event!";
							settings->injectCheckpointEvent->operator()();
						}
					}
					catch (HCMRuntimeException ex)
					{
						PLOG_ERROR << "Heartbeat timer unable to check inject command queue";
					}



					DWORD exitCode = 0;
					if (GetExitCodeProcess(HCMExternalHandle.get(), &exitCode) == FALSE)
					{
						PLOG_ERROR << "GetExitCodeProcess failed, error code: " << GetLastError();
						GlobalKill::killMe();
						return;
					}
					else
					{
						// https://youtu.be/APc8QCGOdUE
						if (exitCode != STILL_ACTIVE)
						{
							PLOG_INFO << "HCMExternal terminated with code: " << exitCode;
							GlobalKill::killMe();
							return;
						}
					}
				}




				auto nextWakeup = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
				std::this_thread::sleep_until(nextWakeup);
			}
		});
}

HeartbeatTimer::~HeartbeatTimer()
{
	PLOG_VERBOSE << "~HeartbeatTimer";
	_thd.join();
	PLOG_VERBOSE << "aye the thread died as it should";
}