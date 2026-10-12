/*
 * 30장: FreeRTOS 설정 (27장 설정 + 틱 없는 Idle) (STM32F411, HSI 16 MHz)
 *
 * 각 항목의 뜻은 FreeRTOS 커널의 examples/template_configuration/FreeRTOSConfig.h와
 * https://www.freertos.org/a00110.html 을 참고한다.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ---- 스케줄러 ---- */
#define configCPU_CLOCK_HZ                      16000000UL
#define configTICK_RATE_HZ                      1000        /* 틱 1 ms */
#define configUSE_PREEMPTION                    1           /* 선점형 */
#define configUSE_TIME_SLICING                  1           /* 같은 우선순위끼리 틱마다 번갈아 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1           /* CLZ 명령으로 가장 높은 우선순위를 빨리 찾는다 */
#define configMAX_PRIORITIES                    6
#define configMINIMAL_STACK_SIZE                128         /* 워드 단위: 128 × 4 = 512 B */
#define configMAX_TASK_NAME_LEN                 12
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                 1
/* 할 일이 없으면 틱 인터럽트까지 멈추고 WFI로 잠든다 (11장, 30.6절).
   포트의 vPortSuppressTicksAndSleep()이 SysTick을 다시 설정해 잃어버린 틱을 보정한다 */
#define configUSE_TICKLESS_IDLE                 1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP   2

/* ---- 메모리: heap_4.c를 쓴다 ---- */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0
#define configTOTAL_HEAP_SIZE                   (24 * 1024)

/* ---- 기능 ---- */
#define configUSE_MUTEXES                       1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            256
#define configUSE_TRACE_FACILITY                1           /* uxTaskGetSystemState() */
#define configUSE_CO_ROUTINES                   0
#define configUSE_NEWLIB_REENTRANT              0           /* 출력은 uart_dma.c의 뮤텍스가 한 번에 한 태스크만 */

/* ---- 디버깅용 훅 ---- */
#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            1
#define configCHECK_FOR_STACK_OVERFLOW          2           /* 문맥 전환 때 스택 끝 16바이트 무늬를 검사 */

void freertos_assert_failed(const char *file, int line);
#define configASSERT(x) do { if ((x) == 0) { freertos_assert_failed(__FILE__, __LINE__); } } while (0)

/* ---- 인터럽트 우선순위: STM32F4는 우선순위 비트 4개 (0이 가장 높음) ---- */
#define configPRIO_BITS                              4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY       15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5      /* 5~15의 ISR만 FromISR API를 부를 수 있다 */
#define configKERNEL_INTERRUPT_PRIORITY      (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ---- 포트의 예외 처리기를 startup.c의 벡터 이름에 바로 연결한다 ---- */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* ---- 쓸 API ---- */
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_xTaskDelayUntil             1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetIdleTaskHandle      1
#define INCLUDE_xSemaphoreGetMutexHolder    1
#define configUSE_EVENT_GROUPS              1

#endif
