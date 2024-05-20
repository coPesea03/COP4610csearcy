#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shirel Baumgartner, Michael Clark, Connor Searcy");
MODULE_DESCRIPTION("Project 2 Elevator");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define ENTRY_SIZE 10000
#define PERMS 0644
#define NUM_FLOORS 5
#define PARENT NULL

static struct proc_ops fops;

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int,int,int);
extern int (*STUB_stop_elevator)(void);

//0 = OFFLINE, 1 = IDLE, 2 = LOADING, 3 = UP, 4 = DOWN
static int state = 0;               //keeps track of state duh
static char *stateString = "OFFLINE";    //converts state to string forproc
static int currentFloor = 1;        //current floor (for proc and up/down)
static int passengerWeight = 0;     //for proc and LOADING while loop
static char decimalWeight[5] = "0.0"; //each increment of this is worth 1/2 pound
/*
    we add a laywer (1.5 lbs)

    we would work with the value 3. (max is 7, so 14)

    lets say we have a struct that outputs it as a real decimal number (format longlong.long) 
    only for proc file (or any front end)
    need helper function to convert

*/
static int passengerCount = 0;      //for proc and LOADING while loop
static int passengersWaiting = 0;   //total queue across all foors
static int passengersServiced = 0;  //total counter for all passengers
static int elevatorGoTo = 1;
bool isStopping = false;
bool shouldPickUp = true;

//********************************************************************//

typedef struct Passenger {
    int type; // V, P, L, B, etc.
    int destinationFloor;
    int startFloor;
    struct list_head newPassenger;
} Passenger;

typedef struct PassengerList{
    struct list_head list;
    int num;
} floorWaitingList;

struct PassengerList currentElevator;

typedef struct Floor {
    int floorNumber;
    struct PassengerList passengers;
} Floor;


Floor floors[NUM_FLOORS]; 


//********************************************************************//


static char *message;

struct elevator_parameter {
    struct task_struct *kthread;
};

int elevator_runs(void*);
void weightConversion(int);

struct elevator_parameter thread1;

struct mutex my_mutex;


