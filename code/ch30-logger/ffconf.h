/*
 * 30장: FatFs R0.15a 설정 (이 프로젝트용)
 * 각 항목의 뜻은 FatFs 배포본의 ffconf.h와 documents/doc/config.html을 참고한다.
 * FFCONF_DEF는 FatFs 버전과 맞아야 한다(R0.15a = 5380). [확인 필요: 다른 버전을 쓸 때]
 */
#define FFCONF_DEF          5380

/* 기능 */
#define FF_FS_READONLY      0
#define FF_FS_MINIMIZE      0
#define FF_USE_FIND         0
#define FF_USE_MKFS         1       /* PC 테스트에서 가짜 카드를 포맷하려고 켠다 */
#define FF_USE_FASTSEEK     0
#define FF_USE_EXPAND       0
#define FF_USE_CHMOD        0
#define FF_USE_LABEL        0
#define FF_USE_FORWARD      0
#define FF_USE_STRFUNC      0       /* f_printf 대신 직접 포맷해 f_write */
#define FF_PRINT_LLI        0
#define FF_PRINT_FLOAT      0
#define FF_STRF_ENCODE      3

/* 이름: 8.3 짧은 이름만 (LOG00001.CSV) */
#define FF_CODE_PAGE        437
#define FF_USE_LFN          0
#define FF_MAX_LFN          255
#define FF_LFN_UNICODE      0
#define FF_LFN_BUF          255
#define FF_SFN_BUF          12
#define FF_FS_RPATH         0

/* 볼륨: SD 카드 하나 */
#define FF_VOLUMES          1
#define FF_STR_VOLUME_ID    0
#define FF_VOLUME_STRS      "SD"
#define FF_MULTI_PARTITION  0
#define FF_MIN_SS           512
#define FF_MAX_SS           512
#define FF_LBA64            0
#define FF_MIN_GPT          0x10000000
#define FF_USE_TRIM         0

/* 시스템 */
#define FF_FS_TINY          1       /* 파일마다 512 B 버퍼를 두지 않고 볼륨 버퍼를 함께 쓴다 (RAM 절약) */
#define FF_FS_EXFAT         0
#define FF_FS_NORTC         1       /* RTC 없음: 파일 시각을 아래 날짜로 고정 (11장의 RTC를 쓰면 get_fattime 구현) */
#define FF_NORTC_MON        1
#define FF_NORTC_MDAY       1
#define FF_NORTC_YEAR       2026
#define FF_FS_NOFSINFO      0
#define FF_FS_LOCK          0
#define FF_FS_REENTRANT     0       /* SD 카드와 FatFs는 logger 태스크 하나만 쓴다 (27장의 소유권) */
#define FF_FS_TIMEOUT       1000
