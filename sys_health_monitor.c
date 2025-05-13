 

#include <linux/module.h>       // Needed by all modules 

#include <linux/kernel.h>       // Needed for KERN_INFO, printk() 

#include <linux/init.h>         // Needed for the __init and __exit macros 

#include <linux/proc_fs.h>      // Needed for proc filesystem 

#include <linux/seq_file.h>     // For seq_file interface 

#include <linux/uaccess.h>      // For copy_to_user (not strictly needed for seq_file read) 

#include <linux/version.h>      // For KERNEL_VERSION MACRO 

#include <linux/timer.h>        // Needed for kernel timers 

#include <linux/jiffies.h>      // Needed for jiffies and msecs_to_jiffies 

#include <linux/spinlock.h>     // Needed for spinlocks 

#include <linux/mm.h>           // Needed for si_meminfo() and struct sysinfo, PAGE_SIZE 

#include <linux/sched.h>        // For CPU load averages (avenrun) 

// #include <linux/fs.h>        // Would be needed for block_device if Disk I/O was enabled 

// #include <linux/blkdev.h>    // Would be needed for blkdev_get_by_path if Disk I/O was enabled 

// #include <linux/genhd.h>     // *** Header causing issues, needed for gendisk if Disk I/O was enabled *** 

  

// --- Group Identification --- 

#define GROUP_NAME "Group 8" 

#define MEMBER_NAMES "Thomas Rafferty, Anthony Pinon, Kendall Reese" 

// --- End Group Identification --- 

  

#define PROC_FILENAME "sys_health" 

#define TIMER_INTERVAL_SECONDS 5 // Set the timer interval (e.g., 5 seconds) 

  

// CPU Load average constants 

#ifndef FSHIFT 

#define FSHIFT 11               /* nr of bits of precision */ 

#endif 

#ifndef FIXED_1 

#define FIXED_1 (1<<FSHIFT)     /* 1.0 as fixed-point */ 

#endif 

#ifndef LOAD_INT 

#define LOAD_INT(x) ((unsigned long)(x) >> FSHIFT) 

#endif 

#ifndef LOAD_FRAC 

#define LOAD_FRAC(x) LOAD_INT(((unsigned long)(x) & (FIXED_1-1)) * 100) 

#endif 

  

// Explicitly declare avenrun as an external symbol 

// This array holds the 1, 5, and 15 minute load averages. 

// It's defined in the kernel (kernel/sched/loadavg.c) but might not be 

// directly visible to modules without this extern declaration on some configurations. 

extern unsigned long avenrun[]; 

  

  

// Buffer for metrics and its lock 

static char current_metrics_buffer[1024]; 

static DEFINE_SPINLOCK(metrics_lock); // To protect the buffer 

  

// Kernel timer 

static struct timer_list sys_health_timer; 

  

// Forward declarations 

static int sys_health_proc_show(struct seq_file *m, void *v); 

static void sys_health_timer_callback(struct timer_list *t); 

  

// This function is called when the /proc file is opened. 

static int sys_health_proc_open(struct inode *inode, struct file *file) 

{ 

    printk(KERN_INFO "[%s - %s] /proc/%s opened.\n", GROUP_NAME, MEMBER_NAMES, PROC_FILENAME); 

    return single_open(file, sys_health_proc_show, NULL); 

} 

  

// This function is called when data is read from the /proc file. 

static int sys_health_proc_show(struct seq_file *m, void *v) 

{ 

    unsigned long flags; 

    char temp_buf[sizeof(current_metrics_buffer)]; 

  

    printk(KERN_DEBUG "[%s - %s] Reading data for /proc/%s.\n", GROUP_NAME, MEMBER_NAMES, PROC_FILENAME); 

  

    spin_lock_irqsave(&metrics_lock, flags); 

    strncpy(temp_buf, current_metrics_buffer, sizeof(temp_buf) - 1); 

    temp_buf[sizeof(temp_buf) - 1] = '\0'; 

    spin_unlock_irqrestore(&metrics_lock, flags); 

  

    seq_printf(m, "--- System Health Report (%s) ---\n", GROUP_NAME); 

    seq_printf(m, "Team Members: %s\n", MEMBER_NAMES); 

    seq_printf(m, "%s", temp_buf); 

    seq_printf(m, "--------------------------------------\n"); 

    return 0; 

} 

  

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0) 

