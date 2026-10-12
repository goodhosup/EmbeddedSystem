/* 19장의 Wi-Fi 스테이션 코드를 모듈로 정리한 것 */
#ifndef WIFI_H
#define WIFI_H

void wifi_start_and_wait(void);     /* IP를 받을 때까지 기다린다. 끊기면 알아서 다시 접속 */
int  wifi_rssi(void);

#endif
