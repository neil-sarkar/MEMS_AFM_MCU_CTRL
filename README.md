MEMS AFM MCU Control 
====================
## Timer Usage
-Timer 1: 1ms tick for PID, 1ms tick general use (Used by Auto Approach)
-Timer 2: Stepper Motor
-Timer 3: ??? (Cannot use prescaler)
-Timer 4: UART

MCU code for ADUC7122 to control MEMS AFM.

This code works for board-v1 (or board_assem).

The last feature added to board-v1 devices is automatic motor approach to a coarse pid setpoint.
