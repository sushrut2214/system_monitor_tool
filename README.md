# System-Monitoring-Tool
A C program reporting memory utilization, connected users and CPU utilization for your system.

@mainpage mySystemStats

@author Huang Xinzi

> This program can help user to get basic system information, including system's memory usage, users' usage, CPU usage and OS information. You can display the information in the way you like, refreshing N times or print out sequentially, showing system or user usage only, or with graphics that can virtualize the system usage change. Also you are free to decide how many times the statistics will be displayed and interval between each time the information prints. Since the newest version implemented concurrency, the program is running more efficiently. In addition, when user hits Ctrl-C, the program will ask the user if it really wants to quit or not. And the program will ignore the Ctrl-Z signal completely.
> 

## How did I solve the problem?

---

1. Divide each functionality into modules:
    1. Write functions to for reading program memory usage, system memory usage information (w/ graph), CPU information (w/ graph), connected users, and operation system information respectively.
    2. Set up pipes in parent and then fork a child which reports some system usage. The child will write the information it read to pipe, and the parent receives these information and print them to the screen.
    3. Set up signal handler when `ctrl-c` and `ctrl-z` are caught.
2. Then design detailed function for each module:
    1. For example, when designing section (II), I first write a function `get_memory_info` to read memory usage information from Linux file "`/proc/meminfo`", and return the calculated memory use for comparing purpose. Then I write a helper function `show_memory_graph` to virtualize the memory usage difference between 2 iterations.
    2. Similar strategy is used when designing section (III) CPU usage.
3. Group the functions which have similar purpose into a single function.
    1. When displaying the system usage (system memory usage, core information, and CPU usage), instead of calling every function in main, I use a single function `show_sys_usage`.
    2. When setting up the signal handler, use one function for setting the signals caught in parent, and another function for setting in children.
4. Add helper functions.
    1. For example, when there is an error, we need a function to show the user error message. So I use a function `handle_error` to display error message and then terminate the program.
    2. I also have functions `move_up` and `move_down` to move the cursor up and down on the screen. This is useful when "refreshing" the screen. We can easily find the correct place for different information.
    3. Since every time we read the system usage information, we need to read the information concurrently. Then I use three helper functions `read_memory_info`, `read_cpu_info`, and `read_user_info` to fork a child and handle each child’s job. Then we can return to the sampling loop in parent so that parent can continue its work.
    4. In addition, I use a function `vertify_arg` to validate user's input argument.
5. Seperate the main driver program and the functions implementation.

## An overview of the functions (including documentation)

---

1. Functions in `mySystemStats.c`
    
    ```c
    void move_up(int lines);
     	/* Move the cursor up to find the correct place printing information.
    		 Takes a positive integer to indicate how many lines to move. */
    
    void move_down(int lines);
     	/* Move the cursor down to find the correct place printing information.
    		 Takes a positive to indicate how many lines to move. */
    
    void ctrlc_handler(int sig);
    /* A signal handling function to reset behaviour for SIGINT. When Ctrl-C
       is hitted, the program will ask the user whether it really wants to quit
       the program or not. If the user type 'y' or 'Y', terminate the program.
       Otherwise continue the execution. */
    
    void ctrlz_handler(int sig);
    /* A signal handling function to reset behaviour for SIGTSTP. When Ctrl-Z
       is hitted, the program will ignore the signal as the program should not
       be run in the background while running interactively. */
    
    void set_signals_parent();
    /* Reset the behaviours of the SIGINT and SIGTSTP signals in parent.
    	 We want to ignore the Ctrl-Z and ask the user whether it really wants
       to quit the program if it hits Ctrl-C. */
    
    void set_signals_child();
    /* Ignore the SIGINT and SIGTSTP signals in child, since we don't want
    	 the signals to interupt the job of child. */
    
    void read_memory_info(int *fd, double prev_used, int graph);
    	/* Fork a child to report memory utilization and write the information 
    		 to the parent. After child has done its work, terminate the child.
    		 If in parent, return to the sampling loop.
    		 Takes an array of two integers that contains file descriptors of a
    		 pipe which connects the child and the parent.
    		 Takes the physical memory used from the previous iteration.
    		 Takes an integer flag to indicate if "--graphics" is been called. */
    
    void read_cpu_info(int *fd, int tdelay);
    	/* Fork a child to report CPU utilization and write the information 
    		 to the parent. After child has done its work, terminate the child.
    		 If in parent, return to the sampling loop.
    		 When reading the cpu usage, sleep tdelay seconds between opening
    		 the Linux file "/proc/stat".
    		 Takes an array of two integers that contains file descriptors of a
    		 pipe which connects the child and the parent.
    		 Takes an integer tdelay to indicate the frequency of refreshing. */
    
    void read_user_info(int *fd);
    	/* Fork a child to report connected users and write the information 
    		 to the parent. After child has done its work, terminate the child.
    		 If in parent, return to the sampling loop.
    		 Takes an array of two integers that contains file descriptors of a
    		 pipe which connects the child and the parent. */
    
    void show_sys_usage(int sample, int tdelay, int graph_flag);
     	/* Print system usage information and keep refreshing the information.
    		 If "--sequential" is called, display the information sequentially
    	   (i.e. w/o refreshing).
    	 	 If "--graphics" is called, virtualize the physical-use change.
    	 	 Takes an integer sample to indicate how many times it will print.
    	 	 Takes an integer tdelay to indicate the frequency of refreshing.
    		 Takes several integer flags to indicate the information desired. */
    
    void vertify_arg(int argc, char *argv[], int *sample, int *tdelay,
     				  int *sys_flag, int *user_flag, int *sequential_flag,
     				  int *graph_flag, int *sample_flag, int *tdelay_flag);
     	/* Validate the command line arguments user inputted.
     	   Use flags to indicate whether an argument is been called. */
    ```
    
