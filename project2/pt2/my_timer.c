
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shirel Baumgartner, Michael Clark, Connor Searcy");
MODULE_DESCRIPTION("A simple Linux kernel module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "my_timer"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* proc_timer;

// Global variables to store the previous time
static long long prev_seconds = -1;
static long prev_nanoseconds = -1;



static ssize_t timer_read(struct file* file, char __user *ubuf, size_t count, loff_t *ppos)
{  
    struct timespec64 ts_now;
    char buf[256];
    int len = 0;
    long long elapsed_seconds;
    long elapsed_nanoseconds;

    // Get the current time
    ktime_get_real_ts64(&ts_now);
	
    printk(KERN_INFO "BEFORE first IF: prev_seconds: %lld, prev_nanoseconds: %09ld", prev_seconds, prev_nanoseconds);
    // Calculate the elapsed time if this is not the first read
    if (prev_seconds >= 0 && prev_nanoseconds >= 0) {
	elapsed_seconds = ts_now.tv_sec - prev_seconds;
        elapsed_nanoseconds = ts_now.tv_nsec - prev_nanoseconds;
        printk(KERN_INFO "AFTER first IF: prev_seconds: %lld, prev_nanoseconds: %09ld", prev_seconds, prev_nanoseconds);
        if (elapsed_nanoseconds < 0) {
            elapsed_seconds -= 1;
            elapsed_nanoseconds += 1000000000;
        }
        
        len += snprintf(buf + len, sizeof(buf) - len, "current time: %lld.%09ld\nelapsed time: %lld.%09ld\n",
                        ts_now.tv_sec, ts_now.tv_nsec, elapsed_seconds, elapsed_nanoseconds);
        
    } else {
    	len += snprintf(buf + len, sizeof(buf) - len, "current time: %lld.%09ld\n", ts_now.tv_sec, ts_now.tv_nsec);       
        printk(KERN_INFO "Made it to else");
    }

    // Update the previous time
    prev_seconds = ts_now.tv_sec;
    prev_nanoseconds = ts_now.tv_nsec;
    

   


    // Output to user
    if (len > count) {
        len = count;
    }
    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read
};

static int __init init_timer(void) {
    proc_timer = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    printk(KERN_INFO "prev_seconds is %lld. prev_nano is %09ld.", prev_seconds, prev_nanoseconds);
    printk(KERN_INFO "New run\n");
    if (proc_timer == NULL)
        return -ENOMEM;
    return 0;
}

static void __exit exit_timer(void) {
    proc_remove(proc_timer);
}

module_init(init_timer);
module_exit(exit_timer);
