/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
/**********************************************************************
 *
 * test-wfi-multicore.c
 *
 * Multicore WFI test based on the machine software interrupt. 
 *
 **********************************************************************/
/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>
#include <metal/machine/platform.h>
#include <metal/machine/inline.h>
#include "interrupt_util.h"

/* typedefs */
typedef enum {ERR_NONE=0, ERR_INTERRUPTS_DISABLED, ERR_INTERRUPTS_ENABLED} error_t;
typedef enum {TEST_MIE_DISABLED=0, TEST_MIE_ENABLED} test_t;
typedef enum {FALSE=0, TRUE} bool_t;

/* defines */
#define NUM_TESTS   2   /* two tests: TEST_MIE_DISABLED and TEST_MIE_ENABLED */
#define NUM_WFI     5   /* each test puts the other harts through 5 WFI wakeups */

/* function prototypes */
int hart_checkin_count (void);
void secondary_main(void);
void synchronize_harts (int hartid);
void test_init (int hartid);
error_t test_driver_hart_0 (test_t test);
error_t test_driver_hart_last (test_t test);
void test_taker (int hartid, test_t test);
void test_wfi (int hartid, test_t test);
void __attribute__((weak, interrupt)) __mtvec_clint_vector_table(void);
void __attribute__((interrupt)) software_handler (void);
__attribute__ ((optimize(0))) void wait (int count);

/* globals */
int global_error[NUM_TESTS];                        /* only written by hart_0 and hart_last */
bool_t harts_go;                                    /* only written by hart_0 and hart_last */
volatile bool_t hart_checkin[__METAL_DT_MAX_HARTS]; /* each hart only writes its index */
volatile int msi_count[__METAL_DT_MAX_HARTS];       /* each hart only writes its index */
volatile int wfi_count[__METAL_DT_MAX_HARTS];       /* each hart only writes its index */


/**********************************************************************
 *
 * Function:    main()
 *
 * Description: Test executive run by all harts.  Only hart_0 will
 *              return.  The rest are parked at WFI.
 *
 * Arguments:   none
 *
 * Returns:     ERR_NONE, if no error 
 *              non-zero for error (ERR_INTERRUPTS_DISABLED and
 *              ERR_INTERTUPTS_ENABLED for each respective test)
 *
 **********************************************************************/
int main() {

    /* this is multicore test... each hart has a unique hartid */
    int hartid=metal_cpu_get_current_hartid();

    /* initialize the test */
    test_init(hartid);

    /* make sure all harts are synchronized before test starts */
    synchronize_harts(hartid);

    /* run the test with MIE disabled */
    test_wfi (hartid, TEST_MIE_DISABLED);

    /* make sure all harts are synchronized before next test starts */
    synchronize_harts(hartid);

    /* run the test with MIE enabled */
    test_wfi (hartid, TEST_MIE_ENABLED);

    /* make sure testing is completed across all harts before returning results */
    synchronize_harts(hartid);

    /* we only want one return() to be executed for simulation purposes */
    /* use a while() here in case a debugger gets attached */
    while (hartid != 0) {
        /* park all harts but hart_0 at WFI */
        __asm__ volatile ("wfi");
    }

    /* hart_0 returns the composite test result */
    return (global_error[0]+global_error[1]);
}


/**********************************************************************
 *
 * Function:    test_wfi ()
 *
 * Description: Run two subtests:
 *              1) hart_0 is the driver and [hart_1 - hart_last] go 
 *                 in and out of WFI  
 *              2) hart_last is the driver and [hart_0 - hart_(last-1)] 
 *                 go in and out of WFI  
 *
 *              Note - this is executed by all harts.
 * 
 * Arguments:   hartid, the hart executing the function
 *              test, the test to execute 
 *
 * Returns:     void (the test result is stored in global_error[])
 *
 **********************************************************************/
void test_wfi (int hartid, test_t test) {

    error_t error = ERR_NONE;

    if (test == TEST_MIE_DISABLED) {
        /* Write mstatus.mie = 0 to disable all machine interrupts */
        interrupt_global_disable();
    }
    else {
        /* default to running TEST_MIE_ENABLED */
        /* Write mstatus.mie = 1 to enable all machine interrupts */
        interrupt_global_enable();
    }

    /* we run two sub-tests */
    /* SUBTEST 1: hart_0 is the test giver, all other harts are tested */
    if (hartid == 0) {
        /* run test driver for hart_0 */
        error = test_driver_hart_0(test);
        global_error[test] = error;
    }
    else {
        /* all other harts take the test */
        test_taker(hartid, test);
    }

    /* sync the harts... */
    /* hart_0 is flipping from a giver to a taker */
    /* hart_last is flipping from a taker to a giver */
    synchronize_harts(hartid);

    /* SUBTEST 2: hart_last is the test giver, all other harts are tested */
    if (error == ERR_NONE) {
        /* repeat the same test with the last hart as the test driver */
        if (hartid == (__METAL_DT_MAX_HARTS-1)) {
            error = test_driver_hart_last(test);
            global_error[test] = error;
        }
        else {
            /* all other harts take the test */
            test_taker(hartid, test);
        }
    }

}


