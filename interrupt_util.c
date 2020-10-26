/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdlib.h>
#include <metal/machine.h>
#include "interrupt_util.h"

/* prototypes */
void clear_software_interrupt (int hartid);
void set_software_interrupt (int hartid);
void interrupt_global_enable (void);
void interrupt_global_disable (void);
void interrupt_software_enable (void);
void interrupt_software_disable (void);
void interrupt_timer_enable (void);
void interrupt_timer_disable (void);
void interrupt_external_enable (void);
void interrupt_external_disable (void);
void interrupt_local_enable (int id);

/* Globals */
void __attribute__((weak, interrupt)) timer_handler (void);
void __attribute__((weak, interrupt)) external_handler (void);
void __attribute__((weak, interrupt)) default_vector_handler (void);
void __attribute__((weak)) default_exception_handler(void);

uint32_t plic_interrupt_lines[METAL_MAX_GLOBAL_EXT_INTERRUPTS];


void __attribute__((weak, interrupt)) timer_handler (void) {
    /* Add functionality if desired */
    while (1);
}

void __attribute__((weak, interrupt)) default_vector_handler (void) {
    /* Add functionality if desired */
    while (1);
}

void __attribute__((weak, interrupt)) external_handler (void) {
    /* Add functionality if desired */
    while (1);
}

void __attribute__((weak)) default_exception_handler(void) {

    /* Read mcause to understand the exception type */
    uint32_t mcause = read_csr(mcause);
    uint32_t mepc = read_csr(mepc);
    uint32_t mtval = read_csr(mtval);
    uint32_t code = MCAUSE_CODE(mcause);

    printf ("Exception Hit! mcause: 0x%08x, mepc: 0x%08x, mtval: 0x%08x\n", mcause, mepc, mtval);
    printf ("Mcause Exception Code: 0x%08x\n", code);
    printf("Now Exiting...\n");

    /* Exit here using non-zero return code */
    exit (0xEE);
}

void interrupt_external_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
}

void interrupt_external_disable (void) {
    unsigned long m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_EXT));
}

void interrupt_global_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
}

void interrupt_global_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mstatus, %1" : "=r"(m) : "r"(METAL_MIE_INTERRUPT));
}

void interrupt_software_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
}

void interrupt_software_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_SW));
}

void interrupt_timer_enable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
}

void interrupt_timer_disable (void) {
    uintptr_t m;
    __asm__ volatile ("csrrc %0, mie, %1" : "=r"(m) : "r"(METAL_LOCAL_INTERRUPT_TMR));
}

void interrupt_local_enable (int id) {
    uintptr_t b = 1 << id;
    uintptr_t m;
    __asm__ volatile ("csrrs %0, mie, %1" : "=r"(m) : "r"(b));
}

void set_software_interrupt (int hartid) {
	int32_t *p = (int32_t*) (METAL_RISCV_CLINT0_0_BASE_ADDRESS + 4*hartid);
	*p = 0x1;
}

void clear_software_interrupt (int hartid)
{
    /* the software interrupt must be cleared manually */
    int32_t *p = (int32_t*) (METAL_RISCV_CLINT0_0_BASE_ADDRESS + 4*hartid);
    *p = 0x0;

    /* ensure the clear is seen before we re-enter WFI */
    __asm__ volatile ("fence");
}

