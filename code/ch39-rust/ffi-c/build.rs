// 32장의 C 코드(crc32.c)를 C 컴파일러로 빌드해 Rust 프로그램에 링크한다
fn main() {
    cc::Build::new()
        .file("../../ch32-bootloader/common/crc32.c")
        .include("../../ch32-bootloader/common")
        .compile("crc32");
    println!("cargo:rerun-if-changed=../../ch32-bootloader/common/crc32.c");
}
