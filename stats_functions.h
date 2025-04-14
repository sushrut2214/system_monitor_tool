/*
 * Header file for 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <utmp.h>
#include <unistd.h>

#ifndef __Stats_header
#define __Stats_header

void handle_error(char *message);

/** @brief Print the memory usage of the current process in C (in kilobytes).
 *  @return Void.
 */
void show_runtime_info();

/** @brief Virtualize the physical memory usage difference.
 *
 *  ::::::@ denoted that the total relative change is negative.
 *  ######* denoted that the total relative change is positive.
 *  |o denoted the total relative change is positive infinitesimal
 *  |@ denoted the total relative change is negative infinitesimal
 *
 *  @param curr_use Current-used physical memory.
 *  @param previous_use Previous-used phisical memory.
 *  @return Void.
 */
void show_memory_graph(double curr_use, double previous_use);

/** @brief Prints the physical and virsual memory usage compared to total (in gigabytes).
 *
 *  Read memory information from the Linux file "/proc/meminfo",
 *  and calculate the memory usage based on following equations:
 *      Total Physical Memory = MemTotal
 *      Used Physical Memory = MemTotal - MemFree - (Buffers + Cached Memory)
 *      Total Virtual Memory = MemTotal + SwapTotal
 *      Used Virtual Memory = Total Virtual Memory - SwapFree - MemFree - (Buffers + Cached Memory)
 *  where,
 *      Cached memory = Cached + SReclaimable
 *  If the "--graphics" argument is used during compiling, then
 *  print the phisical memory usage difference between the current
 *  and previous stage.
 *
 *  @param previous_use The physical memory used from the previous iteration.
 *  @param graph_flag An interger indicating whether "--graphics" argument is used during compiling.
 *  @return Void.
 */
void get_memory_info(double previous_use, int graph_flag);

/** @brief Calculate CPU usage (in percentage) in real-time.
 * 
 *  Read the CPU information from Linux file "/proc/stat", and
 *  calculate the CPU usage percentage based on following equations:
 *      CPU (%) = (use_diff / total_diff) * 100
 *      use_diff = use_curr - use_prev
 *      total_diff = total_curr - total_prev
 *  where,
 *      use = user + nice + system + irq + softirq
 *      total = user + nice + system + idle + iowait + irq + softirq
 *
 *  @param tdelay The period of time system between each time reading CPU info.
 *  @return Void.
 */
void calculate_cpu_use(int tdelay);

/** @brief Virtualize the CPU usage (in percentage).
 *
 *  Use '|' to denote the positive percentage increase.
 *  The number of '|' is in proportion to the CPU usage percentage.
 *
 *  @param percent A double representing CPU usage at current stage.
 *  @return Void.
 */
void show_cpu_graph(double percent);

/** @brief Prints the number of CPU cores and CPU usage percentage.
 *  @return Void.
 */
void show_cpu_info(double cpu_use);

/** @brief Prints User Usage information.
 *
 *  Use getutent() function from <utmp.h> library to get the user usage.
 *  Print user's name, type of the terminal device, and their remote IP
 *  address iff the username exists.
 *
 *  @return Void.
 */
void show_session_user();

/** @brief Prints system information.
 *
 *  Use uname from <sys/utsname.h> library to get the system information.
 *  Print the OS name, release information, architecture, and version of OS.
 *
 *  @return Void.
 */
void show_sys_info();

#endif