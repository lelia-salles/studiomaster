# To get started on a new plugin, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modification

cmake_minimum_required(VERSION 3.15)

if (WIN32)
    #static linking in Windows
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# If we are compiling for Mac OS we want to target OS versions down to 10.9
option(UniversalBinary "Build universal binary for mac" ON)

if (APPLE)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE INTERNAL "")
    if (UniversalBinary)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")     
    endif()
endif()

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the project() command.
project(StudioMaster VERSION 1.7.2)

# ==============================================================================
# INJEÇÃO MASTER MUSICA: FORÇANDO O ASIO SDK GLOBALMENTE LOGO NO INÍCIO
# ==============================================================================
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/asiosdk/common")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/asiosdk/host")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/asiosdk/host/pc")
# ==============================================================================

set(BUILDVERSION 80)

# By default we don't want Xcode schemes to be made for modules, etc
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# No ZERO_CHECK target (it helps bust cache for cmake)
set(CMAKE_SUPPRESS_REGENERATION true) 

# Adds all the module sources so they appear correctly in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# This is a failed attempt to bury ALL_BUILD in Targets/
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Targets")

# Create a /Modules directory in the IDE with the JUCE Module code
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)

# include JUCE
add_subdirectory(deps/juce EXCLUDE_FROM_ALL )

juce_add_modules(deps/ff_meters)

set (FormatsToBuild VST3 Standalone)

# On Mac, a AU version will be built too
if (APPLE)
    list (APPEND FormatsToBuild AU)
endif()

# On Linux, LV2 will be built too
if (UNIX)
    list (APPEND FormatsToBuild LV2)
endif()

