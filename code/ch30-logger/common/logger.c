#include "logger.h"

static void make_name(char *name, uint32_t n)       /* LOG00001.CSV */
{
    const char *pre = "LOG";
    for (int i = 0; i < 3; i++) name[i] = pre[i];
    for (int i = 7; i >= 3; i--) {
        name[i] = (char)('0' + n % 10u);
        n /= 10u;
    }
    name[8] = '.'; name[9] = 'C'; name[10] = 'S'; name[11] = 'V'; name[12] = '\0';
}

static FRESULT fail(logger_t *lg, FRESULT fr)
{
    lg->errors++;
    lg->last_err = fr;
    return fr;
}

FRESULT logger_start(logger_t *lg, uint32_t sync_every)
{
    lg->open = false;
    lg->unsynced = 0;
    lg->lines = 0;
    lg->sync_every = sync_every ? sync_every : 1u;

    FRESULT fr = f_mount(&lg->fs, "", 1);          /* 1: 지금 바로 마운트 (카드가 없으면 여기서 실패) */
    if (fr != FR_OK) {
        return fail(lg, fr);
    }
    /* 비어 있는 다음 번호 찾기: 켤 때마다 새 파일 → 지난 기록을 덮어쓰지 않는다 */
    FILINFO fi;
    uint32_t n = 1;
    for (; n <= 99999u; n++) {
        make_name(lg->name, n);
        if (f_stat(lg->name, &fi) == FR_NO_FILE) {
            break;
        }
    }
    if (n > 99999u) {
        return fail(lg, FR_DENIED);
    }
    fr = f_open(&lg->file, lg->name, FA_WRITE | FA_CREATE_NEW);
    if (fr != FR_OK) {
        return fail(lg, fr);
    }
    lg->open = true;

    UINT bw;
    fr = f_write(&lg->file, LOGREC_HEADER, sizeof LOGREC_HEADER - 1u, &bw);
    if (fr == FR_OK) {
        fr = f_sync(&lg->file);                     /* 빈 파일이라도 디렉터리에 바로 남긴다 */
    }
    return (fr == FR_OK) ? FR_OK : fail(lg, fr);
}

FRESULT logger_write(logger_t *lg, const logrec_t *r)
{
    if (!lg->open) {
        return FR_NOT_READY;
    }
    char line[LOGREC_MAX_LINE];
    size_t len = logrec_format(line, sizeof line, r);
    UINT bw;
    FRESULT fr = f_write(&lg->file, line, (UINT)len, &bw);
    if (fr != FR_OK || bw != len) {
        return fail(lg, fr != FR_OK ? fr : FR_DENIED);   /* bw < len: 카드가 가득 참 */
    }
    lg->lines++;
    if (++lg->unsynced >= lg->sync_every) {
        return logger_flush(lg);
    }
    return FR_OK;
}

FRESULT logger_flush(logger_t *lg)
{
    if (!lg->open) {
        return FR_NOT_READY;
    }
    FRESULT fr = f_sync(&lg->file);
    if (fr != FR_OK) {
        return fail(lg, fr);
    }
    lg->unsynced = 0;
    return FR_OK;
}

FRESULT logger_stop(logger_t *lg)
{
    if (!lg->open) {
        return FR_OK;
    }
    lg->open = false;
    FRESULT fr = f_close(&lg->file);
    f_mount(NULL, "", 0);
    return (fr == FR_OK) ? FR_OK : fail(lg, fr);
}
