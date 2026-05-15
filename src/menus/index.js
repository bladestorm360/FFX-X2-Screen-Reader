// Menus aggregator. Installs menus (field menu + TkWindow), dialog
// (NPC dialogue + cutscene subtitles), save/load, ESC menu, and config
// (+ sphere grid) submodules.

import * as menus from './menus.js';
import * as dialog from './dialog.js';
import * as saveload from './saveload.js';
import * as escmenu from './escmenu.js';
import * as config from './config.js';
import * as status from './status.js';

export function install() {
    menus.install();
    dialog.install();
    saveload.install();
    escmenu.install();
    config.install();
    status.install();
}
