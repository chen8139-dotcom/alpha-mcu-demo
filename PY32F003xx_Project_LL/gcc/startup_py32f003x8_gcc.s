/**
  ******************************************************************************
  * @file    startup_py32f003x8_gcc.s
  * @brief   PY32F003x8 vector table for GNU Arm Embedded Toolchain (macOS).
  *          Ported 1:1 from MDK-ARM/startup_py32f003xx.s (armasm dialect).
  *          - Sets the initial SP
  *          - Sets the initial PC == Reset_Handler,
  *            then calls SystemInit / __libc_init_array / main
  *          - Vector table entry names match py32f0xx_it.c handlers
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

/* Linker symbols used by Reset_Handler */
.word  _sidata
.word  _sdata
.word  _edata
.word  _sbss
.word  _ebss

/* --------------------------------------------------------------------------
   Reset Handler
   -------------------------------------------------------------------------- */
.section .text.Reset_Handler,"ax",%progbits
.weak  Reset_Handler
.type  Reset_Handler, %function
Reset_Handler:
  /* Copy .data from flash to ram */
  ldr   r0, =_sdata
  ldr   r1, =_edata
  ldr   r2, =_sidata
  movs  r3, #0
  b     LoopCopyDataInit

CopyDataInit:
  ldr   r4, [r2, r3]
  str   r4, [r0, r3]
  adds  r3, r3, #4

LoopCopyDataInit:
  adds  r4, r0, r3
  cmp   r4, r1
  bcc   CopyDataInit

  /* Zero fill the .bss segment */
  ldr   r2, =_sbss
  ldr   r4, =_ebss
  movs  r3, #0
  b     LoopFillZerobss

FillZerobss:
  str   r3, [r2]
  adds  r2, r2, #4

LoopFillZerobss:
  cmp   r2, r4
  bcc   FillZerobss

  /* Call the clock system initialization function */
  bl    SystemInit
  /* Call static constructors */
  bl    __libc_init_array
  /* Call the application's entry point */
  bl    main
  bx    lr

.size  Reset_Handler, .-Reset_Handler

/* --------------------------------------------------------------------------
   Default Handler (infinite loop), aliased as weak default for every ISR
   -------------------------------------------------------------------------- */
.section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b     Infinite_Loop
.size  Default_Handler, .-Default_Handler

/* Weak core exception handlers */
.weak      NMI_Handler
.thumb_set NMI_Handler, Default_Handler

.weak      HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler

.weak      SVC_Handler
.thumb_set SVC_Handler, Default_Handler

.weak      PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler

.weak      SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler

/* Weak peripheral interrupt handlers (names must match py32f0xx_it.c) */
.weak      WWDG_IRQHandler
.thumb_set WWDG_IRQHandler, Default_Handler

.weak      PVD_IRQHandler
.thumb_set PVD_IRQHandler, Default_Handler

.weak      RTC_IRQHandler
.thumb_set RTC_IRQHandler, Default_Handler

.weak      FLASH_IRQHandler
.thumb_set FLASH_IRQHandler, Default_Handler

.weak      RCC_IRQHandler
.thumb_set RCC_IRQHandler, Default_Handler

.weak      EXTI0_1_IRQHandler
.thumb_set EXTI0_1_IRQHandler, Default_Handler

.weak      EXTI2_3_IRQHandler
.thumb_set EXTI2_3_IRQHandler, Default_Handler

.weak      EXTI4_15_IRQHandler
.thumb_set EXTI4_15_IRQHandler, Default_Handler

.weak      DMA1_Channel1_IRQHandler
.thumb_set DMA1_Channel1_IRQHandler, Default_Handler

.weak      DMA1_Channel2_3_IRQHandler
.thumb_set DMA1_Channel2_3_IRQHandler, Default_Handler

.weak      ADC_COMP_IRQHandler
.thumb_set ADC_COMP_IRQHandler, Default_Handler

.weak      TIM1_BRK_UP_TRG_COM_IRQHandler
.thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler, Default_Handler

.weak      TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler, Default_Handler

.weak      TIM3_IRQHandler
.thumb_set TIM3_IRQHandler, Default_Handler

.weak      LPTIM1_IRQHandler
.thumb_set LPTIM1_IRQHandler, Default_Handler

.weak      TIM14_IRQHandler
.thumb_set TIM14_IRQHandler, Default_Handler

