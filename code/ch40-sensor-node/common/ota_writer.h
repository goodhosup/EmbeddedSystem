/*
 * 40장: 센서 노드가 링크로 받은 이미지를 다른 슬롯에 쓰는 모듈
 *
 *   ota_begin  다른 슬롯을 지운다 (32장 배치, flash_if.h)
 *   ota_chunk  오프셋 순서대로 쓴다 (링크의 ARQ가 순서와 중복을 보장하지만, 여기서도 확인한다)
 *   ota_end    다 받았는지, 서명이 맞는지(33장) 확인한 뒤 시험 부팅을 요청한다(32장)
 * 서명 확인은 함수 포인터로 받는다: 보드는 Mbed TLS의 secure_verify, PC 테스트는 가짜.
 */
#ifndef OTA_WRITER_H
#define OTA_WRITER_H

#include <stdint.h>

typedef int (*ota_verify_fn)(uint32_t slot_addr, uint32_t size);   /* 0이면 통과 */

typedef enum { OTA_IDLE, OTA_RECEIVING, OTA_DONE, OTA_ERROR } ota_state_t;

typedef enum {
    OTA_OK = 0,
    OTA_E_STATE = -1,      /* 순서가 틀린 명령 (BEGIN 없이 CHUNK 등) */
    OTA_E_SIZE = -2,
    OTA_E_OFFSET = -3,     /* 빠지거나 겹친 조각 */
    OTA_E_FLASH = -4,
    OTA_E_VERIFY = -5,     /* 서명이 맞지 않음: 시험 부팅을 요청하지 않는다 */
} ota_err_t;

typedef struct {
    ota_state_t   state;
    int           slot;
    uint32_t      base, size, next_off;
    uint32_t      version;
    ota_verify_fn verify;
} ota_t;

void      ota_init(ota_t *o, ota_verify_fn verify);
ota_err_t ota_begin(ota_t *o, int target_slot, uint32_t size, uint32_t version);
ota_err_t ota_chunk(ota_t *o, uint32_t offset, const uint8_t *data, uint16_t n);
ota_err_t ota_end(ota_t *o);      /* 성공하면 부트 상태에 pending = 새 슬롯을 기록했다 */

#endif
