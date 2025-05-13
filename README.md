# SCIA360-Linux-Kernel

# Linux Kernel System Health Monitor

**Group:** Group 8
**Members:** Thomas Rafferty, Anthony Pinon, Kendall Reese
**Date:** May 13, 2025

## Description

This project involves the development of a Linux kernel module designed to periodically collect critical system health metrics. These metrics are then exposed to user-space applications via a `/proc` filesystem entry. The initial goal was to monitor CPU load, memory usage, and disk I/O, and to generate alerts when these metrics exceeded configurable thresholds.

Currently, the module successfully monitors and reports:
* **CPU Load Averages** (1, 5, and 15 minutes)
* **Memory Usage** (Total, Free, and Used in MB)

Disk I/O monitoring was planned but is currently disabled due to persistent issues with missing kernel headers (`linux/genhd.h`) in the development environment.

## Features

* **Periodic Monitoring:** Uses a kernel timer to collect data at regular intervals (configurable via `TIMER_INTERVAL_SECONDS` macro, default is 5 seconds).
* **`/proc` Interface:** Creates a read-only entry at `/proc/sys_health` for user-space to access the latest collected metrics.
* **CPU Load Monitoring:** Reports 1, 5, and 15-minute load averages by accessing the kernel's `avenrun` array.
* **Memory Usage Monitoring:** Reports total, free, and used RAM by using the `si_meminfo()` kernel function.
* **Concurrency Safe:** Uses spinlocks (`DEFINE_SPINLOCK`) to protect shared data structures between the timer callback and `/proc` read operations.
* **Informative Logging:** Uses `printk()` to log module loading/unloading, `/proc` access, timer events, and important status messages, including group identification for traceability.

## Prerequisites

* **Linux System:** A Linux system (preferably a VM for development) with kernel headers installed for the running kernel.
* **Build Tools:** `make`, `gcc`, and other standard build essentials.
* **Kernel Headers:** The `linux-headers-$(uname -r)` package corresponding to your kernel version. *Note: This project encountered issues with incomplete headers in the specific VM environment used for development, particularly the `linux/genhd.h` file.*

## Building the Module

1.  **Clone the repository (or place the files in a directory):**
    ```bash
    # git clone <repository_url>
    # cd <repository_directory>
    ```
2.  **Navigate to the source directory** containing `sys_health_monitor.c` and `Makefile`.
3.  **Compile the module:**
    ```bash
    make clean  # Optional: clean previous builds
    make
    ```
    This will produce the kernel module file `sys_health_monitor.ko`.

## Loading and Using the Module

1.  **Load the module:**
    ```bash
    sudo insmod ./sys_health_monitor.ko
    ```
    *(You can also pass module parameters here if they were implemented, e.g., `mem_threshold=200`)*

2.  **Verify module loading and check kernel messages:**
    ```bash
    dmesg | grep "Group 8"
    ```
    You should see messages indicating the module has loaded, the `/proc` entry is created, and the kernel timer has started. After the timer interval, you'll see "Timer callback fired" messages.

3.  **Read system health metrics:**
    ```bash
    cat /proc/sys_health
    ```
    This will display the latest collected CPU load and memory usage. The Disk I/O section will show a message indicating it's disabled.

4.  **Unload the module:**
    ```bash
    sudo rmmod sys_health_monitor
    ```
    Check `dmesg` again to see unloading messages.

## Current Code Files

* `sys_health_monitor.c`: The C source code for the kernel module.
* `Makefile`: The makefile used to compile the module.

## Challenges Encountered

* **Missing `linux/genhd.h` Header:**
    * **Symptom:** Persistent compilation error "fatal error: linux/genhd.h: No such file or directory". This prevented the implementation of the Disk I/O monitoring feature as planned.
    * **Troubleshooting:** Extensive troubleshooting was performed, including verifying the kernel version, checking header paths, multiple attempts to `purge`, `clean`, and `reinstall` specific (`linux-headers-$(uname -r)`) and generic HWE header packages. `apt-get clean` was used to ensure fresh package downloads.
    * **Root Cause Indication:** The command `dpkg -L linux-headers-$(uname -r) | grep genhd.h` consistently produced no output, indicating that the system's package manager database did not recognize `genhd.h` as being provided by the installed kernel headers package, despite `apt` reporting successful download and unpacking of the package. This pointed to a specific, unresolved issue with the package registration or contents within the development VM environment.
    * **Workaround:** The Disk I/O collection code was commented out to allow the rest of the module (CPU, Memory, `/proc` interface, timer) to be developed and tested.
* **Undeclared `avenrun`:**
    * **Symptom:** Compilation error "error: 'avenrun' undeclared".
    * **Resolution:** Added `extern unsigned long avenrun[];` to the C code to explicitly declare this kernel global variable, allowing the linker to find it.

## Future Work

* **Resolve Kernel Header Issues:** Investigate the root cause of the missing `genhd.h` in the development environment (or test in a fresh VM/system) to enable Disk I/O monitoring.
* **Implement Disk I/O Monitoring:** Once headers are accessible, complete the Disk I/O statistics collection (e.g., sectors read/written for block devices).
* **Configurable Thresholds:** Implement module parameters (e.g., `mem_threshold_percent`, `cpu_load_threshold_x100`) to allow users to set alert thresholds when loading the module.
* **Alerting Mechanism:** Use `printk()` with `KERN_WARNING` or higher severity to log alerts to the kernel ring buffer when collected metrics exceed the configured thresholds.
* **Expand Metrics:** Consider adding other relevant system metrics (e.g., network statistics, process counts).
* **Robustness:** Improve error handling and potentially support monitoring multiple disk devices dynamically.

## License

This project is licensed under the **GNU General Public License v2.0 (GPL-2.0)**, consistent with Linux kernel module licensing.