.weak      TIM16_IRQHandler
.thumb_set TIM16_IRQHandler, Default_Handler

.weak      TIM17_IRQHandler
.thumb_set TIM17_IRQHandler, Default_Handler

.weak      I2C1_IRQHandler
.thumb_set I2C1_IRQHandler, Default_Handler

.weak      SPI1_IRQHandler
.thumb_set SPI1_IRQHandler, Default_Handler

.weak      USART1_IRQHandler
.thumb_set USART1_IRQHandler, Default_Handler

.weak      USART2_IRQHandler
.thumb_set USART2_IRQHandler, Default_Handler

/* --------------------------------------------------------------------------
   Vector table (mapped to 0x08000000 at reset, aliased at 0x00000000)
   Same order/count as MDK-ARM startup: 16 core + 32 peripheral entries.
   -------------------------------------------------------------------------- */
.section .isr_vector,"a",%progbits
.type  g_pfnVectors, %object

g_pfnVectors:
  .word  _estack                        /*  0: Top of Stack            */
  .word  Reset_Handler                  /*  1: Reset Handler           */
  .word  NMI_Handler                    /*  2: NMI Handler             */
  .word  HardFault_Handler              /*  3: Hard Fault Handler      */
  .word  0                              /*  4: Reserved                */
  .word  0                              /*  5: Reserved                */
  .word  0                              /*  6: Reserved                */
  .word  0                              /*  7: Reserved                */
  .word  0                              /*  8: Reserved                */
  .word  0                              /*  9: Reserved                */
  .word  0                              /* 10: Reserved                */
  .word  SVC_Handler                    /* 11: SVCall Handler          */
  .word  0                              /* 12: Reserved                */
  .word  0                              /* 13: Reserved                */
  .word  PendSV_Handler                 /* 14: PendSV Handler          */
  .word  SysTick_Handler                /* 15: SysTick Handler         */

  /* External Interrupts (IRQ 0..31) */
  .word  WWDG_IRQHandler                /*  0: Window Watchdog         */
  .word  PVD_IRQHandler                 /*  1: PVD through EXTI Line   */
  .word  RTC_IRQHandler                 /*  2: RTC through EXTI Line   */
  .word  FLASH_IRQHandler               /*  3: FLASH                   */
  .word  RCC_IRQHandler                 /*  4: RCC                     */
  .word  EXTI0_1_IRQHandler             /*  5: EXTI Line 0 and 1       */
  .word  EXTI2_3_IRQHandler             /*  6: EXTI Line 2 and 3       */
  .word  EXTI4_15_IRQHandler            /*  7: EXTI Line 4 to 15       */
  .word  0                              /*  8: Reserved                */
  .word  DMA1_Channel1_IRQHandler       /*  9: DMA1 Channel 1          */
  .word  DMA1_Channel2_3_IRQHandler     /* 10: DMA1 Channel 2 and 3    */
  .word  0                              /* 11: Reserved                */
  .word  ADC_COMP_IRQHandler            /* 12: ADC & COMP1             */
  .word  TIM1_BRK_UP_TRG_COM_IRQHandler /* 13: TIM1 Break/Upd/Trg/Com  */
  .word  TIM1_CC_IRQHandler             /* 14: TIM1 Capture Compare    */
  .word  0                              /* 15: Reserved                */
  .word  TIM3_IRQHandler                /* 16: TIM3                    */
  .word  LPTIM1_IRQHandler             /* 17: LPTIM1                  */
  .word  0                              /* 18: Reserved                */
  .word  TIM14_IRQHandler               /* 19: TIM14                   */
  .word  0                              /* 20: Reserved                */
  .word  TIM16_IRQHandler               /* 21: TIM16                   */
  .word  TIM17_IRQHandler               /* 22: TIM17                   */
  .word  I2C1_IRQHandler                /* 23: I2C1                    */
  .word  0                              /* 24: Reserved                */
  .word  SPI1_IRQHandler                /* 25: SPI1                    */
  .word  0                              /* 26: Reserved                */
  .word  USART1_IRQHandler              /* 27: USART1                  */
  .word  USART2_IRQHandler              /* 28: USART2                  */
  .word  0                              /* 29: Reserved                */
  .word  0                              /* 30: Reserved                */
  .word  0                              /* 31: Reserved                */

.size  g_pfnVectors, .-g_pfnVectors

/************************ (C) COPYRIGHT Puya *****END OF FILE****************/