/**********************************************************************
 *
 * Function:    test_driver_hart_0()
 *
 * Description: The test driver when hart_0 is the giver.
 *
 * Arguments:   test, the test to execute 
 *
 * Returns:     ERR_NONE, if no error
 *              ERR_INTERRUPTS_DISABLED, if TEST_MIE_DISABLED failed
 *              ERR_INTERRUPTS_ENABLED, if TEST_MIE_ENABLED failed
 *
 **********************************************************************/
error_t test_driver_hart_0 (test_t test) {

    int i, j;
    error_t error = ERR_NONE;

    /* make doubly sure other harts are already at WFI */
    wait(500);   	

    /* clear the synchronize flag */
    harts_go = FALSE;

    /* send each hart a software interrupt NUM_WFI times */
    for (i=0; i<NUM_WFI; i++) {
        for (j=1; j<__METAL_DT_MAX_HARTS; j++) {
            set_software_interrupt(j);
        }
        /* give time to the test takers to respond and return to WFI */
        wait(500);
    }

    if (test == TEST_MIE_DISABLED) 	{
        /* check that all harts ONLY woke up from WFI... NUM_WFI times */
        for (j=1; j<__METAL_DT_MAX_HARTS; j++) {
            if ( (wfi_count[j] != NUM_WFI) && (msi_count[j] == 0) ) {
                /* test failed */
                error = ERR_INTERRUPTS_DISABLED;
                break;
            }  
        }  
    }  
    else {
        /* this is TEST_MIE_ENABLED test */
        /* check all harts woke up from WFI and serviced their software handler */
        for (j=1; j<__METAL_DT_MAX_HARTS; j++) {
            if ( (wfi_count[j] != NUM_WFI) || (msi_count[j] != NUM_WFI) ) {
                /* test failed */
                error = ERR_INTERRUPTS_ENABLED;
                break;
            }  
        }  
    }  

    return(error);
}


/**********************************************************************
 *
 * Function:    test_driver_hart_last()
 *
 * Description: The test driver when hart_last is the giver.
 *
 * Arguments:   test, the test to execute 
 *
 * Returns:     ERR_NONE, if no error
 *              ERR_INTERRUPTS_DISABLED, if TEST_MIE_DISABLED failed
 *              ERR_INTERRUPTS_ENABLED, if TEST_MIE_ENABLED failed
 *
 **********************************************************************/
error_t test_driver_hart_last (test_t test) {
    int i, j;
    error_t error = ERR_NONE;

    /* clear the synchronization flag */
    harts_go = FALSE;

    /* make doubly sure other harts are already at WFI */
    wait(500);

    /* send each hart a software interrupt NUM_WFI times */
    for (i=0; i<NUM_WFI; i++) {
        for (j=0; j<__METAL_DT_MAX_HARTS-1; j++) {
            set_software_interrupt(j);
        }
        /* give time to the test takers to respond and return to WFI */
        wait(500);
    }

    if (test == TEST_MIE_DISABLED) 	{
        /* check that all harts ONLY woke up from WFI... NUM_WFI times */
        for (j=0; j<__METAL_DT_MAX_HARTS-1; j++) {
            if ( (wfi_count[j] != NUM_WFI) && (msi_count[j] == 0) ) {
                /* test failed */
                error = ERR_INTERRUPTS_DISABLED;
                break;
            }
        }
    }
    else {
        /* this is TEST_MIE_ENABLED test */
        /* check all harts woke up from WFI and serviced their software handler */
        for (j=0; j<__METAL_DT_MAX_HARTS-1; j++) {
            if ( (wfi_count[j] != NUM_WFI) || (msi_count[j] != NUM_WFI) ) {
                /* test failed */
                error = ERR_INTERRUPTS_ENABLED;
                break;
            }
        }
    }

    return (error);
}


