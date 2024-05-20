# Kernel Project

This project aims to provide a comprehensive understanding of system calls, kernel programming, concurrency, synchronization, and elevator scheduling algorithms. It consists of multiple parts that build upon each other to deepen your knowledge and skills in these areas.

By completing this project, you acquire practical experience in system calls, kernel programming, concurrency, synchronization, and scheduling algorithms. These are essential skills for developing efficient and robust software systems, particularly in operating systems and low-level programming domains. Understanding system calls and kernel programming enables you to interact with and extend the functionality of the operating system, while concurrency, synchronization, and scheduling concepts are crucial for efficient resource management and multitasking in complex systems.

## Group Information

- **Group Number:** 28
- **Group Name:** Group 28
- **Group Members:**
  - Shirel Baumgartner: sb21l@fsu.edu
  - Michael Clark: mtc21c@fsu.edu
  - Connor Searcy: cps21a@fsu.edu


## Division of Labor:

### Part 1: System-Call Tracing

Responsibilities: Create an empty C program named "empty".
Make a copy of the "empty" program and name it "part1".
Add exactly five system calls to the "part1" program. You can find the available system calls for your machine in "/usr/include/unistd.h".
To verify that you have added the correct number of system calls, execute the following commands in the terminal: 
```bash
$ gcc -o empty empty.c 
$ strace -o empty.trace ./empty 
$ gcc -o part1 part1.c 
$ strace -o part1.trace ./part1
```

Completed by: Shirel Baumgartner, Michael Clark, Connor Searcy

### Part 2: Timer Kernel Module

Responsibilities: Develop a kernel module called my_timer that calls the ktime_get_real_ts64() function to obtain the current time. This module should store the time value.
When the my_timer module is loaded using insmod, it should create a proc entry named "/proc/timer".
When the my_timer module is unloaded using rmmod, the /proc/timer entry should be removed.
On each read operation of "/proc/timer", utilize the proc interface to print both the current time and the elapsed time since the last call (if valid).

Example Usage:
```plaintext
$ cat /proc/timer 
current time: 1518647111.760933999 

$ sleep 1

$ cat /proc/timer
current time: 1518647112.768429998
elapsed time: 1.007495999

$ sleep 3

$ cat /proc/timer
current time: 1518647115.774925999
elapsed time: 3.006496001

$ sleep 5

$ cat /proc/timer
current time: 1518647120.780421999
elapsed time: 5.005496000
```

Completed By: Shirel Baumgartner, Michael Clark, Connor Searcy

### Part 3: Elevator Module

Responsibilities: Implement a scheduling algorithm for a office elevator. Create a kernel module elevator to implement this.

The office elevator can hold a maximum of 5 passengers and cannot exceed a maximum weight of 7 lbs
Each worker is randomly chosen as a part-time, lawyer, boss, and visitor, with equal likelihood.
Workers select their starting floor and destination floor.
Workers board the elevator in first-in, first-out (FIFO) order.
Each type of passenger (part-time, lawyer, boss, and visitor) has an assigned weight:
```plaintext
1 lbs for part-time workers
1.5 lbs for lawyers
2 lbs for bosses
0.5 lbs for visitors
```
Workers on floors wait indefinitely to be serviced.
Once a worker boards the elevator, they can only disembark.
The elevator must wait for 2.0 seconds when moving between floors and 1.0 seconds when loading/unloading passengers.
The building has floor 1 as the minimum (lobby) and floor 5 as the maximum.
Workers can arrive at any time, and each floor must handle an arbitrary number of them.

Completed By: Shirel Baumgartner, Michael Clark, Connor Searcy

#### Step 3.1: Add System Calls

Responsibilities: Modify the kernel by adding three system calls to control the elevator and create passengers. 
Assign the following numbers to the system calls:
```plaintext
548 for start_elevator() - activates the elevator for service
549 for issue_request() - creates a request for a passenger, specifying the start floor, destination floor, and type of passenger
550 for stop_elevator() - deactivates the elevator
```
Make these files to add the system calls:
```plaintext
[kernel_version]/syscalls/syscalls.c
[kernel_version]/syscalls/Makefile
```
Mdify the following files to add the system calls:
```plaintext
[kernel_version]/arch/x86/syscalls/syscall_64.tbl
[kernel_version]/include/linux/syscalls.h
[kernel_version]/Makefile
```

Completed By: Shirel Baumgartner, Michael Clark, Connor Searcy

#### Part 3.2: Compile and Install the Kernel

