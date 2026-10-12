/*
 * 30장: SD 카드의 CSV 파일에 기록을 남기는 모듈 (FatFs 위, PC에서 시험)
 *
 *   logger_start  카드 마운트, 비어 있는 다음 파일 이름(LOG00001.CSV ...)으로 새 파일, 머리줄
 *   logger_write  한 줄 추가. sync_every개마다 f_sync로 디렉터리 항목까지 확정
 *   logger_stop   f_close
 *
 * 전원이 갑자기 끊겨도 마지막 f_sync까지의 기록은 남는다. sync를 자주 할수록 안전하지만
 * 카드에 쓰는 횟수(전력, 수명)가 늘어난다 (30.6절).
 */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdint.h>

#include "ff.h"
#include "logrec.h"

typedef struct {
    FATFS    fs;
    FIL      file;
    char     name[13];        /* 8.3 + 널 */
    bool     open;
    uint32_t unsynced;        /* 마지막 f_sync 뒤 쓴 줄 수 */
    uint32_t sync_every;
    uint32_t lines;
    uint32_t errors;
    FRESULT  last_err;
} logger_t;

FRESULT logger_start(logger_t *lg, uint32_t sync_every);
FRESULT logger_write(logger_t *lg, const logrec_t *r);
FRESULT logger_flush(logger_t *lg);
FRESULT logger_stop(logger_t *lg);

#endif
