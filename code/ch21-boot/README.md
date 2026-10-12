# 21장 실습: 부트 과정과 섹션

`stm32/`의 예제는 링커 기호와 변수·함수의 주소를 출력하고, 생성자 함수가 `main`보다 먼저 실행되는지 확인합니다.

```bash
cd stm32
make flash
make inspect        # 섹션 크기(size -A)와 기호 주소(nm)
```

빌드하면 `build/ch21-boot.map`(맵 파일)이 생깁니다. 21.7절을 보며 함께 읽어 보세요.

이 장에서 해설하는 파일은 `code/common/stm32f411/startup.c`와 `stm32f411.ld`입니다.
