@echo off
setlocal
set "TEST_SRC=%~dp0"
set "REPO_ROOT=%~dp0..\..\.."
call "%REPO_ROOT%\Docs\Scripts\evidence_root.bat"
if errorlevel 1 exit /b 1
if /I "%~1"=="--sdl-owner" goto sdlowner
call "%REPO_ROOT%\..\..\..\dev_env.bat"
if not defined VCVARSALL exit /b 1
call "%VCVARSALL%" x86 >nul
if not "%errorlevel%"=="0" exit /b 1
set "TEST_BUILD=%FFX_EVIDENCE_ROOT%\test-build"
if not exist "%TEST_BUILD%" mkdir "%TEST_BUILD%"
pushd "%TEST_BUILD%"
if /I "%~1"=="--review" goto review
if /I "%~1"=="--abi" goto abi
if /I "%~1"=="--threading" goto threading
if /I "%~1"=="--tail" goto tail
if /I "%~1"=="--native-actions" goto nativeactions
if /I "%~1"=="--jecht" goto jecht
if /I "%~1"=="--lightning" goto lightning
if /I "%~1"=="--console" goto console
if /I "%~1"=="--readers" goto readers
if /I "%~1"=="--sphere" goto sphere
if /I "%~1"=="--controller" goto controller
if /I "%~1"=="--input-adapter" goto inputadapter
if /I "%~1"=="--input" goto input
if /I "%~1"=="--from-input" goto input
if /I "%~1"=="--movie" goto movie
if /I "%~1"=="--dialogue" goto dialogue
if /I "%~1"=="--butterflies" goto butterflies
if /I "%~1"=="--minigames" goto minigames
if /I "%~1"=="--minigame-completion" goto minigamecompletion
if /I "%~1"=="--navigation-audio" goto navigationaudio
if /I "%~1"=="--navigation-geometry" goto navigationgeometry
if /I "%~1"=="--sin-routes" goto sinroutes
if /I "%~1"=="--sin-later-maps" goto sinlatermaps
if /I "%~1"=="--sin-collection" goto sincollection
if /I "%~1"=="--settings-defaults" goto settingsdefaults
if /I "%~1"=="--utilities" goto utilities
if /I "%~1"=="--pedestal" goto pedestal
if /I "%~1"=="--reachability" goto reachability
if /I "%~1"=="--profile" goto profile
if /I "%~1"=="--startup" goto startup
if /I "%~1"=="--accessibility" goto accessibility
if /I "%~1"=="--speech" goto speech
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:log_history_fixture.exe "%TEST_SRC%log_history_fixture.cpp" "%TEST_SRC%..\log_history.cpp"
if not "%errorlevel%"=="0" goto failed
log_history_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--logs" goto success
:speech
if not exist PrismRuntime mkdir PrismRuntime
cl /nologo /W4 /EHsc /std:c++17 /MT /LD /Fe:PrismRuntime\prism.dll "%TEST_SRC%fake_prism.cpp" /link /DEF:"%TEST_SRC%fake_prism.def"
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /DFFX_SPEECH_WAKE_TEST /Fe:runtime_tests.exe "%TEST_SRC%runtime_tests.cpp" "%TEST_SRC%..\prism_backend.cpp" "%TEST_SRC%..\speech.cpp" "%TEST_SRC%..\log_capture.cpp" "%TEST_SRC%..\speech_capture.cpp" "%TEST_SRC%..\speech_queue.cpp" "%TEST_SRC%..\speech_domains.cpp" "%TEST_SRC%..\log_history.cpp" "%TEST_SRC%..\text.cpp" "%TEST_SRC%..\settings.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
runtime_tests.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:prism_adapter_fixture.exe "%TEST_SRC%prism_adapter_fixture.cpp" "%TEST_SRC%..\prism_backend.cpp"
if not "%errorlevel%"=="0" goto failed
prism_adapter_fixture.exe .
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--speech" goto success
:dialogue
cl /nologo /W4 /EHa /std:c++17 /MT /Fe:dialogue_voice_fixture.exe "%TEST_SRC%dialogue_voice_fixture.cpp" "%TEST_SRC%..\dialogue_voice.cpp" "%TEST_SRC%..\movie_dialogue.cpp" "%TEST_SRC%..\messages.cpp" "%TEST_SRC%..\message_text.cpp" "%TEST_SRC%..\text_plan.cpp" "%TEST_SRC%..\settings.cpp" /DFFX_READER_TEST "%TEST_SRC%..\owned_retirement.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
dialogue_voice_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--dialogue" goto success
:movie
cl /nologo /W4 /EHa /std:c++17 /MT /Fe:movie_dialogue_fixture.exe "%TEST_SRC%movie_dialogue_fixture.cpp" "%TEST_SRC%..\movie_dialogue.cpp" "%TEST_SRC%..\settings.cpp" /DFFX_READER_TEST /link user32.lib
if not "%errorlevel%"=="0" goto failed
movie_dialogue_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--movie" goto success
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:profile_guard.exe "%TEST_SRC%profile_guard.cpp" "%TEST_SRC%..\profile.cpp" /link advapi32.lib
if not "%errorlevel%"=="0" goto failed
profile_guard.exe
if not "%errorlevel%"=="0" goto failed
:abi
cl /nologo /W4 /EHa /std:c++17 /MT /I"%MINHOOK_ROOT%\include" /Fe:hook_abi.exe "%TEST_SRC%hook_abi.cpp" "%TEST_SRC%..\hooks.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\resource_reset.cpp" "%TEST_SRC%..\hook_calls.cpp" /link "%MINHOOK_ROOT%\bin\MinHook.x86.lib"
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
hook_abi.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--abi" goto success
:threading
cl /nologo /W4 /EHa /std:c++17 /MT /I"%MINHOOK_ROOT%\include" /Fe:hook_threading.exe "%TEST_SRC%hook_threading.cpp" "%TEST_SRC%..\hooks.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\resource_reset.cpp" "%TEST_SRC%..\hook_calls.cpp" /link "%MINHOOK_ROOT%\bin\MinHook.x86.lib"
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
hook_threading.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--threading" goto success
:tail
cl /nologo /W4 /EHa /std:c++17 /MT /I"%MINHOOK_ROOT%\include" /Fe:hook_tail_fixture.exe "%TEST_SRC%hook_tail_fixture.cpp" "%TEST_SRC%..\hooks.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\resource_reset.cpp" "%TEST_SRC%..\hook_calls.cpp" /link "%MINHOOK_ROOT%\bin\MinHook.x86.lib"
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
hook_tail_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--tail" goto success
:nativeactions
cl /nologo /W4 /EHa /std:c++17 /MT /I"%MINHOOK_ROOT%\include" /Fe:hook_native_actions_fixture.exe "%TEST_SRC%hook_native_actions_fixture.cpp" "%TEST_SRC%..\hooks.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\resource_reset.cpp" "%TEST_SRC%..\hook_calls.cpp" /link "%MINHOOK_ROOT%\bin\MinHook.x86.lib"
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
hook_native_actions_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--native-actions" goto success
:jecht
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:jecht_fixture.exe "%TEST_SRC%jecht_fixture.cpp" "%TEST_SRC%..\jecht.cpp" "%TEST_SRC%..\jecht_voice.cpp" "%TEST_SRC%..\lightning.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\navigation_targets.cpp" /link user32.lib /STACK:2097152
if not "%errorlevel%"=="0" goto failed
jecht_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--jecht" goto success
:lightning
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:lightning_fixture.exe "%TEST_SRC%lightning_fixture.cpp" "%TEST_SRC%..\lightning.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\navigation_targets.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
lightning_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--lightning" goto success
:console
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_DEV_UTILITIES /Fe:utility_console_fixture.exe "%TEST_SRC%utility_console_fixture.cpp" /link user32.lib gdi32.lib
if not "%errorlevel%"=="0" goto failed
utility_console_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--console" goto success
:profile
cl /nologo /W4 /EHa /std:c++17 /MT /Fe:profile_extensions_fixture.exe "%TEST_SRC%profile_extensions_fixture.cpp" "%TEST_SRC%..\profile_extensions.cpp" "%TEST_SRC%..\profile_dependencies.cpp"
if not "%errorlevel%"=="0" goto failed
profile_extensions_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--profile" goto success
:startup
cl /nologo /W4 /EHa /std:c++17 /MT /Fe:proxy_startup_fixture.exe "%TEST_SRC%proxy_startup_fixture.cpp" "%TEST_SRC%..\..\proxy\dinput8_proxy.cpp" /link dxguid.lib
if not "%errorlevel%"=="0" goto failed
proxy_startup_fixture.exe
if not "%errorlevel%"=="0" goto failed
proxy_startup_fixture.exe --speech
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--startup" goto success
:accessibility
cl /nologo /W4 /EHsc /std:c++17 /MT /DFFX_READER_TEST /Fe:accessibility_fixture.exe "%TEST_SRC%accessibility_fixture.cpp" "%TEST_SRC%..\mod_menu.cpp" "%TEST_SRC%..\text_plan.cpp" "%TEST_SRC%..\settings.cpp" "%TEST_SRC%..\controller_bindings.cpp" "%TEST_SRC%..\utility_text.cpp" "%TEST_SRC%..\utility_text_plans.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
accessibility_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--accessibility" goto success
copy /Y "%REPO_ROOT%\install\native\SDL3.dll" SDL3.dll >nul
if not "%errorlevel%"=="0" goto failed
:sphere
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:sphere_fixture.exe "%TEST_SRC%sphere_fixture.cpp" "%TEST_SRC%..\sphere_paths.cpp" "%TEST_SRC%..\sphere_sections.cpp" "%TEST_SRC%..\sphere_expert.cpp" "%TEST_SRC%..\sphere_orientation.cpp" "%TEST_SRC%..\sphere_planner.cpp" "%TEST_SRC%..\settings.cpp" "%TEST_SRC%..\owned_retirement.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
sphere_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--sphere" goto success
:controller
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /I"%SDL3_SOURCE%\include" /Fe:controller_bridge_fixture.exe "%TEST_SRC%controller_bridge_fixture.cpp" "%TEST_SRC%..\controller.cpp" "%TEST_SRC%..\controller_bindings.cpp" "%TEST_SRC%..\sdl_runtime.cpp" "%TEST_SRC%..\physical_gamepad.cpp" "%TEST_SRC%..\settings.cpp" /link "%REPO_ROOT%\install\sdl3-x86\SDL3.lib" xinput.lib user32.lib
if not "%errorlevel%"=="0" goto failed
controller_bridge_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--controller" goto success
:navigationaudio
if /I "%~1"=="--navigation-audio" copy /Y "%REPO_ROOT%\install\native\SDL3.dll" SDL3.dll >nul
pushd "%TEST_SRC%.."
rc /nologo /fo "%TEST_BUILD%\test_audio.res" audio_assets.rc
set "RESOURCE_RESULT=%errorlevel%"
popd
if not "%RESOURCE_RESULT%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /DFFX_READER_TEST /I"%SDL3_SOURCE%\include" /Fe:nav_audio_fixture.exe "%TEST_SRC%nav_audio_fixture.cpp" "%TEST_SRC%..\nav_audio.cpp" "%TEST_SRC%..\sdl_runtime.cpp" "%TEST_SRC%..\settings.cpp" /link test_audio.res "%REPO_ROOT%\install\sdl3-x86\SDL3.lib" user32.lib
if not "%errorlevel%"=="0" goto failed
nav_audio_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:nav_cues_fixture.exe "%TEST_SRC%nav_cues_fixture.cpp" "%TEST_SRC%..\nav_cues.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp" "%TEST_SRC%..\butterfly_navigation.cpp" "%TEST_SRC%..\swimming_route.cpp" "%TEST_SRC%..\butterfly_paths.cpp" "%TEST_SRC%..\owned_retirement.cpp"
if not "%errorlevel%"=="0" goto failed
nav_cues_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--navigation-audio" goto success
:butterflies
cl /nologo /W4 /WX /O2 /EHa /DFFX_READER_TEST /std:c++17 /MT /Fe:butterfly_fixture.exe "%TEST_SRC%butterfly_fixture.cpp" "%TEST_SRC%..\butterfly.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\butterfly_processing.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
butterfly_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /O2 /EHsc /std:c++17 /MT /Fe:butterfly_paths_fixture.exe "%TEST_SRC%butterfly_paths_fixture.cpp" "%TEST_SRC%..\butterfly_paths.cpp" "%TEST_SRC%..\nav_geometry.cpp"
if not "%errorlevel%"=="0" goto failed
butterfly_paths_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--butterflies" goto success

