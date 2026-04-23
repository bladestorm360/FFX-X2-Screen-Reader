// Menus aggregator. Installs dialog (field menu + TkWindow), save/load,
// and config (+ sphere grid) submodules.

import * as dialog from './dialog.js';
import * as saveload from './saveload.js';
import * as config from './config.js';

export function install() {
    dialog.install();
    saveload.install();
    config.install();
}
