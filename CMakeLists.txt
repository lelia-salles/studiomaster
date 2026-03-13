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
# DEFINIÇÃO DA FUNÇÃO DE CONSTRUÇÃO DO PLUGIN - MASTER MUSICA
# ==============================================================================
function(sono_add_custom_plugin_target target_name product_name formats is_instrument plugincode)

    # 1. Definição de categorias baseada no argumento is_instrument
    if (is_instrument)
        set (vst3cats "Instrument" "Fx" "Network")
        set (vst2cat "vst2kPlugCategSynth") 
    else()
        set (vst3cats "Fx" "Network")        
        set (vst2cat "vst2kPlugCategEffect")
    endif()

    # 2. Criação do Plugin via JUCE
    juce_add_plugin("${target_name}"
        IS_SYNTH TRUE                          # Habilitado para ser Instrumento
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
        VST3_CATEGORIES ${vst3cats}            # Usa a lista dinâmica ("Instrument" "Fx" "Network")
        AAX_CATEGORY "AAX_ePlugInCategory_None"
        LV2URI "https://mastermusica.com.br/lv2/studiomaster"
        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
        PLIST_TO_MERGE "${MacPList}"
        AU_MAIN_TYPE "kAudioUnitType_MusicEffect" # Híbrido para macOS
        PLUGIN_MANUFACTURER_CODE Mstr
        PLUGIN_CODE ${plugincode}
        FORMATS ${formats}
        DESCRIPTION "StudioMaster - Network Audio Platform"
        PRODUCT_NAME "${product_name}"
    )

    # 3. Automação de cópia após Build (Windows)
    if (WIN32)
        set_target_properties("${target_name}" PROPERTIES
            JUCE_COPY_PLUGIN_AFTER_BUILD TRUE
        )
    endif()

    juce_generate_juce_header("${target_name}")

    # 4. Configurações de Includes e Bibliotecas
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

    # Configurações de plataforma
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
        # Linux
        list (APPEND PlatSourceFiles Source/CrossPlatformUtilsLinux.cpp)
        list (APPEND PLAT_COMPILE_DEFS JUCE_USE_MP3AUDIOFORMAT=1)
    endif()

    # (Aqui seguem suas variáveis SourceFiles e AOOSourceFiles já listadas no projeto)
    # ... [Mantenha a lista de arquivos Source/ e deps/ que você já tem] ...

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
        FF_AUDIO_ALLOW_ALLOCATIONS_IN_MEASURE_BLOCK=0
        SONOBUS_BUILD_VERSION="${VERSION}"
        ${PLAT_COMPILE_DEFS} 
    )

    # ... [Mantenha o bloco juce_add_binary_data com suas imagens e localizações] ...

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

# CHAMADA FINAL ÚNICA
sono_add_custom_plugin_target(StudioMaster "StudioMaster" "${FormatsToBuild}" TRUE "Stm1")