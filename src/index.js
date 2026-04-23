// Entry point: bundler builds this file and all transitive imports into
// install/ffx_sr.js, loaded by frida-gadget at game startup.
//
// Install order matters: navigation registers its tick first so nav transition
// detection runs BEFORE battle updates state.inBattle — the original script
// depended on that ordering for the battle-transition autoscan skip.

import './core/speech.js';   // init log + Tolk
import './core/addresses.js';// resolve RVAs (also verifies memory.js load)

import { start as startFrame } from './core/frame.js';
import { install as installNavigation } from './navigation/index.js';
import { install as installBattle } from './battle/index.js';
import { install as installMenus } from './menus/index.js';
import { logFile } from './core/speech.js';

installNavigation();
installBattle();
installMenus();

startFrame();

logFile('FFX Screen Reader Mod fully loaded');
