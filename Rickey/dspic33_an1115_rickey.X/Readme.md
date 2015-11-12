## dsPIC33F AN-1115 Demo

Build instructions: Make sure you add the appropriate libraries in MPLAB-X, and select the correct build config. This code should build on both XC16 and C30. 

To add the library, right click on project, go into properties, and add the library/object file for the configuration. 

#### Note
- The libraries of interest is libdsp-elf.a or libdsp-coff.a. It's typically located in C:\Program Files (x86)\Microchip\xc16\v1.25\lib
- Read "16-Bit Language Tools Libraries Reference Manual" for details on their differences. 
- Make sure you select libdsp-elf and NOT libq-dsp-elf! They look similiar and are deceiving. 

