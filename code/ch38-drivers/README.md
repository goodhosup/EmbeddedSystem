# 38장 실습: 리눅스 커널 모듈 (Raspberry Pi 5, 커널 6.18에서 경고 없이 빌드 확인)

| 폴더 | 내용 |
| --- | --- |
| `hello/` | 가장 작은 모듈 |
| `counter/` | 문자 장치 `/dev/ch38counter` (miscdevice, 뮤텍스, copy_to_user) |
| `bmp280/` | BMP280 온도를 IIO로 내보내는 I2C 드라이버 + 디바이스 트리 오버레이 |

```bash
sudo apt install linux-headers-$(uname -r)    # 헤더가 없다면 (배포판에 따라 패키지 이름이 다르다)
cd hello && make && sudo insmod hello.ko && sudo dmesg | tail -1 && sudo rmmod hello
```

커널 모듈은 실행 중인 커널을 바꿉니다. 중요한 데이터가 없는 개발용 장치에서 실습하세요.
`config.txt`를 고치기 전에는 복사본을 만들어 두세요.

[확인 필요: 모듈을 올리고 센서를 연결한 실행 결과]
