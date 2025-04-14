/** @file mySystemStats.c
 *  @brief A report different metrics of the utilization of a system
 *
 *  This file can help user to get basic system information, including
 *  system's memory usage, users' usage, CPU usage and OS information.
 *  You can display the information in the way you like, refreshing N
 *  times or print out sequentially, showing system or user usage only,
 *  or with graphics that can virtualize the system usage change.
 *  Also you are free to decide how many times the statistics will be
 *  displayed and interval between each time the information prints.
 *  The newest version implemented concurrency, the program is running
 *  more efficiently. In addition, when user hits Ctrl-C, the program
 *  will ask the user if it really wants to quit or not. And the program
 *  will ignore the Ctrl-Z signal completely.
 *
 *  @author Huang Xinzi
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
#include <signal.h>

#include "stats_functions.h"

/** @brief Move the cursor up.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_up(int lines){
   printf("\033[%dF", lines);
}

/** @brief Move the cursor down.
 *  @param lines The number of lines the cursor moves.
 *  @return Void.
 */
void move_down(int lines){
   printf("\033[%dE", lines);
}

/** @brief A signal handling function to reset behaviour for SIGINT.
 * 
 *  When Ctrl-C is hitted, the program will ask the user whether it
 *  really wants to quit the program or not. If the user type 'y' or
 *  'Y', terminate the program. Otherwise continue the execution.
 * 
 *  @param sig An integer which identify the signal.
 *  @return Void.
 */
void ctrlc_handler(int sig) {
    // signal(sig, ctrlc_handler);  // first ignore the ctrl-c
    fprintf(stderr, "You hit Ctrl-C! Do you really want to quit? [y/n] ");
    char c;         // a char to store user input
    c = getchar();  // read user input (y or n)
    if (c == 'y' || c == 'Y') {
        exit(0);    // if user does what to quit, terminate
    } else {        // else reset the ctrl-c to this handler
        getchar();  // get a newline character

        // reset the ctrl-c to this handler function
        if (signal(sig, ctrlc_handler) == SIG_ERR) {
            perror("signal");
            exit(1);
        }
        move_up(1);         // move up one line
        printf("\033[2K");  // erase the line
    }
}

/** @brief A signal handling function to reset behaviour for SIGTSTP.
 * 
 *  When Ctrl-Z is hitted, the program will ignore the signal as the program
 *  should not be run in the background while running interactively.
 * 
 *  @param sig An integer which identify the signal.
 *  @return Void.
 */
void ctrlz_handler(int sig) {
    // reset the ctrl-z to this handler function
    if (signal(sig, ctrlz_handler) == SIG_ERR) {
        perror("signal");
        exit(1);
    }
    printf("\033[2D");      // erase "^Z"
}

/** @brief Reset the behaviours of the SIGINT and SIGTSTP signals in parent.
 *
 *  We want to ignore the Ctrl-Z as the program should not be run in the
 *  background while running interactively.
 *  And for Ctrl-C, the program will ask the user whether it really wants
 *  to quit the program or not.
 * 
 *  @return Void.
 */
void set_signals_parent() {
    if (signal(SIGINT, ctrlc_handler) == SIG_ERR ||    // reset ctrl-c
        signal(SIGTSTP, ctrlz_handler) == SIG_ERR) {   // reset ctrl-z
        perror("signal");
        exit(1);
    }
}

/** @brief Ignore the SIGINT and SIGTSTP signals in child.
 *
 *  Since we have reset the behaviours of SIGINT and SIGTSTP signal handlers
 *  in parent, the child will also perform these behaviours when receving the
 *  signals. However we don't want the signals to interupt the job of child,
 *  so we just ignore the signal in child.
 * 
 *  @return Void.
 */
void set_signals_child() {
    if (signal(SIGINT, SIG_IGN) == SIG_ERR ||   // ignore the SIGINT signal
        signal(SIGTSTP, SIG_IGN) == SIG_ERR) {  // ignore the SIGTSTP signal
        perror("signal");
        exit(1);
    }
}

/** @brief Fork a child to report memory utilization.
 *
 *  Child process is responsible for reporting the information and write
 *  to the parent. After child has done its work, terminate the child.
 *  If in parent, return control to the main loop (i.e. return the function).
 *  If graph flag is 1 (i.e. "--graphics" is been called), write graphics
 *  for memory to the pipe in child.
 * 
 *  @param fd An array of two integers that contains file descriptors for pipe.
 *  @param prev_used The physical memory used from the previous iteration.
 *  @param gragh An integer to indicate if "--graphics" is been called
 *  @return Void.
 */
