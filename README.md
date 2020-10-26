# test-wfi-multicore

A mulitcore WFI test that ensures that all harts in the system are able to
break out of WFI with machine interrupts enabled and disabled. Hart_0 is used 
as a test conductor to test Hart_1 to Hart_Last.  Hart_Last is used as test
conductor to test Hart_0 to Hart_(Last-1).  

Note: The test requires that a Core-Local Interruptor (CLINT) exists in the design.

Test Result

The test result is 0 for success and non-zero for fail.  For test failures, the 
possible results are:
 1 if only the TEST_MIE_DISABLED failed
 2 if only the TEST_MIE_ENABLED failed
 3 if both tests failed

Test Deployment

In a RTL environment, the files can be incorporated into your tarball workspace and 
executed like a standard example delivered with the tarball.  Follow these steps:
1) Copy all files into a new folder called sifive_coreip/freedom-e-sdk/software/test-wfi-multicore
2) Build the test from the sifive_coreip/freedom-e-sdk level:
   make PROGRAM=test-wfi-multicore TARGET=design-rtl CONFIGURATION=release software
3) Execute the test at the sifive_coreip level:
   make test-wfi-multicore.vcs.out 

In a Freedom Studio environment for testing on FPGA, the source files can be added to 
a new project based on your IP Deliverable. The test requires freedom-metal.  