int elevator_runs(void*) {
    int lockResult;

    for(int p = 0; p<20;p++){
        printk(KERN_INFO "restarded here");
    }
   
    while(!kthread_should_stop()) {   
        if(state == 0){       //OFFLINE (probably shouldnt be used)
            lockResult = mutex_lock_interruptible(&my_mutex);
            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                //printk(KERN_INFO "lock acquired in OFFLINE");
                stateString = "OFFLINE";
                mutex_unlock(&my_mutex);
            }
        }else if(isStopping == true){
            lockResult = mutex_lock_interruptible(&my_mutex);

            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                
                bool hasPassengersToDropOff = (passengerCount > 0);
                
                
                while(hasPassengersToDropOff){
                    //movement
                    struct Passenger *tempPassengerStopping;
                    tempPassengerStopping = list_first_entry(&currentElevator.list, Passenger, newPassenger);
                    elevatorGoTo = tempPassengerStopping->destinationFloor;

                    while(!(elevatorGoTo == currentFloor)){
                        if(elevatorGoTo > currentFloor){
                            ssleep(2);
                            currentFloor++;
                            stateString = "UP";
                        } else if(elevatorGoTo < currentFloor){
                            ssleep(2);
                            currentFloor--;
                            stateString = "DOWN";
                        }
                    }

                    struct list_head *temp;
                    struct list_head *dummy;
                    struct Passenger *tempPassenger;
                    printk(KERN_INFO "lock acquired in isStopping");
                    
                    list_for_each_safe(temp, dummy, &currentElevator.list) {
                        tempPassenger = list_entry(temp, Passenger, newPassenger);
                        if(tempPassenger->destinationFloor == currentFloor) {
                            //0 part timer, 1 lawyer, 2 boss, 3 visitor
                            //decrement weight before we free the memory/deallocate
                            if(tempPassenger->type == 0){
                                passengerWeight -= 2;
                            } else if(tempPassenger->type == 1){
                                passengerWeight -= 3;
                            } else if(tempPassenger->type == 2){
                                passengerWeight -= 4;
                            } else if(tempPassenger->type == 3){
                                passengerWeight -= 1;
                            } 
                             
                            weightConversion(passengerWeight);
                            passengerCount--;
                            printk(KERN_INFO "Dropped off passenger on floor %d. PassengerCount is %d", currentFloor, passengerCount);
                            ssleep(1);

                            list_del(temp);
                            kfree(tempPassenger);
                            passengersServiced++;
                        }
                    }
                    hasPassengersToDropOff = (passengerCount > 0);
                }
                state = 0;
                isStopping = false;
                mutex_unlock(&my_mutex);
            }
        } else if(state == 1){ //IDLE (no passengers waiting)
            lockResult = mutex_lock_interruptible(&my_mutex);
            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                //printk(KERN_INFO "lock acquired in IDLE");
                stateString = "IDLE";
                if(passengersWaiting > 0){
                    state = 2;
                }
                mutex_unlock(&my_mutex);
            }

        } else if(state == 2){ //LOADING PASSENGERS (transfer from floor waiting to elevator list)
            lockResult = mutex_lock_interruptible(&my_mutex);

            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                struct list_head *temp;
                struct list_head *dummy;
                struct Passenger *tempPassenger;
                printk(KERN_INFO "lock acquired in LOADING");
                
                stateString = "LOADING";
                
                //dropoff loop
                list_for_each_safe(temp, dummy, &currentElevator.list) {
                    tempPassenger = list_entry(temp, Passenger, newPassenger);
                    if(tempPassenger->destinationFloor == currentFloor) {
                        int didWeDrop = passengerCount;
                        //0 part timer, 1 lawyer, 2 boss, 3 visitor
                        //decrement weight before we free the memory/deallocate
                        if(tempPassenger->type == 0){
                            passengerWeight -= 2;
                        } else if(tempPassenger->type == 1){
                            passengerWeight -= 3;
                        } else if(tempPassenger->type == 2){
                            passengerWeight -= 4;
                        } else if(tempPassenger->type == 3){
                            passengerWeight -= 1;
                        } 
                        weightConversion(passengersWaiting);
                        passengerCount--;
                        printk(KERN_INFO "Dropped off passenger on floor %d. PassengerCount is %d", currentFloor, passengerCount);
                        ssleep(1);

                        list_del(temp);
                        kfree(tempPassenger);
                        passengersServiced++;
                        
                        if(didWeDrop > passengerCount){
                            shouldPickUp = true;
                        }
                    }
                }    

                // Pick up passengers from the current floor if there is any
                struct Passenger *temp2;

                while(shouldPickUp == true && passengerWeight <= 14 && passengerCount <=5 && floors[(currentFloor - 1)].passengers.num > 0) {
                    temp2 = list_first_entry(&floors[(currentFloor - 1)].passengers.list, Passenger, newPassenger);
                    
                    int weightCheck = passengerWeight;
                    int capacityCheck = passengerCount;
                    if(temp2->type == 0){
                        weightCheck += 2;
                    } else if(temp2->type == 1){
                        weightCheck += 3;
                    } else if(temp2->type == 2){
                        weightCheck += 4;
                    } else if(temp2->type == 3){
                        weightCheck += 1;
                    } 

                    capacityCheck ++;

                    if(weightCheck <= 14 && capacityCheck <=5 && floors[(currentFloor - 1)].passengers.num > 0){
                        list_move_tail(&temp2->newPassenger, &currentElevator.list);
                        floors[(currentFloor - 1)].passengers.num --;
                        passengersWaiting --;
                        passengerWeight = weightCheck;
                        passengerCount = capacityCheck;
                        weightConversion(passengerWeight);

                        printk(KERN_INFO "Picked up passenger from floor %d. PassengerCount is %d", currentFloor, passengerCount);
                        ssleep(1);
                    }else{
                        printk(KERN_INFO "Rejected: Potential Weight and Capacity: %d and %d", weightCheck, capacityCheck);
                        shouldPickUp = false;
                    }
                }

                //switch state accordingly
                if(passengerCount > 0) {           
                    struct Passenger *tempPassenger3;
                    tempPassenger3 = list_first_entry(&currentElevator.list, Passenger, newPassenger);
                    if(currentFloor < tempPassenger3->destinationFloor){
                        elevatorGoTo = tempPassenger3->destinationFloor;
                        state = 3;
                        stateString = "UP";
                        printk(KERN_INFO "Switched to UP");
                    } else if (currentFloor > tempPassenger3->destinationFloor){
                        elevatorGoTo = tempPassenger3->destinationFloor;
                        state = 4;
                        stateString = "DOWN";
                        printk(KERN_INFO "Switched to DOWN");
                    }
                }

                if(passengersWaiting == 0 && passengerCount == 0){
                    state = 1;
                    stateString = "IDLE";
                } else if (passengersWaiting > 0 && passengerCount == 0){
                    for (int i = 0; i < NUM_FLOORS; i++){
                        if(floors[i].passengers.num > 0){
                            printk(KERN_INFO "number of people on floor %d: %d", (i+1), floors[i].passengers.num);
                            elevatorGoTo = i+1;

                            break;
                        }
                    }

                    if(elevatorGoTo > currentFloor){
                        state = 3;
                        stateString = "UP";
                        printk(KERN_INFO "Switched to UP");
                    } else if(elevatorGoTo < currentFloor){
                        state = 4;
                        stateString = "DOWN";
                        printk(KERN_INFO "Switched to DOWN");
                    }
                }
                printk(KERN_INFO "There are  %d people in the elevator\n", passengerCount);
                printk(KERN_INFO "There are  %d people waiting for the elevator\n", passengersWaiting);
                mutex_unlock(&my_mutex);
            }
        } else if(state == 3){ //UP (going up)
            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                printk(KERN_INFO "lock acquired in UP");
                
                for(int i = currentFloor; i < elevatorGoTo; ++i) {
                    ssleep(2);
                    if(currentFloor == 5){
                        break;
                    } else{
                        currentFloor++;
                    }
                    printk(KERN_INFO "Floor Incremented. currentFloor: %d", currentFloor);
                }
                state = 2;
                mutex_unlock(&my_mutex);
            }

        } else if(state == 4){ //DOWN (going down)
            lockResult = mutex_lock_interruptible(&my_mutex);
            if(lockResult){
                printk(KERN_INFO "lock not acquired, operation interrupted by a signal.");
                if(lockResult == -EINTR){
                    printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                    continue;
                }
            } else{
                printk(KERN_INFO "lock acquired in DOWN");
                
                for(int i = currentFloor; i > elevatorGoTo; --i) {                   
                    ssleep(2);
                    if(currentFloor == 1){
                        break;
                    } else{
                        currentFloor--;
                    }
                    printk(KERN_INFO "Floor Decremented. currentFloor: %d", currentFloor);
                }
                state = 2;
                mutex_unlock(&my_mutex);
            }
                  
        } 
    } 

    return 0;

}