static const struct proc_ops proc_sys_health_ops = { 

    .proc_open    = sys_health_proc_open, 

    .proc_read    = seq_read, 

    .proc_lseek   = seq_lseek, 

    .proc_release = single_release, 

}; 

#else 

static const struct file_operations proc_sys_health_ops = { 

    .open    = sys_health_proc_open, 

    .read    = seq_read, 

    .llseek  = seq_lseek, 

    .release = single_release, 

}; 

#endif 

  

static struct proc_dir_entry *sys_health_proc_entry; 

  

// Timer Callback Function 

static void sys_health_timer_callback(struct timer_list *t) 

{ 

    unsigned long flags; 

    struct sysinfo si; 

    unsigned long total_ram_mb, free_ram_mb, used_ram_mb; 

    unsigned long mem_unit_val; 

  

    // CPU Load 

    unsigned long avg_1, avg_5, avg_15; 

    unsigned long frac_1, frac_5, frac_15; 

  

    // --- Disk I/O (Temporarily Disabled due to missing genhd.h) --- 

    char disk_io_status_msg[] = "Disk I/O: Monitoring Disabled (genhd.h missing)"; 

  

  

    printk(KERN_INFO "[%s - %s] Timer callback fired. Collecting metrics...\n", GROUP_NAME, MEMBER_NAMES); 

  

    // --- Collect Memory Usage --- 

    si_meminfo(&si); 

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,23) 

    mem_unit_val = si.mem_unit ? si.mem_unit : PAGE_SIZE; 

#else 

    mem_unit_val = PAGE_SIZE; 

#endif 

    if (mem_unit_val == 0) { 

        printk(KERN_WARNING "[%s - %s] mem_unit_val is zero for memory, defaulting to 0 MB.\n", GROUP_NAME, MEMBER_NAMES); 

        total_ram_mb = 0; free_ram_mb = 0; 

    } else { 

        total_ram_mb = (si.totalram * mem_unit_val) / (1024 * 1024); 

        free_ram_mb  = (si.freeram * mem_unit_val) / (1024 * 1024); 

    } 

    used_ram_mb = total_ram_mb - free_ram_mb; 

  

    // --- Collect CPU Load Averages --- 

    // Now that avenrun is declared as extern, this should work. 

    avg_1 = LOAD_INT(avenrun[0]); frac_1 = LOAD_FRAC(avenrun[0]); 

    avg_5 = LOAD_INT(avenrun[1]); frac_5 = LOAD_FRAC(avenrun[1]); 

    avg_15 = LOAD_INT(avenrun[2]); frac_15 = LOAD_FRAC(avenrun[2]); 

  

    // --- Disk I/O Collection (Skipped) --- 

    printk(KERN_WARNING "[%s - %s] %s\n", GROUP_NAME, MEMBER_NAMES, disk_io_status_msg); 

  

  

    // --- Update the metrics buffer --- 

    spin_lock_irqsave(&metrics_lock, flags); 

    snprintf(current_metrics_buffer, sizeof(current_metrics_buffer), 

             "CPU Load (1m, 5m, 15m): %lu.%02lu, %lu.%02lu, %lu.%02lu\n" 

             "Memory Usage: Total: %lu MB, Free: %lu MB, Used: %lu MB\n" 

             "%s\n" // Disk I/O status message 

             "(Last tick: %lu jiffies)\n", 

             avg_1, frac_1, avg_5, frac_5, avg_15, frac_15, 

             total_ram_mb, free_ram_mb, used_ram_mb, 

             disk_io_status_msg, // Display the disk I/O status 

             jiffies); 

    spin_unlock_irqrestore(&metrics_lock, flags); 

  

    // Rearm the timer for the next interval 

    mod_timer(&sys_health_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_SECONDS * 1000)); 

} 

  

  