2. Functions in `stats_functions.c`
    
    ```c
    void handle_error(char *message);
    	/* Display error message and then terminate the program. */
    
    void show_runtime_info();
     	/* Show how much memory the program use during the compilation. */
    
    void show_memory_graph(double curr_use, double previous_use);
     	/* Using symbols representing the memory usage change.
    	 	 Takes two doubles representing current and previous memory use,
    		 and calculate the change based on the two inputs. */
    
    void get_memory_info(long previous_use, int graph_flag);
     	/* Display both physical and virtual memory usage and the total memory.
    		 If "--graphics" is called, virtualize the physical-use change. */
    
    void calculate_cpu_use(int tdelay);
     	/* Calculate CPU usage (in percentage) in real-time.
     	   Takes an positive integer to indicate how long will it refresh. */
    
    void show_cpu_graph(double percent);
     	/* Using "|" to represent the CPU usage change.
    	 	 Takes a double representing the current CPU usage. */
    
    void show_cpu_info(double cpu_use);
     	/* Prints the number of CPU cores and CPU usage percentage.
    		 Takes a double representing the current CPU usage. */
    
    void show_session_user();
     	/* Display user usage (username, terminal devices, IP address). */
    
    void show_sys_info();
     	/* Display basic system information (OS name, release information,
     	   architecture, OS version, etc.). */
    ```
    

## How to run (use) my program?

---

1. Run with **make**:
    1. `make` or `make mySystemStats`: build the `mySystemStats` executable with warning flags.
    2. `make help`: display help message
    3. `make clean`: remove the `mySystemStats` executable and all object files
2. The program can take the following argument:
    
    ```
    --system	  	Show the system usage only
    --user		  	Show the users usage only
    --graphics		Include a graphical output for system usage sections
    --sequential	Output the system usage sequentially (without "refreshing")
    --samples=N 	Take a positive integer N and display the info N times
    --tdelay=T   	Take a positive integer T and display the info every T secs
    ```
    
3. Assumptions made:
    1. The display order is:
        
        Runtime Information, Memory Usage, Connected Users, CPU Usage, System information.
        
    2. The default value for "`--samples=N`" is 10, and the default value for "`--tdelay=T`" is 1.
    3. All arguments can be used together (even with themselves).
    4. Calling "`--samples=N`" or "`--tdelay=T`" multiple times with same input value will not result in error. But if the values are not consistent with each other, an error will occur.
    5. "`--samples=N`" and "`--tdelay=T`" can be considered as positional arguments (in order: samples tdelay) if they are not flagged. In this case no more than 2 integers can be taken as valid arguments.
    6. The program will intercept signals coming from `Ctrl-Z` and `Ctrl-C`. For the former, it will just ignore it as the program should not be run in the background while running interactively. For the latter, the program will ask the user whether it really wants to quit or not.

## Example Output

---

