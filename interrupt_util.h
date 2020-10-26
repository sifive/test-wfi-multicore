/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>

#ifndef INTERRUPT_UTIL
#define INTERRUPT_UTIL

#if __riscv_xlen == 32
#define MCAUSE_INTR                         0x80000000UL
#define MCAUSE_CAUSE                        0x000003FFUL
#else
#define MCAUSE_INTR                         0x8000000000000000UL
#define MCAUSE_CAUSE                        0x00000000000003FFUL
#endif
#define MCAUSE_CODE(cause)                  (cause & MCAUSE_CAUSE)

/* Compile time options to determine which interrupt modules we have */
#define CLINT_PRESENT                       (METAL_MAX_CLINT_INTERRUPTS > 0)
#define CLIC_PRESENT                        (METAL_MAX_CLIC_INTERRUPTS > 0)
#define PLIC_PRESENT                        (METAL_MAX_PLIC_INTERRUPTS > 0)

#define DISABLE              0
#define ENABLE               1
#define RTC_FREQ            32768

/* Interrupt Specific defines - used for mtvec.mode field, which is bit[0] for
 * designs with CLINT, or [1:0] for designs with a CLIC */
#define MTVEC_MODE_CLINT_DIRECT             0x00
#define MTVEC_MODE_CLINT_VECTORED           0x01
#define MTVEC_MODE_CLIC_DIRECT              0x02
#define MTVEC_MODE_CLIC_VECTORED            0x03

#define MACHINE_SOFTWARE_INTERRUPT_PENDING  0x8
#define MACHINE_TIMER_INTERRUPT_PENDING     0x80
#define MACHINE_EXTERNAL_INTERRUPT_PENDING  0x800

/* prototypes */
extern void clear_software_interrupt (int hartid);
extern void set_software_interrupt (int hartid);
extern void interrupt_global_enable (void);
extern void interrupt_global_disable (void);
extern void interrupt_software_enable (void);
extern void interrupt_software_disable (void);
extern void interrupt_timer_enable (void);
extern void interrupt_timer_disable (void);
extern void interrupt_external_enable (void);
extern void interrupt_external_disable (void);
extern void interrupt_local_enable (int id);

/* Defines to access CSR registers within C code */
#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

/* Globals */
void __attribute__((weak, interrupt)) timer_handler (void);
void __attribute__((weak, interrupt)) external_handler (void);
void __attribute__((weak, interrupt)) default_vector_handler (void);
void __attribute__((weak)) default_exception_handler(void);

#endif