void read_memory_info(int *fd, double prev_used, int graph) {
    if (pipe(fd) == -1) {           // set up pipe for memory utilization
        perror("pipe");
        exit(1);
    }

    int pid;                        // a variable to store child pid
    if ((pid = fork()) == -1) {     // create a child to report the usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd, such that
        // we can output the information in pipe rather than terminal
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }

        set_signals_child();        // set the signals in child
        get_memory_info(prev_used, graph);  // report memory usage
        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);                    // terminate the child
    } else {
        close(fd[STDOUT_FILENO]);   // close the writing end in parent
    }
}

/** @brief Fork a child to report CPU utilization.
 * 
 *  Child process is responsible for reporting the information and write
 *  to the parent. After child has done its work, terminate the child.
 *  If in parent, return control to the main loop (i.e. return the function).
 *  When reading the cpu usage, sleep tdelay seconds between opening the
 *  Linux file "/proc/stat".
 * 
 *  @param fd An array of two integers that contains file descriptors for pipe.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @return Void.
 */
void read_cpu_info(int *fd, int tdelay) {
    if (pipe(fd) == -1) {           // set up pipe for CPU utilization
        perror("pipe");
        exit(1);
    }

    int pid;                        // a variable to store child pid
    if ((pid = fork()) == -1) {     // create a child to report cpu usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }

        set_signals_child();        // set the signals in child
        calculate_cpu_use(tdelay);  // report the CPU usage (sleep for tdelay)
        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);                    // terminate the child
    } else {
        close(fd[STDOUT_FILENO]);   // close the writing end in parent
    }
}

/** @brief Fork a child to report connected user.
 *  
 *  Child process is responsible for reporting the information and write
 *  to the parent. After child has done its work, terminate the child.
 *  If in parent, return control to the main loop (i.e. return the function).
 * 
 *  @param fd An array of two integers that contains file descriptors for pipe.
 *  @return Void.
 */
