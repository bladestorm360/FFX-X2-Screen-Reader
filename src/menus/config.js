// Config menu + Sphere Grid monitoring.

import {
    configCursorAddr, configArrayPtrAddr, configSpecialIdx,
    sgridIsOpenAddr, sgridEnginePtrAddr,
    saveDataAddr, CHARACTER_NAMES, NODE_TYPE_NAMES
} from '../core/addresses.js';
import { decodeFFXSegments, resolveConfigText, resolveLocString } from '../core/charset.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { isValidPtr } from '../core/safeRead.js';
import { registerHotkey } from '../core/frame.js';
import { VK_S } from '../core/input.js';

let prevConfigCursor = -1, prevConfigSelection = -1;
let configActive = false;
let prevSgridOpen = false, prevSgridSelectedNode = -1, prevSgridChrId = -1;

function readConfigItem(cursorIdx) {
    try {
        const arrayPtr = configArrayPtrAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        const itemPtr = arrayPtr.add(cursorIdx * 4).readPointer();
        if (itemPtr.isNull()) return null;
        return {
            numOptions: itemPtr.add(0x04).readS32(),
            curSelection: itemPtr.add(0x08).readS32(),
            visible: itemPtr.add(0x0C).readS32(),
            textId: itemPtr.add(0x18).readU16(),
            ptr: itemPtr
        };
    } catch (e) { return null; }
}

function readSgridNode(engineBase, nodeIdx) {
    try {
        const nodeBase = engineBase.add(0x808 + nodeIdx * 0x28);
        const rawType = nodeBase.add(0x6).readS16();
        const nodeType = (rawType === -1) ? 0xFF : (rawType & 0xFF);
        // Nodes have up to 5 link slots at +0xC/+0x10/+0x14/+0x18/+0x1C
        // (stride 4). Non-zero = link present.
        let linkCount = 0;
        for (let li = 0; li < 5; li++) {
            if (nodeBase.add(0xC + li * 4).readU32() !== 0) linkCount++;
        }
        return {
            type: nodeType,
            typeName: NODE_TYPE_NAMES[nodeType] || null,
            activatedBy: nodeBase.add(0x21).readU8(),
            props: nodeBase.add(0x22).readU8(),
            x: nodeBase.add(0x0).readS16(),
            y: nodeBase.add(0x2).readS16(),
            linkCount: linkCount
        };
    } catch (e) { return null; }
}

function readSgridSlv(chrIdx) {
    try {
        const plySave = saveDataAddr.add(0x55CC + chrIdx * 0x94);
        return {
            available: plySave.add(0x3B).readU8(),
            spent: plySave.add(0x3C).readU8()
        };
    } catch (e) { return null; }
}

// Announce the currently-selected sphere grid node. Called from the open
// hook (initial announce on entry) and the node-selector hook (each move).
// The node selector fn (FUN_00A48D70) has multiple callers -- not all of
// them correspond to user cursor moves (render/animation paths also call
// it). We filter by comparing the new selNode to our cached prev value;
// only announce when the node index actually changed. The alternative
// ("no dedup, trust the hook to fire once") doesn't apply here because
// the hook genuinely fires multiple times per logical cursor move.
function announceSelectedNode() {
    try {
        const eng = sgridEnginePtrAddr.readPointer();
        if (!isValidPtr(eng)) return;
        const selNode = eng.add(0x112FC).readS32();
        if (selNode < 0) return;
        if (selNode === prevSgridSelectedNode) return;  // same node, skip
        const node = readSgridNode(eng, selNode);
        if (!node) return;
        let desc = node.typeName || 'ability node';
        if (!node.typeName) {
            const nctPtr = eng.add(0x11654).readPointer();
            if (isValidPtr(nctPtr)) {
                const nctSegs = decodeFFXSegments(nctPtr, 128, 0);
                if (nctSegs.length > 0) desc = nctSegs[0];
            }
        }
        // Include link count so the user knows how many adjacent nodes
        // they can move to from here.
        const linkStr = (node.linkCount === 1) ? ', 1 link'
            : ', ' + node.linkCount + ' links';
        speak(desc + linkStr, true);
        prevSgridSelectedNode = selNode;
    } catch (e) {}
}

// Announce the current sphere-grid character name. Called on sphere grid
// open and on character switch. S.Lv counts are NOT announced here — the
// user asks for them on demand via the VK_S hotkey (handleSphereAvailHotkey).
function announceCurrentCharacter() {
    try {
        const eng = sgridEnginePtrAddr.readPointer();
        if (!isValidPtr(eng)) return;
        const chrId = eng.add(0x115BC).readU8();
        const name = CHARACTER_NAMES[chrId] || ('chr' + chrId);
        speak(name, true);
        prevSgridChrId = chrId;
    } catch (e) {}
}

// Hotkey handler: read S.Lv available for every party character. Only
// active while the sphere grid is open. Format matches the battle H
// party-HP hotkey: comma-separated "Name N" pairs. CHARACTER_NAMES[0..6]
// are the seven FFX party members.
function handleSphereAvailHotkey() {
    try {
        if (sgridIsOpenAddr.readU8() === 0) return;
        const parts = [];
        for (let i = 0; i < 7; i++) {
            const slv = readSgridSlv(i);
            if (!slv) continue;
            const name = CHARACTER_NAMES[i] || ('chr' + i);
            parts.push(name + ' ' + slv.available);
        }
        if (parts.length > 0) speak(parts.join(', '), true);
    } catch (e) {}
}

