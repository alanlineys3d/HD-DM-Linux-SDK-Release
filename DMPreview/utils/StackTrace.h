#ifndef STACKTRACE_H
#define STACKTRACE_H

#include <execinfo.h> // for backtrace
#include <stdio.h>    // for fprintf, popen, pclose, fgets
#include <stdlib.h>   // for free
#include <string.h>   // for strtok_r, strcmp
#include <unistd.h>   // for readlink
#include <limits.h>   // for PATH_MAX

// Function to print a stack trace, resolving addresses to file:line using addr2line
// Note: Requires the executable to be compiled with debug symbols (-g)
//       and addr2line must be installed on the system.
static inline void PrintStackTraceAddr2Line(int signal = -1) {
    void *callstack[128];
    int frames = backtrace(callstack, sizeof(callstack) / sizeof(void *));
    char **strs = backtrace_symbols(callstack, frames);

    if (strs == NULL) {
        fprintf(stderr, "  [Stack Trace] Error: Could not get backtrace symbols.\n");
        return;
    }

    // Attempt to get the executable path using /proc/self/exe
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0'; // Null-terminate the path
    } else {
        // Fallback if /proc/self/exe fails (e.g., permissions, non-Linux)
        fprintf(stderr, "  [Stack Trace] Warning: Could not determine executable path via /proc/self/exe. Addr2line might fail.\n");
        strcpy(exe_path, "a.out"); // Default or use a known name if applicable
    }

    fprintf(stderr, "--- Stack Trace (%d frames) --- Signal: %d ---\n", frames, signal);
    for (int i = 0; i < frames; ++i) {
        fprintf(stderr, "  [%d] %s\n", i, strs[i]);

        // --- Parse OFFSET and call addr2line ---
        char *symbol_line = strs[i];
        char *offset_start = strchr(symbol_line, '+');
        char *offset_end = strchr(symbol_line, ')');

        // Check if we found '+' and ')' in the expected order
        if (offset_start != NULL && offset_end != NULL && offset_start < offset_end) {
            offset_start++; // Move past '+'
            char offset_str[32]; // Buffer for the offset hex string
            strncpy(offset_str, offset_start, offset_end - offset_start);
            offset_str[offset_end - offset_start] = '\0';

            // Construct addr2line command using the offset
            char cmd[PATH_MAX + 128]; // Command buffer
            // Use flags similar to the successful manual command (-Cfe)
            snprintf(cmd, sizeof(cmd), "addr2line -Cfe %s %s", exe_path, offset_str);

            // Execute addr2line via popen
            FILE *fp = popen(cmd, "r");
            if (fp != NULL) {
                char line[512]; // Buffer for addr2line output
                bool first_line = true;
                while (fgets(line, sizeof(line), fp) != NULL) {
                    // Indent addr2line output
                    fprintf(stderr, "    %s%s", first_line ? ">> " : "   ", line);
                    first_line = false;
                }
                pclose(fp);
            } else {
                 fprintf(stderr, "    >> Error executing addr2line for offset %s\n", offset_str);
            }
        } else {
            // Could not parse offset, maybe a system library frame without offset?
             // Attempt to parse absolute address as fallback for system libs?
            char *addr_start = strchr(symbol_line, '[');
            char *addr_end = strchr(symbol_line, ']');
             if (addr_start != NULL && addr_end != NULL && addr_start < addr_end) {
                 addr_start++; // Move past '['
                 char addr_str[32];
                 strncpy(addr_str, addr_start, addr_end - addr_start);
                 addr_str[addr_end - addr_start] = '\0';
                 fprintf(stderr, "    >> Could not parse offset, symbol line might be from shared library (address: %s).\n", addr_str);
            } else {
                 fprintf(stderr, "    >> Could not parse offset or address from symbol line.\n");
            }
        }
        // --- End addr2line call ---
    }
    fprintf(stderr, "--- End Stack Trace ---\n");

    free(strs); // Free the memory allocated by backtrace_symbols
}

#endif // STACKTRACE_H 