void read_user_info(int *fd) {
    if (pipe(fd) == -1) {           // set up pipe for user connection
        perror("pipe");
        exit(1);
    }

    int pid;                        // a variable to store child pid
    if ((pid = fork()) == -1) {     // create a child to report the usage
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        close(fd[STDIN_FILENO]);    // close the reading end in child

        // change the standard out into our pipe writing fd
        if (dup2(fd[STDOUT_FILENO], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }

        set_signals_child();        // set the signals in child
        show_session_user();        // report user connection
        close(fd[STDOUT_FILENO]);   // close the writing end in child
        exit(0);                    // terminate the child
    } else {
        close(fd[STDOUT_FILENO]);   // close the writing end in parent
    }
}

/** @brief Prints System Usage sample times in every tdelay secs.
 *
 *  If graph flag is 1 (i.e. "--graphics" is been called), print graphics
 *  for memory and CPU usage.
 *  If sequential flag is 1 (i.e. "--sequantial" is been called), print the
 *  sample sequentially without refreshing the screen.
 *
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param sys An integer flag to indicate if "--system" is been called.
 *  @param user An integer flag to indicate if "--user" is been called.
 *  @param graph An integer flag to indicate if "--graphics" is been called.
 *  @param sequential An integer flag to indicate if "--sequential" is been called.
 *  @return Void.
 */
void show_sys_usage(int sample, int tdelay, int sys, int user, int graph, int sequential) {
    // set up three pipes, fd[0] for communication of memory use,
    // fd[1] for connected users, and fd[2] for CPU utilization
    int fd[3][2];

    double cpu_use, prev_used = -1;     // to store current memory / CPU usage
    int n = 0;                          // to store number of users connected
    char mem_info[512], user_info[512]; // to store the reported usage
    FILE *mem_file, *user_file;         // to get what child write

    // sampling sample times to get the update of system usage
    for (int i = 0; i < sample; i ++) {
        if (sys == 1) {
            // create child processes to report system usage
            read_memory_info(fd[0], prev_used, graph);
            read_cpu_info(fd[2], tdelay);

            // open a file to read what child write
            if ((mem_file = fdopen(fd[0][STDIN_FILENO], "r")) == NULL) {
                perror("fdopen");
                exit(1);
            }
        }

        if (user == 1) {
            read_user_info(fd[1]);  // create child processes to report user
            // open a file to read what child write
            if ((user_file = fdopen(fd[1][STDIN_FILENO], "r")) == NULL) {
                perror("fdopen");
                exit(1);
            }
        }

        if (sequential == 1) {
            printf(">>> iteration %d\n", i + 1);   // print iteration title
            show_runtime_info();    // show runtime info for each iteration
        } else if (i == 0) {
            show_runtime_info();    // show runtime info for first iteration
        }

        if (sys == 1 && (sequential == 1 || i == 0)) {
            // print the title for memory use section
            printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n");
            // print empty lines reserving space for memory usage
            for (int j = 0; j < i; j ++) {
                printf("\n");
            }
        }

        if (sys == 1) {
            // if sequential is not called, need to refresh the screen
            if (sequential == 0 && i != 0) {
                move_up(sample - i + 3);     // move up to the memory section
                if (user == 1) move_up(n);   // move through the user section
                if (graph == 1) move_up(i);  // move through the cpu graph
            }

            // Read from child 1 (get memory information)
            if (fgets(mem_info, 512, mem_file) == NULL) {
                perror("fgets");
                exit(1);
            } else {
                // print the information, and store current memory use
                printf("%s", mem_info);
                if (sscanf(mem_info, "%lf GB / ", &prev_used) == -1) {
                    perror("sscanf");
                    exit(1);
                }
            }
            // print empty lines reserving space for memory usage
            for (int j = 1; j < sample - i; j ++) {
                printf("\n");
            }
            if (fclose(mem_file) < 0) {  // close the according file
                perror("fclose");
                exit(1);
            }
            close(fd[0][STDIN_FILENO]);  // close the reading end in parent
            printf("---------------------------------------\n");
        }

        if (user == 1) {
            // if we only want to refresh user section, move up the cursor
            if (sys == 0 && sequential == 0 && i != 0) move_up(n);

            n = 0;    // stores number of user samples
            // Now read from child 2
            while (fgets(user_info, 512, user_file) != NULL) {
                printf("%s", user_info);
                n ++;
            }
            if (fclose(user_file) < 0) { // close the according file
                perror("fclose");
                exit(1);
            }
            close(fd[1][STDIN_FILENO]);  // close the reading end in parent
            if (sys == 0) sleep(tdelay); // if system is not called, sleep
        }

        if (sys == 1) {
            // Now read from child 3 (cpu usage)
            if (read(fd[2][STDIN_FILENO], &cpu_use, sizeof(double)) < 0) {
                perror("read");
                exit(1);
            } else {
                show_cpu_info(cpu_use); // print cpu information (core + cpu usage)

                if (sequential == 1 && graph == 1) {
                    show_cpu_graph(cpu_use);  // show cpu graph if applied
                } else if (graph == 1) {
                    if (i != 0) move_down(i); // move down to the right position
                    show_cpu_graph(cpu_use);  // show cpu graph if applied
                }
            }
            close(fd[2][STDIN_FILENO]); // close the reading end in parent
        }
    }
    if (sys == 1) {
        printf("---------------------------------------\n");
    }
    show_sys_info();
}

/** @brief Validate the command line arguments user gived.
 *
 *  Use flags to indicate whether an argument is been called.
 *  Set the flag variables to 1 if does, and 0 if not.
 *  If any argument call violates the assumptions (see README.txt part c
 *  "Assumptions made:"), report an error in standard error.
 *  Note that "--samples=N" and "--tdelay=T" can be considered as
 *  positional arguments if they are not flagged.
 *
 *  @param argc Number of ommand line arguments.
 *  @param argv The array of strings storing command line arguments.
 *  @param sample Number of times the statistics are going to be collected.
 *  @param tdelay Period of time the statistics refresh (in seconds).
 *  @param sys_flag Point to an integer to indicate if "--system" is been called
 *  @param user_flag Point to an integer to indicate if "--user" is been called
 *  @param sequential_flag Point to an integer to indicate if "--sequential" is been called
 *  @param graph_flag Point to an integer to indicate if "--graphics" is been called
 *  @param sample_flag Point to an integer to indicate if "--sample=N" is been called
 *  @param tdelay_flag Point to an integer to indicate if "--tdelay=T" is been called
 *  @return Void.
 */
void vertify_arg(int argc, char *argv[], int *sample, int *tdelay, int *sys_flag,
    int *user_flag, int *sequential_flag, int *graph_flag, int *sample_flag, int *tdelay_flag) {

    int tmp_sample;  // Store temporary sample size
    int tmp_tdelay;  // Store temporary tdelay secs
    int positional;  // Store temporary positional argument
    int positional_arg = 0;  // Store the number of positional arguments

    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "--system") == 0) {
            *sys_flag = 1;   // set the flag to 1
        } else if (strcmp(argv[i], "--user") == 0) {
            *user_flag = 1;  // set the flag to 1
        } else if (strcmp(argv[i], "--graphics") == 0) {
            *graph_flag = 1; // set the flag to 1
        } else if (strcmp(argv[i], "--sequential") == 0) {
            *sequential_flag = 1; // set the flag to 1
        } else if (sscanf(argv[i], "--samples=%d", &tmp_sample) == 1) {
            if (*sample_flag == 0) {
                // If this is the first "--samples=N" argument called,
                // set the sample value to the input sample value
                // and label sample_flag to 1.
                *sample = tmp_sample;
                *sample_flag = 1;
            } else if (*sample != tmp_sample) {
                // If this value does not corresponds to other sample size value,
                // print an error message.
                handle_error("The value given to \"--samples=N\" should be consistent!");
            }
            if (*sample <= 0) {
                // If the value is negative, print an error messgae
                handle_error("The value given to \"--samples=N\" should be an positive integer!");
            }
        } else if (sscanf(argv[i], "--tdelay=%d", &tmp_tdelay) == 1) {
            if (*tdelay_flag == 0) {
                // If this is the first "--tdelay=T" argument called,
                // set the tdelay value to the input tdelay value
                // and label tdelay_flag to 1.
                *tdelay = tmp_tdelay;
                *tdelay_flag = 1;
            } else if (*tdelay != tmp_tdelay) {
                // If this value does not corresponds to previous tdelay value,
                // print an error message.
                handle_error("The value given to \"--tdelay=T\" should be consistent!");
            }
            if (*tdelay < 0) {
                // If the value is negative, print an error messgae
                handle_error("The value given to \"--tdelay=T\" should be an positive integer");
            }
        } else if (sscanf(argv[i], "%d", &positional) == 1){
            // If the argument is a single integer
            if (positional_arg == 0) {
                // If this is the first integer appeared in the arguments,
                // this would be the sample size value.
                if (*sample_flag == 0 || *sample == positional) {
                    // If it corresponds to other sample size value (if exists),
                    // set sample value to positional and label sample_flag as 1
                    *sample = positional;
                    *sample_flag = 1;
                } else {
                    handle_error("The value given to \"--samples=N\" should be consistent!");
                }
                if (*sample <= 0) {
                    // If the value is negative, print an error messgae
                    handle_error("The value given to \"--samples=N\" should be an positive integer!");
                }
            } else if (positional_arg == 1) {
                // If this is the second integer appeared in the arguments,
                // this would be the tdelay value.
                if (*tdelay_flag == 0 || *tdelay == positional) {
                    // If it corresponds to previous tdelay value (if exists),
                    // set tdelay to positional and label tdelay_flag as 1
                    *tdelay = positional;
                    *tdelay_flag = 1;
                } else {
                    handle_error("The value given to \"--tdelay=T\" should be consistent!");
                }
                if (*sample <= 0) {
                    // If the value is negative, print an error messgae
                    handle_error("The value given to \"--tdelay=T\" should be an positive integer!");
                }
            } else {
                // If we already have 2 single integers as arguments,
                // then print an error message
                handle_error("No more than 2 single integers can be taken as valid arguments.");
            }
            positional_arg ++;
        } else {
            // If the statement is in other format, print an error message
            fprintf(stderr, "Invalid arguments: \"%s\"\n", argv[i]);
            exit(0);
        }
    }
}

/** @brief Report different metrics of the utilization of a given system.
 *
 *  @param argc Number of ommand line arguments.
 *  @param argv The array of strings storing command line arguments.
 *  @return An integer.
 */
int main (int argc, char *argv[]) {
    // initialize sample and tdelay to their defalut value
    int sample = 10, tdelay = 1;
    // create flags for each argument to check if they're called
    int sys = 0, user = 0, sequential = 0, graph = 0, sample_f = 0, tdelay_f = 0;

    // validate the arguments
    vertify_arg(argc, argv, &sample, &tdelay, &sys, &user, &sequential, &graph,
        &sample_f, &tdelay_f);

    // print the values of sample size and tdelay
    printf("Nbr of samples: %d -- every %d secs\n", sample, tdelay);

    // set defalut behaviour
    // if no "--system" or "--user" called, display the usage for both
    if (sys == 0 && user == 0) {
        sys = 1;
        user = 1;
    }

    // set signals for the parent
    set_signals_parent();

    // Display system (Memory / User / CPU) usage information
    show_sys_usage(sample, tdelay, sys, user, graph, sequential);
    
    return 0;
}