export function install() {
    attachRva(0x4CB0B0, {
        onEnter: function(args) { this.param1 = args[0]; },
        onLeave: function() {
            if (!isValidPtr(this.param1)) return;
            const itemState = this.param1.add(0x1C).readS32();
            if (itemState < 3) { if (!configActive) configActive = true; return; }
            if (itemState > 3) return;
            configActive = true;
            const cursor = configCursorAddr.readU8();
            const item = readConfigItem(cursor);
            if (!item) return;
            const cursorChanged = (cursor !== prevConfigCursor);
            const selectionChanged = (item.curSelection !== prevConfigSelection);
            if (!cursorChanged && !selectionChanged) return;
            const specialIdx = configSpecialIdx.readS32();
            const isSpecial = (cursor === specialIdx);
            let settingName = '', optionName = '';
            if (isSpecial) {
                settingName = resolveLocString(0x1CD) || 'Music';
                const musicOptIds = [0x1CE, 0x1D0];
                optionName = (item.curSelection >= 0 && item.curSelection < musicOptIds.length)
                    ? (resolveLocString(musicOptIds[item.curSelection]) || 'option ' + item.curSelection)
                    : 'option ' + item.curSelection;
            } else {
                settingName = resolveConfigText(item.textId) || ('setting ' + item.textId);
                try {
                    const optTextId = item.ptr.add(0x1C + item.curSelection * 4).readU32();
                    optionName = resolveConfigText(optTextId) || ('option ' + item.curSelection);
                } catch (e) { optionName = 'option ' + item.curSelection; }
            }
            speak(cursorChanged ? (settingName + ': ' + optionName) : optionName, true);
            prevConfigCursor = cursor;
            prevConfigSelection = item.curSelection;
        }
    }, 'Config fn_ctrl');

    // Sphere grid render/update (FUN_00A53570). Decompile line 1460
    // unconditionally writes sgridIsOpenAddr = 1 and then runs
    // animation/layout work — meaning this function is called every
    // frame while sphere grid is active, not just on open. To catch the
    // open EVENT (closed -> open transition), capture the flag byte in
    // onEnter and only announce when it was 0 before the write. Same
    // pattern as the TkWindow visibility setter (FUN_008B5F60).
    attachRva(0x653570, {
        onEnter: function() {
            this.openBefore = 1;
            try { this.openBefore = sgridIsOpenAddr.readU8(); } catch (e) {}
        },
        onLeave: function() {
            if (this.openBefore !== 0) return; // already open, no transition
            try {
                if (sgridIsOpenAddr.readU8() === 0) return;
                prevSgridOpen = true;
                prevSgridSelectedNode = -1;  // force initial node announce
                prevSgridChrId = -1;          // force initial character announce
                announceCurrentCharacter();
                announceSelectedNode();
            } catch (e) {}
        }
    }, 'Sphere grid open');

    // Sphere grid close: three writers found that set sgridIsOpenAddr = 0
    // (FUN_00A53DE0 / FUN_00A54B40 / FUN_00A56060). All reset our cache.
    function makeCloseHandler() {
        return {
            onLeave: function() {
                try {
                    if (sgridIsOpenAddr.readU8() !== 0) return;
                    prevSgridOpen = false;
                    prevSgridSelectedNode = -1;
                    prevSgridChrId = -1;
                } catch (e) {}
            }
        };
    }
    attachRva(0x653DE0, makeCloseHandler(), 'Sphere grid close A');
    attachRva(0x654B40, makeCloseHandler(), 'Sphere grid close B');
    attachRva(0x656060, makeCloseHandler(), 'Sphere grid close C');

    // Sphere grid node-cursor move: FUN_00A48D70 writes
    // Native binding/layout evidence: DAT_02305834. Refer to the external original; copied implementation removed.
    // selection change; hook fires per cursor move while sphere grid open.
    attachRva(0x648D70, {
        onLeave: function() {
            try {
                if (sgridIsOpenAddr.readU8() === 0) return;
                announceSelectedNode();
            } catch (e) {}
        }
    }, 'Sphere grid node selector');

    // Sphere grid character switch: FUN_008A9810 (RVA 0x4A9810) is the
    // shoulder-button resolver called from the char-switch branch of the
    // sphere-grid state machine (FUN_00A58EC0 line 5091). Its return value
    // is the new current-character id written to eng+0x115BC. Fires once
    // per L1/R1 press. Guard on sgridIsOpenAddr in case the function has
    // other callers, and diff against prevSgridChrId so an edge press
    // that resolves to the same character doesn't re-announce.
    attachRva(0x4A9810, {
        onLeave: function() {
            try {
                if (sgridIsOpenAddr.readU8() === 0) return;
                const eng = sgridEnginePtrAddr.readPointer();
                if (!isValidPtr(eng)) return;
                const chrId = eng.add(0x115BC).readU8();
                if (chrId === prevSgridChrId) return;
                announceCurrentCharacter();
            } catch (e) {}
        }
    }, 'Sphere grid char switch');

    registerHotkey(VK_S, handleSphereAvailHotkey, 'sgrid.sphere-avail');

    logFile('Menus/config feature installed');
}
