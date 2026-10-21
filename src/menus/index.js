// Menus aggregator. Installs menus (field menu + TkWindow), dialog
// (NPC dialogue + cutscene subtitles), save/load, ESC menu, and config
// (+ sphere grid) submodules.

import * as menus from './menus.js';
import * as dialog from './dialog.js';
import * as saveload from './saveload.js';
import * as escmenu from './escmenu.js';
import * as config from './config.js';
import * as status from './status.js';
import * as items from './items.js';
import * as commandBars from './commandBars.js';
import * as fieldLists from './fieldLists.js';
import * as partyLists from './partyLists.js';
import * as help from './help.js';
import * as sphereLists from './sphereLists.js';
import * as repairs from './repairs.js';

export function install() {
    menus.install();
    dialog.install();
    saveload.install();
    escmenu.install();
    config.install();
    status.install();
    items.install();
    commandBars.install();
    fieldLists.install();
    partyLists.install();
    help.install();
    sphereLists.install();
    repairs.install();
}
