/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name     : irqfiq_handler.s
******************************************************************************
* Device(s)     : RZ/A1L
* Tool-Chain    : GNUARM-NONE-EABI-v16.01
* H/W Platform  : Platform Independent
* Description   : This is the code provides access to the co-processors
*
*                 Errata 733075
*                 Under certain circumstances a software read transaction
*                 from the ICCIAR register results in the GIC returning
*                 the incorrect interrupt ID.
*                 This erratum can result in a deadlock on the CPU interface.
*
*                 Errata 801120
*                 The Interrupt Acknowledge Register (ICCIAR, also known as
*                 GICC_IAR) is a read-sensitive register that returns the ID
*                 of a pending interrupt for a CPU. At the same time it also
*                 acknowledges that interrupt and updates its state.
*                 This defect means that the GIC-390 acknowledges the
*                 interrupt but returns a different interrupt ID.
********************************************************************************/
/*******************************************************************************
* History       : DD.MM.YYYY Version Description
*               : 06.07.2016 1.00    Ported from RZA1H v1.01 (dated 3.7.15)
*******************************************************************************/

/* Standard definitions of mode bits and interrupt (I & F) flags in PSRs */
    .equ    SYS_MODE          , 0x1F
    .equ    INTC_ICCIAR_MASK  , 0x3FF
    .equ    INTC_ICCIAR_ADDR  , 0xE820200C
    .equ    INTC_ICCEOIR_ADDR , 0xE8202010
    .equ    INTC_ICCHPIR_ADDR , 0xE8202018
    .equ    INTC_ICDIPR0_ADDR , 0xE8201400

/* ================================================================== */
/* Entry point for the FIQ handler */
/* ================================================================== */
    .global  irq_handler
    .global  fiq_handler

    .func read_intc_icciar_addr
read_intc_icciar_addr:

    /*; ++REE_SS Addressing ARM Errata 801120 */
    /*; Perform a dummy read to - ensure subsequent ICCIAR data */
    /*; will be correct */
    LDR     r2, =INTC_ICCHPIR_ADDR
    LDR     r2, [r2]
    LDR     r0, [r2]

    /*; Attempt to activate interrupt and get its ID */
    /*; Load in to R3 - if valid it will be used later as ICCIAR */
    LDR     r2, =INTC_ICCIAR_ADDR
    LDR     r3, [r2]

    /*; Extract the interrupt ID (removing the SGI source CPU ID) */
    LDR     r1, =INTC_ICCIAR_MASK
    AND     r3, r3, r1

    /*; Read data of 0x0 (SGI ID0 from CPU0) is possibly affected by 733075 */
    CMP     r3, #0
    BEQ     errata_733075_workaround

    /*; Interrupt IDs 0x3FE and 0x3FF are also possibly affected */
    LDR     r1, =1022
    CMP     r3, r1
    BGE     errata_733075_workaround

    B       post_733075_workaround

errata_733075_workaround:
    /*; Perform a read from ulICDIPR0 and write value back */
    /*; It is sufficient to write the value that is already in the register. */
    /*; You can obtain the value to be written to the ICDIPR0 register by */
    /*; reading from it, or through other means" */
    LDR     r2, =INTC_ICDIPR0_ADDR
    LDR     r0, [r2]
    STR     r0, [r2]
    DSB

    LDR     r2, =INTC_ICCHPIR_ADDR
    LDR     r2, [r2]
    LDR     r0, [r2]

    /*; Attempt to activate interrupt and get its ID */
    /*; Load in to R3 - if valid it will be used later as ICCIAR */
    LDR     r2, =INTC_ICCIAR_ADDR
    LDR     r3, [r2]

post_733075_workaround:
    MOV     r0, r3
    B       fi_common_icciar_read
    .endfunc

    .func fi_common_handler
fi_common_do_intc_handler_interrupt:
    BL      R_INTC_InterruptHandler
    B       fi_common_continue

fi_common_handler:
    B       read_intc_icciar_addr
fi_common_icciar_read:
    PUSH    {r0}
    MOV     r1, sp
    AND     r1, r1, #4
    SUB     sp, sp, r1
    PUSH    {r1, lr}
    CMP     r4, #0
    BEQ     fi_common_do_intc_handler_interrupt
    BL      R_INTC_FiqHandler
fi_common_continue:
    POP     {r1, lr}
    ADD     sp, sp, r1
    POP     {r0}
    LDR     r2, =INTC_ICCEOIR_ADDR
    STR     r0, [r2]
fi_common_handler_end:
    POP     {r0-r4, r12}
    RFEIA   sp!
    .endfunc

/* ================================================================== */
/* Entry point for the FIQ handler */
/* ================================================================== */
    .func fiq_handler
fiq_handler:
    SUB     lr, lr, #4
    SRSDB   sp!, #SYS_MODE
    CPS     #SYS_MODE
    PUSH    {r0-r4, r12}
    MOV     r4, #1
    B       fi_common_handler
    .endfunc

/* ================================================================== */
/* Entry point for the IRQ handler */
/* ================================================================== */
    .func irq_handler
irq_handler:
    SUB     lr, lr, #4
    SRSDB   sp!, #SYS_MODE
    CPS     #SYS_MODE
    PUSH    {r0-r4, r12}
    MOV     r4, #0
    B       fi_common_handler
    .endfunc

    .end

