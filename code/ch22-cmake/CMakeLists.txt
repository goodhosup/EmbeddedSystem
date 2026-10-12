# 22장 실습: CMake로 STM32 펌웨어 빌드
#
#   cmake -B build-debug   -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
#   cmake -B build-release -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-debug
#   cmake --build build-debug --target flash
cmake_minimum_required(VERSION 3.20)
project(ch22_firmware C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(COMMON ${CMAKE_CURRENT_SOURCE_DIR}/../common/stm32f411)

# ---- 버전 정보: git에서 가져와 헤더로 만든다 (22.6절) ----
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE FW_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(NOT FW_GIT_VERSION)
    set(FW_GIT_VERSION "unknown")
endif()
configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h @ONLY)

# ---- 보드 독립 코드: 31장에서 PC용 테스트에도 그대로 링크한다 ----
add_library(app STATIC src/average.c)
target_include_directories(app PUBLIC src)
target_compile_options(app PRIVATE -Wall -Wextra)

# ---- 보드 코드: 스타트업과 벡터 테이블은 "참조되지 않아도" 들어가야 하므로 오브젝트 라이브러리로 ----
add_library(board OBJECT ${COMMON}/startup.c ${COMMON}/uart2.c ${COMMON}/syscalls.c)
target_include_directories(board PUBLIC ${COMMON})

# ---- 실행 파일 ----
add_executable(firmware src/main.c)
set_target_properties(firmware PROPERTIES SUFFIX ".elf")
target_include_directories(firmware PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(firmware PRIVATE -Wall -Wextra)
target_link_libraries(firmware PRIVATE app board)
target_link_options(firmware PRIVATE
    -T${COMMON}/stm32f411.ld
    --specs=nano.specs -nostartfiles
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/firmware.map)

add_custom_command(TARGET firmware POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:firmware> ${CMAKE_CURRENT_BINARY_DIR}/firmware.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:firmware>
    COMMENT "making firmware.bin")

add_custom_target(flash
    COMMAND openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
            -c "program $<TARGET_FILE:firmware> verify reset exit"
    DEPENDS firmware
    USES_TERMINAL)
