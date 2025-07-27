cmake_minimum_required(VERSION 3.18)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable CUDA
enable_language(CUDA)
find_package(CUDAToolkit REQUIRED)

# Set CUDA properties
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES "86")

# Collect all source files
file(GLOB_RECURSE CPP_SOURCES 
    src/*.cpp
    src/*.h
)

# Add main files
set(MAIN_SOURCES
    AirKeyboardGUI.cpp
    AirKeyboardGUI.h
    globals.h
    config.h
)

# Add CUDA sources
set(CUDA_SOURCES
    src/capture/FrameProcessor.cu
)

# Windows-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -DUNICODE -D_UNICODE)
    
    set(RESOURCE_FILES 
        AirKeyboardGUI.rc
        resource.h
    )
endif()

# Create executable
add_executable(AirKeyboardGUI WIN32
    ${MAIN_SOURCES}
    ${CPP_SOURCES}
    ${CUDA_SOURCES}
    ${RESOURCE_FILES}
)

# Link libraries
target_link_libraries(AirKeyboardGUI PRIVATE
    CUDA::cudart
    CUDA::cuda_driver
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(AirKeyboardGUI PRIVATE
        dwmapi
        user32
        gdi32
        kernel32
        comctl32
        ole32
        oleaut32
        uuid
        mf
        mfplat
        mfreadwrite
        mfuuid
        strmiids
    )
endif()

# Copy required files to build directory
configure_file(app.manifest ${CMAKE_BINARY_DIR}/app.manifest COPYONLY)
configure_file(scripts/frame_postprocessor.py ${CMAKE_BINARY_DIR}/AirKeyboardGUI/frame_postprocessor.py COPYONLY)
configure_file(scripts/hand_landmarker.task ${CMAKE_BINARY_DIR}/AirKeyboardGUI/hand_landmarker.task COPYONLY)
configure_file(keyboard_icon.ico ${CMAKE_BINARY_DIR}/AirKeyboardGUI/keyboard_icon.ico COPYONLY)