// Module Initialization Function 

static int __init sys_health_init(void) 

{ 

    unsigned long flags; 

    printk(KERN_INFO "[%s - %s] Loading System Health Monitor module.\n", GROUP_NAME, MEMBER_NAMES); 

  

    sys_health_proc_entry = proc_create(PROC_FILENAME, 0444, NULL, &proc_sys_health_ops); 

    if (sys_health_proc_entry == NULL) { 

        printk(KERN_ERR "[%s - %s] Failed to create /proc/%s entry.\n", GROUP_NAME, MEMBER_NAMES, PROC_FILENAME); 

        return -ENOMEM; 

    } 

    printk(KERN_INFO "[%s - %s] /proc/%s entry created successfully.\n", GROUP_NAME, MEMBER_NAMES, PROC_FILENAME); 

  

    spin_lock_irqsave(&metrics_lock, flags); 

    snprintf(current_metrics_buffer, sizeof(current_metrics_buffer), 

             "CPU Load: N/A\nMemory Usage: N/A\nDisk I/O: N/A\n(Awaiting first timer tick)\n"); 

    spin_unlock_irqrestore(&metrics_lock, flags); 

  

    printk(KERN_INFO "[%s - %s] Initializing kernel timer.\n", GROUP_NAME, MEMBER_NAMES); 

    timer_setup(&sys_health_timer, sys_health_timer_callback, 0); 

  

    if (mod_timer(&sys_health_timer, jiffies + msecs_to_jiffies(TIMER_INTERVAL_SECONDS * 1000))) { 

        printk(KERN_ERR "[%s - %s] Error: Failed to add/start kernel timer!\n", GROUP_NAME, MEMBER_NAMES); 

        proc_remove(sys_health_proc_entry); 

        return -EFAULT; 

    } else { 

        printk(KERN_INFO "[%s - %s] Kernel timer started to fire every %d seconds.\n", GROUP_NAME, MEMBER_NAMES, TIMER_INTERVAL_SECONDS); 

    } 

  

    return 0; 

} 

  

// Module Cleanup Function 

static void __exit sys_health_exit(void) 

{ 

    printk(KERN_INFO "[%s - %s] Unloading System Health Monitor module.\n", GROUP_NAME, MEMBER_NAMES); 

  

    printk(KERN_INFO "[%s - %s] Stopping kernel timer.\n", GROUP_NAME, MEMBER_NAMES); 

    if (del_timer_sync(&sys_health_timer)) { 

        printk(KERN_INFO "[%s - %s] Kernel timer was active and has been stopped.\n", GROUP_NAME, MEMBER_NAMES); 

    } else { 

        printk(KERN_INFO "[%s - %s] Kernel timer was not active or already stopped.\n", GROUP_NAME, MEMBER_NAMES); 

    } 

  

    if (sys_health_proc_entry) { 

        proc_remove(sys_health_proc_entry); 

        printk(KERN_INFO "[%s - %s] /proc/%s entry removed.\n", GROUP_NAME, MEMBER_NAMES, PROC_FILENAME); 

    } 

} 

  

module_init(sys_health_init); 

module_exit(sys_health_exit); 

  

MODULE_LICENSE("GPL"); 

MODULE_AUTHOR(MEMBER_NAMES " (" GROUP_NAME ")"); 

MODULE_DESCRIPTION("A Linux kernel module to periodically collect system metrics (CPU, Memory), expose them via /proc, and generate alerts. Disk I/O disabled due to header issues."); 

MODULE_VERSION("0.5.1"); // Incremented version for avenrun extern fix 

 
