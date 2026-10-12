//! 39장: 이미 검증된 C 코드를 Rust에서 그대로 쓰기 (FFI)
//!   cargo run   /   cargo test

use core::ffi::c_void;

extern "C" {
    // common/crc32.h:  uint32_t crc32_update(uint32_t crc, const void *buf, size_t n);
    fn crc32_update(crc: u32, buf: *const c_void, n: usize) -> u32;
}

/// C 함수를 안전한 Rust 함수로 감싼다: 슬라이스의 포인터와 길이가 항상 짝이 맞는다
pub fn crc32(data: &[u8]) -> u32 {
    // SAFETY: data는 유효한 슬라이스이고, C 함수는 n바이트만 읽고 포인터를 보관하지 않는다
    unsafe { crc32_update(0, data.as_ptr().cast(), data.len()) }
}

fn main() {
    let crc = crc32(b"123456789");
    println!("crc32(\"123456789\") = 0x{crc:08X}");
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn standard_check_value() {
        assert_eq!(crc32(b"123456789"), 0xCBF4_3926);   // 32장 test_crc32와 같은 값
    }

    #[test]
    fn empty_input() {
        assert_eq!(crc32(&[]), 0);
    }
}
