# 22장: Arm GNU Toolchain용 CMake 툴체인 파일 (STM32F411, Cortex-M4F)
#   cmake -B build-debug -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug

set(CMAKE_SYSTEM_NAME      Generic)      # 운영체제 없음
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY      arm-none-eabi-objcopy CACHE FILEPATH "")
set(CMAKE_SIZE         arm-none-eabi-size    CACHE FILEPATH "")

# 구성 단계의 컴파일러 시험에서 실행 파일을 링크하지 않는다 (링커 스크립트 없이는 실패하므로)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT             "${CPU_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${CPU_FLAGS} -Wl,--gc-sections")

# 빌드 종류별 최적화: CMake 기본값(-O3 등) 대신 임베디드에 맞게
# *_INIT으로 정하면 CMake가 뒤에 자기 기본값(" -O3 -DNDEBUG")을 덧붙이므로 캐시 변수로 정한다
set(CMAKE_C_FLAGS_DEBUG            "-Og -g3"         CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE          "-Os -g -DNDEBUG" CACHE STRING "")   # 릴리스도 디버그 정보는 남긴다(.elf에만 들어감)

# 라이브러리와 헤더를 PC가 아니라 툴체인 쪽에서만 찾는다
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