int start_elevator(void) {
   
    if(state == 0) {
         thread1.kthread = kthread_run(elevator_runs, NULL, "start");
         printk(KERN_INFO "Elevator started");
         state = 1;
         stateString = "IDLE";
    } else{
        printk(KERN_INFO "Elevator Already Running");
    }

    return 0;
}

int issue_request(int start_floor, int destination_floor, int type){
    // Allocate memory for a new Passenger instance
    Passenger* newPassenger = kmalloc(sizeof(Passenger), GFP_KERNEL);
    if (!newPassenger) return -ENOMEM; // Handle memory allocation failure
    printk(KERN_INFO "New passenger allocated: start floor %d, destination floor %d, type %d\n", start_floor, destination_floor, type);

    // Populate the new passenger's details
    newPassenger->startFloor = start_floor;
    newPassenger->destinationFloor = destination_floor;
    newPassenger->type = type;
    INIT_LIST_HEAD(&newPassenger->newPassenger);

    // Safeguard to ensure the start_floor is within bounds
    if (start_floor < 0 || start_floor > NUM_FLOORS) {
        kfree(newPassenger); // Prevent memory leak
        return -EINVAL; // Invalid argument
    }

    bool tryLock = true;
    int lockResult;
    while(tryLock){
        lockResult = mutex_lock_interruptible(&my_mutex);
        switch (lockResult){
            case -EINTR:
                printk(KERN_INFO "handle interruption. lockResult = %d (should be -EINTR)", lockResult);
                continue;
            case 0:
                tryLock = false;
                break;
        }
    }
    printk(KERN_INFO "lock acquired in IR");

    // Add the new passenger to the start floor's passenger list
    list_add_tail(&newPassenger->newPassenger, &floors[(start_floor - 1)].passengers.list);
    floors[(start_floor - 1)].passengers.num++;
    printk(KERN_INFO "Passenger added to floor %d list. Total passengers on this floor: %d\n", start_floor, floors[(start_floor - 1)].passengers.num);

    printk(KERN_INFO "Issue request processed: Passenger for floor %d heading to floor %d of type %d added successfully.\n", start_floor, destination_floor, type);

    passengersWaiting++;
    printk(KERN_INFO "passengers waiting count is now: %d\n", passengersWaiting);
    // Critical section end
    mutex_unlock(&my_mutex);

    return 0; // Success
}

