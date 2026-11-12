cmake_minimum_required(VERSION 3.22)
project(Thump VERSION 0.6.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
# Pinned JUCE 8 security release; a checkout can be supplied via -DJUCE_LOCAL_PATH=/path/to/JUCE
if (DEFINED JUCE_LOCAL_PATH)
  add_subdirectory("${JUCE_LOCAL_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                        GIT_TAG 8.0.17-security GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Thump
    COMPANY_NAME "THUMP Audio"
    BUNDLE_ID "audio.thump.bassprocessor"
    PLUGIN_MANUFACTURER_CODE Thmp
    PLUGIN_CODE Thm1
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3 Standalone
    PRODUCT_NAME "THUMP")

target_sources(Thump PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Thump PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Thump PRIVATE juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Offline measurement executable uses the exact THUMP processor DSP and editor code.
# It does not modify the production plugin or its parameter names.
juce_add_console_app(ThumpBench PRODUCT_NAME "THUMP DSP Bench")
target_sources(ThumpBench PRIVATE
    tests/ThumpBench.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(ThumpBench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(ThumpBench PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(ThumpBench PRIVATE juce::juce_audio_utils juce::juce_dsp
    juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)
