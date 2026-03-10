
#include "crc.hpp"

namespace crc
{
    inline LONG enable_events = 0;
    inline LONG disable_events = 0;

	long __stdcall vectored_exception_handler(EXCEPTION_POINTERS* info) {

		if (info->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
		{
			if (info->ContextRecord->Dr1 == BASE_OFFSET(0x1F9CCE26))
			{
				hook::enable_all();
                const LONG count = InterlockedIncrement(&enable_events);
                if (count <= 8)
                {
                    debug::log("crc single-step enable_all hit #%ld", count);
                }

				info->ContextRecord->Dr1 = BASE_OFFSET(0x3000);

				return EXCEPTION_CONTINUE_EXECUTION;
			}

			else if (info->ContextRecord->Dr1 == BASE_OFFSET(0x3000))
			{
				hook::disable_all();
                const LONG count = InterlockedIncrement(&disable_events);
                if (count <= 8)
                {
                    debug::log("crc single-step disable_all hit #%ld", count);
                }

				info->ContextRecord->Dr1 = BASE_OFFSET(0x1F9CCE26);

				return EXCEPTION_CONTINUE_EXECUTION;
			}

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	__declspec(noinline) void crc_runtime()
	{
		THREADENTRY32 te32;
		HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

		if (hThreadSnap)
		{
			te32.dwSize = sizeof(THREADENTRY32);

			if (!Thread32First(hThreadSnap, &te32))
			{
				CloseHandle(hThreadSnap);
			}
			else {
				do
				{
					if (te32.th32OwnerProcessID == GetCurrentProcessId() && te32.th32ThreadID != GetCurrentThreadId())
					{
						HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, 0, te32.th32ThreadID);
						if (hThread)
						{
							CONTEXT context;
							context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
							SuspendThread(hThread);

							if (GetThreadContext(hThread, &context))
							{
								context.Dr0 = 0;
								context.Dr1 = BASE_OFFSET(0x3000);
								context.Dr2 = 0;
								context.Dr3 = 0;

								context.Dr7 = (1 << 0) | (1 << 20) | (1 << 21) | (1 << 2) | (1 << 4) | (1 << 6);

								SetThreadContext(hThread, &context);
							}

							ResumeThread(hThread);
							CloseHandle(hThread);
						}
					}
				} while (Thread32Next(hThreadSnap, &te32));
				CloseHandle(hThreadSnap);
			}
		}
	}

	__declspec(noinline) void runtime()
	{
		AddVectoredExceptionHandler(1, vectored_exception_handler);
        debug::log("vectored exception handler installed");

		crc_runtime();
        debug::log("crc runtime armed");
	}
}