if (AAX_SDK_PATH)
    juce_set_aax_sdk_path (${AAX_SDK_PATH})
    if (APPLE OR (NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")))
      list (APPEND FormatsToBuild AAX)
    endif()
endif()

if (VST2_SDK_PATH)
    juce_set_vst2_sdk_path (${VST2_SDK_PATH})
    list (APPEND FormatsToBuild VST)
endif()

set (MacPList "<plist version=\"1.0\">
<dict>
<key>CFBundleVersion</key>
<string>${BUILDVERSION}</string>
<key>CFBundleURLTypes</key>
<array>
     <dict>
    <key>CFBundleURLName</key>
    <string>br.com.mastermusica.studiomaster</string> <key>CFBundleURLSchemes</key>
    <array>
         <string>studiomaster</string> </array>
     </dict>
   </array>
</dict>
</plist>")

# ==============================================================================
# DEFINIÇÃO DA FUNÇÃO DE CONSTRUÇÃO DO PLUGIN
# ==============================================================================
function(sono_add_custom_plugin_target target_name product_name formats is_instrument plugincode)

    if (is_instrument)
        set (vst3cats "Instrument" "Fx" "Network")
        set (vst2cat "vst2kPlugCategSynth") 
    else()
        set (vst3cats "Fx" "Network")        
        set (vst2cat "vst2kPlugCategEffect")
    endif()

    juce_add_plugin("${target_name}"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COMPANY_NAME "Mastermusica"
        BUNDLE_ID "br.com.mastermusica.studiomaster"
        MICROPHONE_PERMISSION_ENABLED TRUE
        ICON_BIG "images/sonobus_icon_mac_1024.png"
        ICON_SMALL "images/sonobus_icon_mac_256.png"
        NEEDS_WEB_BROWSER FALSE
        VST2_CATEGORY "${vst2cat}"
        VST3_CATEGORIES ${vst3cats}
        AAX_CATEGORY "AAX_ePlugInCategory_None"
        LV2URI "https://mastermusica.com.br/lv2/studiomaster"
        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
        PLIST_TO_MERGE "${MacPList}"
        AU_MAIN_TYPE "kAudioUnitType_MusicEffect"
        PLUGIN_MANUFACTURER_CODE Mstr
        PLUGIN_CODE ${plugincode}
        FORMATS ${formats}
        DESCRIPTION "StudioMaster - Network Audio Platform"
        PRODUCT_NAME "${product_name}"
    )

    if (WIN32)
        set_target_properties("${target_name}" PROPERTIES
            JUCE_COPY_PLUGIN_AFTER_BUILD TRUE
        )
    endif()

    juce_generate_juce_header("${target_name}")

    set (HEADER_INCLUDES
        deps/aoo/lib
        deps/aoo/deps
    )

    set (LIB_PATHS "")

    set (PLAT_COMPILE_DEFS
        $<$<CONFIG:Debug>:LOGLEVEL=2>
        USE_CODEC_OPUS=1
        AOO_TIMEFILTER_CHECK=0
        AOO_STATIC
    )

    set(PlatSourceFiles Source/CrossPlatformUtils.h)

    if (APPLE)
        list (APPEND HEADER_INCLUDES deps/mac/include)
        list (APPEND LIB_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/mac/lib)
        list (APPEND PlatSourceFiles Source/CrossPlatformUtilsMac.mm)
    elseif (WIN32)
        list (APPEND HEADER_INCLUDES deps/windows ../asiosdk/common)
        list (APPEND PlatSourceFiles Source/CrossPlatformUtilsWindows.cpp)

        message (STATUS "Win generator platform is: ${CMAKE_VS_PLATFORM_NAME}" )
        if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
            list (APPEND LIB_PATHS
                $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Debug32>
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Release32>
            )
        else()
            list (APPEND LIB_PATHS
                $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Debug>
                $<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/deps/windows/Release>
            )
        endif()

        list (APPEND PLAT_COMPILE_DEFS _USE_MATH_DEFINES WINVER=0x0601 _WIN32_WINNT=0x0601)
    else()
        list (APPEND PlatSourceFiles Source/CrossPlatformUtilsLinux.cpp)
        list (APPEND PLAT_COMPILE_DEFS JUCE_USE_MP3AUDIOFORMAT=1)
    endif()

    set(SourceFiles
        ${PlatSourceFiles}
        Source/AutoUpdater.cpp
        Source/AutoUpdater.h
        Source/BeatToggleGrid.cpp
        Source/BeatToggleGrid.h
        Source/ChannelGroup.cpp
        Source/ChannelGroup.h
        Source/ChannelGroupsView.cpp
        Source/ChannelGroupsView.h
        Source/ChatView.cpp
        Source/ChatView.h
        Source/CompressorView.h
        Source/ConnectView.cpp
        Source/ConnectView.h
        Source/DebugLogC.h
        Source/EffectParams.cpp
        Source/EffectParams.h
        Source/EffectsBaseView.h
        Source/ExpanderView.h
        Source/GenericItemChooser.cpp
        Source/GenericItemChooser.h
        Source/JitterBufferMeter.cpp
        Source/JitterBufferMeter.h
        Source/LatencyMatchView.cpp
        Source/LatencyMatchView.h
        Source/LatencyMeasurer.cpp
        Source/LatencyMeasurer.h
        Source/LevelMeterLookAndFeelMethods.h
        Source/LocalLatencyMeasurer.h
        Source/MVerb.h
        Source/Metronome.cpp
        Source/Metronome.h
        Source/MonitorDelayView.h
        Source/OptionsView.cpp
        Source/OptionsView.h
        Source/ParametricEqView.h
        Source/PeersContainerView.cpp
        Source/PeersContainerView.h
        Source/PolarityInvertView.h
        Source/RandomSentenceGenerator.cpp
        Source/RandomSentenceGenerator.h
        Source/ReverbSendView.h
        Source/ReverbView.h
        Source/RunCumulantor.cpp
        Source/RunCumulantor.h
        Source/RunningCumulant.h
        Source/SampleEditView.cpp
        Source/SampleEditView.h
        Source/SonoCallOutBox.cpp
        Source/SonoCallOutBox.h
        Source/SonoChoiceButton.cpp
        Source/SonoChoiceButton.h
        Source/SonoDrawableButton.cpp
        Source/SonoDrawableButton.h
        Source/SonoLookAndFeel.cpp
        Source/SonoLookAndFeel.h
        Source/SonoMultiStateDrawableButton.cpp
        Source/SonoMultiStateDrawableButton.h
        Source/SonoPlaybackProgressButton.cpp
        Source/SonoPlaybackProgressButton.h
        Source/SonoStandaloneFilterApp.cpp
        Source/SonoStandaloneFilterWindow.h
        Source/SonoTextButton.cpp
        Source/SonoTextButton.h
        Source/SonoUtility.h
        Source/Soundboard.cpp
        Source/Soundboard.h
        Source/SoundboardButtonColors.h
        Source/SoundboardChannelProcessor.cpp
        Source/SoundboardChannelProcessor.h
        Source/SoundboardEditView.cpp
        Source/SoundboardEditView.h
        Source/SoundboardProcessor.cpp
        Source/SoundboardProcessor.h
        Source/SoundboardView.cpp
        Source/SoundboardView.h
        Source/SoundSampleButtonColourPicker.cpp
        Source/SoundSampleButtonColourPicker.h
        Source/SonobusPluginEditor.cpp
        Source/SonobusPluginEditor.h
        Source/SonobusPluginProcessor.cpp
        Source/SonobusPluginProcessor.h
        Source/SonobusTypes.h
        Source/SuggestNewGroupView.cpp
        Source/SuggestNewGroupView.h
        Source/VDONinjaView.h
        Source/VersionInfo.cpp
        Source/VersionInfo.h
        Source/WaveformTransportComponent.h
        Source/faustCompressor.h
        Source/faustExpander.h
        Source/faustLimiter.h
        Source/faustParametricEQ.h
        Source/mtdm.h
        Source/zitaRev.h
    )

    set(AOOSourceFiles        
        deps/aoo/lib/src/SLIP.hpp
        deps/aoo/lib/src/client.cpp
        deps/aoo/lib/src/client.hpp
        deps/aoo/lib/src/codec_opus.cpp
        deps/aoo/lib/src/codec_pcm.cpp
        deps/aoo/lib/src/common.cpp
        deps/aoo/lib/src/common.hpp
        deps/aoo/lib/src/lockfree.hpp
        deps/aoo/lib/src/net_utils.cpp
        deps/aoo/lib/src/net_utils.hpp
        deps/aoo/lib/src/server.cpp
        deps/aoo/lib/src/server.hpp
        deps/aoo/lib/src/sink.cpp
        deps/aoo/lib/src/sink.hpp
        deps/aoo/lib/src/source.cpp
        deps/aoo/lib/src/source.hpp
        deps/aoo/lib/src/sync.cpp
        deps/aoo/lib/src/sync.hpp
        deps/aoo/lib/src/time.cpp
        deps/aoo/lib/src/time.hpp
        deps/aoo/lib/src/time_dll.hpp
        deps/aoo/lib/aoo/aoo.h
        deps/aoo/lib/aoo/aoo.hpp
        deps/aoo/lib/aoo/aoo_net.h
        deps/aoo/lib/aoo/aoo_net.hpp
        deps/aoo/lib/aoo/aoo_opus.h
        deps/aoo/lib/aoo/aoo_pcm.h
        deps/aoo/lib/aoo/aoo_types.h
        deps/aoo/lib/aoo/aoo_utils.hpp
        
        deps/aoo/deps/md5/md5.c
        deps/aoo/deps/md5/md5.h
        deps/aoo/deps/oscpack/osc/OscOutboundPacketStream.cpp
        deps/aoo/deps/oscpack/osc/OscPrintReceivedElements.cpp
        deps/aoo/deps/oscpack/osc/OscReceivedElements.cpp
        deps/aoo/deps/oscpack/osc/OscTypes.cpp
        deps/aoo/deps/oscpack/osc/MessageMappingOscPacketListener.h
        deps/aoo/deps/oscpack/osc/OscException.h
        deps/aoo/deps/oscpack/osc/OscHostEndianness.h
        deps/aoo/deps/oscpack/osc/OscOutboundPacketStream.h
        deps/aoo/deps/oscpack/osc/OscPacketListener.h
        deps/aoo/deps/oscpack/osc/OscPrintReceivedElements.h
        deps/aoo/deps/oscpack/osc/OscReceivedElements.h
        deps/aoo/deps/oscpack/osc/OscTypes.h
    )

    target_sources("${target_name}" PRIVATE 
        ${SourceFiles} 
        ${AOOSourceFiles}
    )

    set_target_properties("${target_name}" PROPERTIES FOLDER "")

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "" FILES ${SourceFiles})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/deps/aoo PREFIX "aoo" FILES ${AOOSourceFiles})
    source_group("JUCE Library Code" REGULAR_EXPRESSION "juce_")

    target_include_directories("${target_name}" PUBLIC ${HEADER_INCLUDES})
    target_compile_features("${target_name}" PRIVATE cxx_std_17)

    foreach(target ${formats} "All")
        if(TARGET ${target_name}_${target})
            set_target_properties(${target_name}_${target} PROPERTIES
                FOLDER "Targets"
                XCODE_GENERATE_SCHEME ON)
        endif()
    endforeach()

    target_compile_definitions("${target_name}"
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        JUCE_USE_WINDOWS_MEDIA_FORMAT=1
        JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
        JUCE_ASIO=1
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=0
        JUCE_JACK=1
        JUCE_ALSA=1
        JUCE_USE_ANDROID_OBOE=1
        JUCE_USE_OBOE_STABILIZED_CALLBACK=1
        FF_AUDIO_ALLOW_ALLOCATIONS_IN_MEASURE_BLOCK=0
        SONOBUS_BUILD_VERSION="${VERSION}"
        ${PLAT_COMPILE_DEFS} 
    )

    juce_add_binary_data("${target_name}_SBData" SOURCES
        Source/wordmaker.g
        Source/GoNotoKurrent-Regular.ttf
        Source/DejaVuSans.ttf
        localization/localized_de.txt
        localization/localized_es.txt
        localization/localized_fr.txt
        localization/localized_it.txt
        localization/localized_ja.txt
        localization/localized_nl.txt
        localization/localized_pt-br.txt
        localization/localized_pt-pt.txt
        localization/localized_ko.txt
        localization/localized_ru.txt
        localization/localized_zh-hans.txt
        images/arrow-up-narrow.svg
        images/arrow-down-narrow.svg
        images/dots.svg
        images/bar_click.wav
        images/beat_click.wav
        images/chat.svg
        images/chat_dots.svg
        images/checkmark.svg
        images/chevron_forward.svg
        images/continue.svg
        images/copy_icon.svg
        images/dice_icon_128.png
        images/dispfull.svg
        images/dispminimal.svg
        images/dots.svg
        images/dots_icon.png
        images/dots_menu.png
        images/expand_arrow_active.svg
        images/expand_arrow_inactive.svg
        images/eyedropper.svg
        images/eye.svg
        images/eye-off.svg
        images/folder_icon.svg
        images/hear-others.svg
        images/hold.svg
        images/incoming_allowed.svg
        images/incoming_allowed_active.svg
        images/incoming_disallowed.svg
        images/keyboard.svg
        images/keyboard_disabled.svg
        images/keypad-num-disabled.svg
        images/keypad-num.svg
        images/lgc_bar.wav
        images/link.svg
        images/link_all.svg
        images/link_up.svg
        images/list_icon1.png
        images/loop_icon.svg
        images/loop_off_icon.png
        images/met.svg
        images/mic.svg
        images/mic_disabled.svg
        images/mic_pointing.svg
        images/mesg-unread.svg
        images/move_updown.svg
        images/mute-others.svg
        images/network.svg
        images/oneshot.svg
        images/outgoing_allowed.svg
        images/outgoing_allowed_active.svg
        images/outgoing_disallowed.svg
        images/paste_icon.svg
        images/pause_icon.svg
        images/people.png
        images/people.svg
        images/person.png
        images/person.svg
        images/play_back_to_back.svg
        images/play_icon.svg
        images/play_simultaneous.svg
        images/play_background.svg
        images/plus_icon.svg
        images/power.svg
        images/power_sel.svg
        images/record.svg
        images/record_active.svg
        images/record_active_alt.svg
        images/rectape.svg
        images/reset_buffer_icon.svg
        images/send.svg
        images/send_group.svg
        images/send_group_small.svg
        images/replay_icon.svg
        images/settings_icon.svg
        images/skipback_icon.svg
        images/skipforward_icon.svg
        images/sonobus_logo_96.png
        images/sonobus_title_small.png
        images/soundboard.svg
        images/speaker.svg
        images/speaker_disabled.svg
        images/speaker_disabled_grey.svg
        images/stop.svg
        images/toggle.svg
        images/triangle_disclosure.svg
        images/triangle_disclosure_right.svg
        images/urei_main.wav
        images/videocam-outline.svg
        images/x_icon.svg
    )

    set_target_properties(${target_name}_SBData PROPERTIES FOLDER "Targets")

    if (UNIX AND NOT APPLE)
       target_compile_options("${target_name}_SBData" PRIVATE -fPIC)
       find_library(OPUS_LIB opus)
       if (NOT OPUS_LIB)
         message(FATAL_ERROR "opus library not found, please install libopus develop package")
       endif()

       if (JUCE_LINUX_TARGET_ARCHITECTURE MATCHES "arm" )
          message(STATUS "ARM platform, adding -march=native")
          target_compile_options(${target_name} PUBLIC -march=native)
       endif()
     
       if (TARGET ${target_name}_Standalone)
           string(TOLOWER ${target_name} tmptargname)
           set_target_properties("${target_name}_Standalone" PROPERTIES OUTPUT_NAME ${tmptargname})
       endif()
    endif()

    target_link_directories("${target_name}" INTERFACE ${LIB_PATHS})

    target_link_libraries("${target_name}" 
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_cryptography
            juce::juce_audio_plugin_client
            ff_meters
            ${target_name}_SBData
            opus
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
    )
            
endfunction()

sono_add_custom_plugin_target(StudioMaster "StudioMaster" "${FormatsToBuild}" TRUE "Stm1")