//! 39장: stm32f4xx-hal로 6장, 7장, 14장의 일을 Rust로
//!   LD2 (PA5) 깜박임, B1 (PC13, 누르면 Low) 누르면 빠르게, USART2 (PA2)로 상태 출력
#![no_std]
#![no_main]

use core::fmt::Write;
use cortex_m_rt::entry;
use panic_halt as _;
use stm32f4xx_hal::{pac, prelude::*, rcc::Config};

#[entry]
fn main() -> ! {
    // 주변장치는 프로그램 전체에서 단 한 번만 가져올 수 있다 (두 번째 take()는 None)
    let dp = pac::Peripherals::take().unwrap();
    let cp = cortex_m::peripheral::Peripherals::take().unwrap();

    let mut rcc = dp.RCC.freeze(Config::hsi().sysclk(16.MHz()));   // 3장: HSI 16 MHz

    let gpioa = dp.GPIOA.split(&mut rcc);
    let gpioc = dp.GPIOC.split(&mut rcc);

    // 핀의 모드가 "타입"이 된다: pa5는 이 줄 뒤로 출력 핀 타입이고, 원래 pa5는 다시 쓸 수 없다
    let mut led = gpioa.pa5.into_push_pull_output();
    let button = gpioc.pc13.into_floating_input();                 // 보드에 외부 풀업이 있다

    let mut tx = dp.USART2.tx(gpioa.pa2, 115200.bps(), &mut rcc).unwrap();
    let mut delay = cp.SYST.delay(&rcc.clocks);

    writeln!(tx, "ch39 Rust on STM32F411\r").ok();
    let mut toggles: u32 = 0;
    loop {
        let fast = button.is_low();
        led.toggle();
        toggles = toggles.wrapping_add(1);
        if toggles % 10 == 0 {
            writeln!(tx, "toggles: {toggles}, button: {}\r", if fast { "pressed" } else { "released" }).ok();
        }
        delay.delay_ms(if fast { 100u32 } else { 500u32 });
    }
}
