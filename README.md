# Emulating MPS2 Cortex M3 AN385 on QEMU

## Requirements
1. GNU Arm Embedded Toolchain download [here](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
3. qemu-arm-system download [here](https://www.qemu.org/download)
2. Make (tested on version 3.82)
4. Linux OS (tested on Ubuntu 18.04)

## How to download
Navigate to a parent directory of your choice and run the following command
```
$ git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules --depth 1
```
The previous command should create a directory named **FreeRTOS**

### How to build the application
Navigate with the command line to FreeRTOS/App/CORTEX\_M3\_MPS2\_QEMU\_GCC
For build run:

```
$ export PATH=<pathtoarmtoolchain>:$PATH
# <pathtoarmtoolchain>: <full_path>.../Source/portable/GCC/ARM_CM3

$ make
```
For a versions with debugging symbols and no optimizations **-O0**, run:
```
$ make DEBUG=1
```

### How to run the app
run:
```
$ make run
```
### Application Expectations
after running the app you should be seeing the logs with respect to the scheduling of the task set that's executed

### Users Manual
In order to use the scheduler the very first thing to do is to set the constant
SCHEDULING_ALGORITHM in scheduler.h

After you can write your own task function or use the already existing one in
main_app.c (dispatchTask)

Then inside the main_app function in main_app.c, call these functions in an order:
    1) SchedulerInit();
    2) SchedulerTaskCreate(/*with your own set of parameters and task func*/);
       repeat this function call as many times as the tasks you would like to have.
    3) SchedulerStart();

Now once you run the command make run, in the project directory from the terminal, you will see the scheduler working.


## How to start debugging
1. gdb
<P>
Append the -s and -S switches to the previous command (qemu-system-arm)<br>
-s: allow gdb to be attached to the process remotely at port 1234 <br>
-S: start the program in the paused state <br>

run: (make sure you build the debug version)
```
$ arm-none-eabi-gdb -q ./build/RTOSApp.axf

(gdb) target remote :1234
(gdb) break main
(gdb) c
```
