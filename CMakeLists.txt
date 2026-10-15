cmake_minimum_required(VERSION 3.22)
project(AuroraGuitarStudio VERSION 0.2.0)
set(CMAKE_CXX_STANDARD 17)

# Use a local JUCE checkout if AURORA_JUCE_DIR is given, otherwise download it.
set(AURORA_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout")
if(AURORA_JUCE_DIR)
    add_subdirectory(${AURORA_JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

option(AURORA_COPY "Copy plugin to system folders after build" ON)
if(APPLE)
    set(AURORA_FORMATS VST3 AU Standalone)
else()
    set(AURORA_FORMATS VST3 Standalone)
endif()

juce_add_plugin(AuroraGuitarStudio
    COMPANY_NAME "YourName"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Aurg
    FORMATS ${AURORA_FORMATS}
    PRODUCT_NAME "Aurora Guitar Studio"
    COPY_PLUGIN_AFTER_BUILD ${AURORA_COPY})

target_sources(AuroraGuitarStudio PRIVATE Source/PluginProcessor.cpp)

target_compile_definitions(AuroraGuitarStudio PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AuroraGuitarStudio
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

option(AURORA_TESTS "Build the offline DSP test" OFF)
if(AURORA_TESTS)
    juce_add_console_app(AuroraTest PRODUCT_NAME "AuroraTest")
    target_sources(AuroraTest PRIVATE test/Test.cpp Source/PluginProcessor.cpp)
    target_compile_definitions(AuroraTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(AuroraTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
