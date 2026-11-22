cmake_minimum_required(VERSION 3.22)

# Must be set before project() to take effect.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(Shizuka VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Branding: change these to your own ----------------------------------
set(MB_COMPANY      "Damp Audio")
set(MB_BUNDLE_ID    "com.dampaudio.shizuka")
set(MB_MFR_CODE     "Dmpx")   # 4 chars, at least one uppercase
set(MB_PLUGIN_CODE  "Szk1")   # 4 chars, exactly one uppercase, unique per plugin
# ---------------------------------------------------------------------------

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Shizuka
    COMPANY_NAME              "${MB_COMPANY}"
    BUNDLE_ID                 "${MB_BUNDLE_ID}"
    PRODUCT_NAME              "Shizuka"
    PLUGIN_MANUFACTURER_CODE  ${MB_MFR_CODE}
    PLUGIN_CODE               ${MB_PLUGIN_CODE}
    FORMATS                   AU VST3 Standalone
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    AU_MAIN_TYPE              kAudioUnitType_Effect
    VST3_CATEGORIES           Fx Tools)

target_sources(Shizuka PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Shizuka PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Shizuka
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