:minigames
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:minigame_fixture.exe "%TEST_SRC%minigame_fixture.cpp" "%TEST_SRC%..\minigames.cpp" "%TEST_SRC%..\minigame_memory.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\navigation_targets.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
minigame_fixture.exe
if not "%errorlevel%"=="0" goto failed
minigame_fixture.exe --installed-profile
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--minigames" goto success
:readers
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:reader_fixture.exe "%TEST_SRC%reader_fixture.cpp" "%TEST_SRC%..\text.cpp" "%TEST_SRC%..\game_text.cpp" "%TEST_SRC%..\field_menus.cpp" "%TEST_SRC%..\save_load_menu.cpp" "%TEST_SRC%..\field_details.cpp" "%TEST_SRC%..\field_targets.cpp" "%TEST_SRC%..\naming.cpp" "%TEST_SRC%..\aeon_attributes.cpp" "%TEST_SRC%..\navigation.cpp" "%TEST_SRC%..\puzzle_guide_owned.cpp" "%TEST_SRC%..\puzzle_guide_targets.cpp" "%TEST_SRC%..\navigation_scene.cpp" "%TEST_SRC%..\navigation_observations.cpp" "%TEST_SRC%..\navigation_intents.cpp" "%TEST_SRC%..\utility_memory.cpp" "%TEST_SRC%..\pedestal_rules.cpp" "%TEST_SRC%..\reachability_capture.cpp" "%TEST_SRC%..\reachability_worker.cpp" "%TEST_SRC%..\nav_memory.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp" "%TEST_SRC%..\nav_exits.cpp" "%TEST_SRC%..\exit_labels.cpp" "%TEST_SRC%..\swimming_navigation.cpp" "%TEST_SRC%..\navigation_labels.cpp" "%TEST_SRC%..\nav_interactions.cpp" "%TEST_SRC%..\nav_facing.cpp" "%TEST_SRC%..\navigation_targets.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\atel_memory.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\messages.cpp" "%TEST_SRC%..\message_text.cpp" "%TEST_SRC%..\button_glyphs.cpp" "%TEST_SRC%..\field_controls.cpp" "%TEST_SRC%..\dialogue_voice.cpp" "%TEST_SRC%..\movie_dialogue.cpp" "%TEST_SRC%..\help.cpp" "%TEST_SRC%..\monster_arena.cpp" "%TEST_SRC%..\copied_text.cpp" "%TEST_SRC%..\copied_controls.cpp" "%TEST_SRC%..\intro_cue.cpp" "%TEST_SRC%..\airship_search.cpp" "%TEST_SRC%..\airship_destinations.cpp" "%TEST_SRC%..\game_binding_input.cpp" "%TEST_SRC%..\system_menus.cpp" "%TEST_SRC%..\system_menu_capture.cpp" "%TEST_SRC%..\ready_text.cpp" "%TEST_SRC%..\status_review.cpp" "%TEST_SRC%..\status_grid.cpp" "%TEST_SRC%..\shops.cpp" "%TEST_SRC%..\confirmations.cpp" "%TEST_SRC%..\esc_settings.cpp" "%TEST_SRC%..\sphere_grid.cpp" "%TEST_SRC%..\sphere_jump.cpp" "%TEST_SRC%..\sphere_data.cpp" "%TEST_SRC%..\sphere_paths.cpp" "%TEST_SRC%..\sphere_sections.cpp" "%TEST_SRC%..\sphere_expert.cpp" "%TEST_SRC%..\sphere_orientation.cpp" "%TEST_SRC%..\sphere_planner.cpp" "%TEST_SRC%..\sphere_owned.cpp" "%TEST_SRC%..\sphere_text.cpp" "%TEST_SRC%..\battle.cpp" "%TEST_SRC%..\battle_hud.cpp" "%TEST_SRC%..\battle_graphics.cpp" "%TEST_SRC%..\battle_sensor.cpp" "%TEST_SRC%..\battle_announcements.cpp" "%TEST_SRC%..\battle_results.cpp" "%TEST_SRC%..\battle_status.cpp" "%TEST_SRC%..\status_animation.cpp" "%TEST_SRC%..\overdrive.cpp" "%TEST_SRC%..\game_details.cpp" "%TEST_SRC%..\selection_details.cpp" "%TEST_SRC%..\esc_details.cpp" "%TEST_SRC%..\esc_capture.cpp" "%TEST_SRC%..\text_plan.cpp" "%TEST_SRC%..\ready_game_text.cpp" "%TEST_SRC%..\settings.cpp" "%TEST_SRC%..\butterfly_navigation.cpp" "%TEST_SRC%..\swimming_route.cpp" "%TEST_SRC%..\butterfly_paths.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\utility_text.cpp" "%TEST_SRC%..\utility_text_plans.cpp" /link user32.lib gdi32.lib
if not "%errorlevel%"=="0" goto failed
reader_fixture.exe
if not "%errorlevel%"=="0" goto failed
reader_fixture.exe --installed-profile
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--readers" goto success
:reachability
cl /nologo /W4 /WX /EHsc /std:c++17 /MT /c /DdestinationUnreachable=fixtureDestination /Fo:reachability_zones.obj "%TEST_SRC%..\nav_zones.cpp"
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /EHsc /std:c++17 /MT /DFFX_READER_TEST /Fe:reachability_fixture.exe "%TEST_SRC%reachability_fixture.cpp" "%TEST_SRC%..\reachability_capture.cpp" "%TEST_SRC%..\reachability_worker.cpp" "%TEST_SRC%..\nav_geometry.cpp" reachability_zones.obj
if not "%errorlevel%"=="0" goto failed
reachability_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /EHsc /std:c++17 /MT /Fe:owned_mesh_decoder_fixture.exe "%TEST_SRC%owned_mesh_decoder_fixture.cpp"
if not "%errorlevel%"=="0" goto failed
owned_mesh_decoder_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--reachability" goto success
:sinroutes
if /I not "%~1"=="--sin-routes" goto navigationgeometry
cl /nologo /W4 /WX /O2 /EHsc /std:c++17 /MT /Fe:sin_route_fixture.exe "%TEST_SRC%sin_route_fixture.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp"
if not "%errorlevel%"=="0" goto failed
sin_route_fixture.exe > sin-production-routes.json
if not "%errorlevel%"=="0" goto failed
goto success
:sinlatermaps
cl /nologo /W4 /WX /O2 /EHsc /std:c++17 /MT /Fe:sin_later_maps_fixture.exe "%TEST_SRC%sin_later_maps_fixture.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp"
if not "%errorlevel%"=="0" goto failed
sin_later_maps_fixture.exe > sin-later-production-routes.json
if not "%errorlevel%"=="0" goto failed
goto success
:minigamecompletion
if /I not "%~1"=="--minigame-completion" goto sincollection
python "%TEST_SRC%prepare_minigame_completion.py"
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:minigame_completion_fixture.exe "%TEST_SRC%minigame_completion_fixture.cpp" "%TEST_SRC%..\minigame_completion.cpp" "%TEST_SRC%..\utility_memory.cpp" "%TEST_SRC%..\utility_text.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\navigation_targets.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
minigame_completion_fixture.exe
if not "%errorlevel%"=="0" goto failed
goto success
:sincollection
if /I not "%~1"=="--sin-collection" goto navigationgeometry
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:sin_collection_fixture.exe "%TEST_SRC%sin_collection_fixture.cpp" "%TEST_SRC%..\sin_collection.cpp" "%TEST_SRC%..\utility_memory.cpp" "%TEST_SRC%..\utility_text.cpp" "%TEST_SRC%..\atel_resources.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\navigation_targets.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
sin_collection_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /O2 /EHa /std:c++17 /MT /I"%MINHOOK_ROOT%\include" /Fe:sin_collection_hook_fixture.exe "%TEST_SRC%sin_collection_hook_fixture.cpp" "%TEST_SRC%..\sin_collection.cpp" "%TEST_SRC%..\utility_text.cpp" "%TEST_SRC%..\hooks.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\resource_reset.cpp" "%TEST_SRC%..\hook_calls.cpp" /link "%MINHOOK_ROOT%\bin\MinHook.x86.lib"
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
sin_collection_hook_fixture.exe
if not "%errorlevel%"=="0" goto failed
goto success
:navigationgeometry
cl /nologo /W4 /WX /O2 /EHsc /std:c++17 /MT /Fe:cross_water_fixture.exe "%TEST_SRC%cross_water_fixture.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp"
if not "%errorlevel%"=="0" goto failed
cross_water_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:navigation_fixture.exe "%TEST_SRC%navigation_fixture.cpp" "%TEST_SRC%..\atel.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp"
if not "%errorlevel%"=="0" goto failed
navigation_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--navigation-geometry" goto success
:pedestal
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /DFFX_RETIREMENT_TEST /Fe:pedestal_fixture.exe "%TEST_SRC%pedestal_fixture.cpp" "%TEST_SRC%..\pedestal_rules.cpp" "%TEST_SRC%..\pedestal_state.cpp" "%TEST_SRC%..\pedestal_keys.cpp" "%TEST_SRC%..\game_binding_input.cpp" "%TEST_SRC%..\pedestal_input.cpp" "%TEST_SRC%..\nav_interactions.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp" "%TEST_SRC%..\owned_retirement.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
pedestal_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--pedestal" goto success
:review
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:context_fixture.exe "%TEST_SRC%context_fixture.cpp" "%TEST_SRC%..\game_context.cpp"
if not "%errorlevel%"=="0" goto failed
context_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /DFFX_READER_TEST /Fe:auto_walk_fixture.exe "%TEST_SRC%auto_walk_fixture.cpp" "%TEST_SRC%..\auto_walk.cpp" "%TEST_SRC%..\movement_keys.cpp" "%TEST_SRC%..\game_binding_input.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
auto_walk_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHsc /std:c++17 /MT /Fe:blitz_review_fixture.exe "%TEST_SRC%blitz_review_fixture.cpp" "%TEST_SRC%..\blitz_review.cpp" "%TEST_SRC%..\text_plan.cpp"
if not "%errorlevel%"=="0" goto failed
blitz_review_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--review" goto success
:input
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:input_fixture.exe "%TEST_SRC%input_fixture.cpp" "%TEST_SRC%..\input.cpp" "%TEST_SRC%..\direct_input.cpp" /link user32.lib dxguid.lib
if not "%errorlevel%"=="0" goto failed
input_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /WX /EHa /std:c++17 /MT /Fe:direct_keyboard_fixture.exe "%TEST_SRC%direct_keyboard_fixture.cpp" "%TEST_SRC%..\direct_input.cpp" /link user32.lib dxguid.lib
if not "%errorlevel%"=="0" goto failed
copy /Y "%MINHOOK_ROOT%\bin\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
direct_keyboard_fixture.exe
if not "%errorlevel%"=="0" goto failed
:inputadapter
cl /nologo /W4 /WX /EHa /std:c++17 /MT /Fe:direct_input_adapter_fixture.exe "%TEST_SRC%direct_input_adapter_fixture.cpp" "%TEST_SRC%..\direct_input.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
direct_input_adapter_fixture.exe
if not "%errorlevel%"=="0" goto failed
if /I "%~1"=="--input-adapter" goto success
if /I "%~1"=="--input" goto success
cl /nologo /W4 /EHsc /MT /Fe:proxy_forwarding.exe "%TEST_SRC%proxy_forwarding.cpp" /link dxguid.lib
if not "%errorlevel%"=="0" goto failed
copy /Y "%REPO_ROOT%\install\native\dinput8.dll" dinput8.dll >nul
if not "%errorlevel%"=="0" goto failed
copy /Y "%REPO_ROOT%\install\native\MinHook.x86.dll" MinHook.x86.dll >nul
if not "%errorlevel%"=="0" goto failed
proxy_forwarding.exe
if not "%errorlevel%"=="0" goto failed
:utilities
python "%TEST_SRC%prepare_utilities.py"
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /DFFX_DEV_UTILITIES /Fe:utility_fixture.exe "%TEST_SRC%utility_fixture.cpp" "%TEST_SRC%..\utilities.cpp" "%TEST_SRC%..\utility_context.cpp" "%TEST_SRC%..\utility_memory.cpp" "%TEST_SRC%..\utility_scripts.cpp" "%TEST_SRC%..\gagazet.cpp" "%TEST_SRC%..\utility_trials.cpp" "%TEST_SRC%..\utility_warps.cpp" "%TEST_SRC%..\navigation_targets.cpp" "%TEST_SRC%..\puzzle_solver.cpp" "%TEST_SRC%..\zanarkand_patterns.cpp" "%TEST_SRC%..\via_purifico.cpp" "%TEST_SRC%..\puzzle_completion.cpp" "%TEST_SRC%..\puzzle_guide.cpp" "%TEST_SRC%..\puzzle_guide_owned.cpp" "%TEST_SRC%..\puzzle_tiles.cpp" "%TEST_SRC%..\nav_interactions.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp" "%TEST_SRC%..\pedestal_state.cpp" "%TEST_SRC%..\pedestal_rules.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\utility_text.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
utility_fixture.exe
if not "%errorlevel%"=="0" goto failed
cl /nologo /W4 /EHa /std:c++17 /MT /DFFX_READER_TEST /Fe:release_utility_fixture.exe "%TEST_SRC%utility_fixture.cpp" "%TEST_SRC%..\utilities.cpp" "%TEST_SRC%..\utility_context.cpp" "%TEST_SRC%..\utility_memory.cpp" "%TEST_SRC%..\utility_scripts.cpp" "%TEST_SRC%..\gagazet.cpp" "%TEST_SRC%..\utility_trials.cpp" "%TEST_SRC%..\utility_warps.cpp" "%TEST_SRC%..\navigation_targets.cpp" "%TEST_SRC%..\puzzle_solver.cpp" "%TEST_SRC%..\zanarkand_patterns.cpp" "%TEST_SRC%..\via_purifico.cpp" "%TEST_SRC%..\puzzle_completion.cpp" "%TEST_SRC%..\puzzle_guide.cpp" "%TEST_SRC%..\puzzle_guide_owned.cpp" "%TEST_SRC%..\puzzle_tiles.cpp" "%TEST_SRC%..\nav_interactions.cpp" "%TEST_SRC%..\nav_geometry.cpp" "%TEST_SRC%..\nav_zones.cpp" "%TEST_SRC%..\pedestal_state.cpp" "%TEST_SRC%..\pedestal_rules.cpp" "%TEST_SRC%..\owned_retirement.cpp" "%TEST_SRC%..\utility_text.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto failed
release_utility_fixture.exe
if not "%errorlevel%"=="0" goto failed
goto success
:sdlowner
call "%~dp0run_sdl_owner_tests.bat"
exit /b %errorlevel%
:success
popd
exit /b 0
:settingsdefaults
rem Keep settings-file tests isolated from other fixture executable settings.
if not exist settings-defaults mkdir settings-defaults
pushd settings-defaults
cl /nologo /W4 /WX /O2 /EHsc /std:c++17 /MT /DFFX_READER_TEST /Fe:settings_defaults_fixture.exe "%TEST_SRC%settings_defaults_fixture.cpp" "%TEST_SRC%..\settings.cpp" /link user32.lib
if not "%errorlevel%"=="0" goto settingsdefaultsfailed
settings_defaults_fixture.exe
if not "%errorlevel%"=="0" goto settingsdefaultsfailed
popd
goto success
:settingsdefaultsfailed
popd
goto failed
:failed
popd
exit /b 1
