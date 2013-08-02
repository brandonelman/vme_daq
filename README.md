vme_daq
=======

Attempting to create a DAQ Program for use with a VME System



VME CRATE LAYOUT:
-----------------

 1. VME controller , VX2718 
   * Base Address: 0x00000000
   * Optical link.

 2. Waveform Digitizer , V1729A
   * Base Address: 0x30010000
   * IRQ Enabled: 3

 3. C.F.D., V812
   * Constant Fraction Discriminator 
   * Base Address: 0xEE000000
   * Accepts 16 Negative Inputs 
   * Produces 16 ECL Outputs, adjustable width from 15ns to 250 ns
   * Thresholds can be set between -1 mV to -255 mV (1 mV steps) 
   * Adjustable Dead time, MAJORITY threshold, and pattern of inhibit 

 4. Leading Edge, V895 
   * Base Address: 0x22000000
   * Leading Edge Discriminator 
  
 5. General Purpose VME Board, V1495
   * Base Address: 0xAAAA0000
    
