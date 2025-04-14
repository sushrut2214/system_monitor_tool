/** @file stats_functions.c
 *  @brief A report different metrics of the utilization of a system
 *
 *  This file imcludes the functions that report basic system information,
 *  including runtime memory, system's memory usage, connected users, CPU
 *  usage and OS information.
 *
 *  @author Huang Xinzi
 */

#include "stats_functions.h"

/** @brief Display error message and then terminate the program.
 *  @return Void.
 */
void handle_error(char *message) {
    fprintf(stderr, "%s\n", message);
    exit(0);
}

/** @brief Print the memory usage of the current process in C (in kilobytes).
 *  @return Void.
 */
void show_runtime_info() {
    struct rusage r_usage; // A variable to store resource usage section
    if (getrusage(RUSAGE_SELF,&r_usage) < 0) { // Get the resource usage statistics
        perror("rusage"); // If fail to get, report the error
        exit(1);
    } else {
        // Print the maximum resident set size used (in kilobytes).
        printf(" Memory usage: %ld kilobytes\n", r_usage.ru_maxrss);
        printf("---------------------------------------\n");
    }
}

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
void show_memory_graph(double curr_use, double previous_use) {
    // Get the difference between current-used and previous-used phisical memory.
    double diff = curr_use - previous_use;

    printf("  |"); // A sign indicating the start of our graph

    if ((diff < 0.01 && diff >= 0.0) || previous_use == -1) {
        // If the difference is a positive infinitesimal or it's the first iteration
        printf("o 0.00 (%.2f)\n", curr_use);
    } else if (diff <= 0.0 && diff > -0.01) {
        // If the difference is a negative infinitesimal
        printf("@ 0.00 (%.2f)\n", curr_use);
    } else if (diff >= 0.01) {
        // If the difference is positive, print '#' in proportion
        for (int i = 0; i < diff * 100; i ++) {
            printf("#");
        }
        // Print an asterisk to indicate the graph is end
        printf("* %.2f (%.2f)\n", diff, curr_use);
    } else {
        // If the difference is positive, print ':' in proportion
        for (int i = 0; i > diff * 100; i --) {
            printf(":");
        }
        // Print '@' to indicate the graph is end
        printf("@ %.2f (%.2f)\n", diff, curr_use);
    }
}

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
void get_memory_info(double previous_use, int graph_flag) {
    FILE *meminfo;   // A file pointer pointing to "/proc/meminfo"
    char line[200];  // A string storing each line of the file
    long totalram, freeram, bufferram, cachedram, totalswap, freeswap, sreclaimable;
    long phys_used, total_phys, virtual_used, total_virtual;

    // If cannot open the file, report an error
    if((meminfo = fopen("/proc/meminfo", "r")) == NULL) {
        perror("fopen");
        exit(1);
    }

    // Loop the file util read all the information we need
    while(fgets(line, sizeof(line), meminfo)) {
        if(sscanf(line, "MemTotal: %ld kB", &totalram) == 1) {
            totalram *= 1024;       // Convert into kilobytes
            total_phys = totalram;  // Total Physical Memory = MemTotal
        } else if (sscanf(line, "MemFree: %ld kB", &freeram) == 1){
            freeram *= 1024;
        } else if (sscanf(line, "Buffers: %ld kB", &bufferram) == 1) {
            bufferram *= 1024;
        } else if (sscanf(line, "Cached: %ld kB", &cachedram) == 1) {
            cachedram *= 1024;
        } else if (sscanf(line, "SwapTotal: %ld kB", &totalswap) == 1) {
            totalswap *= 1024;
            total_virtual = totalram + totalswap;  // Total Virtual Memory = MemTotal + SwapTotal
        } else if (sscanf(line, "SwapFree: %ld kB", &freeswap) == 1) {
            freeswap *= 1024;
        } else if (sscanf(line, "SReclaimable: %ld kB", &sreclaimable) == 1) {
            sreclaimable *= 1024;
            cachedram += sreclaimable;  // Cached memory = Cached + SReclaimable
            // Used Physical Memory = MemTotal - MemFree - (Buffers + Cached Memory)
            phys_used = (totalram - freeram) - (bufferram + cachedram);
            // Used Virtual Memory = MemTotal + SwapTotal - SwapFree - MemFree - (Buffers + Cached Memory)
            virtual_used = phys_used + totalswap - freeswap;

            // After reading the last argument, close the file.
            // (Assume the informtion in "/proc/meminfo" is in default order)
            // If fails, report an error.
            if (fclose(meminfo) != 0) {
                perror("fclose");
                exit(1);
            }
        }
    }

    // write to the standrad out (use dup2 to redirect to the pipe writing end)
    printf("%.2f GB / %.2f GB  -- %.2f GB / %.2f GB", phys_used * 1e-9,
        total_phys * 1e-9, virtual_used * 1e-9, total_virtual * 1e-9);

    // If the "--graphics" argument is used during compiling,
    // virtualize the physical memory usage difference.
    if (graph_flag == 1) {
        show_memory_graph(phys_used * 1e-9, previous_use);
    } else {
        printf("\n");
    }
}

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
void calculate_cpu_use(int tdelay) {
    FILE * stat; // A file pointer pointing to "/proc/stat"

    // If fail to open the file, report an error
    if((stat = fopen("/proc/stat", "r")) == NULL) {
        perror("fopen");
        exit(1);
    }

    // Variables storing the CPU information read from "/proc/stat"
    int user_prev, nice_prev, system_prev, idle_prev, iowait_prev, irq_prev, softirq_prev;
    int user_cur, nice_cur, system_cur, idle_cur, iowait_cur, irq_cur, softirq_cur;
    
    // Read the first line of the file "/proc/stat"
    fscanf(stat, "cpu %d %d %d %d %d %d %d", &user_prev, &nice_prev, &system_prev,
        &idle_prev, &iowait_prev, &irq_prev, &softirq_prev);

    // Wait for a period of time
    sleep(tdelay);

    // Read the file again, store the new CPU info in xxx_cur variables
    rewind(stat);
    fscanf(stat, "cpu %d %d %d %d %d %d %d", &user_cur, &nice_cur, &system_cur,
        &idle_cur, &iowait_cur, &irq_cur, &softirq_cur);

    // Close the file. If fails, report an error.
    if (fclose(stat) != 0) {
        perror("fclose");
        exit(1);
    }

    // idle = idle + iowait
    int idle_previous = idle_prev + iowait_prev;
    int idle_current = idle_cur + iowait_cur;

    // use = user + nice + system + irq + softirq
    int use_prev = user_prev + nice_prev + system_prev + irq_prev + softirq_prev;
    int use_cur = user_cur + nice_cur + system_cur + irq_cur + softirq_cur;

    // use_diff = use_curr - use_prev
    int numerator = use_cur - use_prev;
    // total = user + nice + system + idle + iowait + irq + softirq = use + idle
    // total_diff = total_curr - total_prev = use_diff + idle_diff
    int denominator = numerator + idle_current - idle_previous;

    // CPU (%) = (use_diff / total_diff) * 100
    double cpu_use = 100 * (double)numerator / (double)denominator;

    // write cpu usage to the stdout (use dup2 to redirect to pipe writing end)
    if (write(fileno(stdout), &cpu_use, sizeof(double)) == -1) {
        perror("write");
        exit(1);
    }
}

