# 39장 실습: 임베디드 Rust

| 폴더 | 내용 | 실행 |
| --- | --- | --- |
| `hal-blinky/` | stm32f4xx-hal 0.23: LED(PA5), 버튼(PC13), USART2 | `cargo run --release` (probe-rs) |
| `embassy-async/` | Embassy(stm32 0.6, executor 0.10): async 태스크, EXTI await, 채널 | `cargo run --release` (defmt 로그) |
| `ffi-c/` | 32장 crc32.c를 Rust에서 부르기 (PC) | `cargo test` |

```bash
rustup target add thumbv7em-none-eabihf
cargo install probe-rs-tools
```

크레이트 버전은 2026년 하반기 crates.io 기준이며, API는 각 크레이트의 해당 태그 예제에 맞췄습니다.

[확인 필요: Rust 툴체인으로 세 예제의 빌드와 보드 실행 확인 (원고 작성 환경에 Rust 미설치)]
