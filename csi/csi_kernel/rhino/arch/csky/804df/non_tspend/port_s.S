/*
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <k_config.h>
#include <csi_config.h>

.extern g_active_task
.extern g_preferred_ready_task
.import csky_deal_tee_caller_task
.import csky_get_tee_caller_task

/******************************************************************************
 *                                 EXPORT FUNCTIONS
 ******************************************************************************/

.global cpu_intrpt_save
.global cpu_intrpt_restore
.global cpu_task_switch
.global cpu_intrpt_switch
.global cpu_first_task_start
.import g_irqvector

/******************************************************************************
 *                                 EQUATES
 ******************************************************************************/

/******************************************************************************
 *                                 CODE GENERATION DIRECTIVES
 ******************************************************************************/

.text
.align 2

/******************************************************************************
 * Functions:
 *     size_t cpu_intrpt_save(void);
 *     void   cpu_intrpt_restore(size_t psr);
 ******************************************************************************/

.type cpu_intrpt_save, %function
cpu_intrpt_save:
    mfcr    r0, psr
    psrclr  ie
    rts

.type cpu_intrpt_restore, %function
cpu_intrpt_restore:
    mtcr    r0, psr
    rts

/******************************************************************************
 * Functions:
 *     void cpu_intrpt_switch(void);
 *     void cpu_task_switch(void);
 ******************************************************************************/

.type cpu_task_switch, %function
cpu_task_switch:
    lrw     r0, g_intrpt_nested_level
    ldb     r0, (r0)
    cmpnei  r0, 0
    jbf     __task_switch

    lrw     r0, g_active_task
    lrw     r1, g_preferred_ready_task
    ldw     r2, (r1)
    stw     r2, (r0)

    rts

.type cpu_intrpt_switch, %function
cpu_intrpt_switch:
    lrw     r0, g_active_task
    lrw     r1, g_preferred_ready_task
    ldw     r2, (r1)
    stw     r2, (r0)

    rts

/******************************************************************************
 * Functions:
 *     void cpu_first_task_start(void);
 ******************************************************************************/

.type cpu_first_task_start, %function
cpu_first_task_start:
    psrclr  ie
    jbr     __task_switch_nosave

/******************************************************************************
 * Functions:
 *     void __task_switch(void);
 ******************************************************************************/

.type __task_switch, %function
__task_switch:
    subi     sp, 196
    stm      r0-r13, (sp)
    stw      r15, (sp, 56)
    addi     r0, sp, 60
    stm      r16-r31, (r0)
    addi     r0, 64
    fstms    vr0-vr15, (r0)
    mfcr     r1, psr
    stw      r1, (r0, 64)
    stw      r15, (r0, 68)

    lrw      r2, g_active_task
    ldw      r2, (r2)
    stw      sp, (r2)

#if (RHINO_CONFIG_TASK_STACK_OVF_CHECK > 0)
    jbsr     krhino_stack_ovf_check
#endif

__task_switch_nosave:

#ifdef CONFIG_SUPPORT_REE_SCHEDULE_IN_TEE
    jbsr     csky_get_tee_caller_task
#endif

    lrw      r4, g_preferred_ready_task
    lrw      r5, g_active_task
    ldw      r6, (r4)
    stw      r6, (r5)

#ifdef CONFIG_SUPPORT_REE_SCHEDULE_IN_TEE
    jbsr     csky_deal_tee_caller_task
#endif

#ifdef CONFIG_STACK_GUARD
    jbsr     csky_set_stackbound
#endif
    ldw      sp, (r6)

#ifdef CONFIG_STACK_GUARD
    mfcr     r3, cr<0, 4>
    bseti    r3, 0
    bseti    r3, 1
    mtcr     r3, cr<0, 4>
#endif

    ldw      r0, (sp, 192)
    mtcr     r0, epc
    ldw      r0, (sp, 188)
    mtcr     r0, epsr

    ldm      r0-r13, (sp)
    ldw      r15, (sp, 56)
    addi     sp, 60
    ldm      r16-r31, (sp)
    addi     sp, 64
    fldms    vr0-vr15, (sp)
    addi     sp, 72

    rte

/******************************************************************************
 * Functions:
 *     void NOVIC_IRQ_Default_Handler(void);
 * novic default irq entry
 ******************************************************************************/

.global NOVIC_IRQ_Default_Handler
.type   NOVIC_IRQ_Default_Handler, %function
NOVIC_IRQ_Default_Handler:
    psrset   ee
    subi    sp, 196
    stm     r0-r13, (sp)
    stw     r15, (sp, 56)
    addi    r0, sp, 60
    stm     r16-r31, (r0)
    addi    r0, 64
    fstms   vr0-vr15, (r0)
    mfcr    r1, epsr
    stw     r1, (r0, 64)
    mfcr    r1, epc
    stw     r1, (r0, 68)

    lrw      r2, g_active_task
    ldw      r2, (r2)
    stw      sp, (r2)

#ifdef CONFIG_STACK_GUARD
    lrw      r0, g_top_irqstack
    mtcr     r0, cr<1, 4>
    subi     r0, CONFIG_ARCH_INTERRUPTSTACK
    mtcr     r0, cr<2, 4>

    mfcr     r0, cr<0, 4>
    bseti    r0, 0
    bseti    r0, 1
    mtcr     r0, cr<0, 4>
#endif

    lrw      r0, g_active_task
    ldw      r0, (r0)
    stw      sp, (r0)

    lrw      sp, g_top_irqstack

#if (RHINO_CONFIG_TASK_STACK_OVF_CHECK > 0)
    jbsr     krhino_stack_ovf_check
#endif

#ifdef CONFIG_SUPPORT_REE_SCHEDULE_IN_TEE
    jbsr     csky_get_tee_caller_task
#endif

    lrw      r1, g_irqvector
    mfcr     r0, psr
    lsri     r0, 16
    sextb    r0
    subi     r0, 32
    lsli     r0, 2
    add      r1, r0
    ldw      r5, (r1)
    lsri     r0, 2
    mov      r4, r0
    jbsr     krhino_intrpt_enter_hook
    mov      r0, r4
    jsr      r5
    mov      r0, r4
    jbsr     krhino_intrpt_exit_hook

#ifdef CONFIG_SUPPORT_REE_SCHEDULE_IN_TEE
    jbsr     csky_deal_tee_caller_task
#endif

#ifdef CONFIG_STACK_GUARD
    jbsr     csky_set_stackbound
#endif

    lrw      r0, g_active_task
    ldw      r0, (r0)
    ldw      sp, (r0)

#ifdef CONFIG_STACK_GUARD
    mfcr     r3, cr<0, 4>
    bseti    r3, 0
    bseti    r3, 1
    mtcr     r3, cr<0, 4>
#endif

    ldw     r0, (sp, 192)
    mtcr    r0, epc
    ldw     r0, (sp, 188)
    mtcr    r0, epsr

    ldm     r0-r13, (sp)
    ldw     r15, (sp, 56)
    addi    sp, 60
    ldm     r16-r31, (sp)
    addi    sp, 64
    fldms   vr0-vr15, (sp)
    addi    sp, 72

    rte
