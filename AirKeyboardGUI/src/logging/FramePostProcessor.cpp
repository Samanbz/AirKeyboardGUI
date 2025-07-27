#include "FramePostProcessor.h"

FramePostProcessor::FramePostProcessor(const std::string& dir)
    : watch_dir(dir) {
    ZeroMemory(&process_info, sizeof(process_info));
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESHOWWINDOW;
    startup_info.wShowWindow = SW_MINIMIZE;
}

bool FramePostProcessor::SpawnWorker() {
    std::string command = "python frame_postprocessor.py \"" + watch_dir + "\" --workers 8";

    OutputDebugStringA(("Running command: " + command + "\n").c_str());

    std::vector<char> cmd_line(command.begin(), command.end());
    cmd_line.push_back('\0');

    BOOL result = CreateProcessA(
        nullptr,          // No module name (use command line)
        cmd_line.data(),  // Command line
        nullptr,          // Process handle not inheritable
        nullptr,          // Thread handle not inheritable
        FALSE,            // Set handle inheritance to FALSE
        0,                // No creation flags
        nullptr,          // Use parent's environment block
        nullptr,          // Use parent's starting directory
        &startup_info,    // Pointer to STARTUPINFO structure
        &process_info     // Pointer to PROCESS_INFORMATION structure
    );

    if (result) {
        process_active = true;
        std::string debug_message = "Python file processor spawned with PID: " + std::to_string(process_info.dwProcessId) +
                                    " watching: " + watch_dir + "\n";
        OutputDebugStringA(debug_message.c_str());
        return true;
    } else {
        DWORD error = GetLastError();
        std::string debug_message = "Failed to spawn Python file processor. Error: " + std::to_string(error) + "\n";
        OutputDebugStringA(debug_message.c_str());
        return false;
    }
}

void FramePostProcessor::terminateWorker() {
    if (process_active && process_info.hProcess) {
        // Create a shutdown signal file
        std::ofstream shutdown_signal(watch_dir + "/.shutdown");
        shutdown_signal.close();

        OutputDebugStringA("Signaled Python worker to shutdown...\n");

        // Wait for process to exit gracefully (up to 30 seconds)
        DWORD wait_result = WaitForSingleObject(process_info.hProcess, 30000);

        if (wait_result == WAIT_TIMEOUT) {
            OutputDebugStringA("Python worker didn't exit gracefully, forcing termination.\n");
            TerminateProcess(process_info.hProcess, 0);
        } else {
            OutputDebugStringA("Python worker exited gracefully.\n");
        }

        CloseHandle(process_info.hProcess);
        CloseHandle(process_info.hThread);
        process_active = false;
    }
}

bool FramePostProcessor::isRunning() {
    if (!process_active || !process_info.hProcess) {
        return false;
    }

    DWORD exit_code;
    if (GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        return exit_code == STILL_ACTIVE;
    }
    return false;
}

FramePostProcessor::~FramePostProcessor() {
    terminateWorker();
}