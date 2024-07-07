# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(home_automation)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_include_directories(app PRIVATE
   include
)

set (APP_SOURCES 
   src/gpio_handler.c
   src/mqtt_handler.c
   src/main.c
)

target_sources_ifdef(CONFIG_WIFI app PRIVATE "src/wifi_conn_mgr.c")
target_sources_ifdef(CONFIG_HTTP_SERVER app PRIVATE "src/http_server.c")

target_sources(app PRIVATE 
   ${APP_SOURCES}
)

target_link_libraries(app PRIVATE zephyr_interface zephyr)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section_ifdef(CONFIG_HTTP_SERVER NAME
				http_resource_desc_test_http_service
				KVMA RAM_REGION GROUP RODATA_REGION)
        
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

# Generate the HTML content
foreach(web_resource
  index.html
  main.js
    )
  generate_inc_file_for_target(
    app
    web_resources/${web_resource}
    ${gen_dir}/${web_resource}.gz.inc
    --gzip
  )
endforeach()