/* hello future connor and michael, its past connor
you incinerated the vm so bad that you couldnt even roll back. congrats on
being able to recover it somehow. you need to ask the professor or TA why the lock
doesnt relase at the end of ^issue request. when you ran ./producer 5 it made one and then
exploded. why? i dont know! obviously a deadlock, but no clue as to why.


*/

int stop_elevator(void) {
    // Attempt to acquire the lock first.
    if (mutex_lock_interruptible(&my_mutex))
        return -EINTR; // If the lock wasn't acquired due to an interrupt.
    
    // Check if the elevator is already stopping or is offline.
    if (state == 0 || isStopping == true) { // Assuming 0 is OFFLINE.
        mutex_unlock(&my_mutex);
        printk(KERN_INFO "Elevator is already stopped or in the process of stopping.");
        return 1; // Indicates already stopped or stopping.
    }
    isStopping = true;

    state = 0; 
    stateString = "OFFLINE";

    mutex_unlock(&my_mutex);
    printk(KERN_INFO "Elevator stopping initiated.");

    return 0; // Successfully initiated stopping.
}
    
//**************************************************************************************//

char passenger_type_to_char(int type) {
    switch (type) {
        case 0: return 'P'; // Part-timers
        case 1: return 'L'; // Lawyers
        case 2: return 'B'; // Bosses
        case 3: return 'V'; // Visitors
        default: return '?'; // Unknown type
    }
}

void weightConversion(int weight){
    decimalWeight[0] = '\0';

    if((weight % 2) == 1){
        snprintf(decimalWeight, 4, "%d.5", ((weight - 1) / 2));
    } else if((weight % 2) == 0){
        snprintf(decimalWeight, 4, "%d.0", (weight / 2));
    }
}

void append_elevator_status(char *buf, int *len, struct PassengerList *elevator) {
    struct list_head *pos;
    list_for_each(pos, &elevator->list) {
        Passenger *p = list_entry(pos, Passenger, newPassenger);
        *len += snprintf(buf + *len, ENTRY_SIZE - *len, "%c%d ", passenger_type_to_char(p->type), p->destinationFloor);
    }
    *len += snprintf(buf + *len, ENTRY_SIZE - *len, "\n");
}

