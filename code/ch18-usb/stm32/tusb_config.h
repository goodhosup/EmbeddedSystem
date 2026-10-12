/* TinyUSB 설정 (18장): 장치 모드, Full Speed, CDC 하나 */
#ifndef TUSB_CONFIG_H
#define TUSB_CONFIG_H

#define CFG_TUSB_OS              OPT_OS_NONE
#define BOARD_TUD_RHPORT         0
#define CFG_TUD_ENABLED          1
#define CFG_TUD_MAX_SPEED        OPT_MODE_FULL_SPEED
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)   /* 옛 버전 호환 */
#define CFG_TUSB_MEM_ALIGN       __attribute__((aligned(4)))

#define CFG_TUD_ENDPOINT0_SIZE   64

#define CFG_TUD_CDC              1
#define CFG_TUD_CDC_RX_BUFSIZE   64
#define CFG_TUD_CDC_TX_BUFSIZE   64
#define CFG_TUD_CDC_EP_BUFSIZE   64

#endif
