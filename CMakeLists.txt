cmake_minimum_required(VERSION 3.27)

project(FAUNA VERSION 1.2)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FAUNA
    FORMATS VST3 AU
    COMPANY_NAME "JLVX"
    COMPANY_EMAIL "prodbyjlv@gmail.com"
    PLUGIN_NAME "FAUNA"
    PRODUCT_NAME "FAUNA"
    PLUGIN_MANUFACTURER_CODE "JLVX"
    PLUGIN_CODE "Funa"
)

target_sources(FAUNA PRIVATE
    Source/AudioStreamer.cpp
    Source/AudioStreamer.h
    Source/WebServer.cpp
    Source/WebServer.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/QrCode.cpp
)

target_link_libraries(FAUNA PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_processors_headless
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_osc
)

target_compile_features(FAUNA PRIVATE cxx_std_17)

target_compile_definitions(FAUNA PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

juce_generate_juce_header(FAUNA)