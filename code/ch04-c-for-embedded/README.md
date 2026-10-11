# 4장 실습: PC에서 임베디드 C의 함정 확인하기

하드웨어 없이 PC의 `gcc`로 실행합니다.

```bash
make run    # 비트 연산, 구조체 오프셋, 정수 승격 예제 실행
make asm    # volatile 유무에 따른 어셈블리 비교 (volatile_no.s, volatile_yes.s)
```

빌드할 때 `main.c`에서 `-Wsign-compare` 경고 두 개가 나옵니다. **의도된 경고**입니다. 4.8절의 정수 승격과 부호 함정을 컴파일러가 실제로 잡아 준다는 것을 보여 주려고 일부러 남겨 두었습니다.
