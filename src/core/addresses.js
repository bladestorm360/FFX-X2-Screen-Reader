// Hardcoded RVAs, enum tables, structure offsets. Resolved against `base` at load.

import { rva } from './memory.js';
import { logFile } from './speech.js';

// ── Character names & battle window types ──
export const CHARACTER_NAMES = ['Tidus','Yuna','Auron','Kimahri','Wakka','Lulu','Rikku','Seymour',
    'Valefor','Ifrit','Ixion','Shiva','Bahamut','Anima','Yojimbo',
    'Cindy','Sandy','Mindy','Dummy','Dummy2'];

export const BTL_WIN_SIZE = 0xF0;

export const BTL_WIN_TYPES = {
    0x00: 'Main', 0x01: 'Black Magic', 0x02: 'White Magic', 0x03: 'Skill',
    0x04: 'Overdrive', 0x05: 'Summon', 0x06: 'Item', 0x07: 'Weapon',
    0x0A: 'Change', 0x0C: 'Left', 0x0D: 'Right', 0x0E: 'Special',
    0x0F: 'Armor', 0x11: 'Use', 0x14: 'Mix', 0x15: 'Spare Change',
    0x16: 'Yojimbo Pay'
};

export const NODE_TYPE_NAMES = {
    0x00:'Lock Lv.3', 0x01:'Empty',
    0x02:'Strength +1', 0x03:'Strength +2', 0x04:'Strength +3', 0x05:'Strength +4',
    0x06:'Defense +1', 0x07:'Defense +2', 0x08:'Defense +3', 0x09:'Defense +4',
    0x0A:'Magic +1', 0x0B:'Magic +2', 0x0C:'Magic +3', 0x0D:'Magic +4',
    0x0E:'Magic Def +1', 0x0F:'Magic Def +2', 0x10:'Magic Def +3', 0x11:'Magic Def +4',
    0x12:'Agility +1', 0x13:'Agility +2', 0x14:'Agility +3', 0x15:'Agility +4',
    0x16:'Luck +1', 0x17:'Luck +2', 0x18:'Luck +3', 0x19:'Luck +4',
    0x1A:'Evasion +1', 0x1B:'Evasion +2', 0x1C:'Evasion +3', 0x1D:'Evasion +4',
    0x1E:'Accuracy +1', 0x1F:'Accuracy +2', 0x20:'Accuracy +3', 0x21:'Accuracy +4',
    0x22:'HP +200', 0x23:'HP +300', 0x24:'MP +40', 0x25:'MP +20', 0x26:'MP +10',
    0x27:'Lock Lv.1', 0x28:'Lock Lv.2', 0x29:'Lock Lv.4',
    0xFF:'NULL'
};

export const SCENE_NAMES = {
    0x01: 'Main Menu', 0x02: 'Abilities', 0x04: 'Equip', 0x06: 'Status',
    0x07: 'Aeons', 0x09: 'Formation', 0x0A: 'Config', 0x0B: 'Help',
    0x15: 'Overdrive'
};

export const TITLE_ITEMS = ['New Game', 'Load'];

export const PERM_STATUS_NAMES = ['KO','zombie','petrify','poison','power break','magic break',
    'armor break','mental break','confuse','berserk','provoke','threaten'];
export const TEMP_STATUS_NAMES = ['sleep','silence','darkness','shell','protect','reflect',
    'nul-water','nul-fire','nul-thunder','nul-ice','regen','haste','slow'];
export const EXTRA_STATUS_NAMES = ['scan','','','','','','shield','boost','eject','auto-life',
    'curse','defend','guard','sentinel','doom'];

// ── Battle system addresses ──
export const btlBaseAddr       = rva(0xD2A8D0);
export const btlStateAddr      = rva(0xD2A8D0 + 0x10);
export const btlWinArrayAddr   = rva(0xF3C910);
export const btlCurWinIdxAddr  = rva(0x1FCC092);
export const btlCommandBinAddr = rva(0xD2A8D0 + 0x5C);
export const btlItemBinAddr    = rva(0xD2A8D0 + 0x70);
export const saveDataAddr      = rva(0xD2CA90);
export const INVENTORY_IDS_OFF = 0x3ECC;
export const INVENTORY_CNT_OFF = 0x40CC;
export const monsterChrsAddr   = rva(0xD34460);
export const playerChrsAddr    = rva(0xD334CC);
export const CHR_SIZE = 0xF90;
export const CHR_RAM_OFFSET = 0x540;
export const CHR_IN_BATTLE = 0xDC8;
export const btlRewardAddr = rva(0x1F10EA0);
export const helpTextAddr  = rva(0xF3F4F8);

// ── Field menu ──
export const fieldMenuCursorAddr = rva(0x1471508);
export const fieldMenuPlayerAddr = rva(0x147150C);
export const fieldMenuItemTable  = rva(0x1471528);
export const eventIdAddr         = rva(0xEFBBF8);

// ── TkWindow (title screen, popups) ──
export const tkWinArrayAddr = rva(0x14676B0);
export const tkCurArrayAddr = rva(0x1468A50);

// ── Sphere Grid ──
export const sgridIsOpenAddr    = rva(0x1685F70);
export const sgridEnginePtrAddr = rva(0x1F05834);

// ── Config menu ──
export const configCursorAddr   = rva(0x146A404);
export const configCountAddr    = rva(0x146A408);
export const configArrayPtrAddr = rva(0x146A40C);
export const configSpecialIdx   = rva(0x146A3F8);

// ── Navigation ──
export const NAV_ACTOR_SIZE = 0x880;
export const NAV_MAX_ACTORS = 128;
export const NAV_UNITS_PER_STEP = 11.13;
export const NAV_MAX_DISTANCE = 500;
export const RVA_ACTOR_ARRAY  = 0x1FC44E4;
export const RVA_TRIS     = 0xF01A44;
export const RVA_TRICOUNT = 0xF01A48;
export const RVA_VERTICES = 0xF01A4C;
export const RVA_SCALE    = 0xF01A50;
export const RVA_CURRENT_WORKER = 0xF270A4;
export const RVA_ATEL_CONTROLLER = 0xF26AE8;
export const RVA_MS_CAMERA_NOW = 0x1F11430;
export const CAM_EYE_OFFSET    = 0x40;
export const CAM_TARGET_OFFSET = 0x65C;

logFile('Addresses resolved');
