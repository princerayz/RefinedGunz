#include "stdafx.h"
#include "MProcess.h"

#ifdef _WIN32
#include "MWindows.h"
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

uintptr_t MProcess::Start(const char* File, const char * const * argv)
{
	const char* default_argv[] = {nullptr};
	if (argv == nullptr)
	{
		argv = default_argv;
	}

#ifdef WIN32

	char CommandLine[4096];
	auto End = CommandLine;
	auto Add = [&](const char* String) {
		End = strcpy_safe(End, std::end(CommandLine) - End, String);
	};
	Add(argv[0] ? argv[0] : File);
	for (auto p = argv + 1; *p; ++p)
	{
		Add(" ");
		Add(*p);
	}

	STARTUPINFO si{};
	PROCESS_INFORMATION pi{};

	si.cb = sizeof(si);

	auto CreateProcessResult = CreateProcessA(
		NULL,  // Application name
		CommandLine, // Command line
		NULL,  // Process attributes
		NULL,  // Thread attributes
		FALSE, // Inherit handles
		0,     // Creation flags
		NULL,  // Environment
		NULL,  // Current directory
		&si,   // Startup info
		&pi);  // Process info

	if (CreateProcessResult == FALSE)
	{
		return 0;
	}

	auto Handle = reinterpret_cast<uintptr_t>(pi.hProcess);
	return Handle;

#else

	auto pid = fork();
	if (pid == -1)
	{
		return 0;
	}
	else if (pid == 0)
	{
		execvp(File, const_cast<char* const*>(argv));
	}
	else
	{
		return static_cast<uintptr_t>(pid);
	}

	return 0;

#endif
}

MProcess::AwaitResult MProcess::Await(uintptr_t Handle, u32 Timeout)
{
	AwaitResult ret{};

#ifdef WIN32

	auto WinHandle = reinterpret_cast<HANDLE>(Handle);

	auto WaitResult = WaitForSingleObject(WinHandle, Timeout);
	if (WaitResult == INFINITE)
	{
		ret.TimedOut = true;
		ret.ExitCode = 0;
		return ret;
	}
	
	ret.TimedOut = false;

	DWORD ExitCode = 0;
	auto GotExitCode = GetExitCodeProcess(WinHandle, &ExitCode) != FALSE;
	ret.ExitCode = ExitCode;

	return ret;

#else

	auto pid = static_cast<pid_t>(Handle);
	int status = 0;

	auto GetExitCode = [&] { return WIFEXITED(status) ? WEXITSTATUS(status) : 0; };

	if (Timeout == Infinite)
	{
		auto waitret = waitpid(pid, &status, 0);
		if (waitret == -1 || waitret == 0)
		{
			ret.TimedOut = false;
			ret.ExitCode = 0;
			return ret;
		}

		ret.TimedOut = false;
		ret.ExitCode = GetExitCode();
		return ret;
	}
	else
	{
		auto GetTime = [&] { return std::chrono::high_resolution_clock::now(); };
		auto Start = GetTime();
		while (true)
		{
			auto waitret = waitpid(pid, &status, WNOHANG);
			if (waitret != -1 && waitret != 0)
			{
				ret.TimedOut = false;
				ret.ExitCode = GetExitCode();
				return ret;
			}

			if (GetTime() - Start >= std::chrono::milliseconds(Timeout))
			{
				ret.TimedOut = true;
				ret.ExitCode = 0;
				return ret;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

#endif
}

bool MProcess::Terminate(uintptr_t Handle)
{
#ifdef WIN32

	return ::TerminateProcess(reinterpret_cast<HANDLE>(Handle), 1) != FALSE;

#else

	return kill(static_cast<pid_t>(Handle), SIGTERM) == 0;

#endif
}
