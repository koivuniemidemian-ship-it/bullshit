cmake_minimum_required(VERSION 3.22)
project(LeGoo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(LeGoo
    COMPANY_NAME            "ballhogaudio"
    COMPANY_WEBSITE         ""
    BUNDLE_ID               com.ballhogaudio.legoo
    PLUGIN_MANUFACTURER_CODE Bhga
    PLUGIN_CODE             LGoo
    FORMATS                 VST3 AU
    PRODUCT_NAME            "Le Goo"
    DESCRIPTION             "Harmonic aura distortion"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    NEEDS_WEB_BROWSER       TRUE
    VST3_CATEGORIES         Fx Distortion
    AU_MAIN_TYPE            kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(LeGooData SOURCES Resources/index.html)

target_sources(LeGoo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(LeGoo PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(LeGoo
    PRIVATE
        LeGooData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
