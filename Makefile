# Makefile for sys_health_monitor kernel module

# The name of your object file (source .c file with .o extension)
# This tells kbuild that we are building a loadable module named sys_health_monitor.o
# from sys_health_monitor.c.
obj-m += sys_health_monitor.o

# Get the current kernel version string (e.g., 6.11.0-25-generic)
# This is used to find the correct kernel build directory.
KVERSION = $(shell uname -r)

# Path to the kernel source/build directory.
# This is where the Makefiles and configurations for building external modules are located.
# For most systems, this points to /lib/modules/$(KVERSION)/build, which is often a
# symbolic link to the actual kernel headers/source directory (e.g., /usr/src/linux-headers-...).
KDIR = /lib/modules/$(KVERSION)/build

# Present Working Directory.
# This variable holds the current directory where your source code and this Makefile reside.
# It's passed to the kernel's Makefile system so it knows where to find your module's source.
PWD = $(shell pwd)

# Default target: 'all'
# This is the target that 'make' will build if no specific target is given on the command line.
# It invokes 'make' again, but this time:
#   -C $(KDIR): Change directory to the kernel build directory.
#   M=$(PWD):  Tell the kernel's Makefile system that the external module source is in our PWD.
#   modules:   The target to build (i.e., compile the kernel module).
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Target for cleaning up build files: 'clean'
# This target is used to remove all compiled files and intermediate files generated during the build.
# It also invokes 'make' in the kernel build directory with the 'clean' target.
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# Optional: convenient targets for loading and unloading the module
# These are not strictly part of the build but can be useful during development.
# You would run them like: 'make load' or 'make unload'

# 'make load': Inserts the compiled module into the kernel.
# '$<' is an automatic variable in Make that refers to the first prerequisite (sys_health_monitor.ko).
# '$(MODULE_PARAMS)' is a placeholder; if your module accepted parameters, you could set this
# variable when calling make, e.g., 'make load MODULE_PARAMS="mem_threshold=200"'
load: sys_health_monitor.ko
	sudo insmod $< $(MODULE_PARAMS)

# 'make unload': Removes the module from the kernel.
unload:
	sudo rmmod sys_health_monitor

# Target to view kernel log messages (useful for debugging)
# 'dmesg -wH' shows kernel messages in a human-readable format and follows new messages.
log:
	dmesg -wH

# Phony targets: These are targets that don't represent actual files.
# It's good practice to declare them to avoid conflicts if files with these names are created.
.PHONY: all clean load unload log

