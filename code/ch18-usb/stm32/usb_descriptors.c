/*
 * USB 디스크립터 (18.4절): CDC ACM 가상 시리얼 장치 하나
 *
 * VID/PID: 0x1209/0x0001은 pid.codes가 시험용으로 열어 둔 번호다. 혼자 시험하는 데만 쓰고,
 * 제품에는 자기 회사의 VID를 쓰거나 pid.codes 등에서 PID를 받아야 한다(18.9절).
 * [확인 필요: pid.codes 시험용 PID 정책]
 */
#include <string.h>

#include "tusb.h"

#define USB_VID 0x1209
#define USB_PID 0x0001

static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,          /* 인터페이스 묶음(IAD)을 쓰는 복합 장치 */
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&desc_device;
}

enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_TOTAL };

#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t desc_configuration[] = {
    /* 구성: 번호 1, 인터페이스 2개, 문자열 없음, 전체 길이, 속성 없음, 최대 100 mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: 인터페이스 번호, 문자열 4, 알림 EP와 크기, 데이터 OUT/IN EP, 크기 */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return desc_configuration;
}

static const char *const strings[] = {
    "",                          /* 0: 언어 (아래에서 따로 처리) */
    "Embedded Book",             /* 1: 제조사 */
    "ch18 CDC Demo",             /* 2: 제품 */
    "0001",                      /* 3: 일련번호 */
    "ch18 CDC",                  /* 4: CDC 인터페이스 */
};

static uint16_t desc_str[32];

/* 문자열 디스크립터는 UTF-16LE. 첫 워드 = 길이(바이트) | 종류 << 8 */
const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t n;
    if (index == 0) {
        desc_str[1] = 0x0409;                        /* 영어(미국) */
        n = 1;
    } else {
        if (index >= sizeof(strings) / sizeof(strings[0])) {
            return NULL;
        }
        const char *s = strings[index];
        n = strlen(s);
        if (n > 31) {
            n = 31;
        }
        for (size_t i = 0; i < n; i++) {
            desc_str[1 + i] = (uint8_t)s[i];         /* ASCII → UTF-16 */
        }
    }
    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));
    return desc_str;
}
