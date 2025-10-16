# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "esp-idf/mbedtls/x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "flasher_args.json.in"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "littlefs_py_venv"
  "project_elf_src_esp32s3.c"
  "test_temporal_app.bin"
  "test_temporal_app.map"
  "x509_crt_bundle.S"
  )
endif()