```
$ time ./mySystemStats 5 --graphics
Nbr of samples: 5 -- every 1 secs
 Memory usage: 2488 kilobytes
---------------------------------------
### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)
3.44 GB / 8.14 GB  -- 6.36 GB / 18.73 GB  |o 0.00 (3.44)
3.45 GB / 8.14 GB  -- 6.37 GB / 18.73 GB  |o 0.00 (3.45)
3.45 GB / 8.14 GB  -- 6.38 GB / 18.73 GB  |@ 0.00 (3.45)
3.45 GB / 8.14 GB  -- 6.38 GB / 18.73 GB  |@ 0.00 (3.45)
3.45 GB / 8.14 GB  -- 6.38 GB / 18.73 GB  |o 0.00 (3.45)
---------------------------------------
### Sessions/users ###
 zha10849   pts/102 (tmux(538474).%0)
 zha10849   pts/97 (tmux(538474).%3)
 vainerga   pts/295 (tmux(2313825).%2)
 zha10849   pts/308 (tmux(2052441).%4)
 zha10849   pts/315 (tmux(2052441).%5)
 lamerin2   pts/710 (tmux(1221719).%0)
 effendia   pts/737 (tmux(1273544).%0)
 vainerga   pts/700 (tmux(2313825).%0)
 jookendr   pts/802 (tmux(2856911).%0)
 lamerin2   pts/856 (tmux(3054602).%0)
 wangz725   pts/1031 (99.229.78.191)
 jookendr   pts/1040 (70.55.37.244)
 huan2534   pts/1069 (184.147.220.161)
 jianganq   pts/1084 (69.166.116.217)
---------------------------------------
Number of cores: 3
 total cpu use = 1.00%
    |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| 33.33
    |||||||||||||||||||||||||||||||||||||||||||||||||||||||||| 29.10
    || 1.01
    ||| 1.67
    || 1.00
---------------------------------------
### System Information ###
 System Name = Linux
 Machine Name = mathlab
 Version = #154-Ubuntu SMP Thu Jan 5 17:03:22 UTC 2023
 Release = 5.4.0-137-generic
 Architecture = x86_64
---------------------------------------

real    0m5.007s
user    0m0.001s
sys 0m0.003s
```

## Additional Information

---

1. Calculate the memory usage based on following equations:
    
    $$
    \begin{align*} &\text{total physical memory = MemTotal} \\ &\text{used physical memory = MemTotal - MemFree - (Buffers + Cached Memory)} \\ &\text{total virtual memory = MemTotal + SwapTotal} \\ &\text{used virtual memory = total virtual memory - SwapFree - MemFree - (Buffers + Cached Memory)} \\ \\&\text{where, Cached memory = Cached + SReclaimable} \end{align*}
    $$
    
    Reference: [https://stackoverflow.com/questions/41224738/how-to-calculate-system-memory-usage-from-proc-meminfo-like-htop](https://stackoverflow.com/questions/41224738/how-to-calculate-system-memory-usage-from-proc-meminfo-like-htop)
    
2. Virtualize the physical memory usage difference:
    - `::::::@` denoted that the total relative change is negative.
    - `######*` denoted that the total relative change is positive.
    - `|o` denoted the total relative change is positive infinitesimal
    - `|@` denoted the total relative change is negative infinitesimal
3. Calculate CPU utilization
    1. Take two samples of the CPU usage, e.g. at times $t_1$ and $t_2$ (sleep tdelay seconds in between)*. Use the `/proc/stat` file to read the data from the CPU times for each sample.
    2. Let $t_\text{total}$ be total CPU time since boot, $t_\text{idle}$ be total idle CPU time since boot, and $t_\text{usage}$ be total used CPU time since boot. Total CPU usage in real-time is then just the ratio of $\Delta t_\text{usage}$ to $\Delta t_\text{total}$.
        
        $$
        \begin{align*} &t_\text{total}\ = \rm user_i + nice_i + system_i + idle_i + iowait_i + irq_i + softirq_i \\ &t_\text{idle}\ \  = \text{idle}_i + \text{iowait}_i \\ &t_\text{usage} = t_\text{total} - t_\text{idle} \end{align*} \implies \text{CPU }(\%) = \frac{\Delta t_\text{usage}}{\Delta t_\text{total}} \times 100 \%
        $$
        
    3. Reference: [https://www.kgoettler.com/post/proc-stat/](https://www.kgoettler.com/post/proc-stat/)
