cmake_minimum_required(VERSION 3.10)
project(SNEngineUtils)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(PkgConfig)

# Add tinyfiledialogs library
add_library(tinyfiledialogs tinyfiledialogs.c)

# Cleaner executable
add_executable(SNEngine_Cleaner cleaner.cpp)
target_link_libraries(SNEngine_Cleaner)

# Symbols setup executable
add_executable(SNEngine_Symbols presetup.cpp tinyfiledialogs)
target_link_libraries(SNEngine_Symbols)

# Platform-specific configurations
if(WIN32)
    # Windows-specific libraries for tinyfiledialogs
    target_link_libraries(SNEngine_Symbols shlwapi ole32 comdlg32 comctl32)
    
    # Windows-specific libraries for cleaner
    target_link_libraries(SNEngine_Cleaner shlwapi)
elseif(UNIX AND NOT APPLE)
    # Linux-specific
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GTK3 gtk+-3.0)
        if(GTK3_FOUND)
            target_link_libraries(SNEngine_Symbols ${GTK3_LIBRARIES})
            target_compile_options(SNEngine_Symbols PRIVATE ${GTK3_CFLAGS_OTHER})
        endif()
    endif()
endif()

# Install targets
install(TARGETS SNEngine_Cleaner SNEngine_Symbols
    RUNTIME DESTINATION bin
)