/**********************************************************************
 *
 * Function:    test_taker()
 *
 * Description: The WFI test loop for harts under test. 
 *              This is run simultaneously by all harts under test.
 *
 * Arguments:   hartid, the hart taking the test
 *              test, the text to execute 
 *
 * Returns:     void
 *
 **********************************************************************/
void test_taker (int hartid, test_t test) {

    /* each hart clears its counters */
    msi_count[hartid] = 0x0;
    wfi_count[hartid] = 0x0;

    /* enter the WFI test loop */
    while (wfi_count[hartid] < NUM_WFI) {
        __asm__ volatile ("wfi");
        wfi_count[hartid] += 1;
        if (test == TEST_MIE_DISABLED) {
            /* must manually clear the interrupt */
            clear_software_interrupt(hartid);
        }
    }

}


/**********************************************************************
 *
 * Function:    test_init()
 *
 * Description: Set the vector table and handling mode.  Enable the 
 *              software interrupt used to bring harts out of WFI.
 *
 * Arguments:   hartid, the hart to init
 *
 * Returns:     void
 *
 **********************************************************************/
void test_init (int hartid) {

    uint32_t i, mode = MTVEC_MODE_CLINT_VECTORED;
    uintptr_t mtvec_base;

    /* Write mstatus.mie = 0 to disable all machine interrupts prior to setup */
    interrupt_global_disable();

    /* Setup mtvec to point to our exception handler table using mtvec.base,
     * and assign mtvec.mode = 1 for CLINT vectored mode of operation.  */
    mtvec_base = (uintptr_t)&__mtvec_clint_vector_table;
    write_csr (mtvec, (mtvec_base | mode));

    /* make sure there aren't any lingering SIPs set from startup */
    clear_software_interrupt (hartid);

    /* enable only software interrupts in mie register */
    interrupt_software_enable();

}


/**********************************************************************
 *
 * Function:    software_handler()
 *
 * Description: Interrupt handler for the machine software interrupt.
 *              This is common to all harts.
 *
 **********************************************************************/
void __attribute__((interrupt)) software_handler (void) {

    /* increment this hart's software interrupt counter */
    int hartid = metal_cpu_get_current_hartid();
    msi_count[hartid] += 1;

    /* clear the interrupt... must be done manually */
    int32_t *p = (int32_t*) (METAL_RISCV_CLINT0_0_BASE_ADDRESS + 4*hartid);
    *p = 0x0;

    /* ensure the clear is seen before we re-enter WFI */
    /* REQUIRED */
    __asm__ volatile ("fence");
}


/**********************************************************************
 *
 * Function:    synchronize_harts()
 *
 * Description: Get all harts lined up for test execution.  
 *              This is run simultaneously by all harts.
 *              hart_0 checks that all harts have checked in, before
 *              letting them leave.
 *
 * Arguments:   hartid, the hart getting synchronized
 *
 **********************************************************************/
void synchronize_harts (int hartid) {

    /* harts check in... but only check out once ALL have checked in */
    hart_checkin[hartid] = TRUE;

    /* hart_0 confirms all harts have checked in */
    if (hartid == 0) {
        while (harts_go == FALSE) {
            if (hart_checkin_count() == __METAL_DT_MAX_HARTS) {
                harts_go = TRUE;
            }
        }
    }
    else {
        /* other harts wait for the go flag to be set by hart_0 */
        while (harts_go == FALSE) {
            wait(hartid);
        }
    }

    /* clear the checkin for the next call */
    hart_checkin[hartid] = FALSE;
}


/**********************************************************************
 *
 * Function:    hart_checkin_count()
 *
 * Description: Determine the number of harts that have checked in.
 *
 * Arguments:   void
 *
 * Returns:     the number of harts that have hart_checkin[] = TRUE
 * 
 **********************************************************************/
int hart_checkin_count (void)
{
    int i, count=0;

    for (i=0; i<__METAL_DT_MAX_HARTS; i++) {
        if (hart_checkin[i] == TRUE) {
            count +=1;
        }
    }

    return (count);
}


/**********************************************************************
 *
 * Function:    secondary_main()
 *
 * Description: Override Freedom Metal's secondary_main() as all cores
 *              need to run the test.
 *
 **********************************************************************/
void secondary_main(void) {

    /* do nothing special... just deliver all harts to main() */
    main();
}


/**********************************************************************
 *
 * Function:    wait()
 *
 * Description: Delay processor execution an inexact amount of time.
 *
 * Arguments:   count, the number of willies to wait
 *
 **********************************************************************/
__attribute__ ((optimize(0))) void wait (int count)
{
    while (count-- > 0);
}