Responsibilities: Disable certain certificates when adding system calls
Compile the kernel with the new system calls. The kernel should be compiled with the following options:
```bash
$ make menuconfig
$ make -j$(nproc)
$ sudo make modules_install
$ sudo make install
$ sudo reboot
```
Check that you installed your kernel by typing this into the terminal:
```bash
$ uname -r
```

Completed By: Michael Clark and Connor Searcy

#### Part 3.3: Test System Calls

Responsibilities: Test if you successfully added the system called to your installed kernel with the provided tests in your starter file in the directory part3/tests/

Run the following commands:
```bash
$ make 
$ sudo insmod syscalls.ko
$ ./test_syscalls
```
You should get a message that tells you if you have the system calls installed or not.

Completed By: Connor Searcy

#### Part 3.4: Implement Elevator

Responsibilities: Implement the elevator kernel module. The module should be named "elevator" and should be loaded using insmod. The module should be unloaded using rmmod.

  -  Use linked lists to handle the number of passengers per floor/elevator.
  -  Use a kthread to control the elevator movement.
  -  Use a mutex to control shared data access between floor and elevators.
  -  Use kmalloc to allocate dynamic memory for passengers.

#### Part 3.5: Write to Proc File

Responsibilities: The module must provide a proc entry named /proc/elevator. 
The following information should be printed (labeled appropriately):

  - The elevator's movement state:
    - OFFLINE: when the module is installed but the elevator isn't running (initial state)
    - IDLE: elevator is stopped on a floor because there are no more passengers to service
    - LOADING: elevator is stopped on a floor to load and unload passengers
    - UP: elevator is moving from a lower floor to a higher floor
    - DOWN: elevator is moving from a higher floor to a lower floor
  - The current floor the elevator is on
  - The elevator's current load (weight)
  - A list of passengers in the elevator
  - The total number of passengers waiting
  - The total number of passengers serviced

For each floor of the building, the following should be printed:

  - An indicator of whether or not the elevator is on the floor.
  - The count of waiting passengers.
  - For each waiting passenger, 2 characters indicating the passenger type and destination floor.

Example Proc File:
```plaintext
Elevator state: LOADING
Current floor: 4
Current load: 4.5 lbs
Elevator status: P5 B2 P4 V1

[ ] Floor 6: 1 V3
[ ] Floor 5: 0
[*] Floor 4: 2 L1 B2
[ ] Floor 3: 2 L4 P5 
[ ] Floor 2: 0
[ ] Floor 1: 0 

Number of passengers: 4
Number of passengers waiting: 5
Number of passengers serviced: 61
```
P is for part-timers, L is for lawyers, B is for bosses, V is for visitors.

Completed By: Shirel Baumgartner, Michael Clark, Connor Searcy

#### Part 3.6: Test Elevator

Responsibilities: Interact with two provided user-space applications that enable communication with the kernel module:

  - producer.c: creates passengers and issues requests to the elevator
```bash
$ ./producer [number_of_passengers]
```
  - consumer.c: calls the start_elevator() or the stop_elevator() system call.
    - If the flag is --start, the program starts the elevator.
    - If the flag is --stop, the program stops the elevator.

You can use the following command to see your elevator in action:
```bash
$ watch -n [snds] cat [proc_file]
```
Completed By: Shirel Baumgartner, Michael Clark, Connor Searcy


### Part 4: Demo

You will be required to demonstrate:

  - Project source code
  - Kernel system calls
  - Successful compilation of your elevator
  - Successful installation of your elevator
  - Successful execution of system calls
  - Execution of arbitrary number of test cases on your elevator for 3 minutes
  - Successful removal of your elevator

### Extra Credit

Responsibilities: [Description] Completed By: n/a


## File Listing:

```plaintext
project-2-os-group-28/
├── pt1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
├── pt2/
│   ├── Makefile
│   ├── my_timer.c
├── pt3/
│   ├── elevator.c
│   ├── Makefile
├── README.md
```
## How to Compile & Execute:

```bash
git clone https://github.com/FSU-COP4610-S24/project-2-os-group-28.git
```
Compile the program on VM using the provided Makefile. To execute, run the executable named "elevator" created by the makefile after inserting
the elevator kernel module.

sudo insmod elevator.ko
./consumer --start
./producer [number of passengers]
./consumer --stop
make 
```
This will build the executable at: 

## Requirements: 
### Compiler: 

### Dependencies: 


## Bugs

1. When running the elevator an incorrect weight might be shown.
2. When running the timer module it will always output the elapsed time even though no time has elapsed.
3. 
## Extra Credit

Extra Credit 1: N/A Extra Credit 2: N/A Extra Credit 3: N/A

## Considerations
