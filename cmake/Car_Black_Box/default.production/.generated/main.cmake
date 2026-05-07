include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(Car_Black_Box_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(Car_Black_Box_default_default_XC8_FILE_TYPE_assemble)
add_library(Car_Black_Box_default_default_XC8_assemble OBJECT ${Car_Black_Box_default_default_XC8_FILE_TYPE_assemble})
    Car_Black_Box_default_default_XC8_assemble_rule(Car_Black_Box_default_default_XC8_assemble)
    list(APPEND Car_Black_Box_default_library_list "$<TARGET_OBJECTS:Car_Black_Box_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(Car_Black_Box_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(Car_Black_Box_default_default_XC8_assemblePreprocess OBJECT ${Car_Black_Box_default_default_XC8_FILE_TYPE_assemblePreprocess})
    Car_Black_Box_default_default_XC8_assemblePreprocess_rule(Car_Black_Box_default_default_XC8_assemblePreprocess)
    list(APPEND Car_Black_Box_default_library_list "$<TARGET_OBJECTS:Car_Black_Box_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(Car_Black_Box_default_default_XC8_FILE_TYPE_compile)
add_library(Car_Black_Box_default_default_XC8_compile OBJECT ${Car_Black_Box_default_default_XC8_FILE_TYPE_compile})
    Car_Black_Box_default_default_XC8_compile_rule(Car_Black_Box_default_default_XC8_compile)
    list(APPEND Car_Black_Box_default_library_list "$<TARGET_OBJECTS:Car_Black_Box_default_default_XC8_compile>")

endif()


# Main target for this project
add_executable(Car_Black_Box_default_image_BrvdpRgD ${Car_Black_Box_default_library_list})

set_target_properties(Car_Black_Box_default_image_BrvdpRgD PROPERTIES
    OUTPUT_NAME "default-production"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    RUNTIME_OUTPUT_DIRECTORY "${Car_Black_Box_default_output_dir}")
target_link_libraries(Car_Black_Box_default_image_BrvdpRgD PRIVATE ${Car_Black_Box_default_default_XC8_FILE_TYPE_link})

# Add the link options from the rule file.
Car_Black_Box_default_link_rule( Car_Black_Box_default_image_BrvdpRgD)


