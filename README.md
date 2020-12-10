
# CS5463-Embedded-System-Final-Project
## Introduction 
• Microcontroller Board: Texas Instruments MSP430FR6989 LaunchPad 
• This project will combine the knowledge of all the mini projects, sensor/adc, and LCD. 
- _Button and LED 
- _UART 
- _Various Interrupts 
- _LCD 
- _ADC

## Requirements
Please use interrupts to realize the following functions:
1) The LCD displays the temperature in oF when button 1 is pressed.
2) The LCD displays the temperature in oC when button 2 is pressed.
3) Create an array of size 10 in FIFO structure to store the temperatures in oC sampled during
past 10 seconds (one per second).
4) When computer sends a command (You can design the command as you like) through UART
to the board, the cpu on the board will calculate the average of the 10 temperatures in oC
collected in the past 10 seconds). The board will then send back the result to your computer.

If you finish the four functions correctly, your group will get full marks for this project.
If you could implement any further function to the system such as follows, you will get bonus.
Bonus tasks: (Will give your some examples soon…)
1) task scheduling
2) semaphore

## Implementation
Note: Due the serial communication need to change clock rate, it is hard to calculate the time. We keep basic requirements, FCFS scheduling and semaphore into seperate projects.

### Basic
1) The LCD displays the temperature in oF when button 1 is pressed.
2) The LCD displays the temperature in oC when button 2 is pressed.
3) Create an array of size 10 in FIFO structure to store the temperatures in oC sampled during
past 10 seconds (one per second).
4) Create UART command based interface. 
When computer sends command '1' through UART to the board, The board will then send back the temperature queue to your computer and displayed in the terminal. 
When computer sends command '2' through UART to the board, the cpu on the board will calculate the average of the 10 temperatures in oC collected in the past 10 seconds). The board will then send back the result to your computer and displayed in the terminal.

### Bonus - FCFS Scheduling
For scheduling algorithm, we've implemented FCFS scheduling.
Our implementation works fine for the dataset provided in the code and also some other datasets that we've tested.
During FCFS, while a process is running, the green LED on the board stays on. when the process finish working and a new process starts, the red LED blink once. THat's an indicator that the earlier process finished working.
If there's no process on the queue, no lights are on. 
We've explained the testcase int the video.

### Bonus - Semaphore
For the second bonus problem, we've implemented semaphore.
Our implementation works fine for the dataset provided in the code and also some other datasets that we've tested.
During Semaphore, while a process is running, the green LED on the board stays on. When the process finish working and a new process starts, there is a little gap while no LED is on.
When one process is accessing the CPU and another process tries, it actually cannot access but goes in the queue. The Red LED blinks once at that time which says that some process tried to access the CPU but couldn't.
If there's no process on the queue, no lights are on. 
We've explained the testcase in the video.