void append_floor_status(char *buf, int *len, Floor *floors, int currentFloor) {
    for (int i = NUM_FLOORS; i >= 1; i--) {
        *len += snprintf(buf + *len, ENTRY_SIZE - *len, "[%c] Floor %d: ", (i == currentFloor) ? '*' : ' ', i);
        if (!list_empty(&floors[i-1].passengers.list)) {
            struct list_head *pos;
            list_for_each(pos, &floors[i-1].passengers.list) {
                Passenger *p = list_entry(pos, Passenger, newPassenger);
                *len += snprintf(buf + *len, ENTRY_SIZE - *len, "%c%d ", passenger_type_to_char(p->type), p->destinationFloor);
            }
        } else {
            *len += snprintf(buf + *len, ENTRY_SIZE - *len, "0");
        }
        *len += snprintf(buf + *len, ENTRY_SIZE - *len, "\n");
    }
}


static int elevator_proc_open(struct inode *sp_inode, struct file *sp_file) {
    message = kmalloc(ENTRY_SIZE, GFP_KERNEL); // Allocate space for the message
    if (message == NULL) {
        printk(KERN_WARNING "elevator_proc_open: allocation failed");
        return -ENOMEM;
    }

    int len = 0; // Length of the current message
    // Initialize message to an empty string to safely use snprintf
    message[0] = '\0';

    // Prepare your message content
    len += snprintf(message + len, ENTRY_SIZE - len, "Elevator state: %s\n", stateString);
    len += snprintf(message + len, ENTRY_SIZE - len, "Current floor: %d\n", currentFloor);
    len += snprintf(message + len, ENTRY_SIZE - len, "Current load: %s\n", decimalWeight);

    len += snprintf(message + len, ENTRY_SIZE - len, "Elevator Status: ");
    append_elevator_status(message, &len, &currentElevator);
    append_floor_status(message, &len, floors, currentFloor);;


    len += snprintf(message + len, ENTRY_SIZE - len, "Passenger count: %d\n", passengerCount);
    len += snprintf(message + len, ENTRY_SIZE - len, "Passengers Waiting: %d\n", passengersWaiting);
    len += snprintf(message + len, ENTRY_SIZE - len, "Passengers Serviced: %d\n", passengersServiced);

    return 0; // Indicate successful open
}

ssize_t elevator_proc_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset) {
    // Use simple_read_from_buffer to safely handle the copy and offset management
    return simple_read_from_buffer(buf, size, offset, message, strlen(message));
}

int elevator_proc_release(struct inode *sp_inode, struct file *sp_file) {
    if (message) {
        kfree(message);
        message = NULL; // Ensure the pointer is cleared after freeing
    }
    return 0;
}

// /*************************************************************************************/
static int __init elevator_init(void) {
    for (int i = 0; i < NUM_FLOORS; i++) {
        floors[i].floorNumber = i + 1;
        INIT_LIST_HEAD(&floors[i].passengers.list);
        floors[i].passengers.num = 0;
    }
    
    INIT_LIST_HEAD(&currentElevator.list);

    STUB_start_elevator = start_elevator;
        STUB_issue_request = issue_request;
        STUB_stop_elevator = stop_elevator;

        fops.proc_open = elevator_proc_open;
    	fops.proc_read = elevator_proc_read;
    	fops.proc_release = elevator_proc_release;

        if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {
    		printk(KERN_WARNING "failed to create PROC");
    		remove_proc_entry(ENTRY_NAME, NULL);
    		return -ENOMEM;
    	}

    mutex_init(&my_mutex);

    return 0;  // Return 0 to indicate successful loading
}

static void __exit elevator_exit(void) {
    if(thread1.kthread){
        kthread_stop(thread1.kthread);
        thread1.kthread = NULL;
    }

    STUB_start_elevator = NULL;
        STUB_issue_request = NULL;
        STUB_stop_elevator = NULL;

    remove_proc_entry(ENTRY_NAME, NULL);
    printk(KERN_NOTICE "Removing /proc/%s\n", ENTRY_NAME);

    mutex_destroy(&my_mutex);

}

module_init(elevator_init);  // Specify the initialization function
module_exit(elevator_exit);  // Specify the exit/cleanup function

