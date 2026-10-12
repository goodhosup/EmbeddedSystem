# 29장: Zephyr 애플리케이션
#   west build -b nucleo_f411re code/ch29-zephyr
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch29_zephyr)

target_sources(app PRIVATE src/main.c)