/** @brief Virtualize the CPU usage (in percentage).
 *
 *  Use '|' to denote the positive percentage increase.
 *  The number of '|' is in proportion to the CPU usage percentage.
 *
 *  @param percent A double representing CPU usage at current stage.
 *  @return Void.
 */
void show_cpu_graph(double percent) {
    printf("\t");
    // Print '|' in proportion to the CPU usage percentage
    for (int i = 0; i < (int) (percent * 2); i ++) {
        printf("|");
    }
    // Print the CPU usage percentage at the end of the graph
    printf(" %.2f\n", percent);
}

/** @brief Prints the number of CPU cores and CPU usage percentage.
 *  @return Void.
 */
void show_cpu_info(double cpu_use) {
    // Get the number of processors which are currently online.
    int cores = (int) sysconf(_SC_NPROCESSORS_ONLN);

    if (cores < 0) {  // If on error (return -1), report the error.
        perror("sysconf");
        exit(1);
    } else {          // Otherwise print the information.
        printf("Number of cores: %d\n", cores);
    }
    
    // display cpu usage
    printf(" total cpu use = %.2f%%\n", cpu_use);
}

/** @brief Prints User Usage information.
 *
 *  Use getutent() function from <utmp.h> library to get the user usage.
 *  Print user's name, type of the terminal device, and their remote IP
 *  address iff the username exists.
 *
 *  @return Void.
 */
void show_session_user() {
    struct utmp *users; // a variable to store user information
    setutent();         // rewinds the file pointer to the beginning of the utmp file
    users = getutent(); // get the information about who is currently using the system

    printf("### Sessions/users ###\n");

    // while we still have users, print their information
    while(users != NULL) {
        // print user information if username exists
        if (users -> ut_type == USER_PROCESS) {
        	printf(" %s\t%s (%s)\n", users -> ut_user,
                users -> ut_line, users -> ut_host);
        }
        users = getutent();  // get the next user information
    }
    endutent();        // closes the utmp file
    printf("---------------------------------------\n");
}

/** @brief Prints system information.
 *
 *  Use uname from <sys/utsname.h> library to get the system information.
 *  Print the OS name, release information, architecture, and version of OS.
 *
 *  @return Void.
 */
void show_sys_info() {
    struct utsname uts;       // A variable to store system information

    if (uname(&uts) < 0) {    // Get the system information
        perror("Get Uname");  // If fails, report an error
        exit(1);
    }

    printf("### System Information ###\n");
    printf(" System Name = %s\n", uts.sysname);
    printf(" Machine Name = %s\n", uts.nodename);
    printf(" Version = %s\n", uts.version);
    printf(" Release = %s\n", uts.release);
    printf(" Architecture = %s\n", uts.machine);
    printf("---------------------------------------\n");
}