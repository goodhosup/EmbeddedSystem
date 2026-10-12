//! 39장: Embassy로 26~27장의 일을 async로
//!   blink 태스크: 채널로 받은 주기로 LD2 (PA5) 깜박임
//!   main: B1 (PC13) 하강 에지를 await하다가, 누를 때마다 주기를 바꿔 채널로 보낸다
#![no_std]
#![no_main]

use defmt::*;
use embassy_executor::Spawner;
use embassy_stm32::exti::{self, ExtiInput};
use embassy_stm32::gpio::{Level, Output, Pull, Speed};
use embassy_stm32::{bind_interrupts, interrupt};
use embassy_sync::blocking_mutex::raw::ThreadModeRawMutex;
use embassy_sync::channel::Channel;
use embassy_time::{Duration, Ticker};
use {defmt_rtt as _, panic_probe as _};

bind_interrupts!(struct Irqs {
    EXTI15_10 => exti::InterruptHandler<interrupt::typelevel::EXTI15_10>;
});

// 27장의 큐: 크기 4, 정적 할당. 두 태스크가 함께 쓰므로 'static
static PERIOD: Channel<ThreadModeRawMutex, u64, 4> = Channel::new();

#[embassy_executor::task]
async fn blink(mut led: Output<'static>) {
    let mut ticker = Ticker::every(Duration::from_millis(500));
    loop {
        // 새 주기가 왔으면 바꾸고, 아니면 다음 틱까지 기다린다 (그동안 CPU는 잠든다)
        if let Ok(ms) = PERIOD.try_receive() {
            info!("blink period {} ms", ms);
            ticker = Ticker::every(Duration::from_millis(ms));
        }
        led.toggle();
        ticker.next().await;
    }
}

#[embassy_executor::main]
async fn main(spawner: Spawner) {
    let p = embassy_stm32::init(Default::default());
    info!("ch39 Embassy on STM32F411");

    let led = Output::new(p.PA5, Level::Low, Speed::Low);
    spawner.spawn(unwrap!(blink(led)));                     // 26장의 xTaskCreate

    // 7장의 EXTI를 await로: 에지가 올 때까지 이 async 함수가 멈춰 있다
    let mut button = ExtiInput::new(p.PC13, p.EXTI13, Pull::None, Irqs);
    let periods = [500u64, 200, 50];
    let mut i = 0;
    loop {
        button.wait_for_falling_edge().await;
        i = (i + 1) % periods.len();
        PERIOD.send(periods[i]).await;
        button.wait_for_rising_edge().await;                // 뗄 때까지 (간단한 디바운스 대신)
    }
}
