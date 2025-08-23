// util/process.hpp -- Ugly Windows-specific code for determining if more than one copy of the binary is running at once.
// I think I found this on Stack Overflow or something and modified it a bit. Here be monsters.
// Raine "Gravecat" Simmons, 2019, 2023, 2025.

#pragma once
#ifdef GORP_TARGET_WINDOWS

#include <string>
#include <windows.h>


namespace gorp
{

class Process
{
public:
    static WINBOOL CALLBACK btf_callback(HWND window_handle, LPARAM pid);   // Used internally with bring_to_foreground()
    static void check_if_already_running(); // Checks if gorp.exe is already running; if so, brings that window to the foreground then exits quietly.

private:
    // DO NOT ADJUST THESE VALUES. Just leave them alone.
    static constexpr int    WINX_FP_MAX_PROCESSES = 1024;
    static constexpr int    WINX_P2N_BUFFER_SIZE =  1024;

    static void         bring_to_foreground(DWORD pid);         // Brings the gorp.exe window to the foreground if possible.
    static DWORD        find_process_id(std::string filename);  // Finds a given process ID, given a specified executable filename.
    static std::string  pid_to_name(DWORD pid);                 // Takes a Windows process ID and returns the name of the process.
};

}       // namespace gorp
#endif  // GORP_TARGET_WINDOWS
