// Copy the current map's ATEL data while handling a navigation event.
// All returned data is JS-owned. Never retain game pointers across scans.
import { isValidPtr, safeReadByteArray } from '../core/safeRead.js';
import { decodeInstructions } from './atel.js';

const LIMIT = 0x100000;

function table(chunk, offset, count, width, read) {
    if (offset > LIMIT || count > 4096 || offset + count * width > LIMIT) {
        throw new Error('Invalid ATEL table bounds');
    }
    const values = [];
    for (let i = 0; i < count; i++) values.push(read(chunk.add(offset + i * width)));
    return values;
}

export function readScript(chunk) {
    if (!isValidPtr(chunk)) throw new Error('Invalid ATEL chunk');
    const codeLength = chunk.readU32();
    const codeOffset = chunk.add(0x30).readU32();
    const workerCount = chunk.add(0x34).readU16();
    if (!codeLength || codeLength > LIMIT || codeOffset + codeLength > LIMIT ||
        !workerCount || workerCount > 1024) throw new Error('Invalid ATEL script bounds');
    const bytes = safeReadByteArray(chunk.add(codeOffset), codeLength);
    if (!bytes) throw new Error('Unreadable ATEL bytecode');
    const code = new Uint8Array(bytes);
    const workers = [];
    for (let i = 0; i < workerCount; i++) {
        const offset = chunk.add(0x38 + i * 4).readU32();
        if (!offset || offset + 0x34 > LIMIT) throw new Error('Invalid ATEL worker offset');
        const header = chunk.add(offset);
        workers.push({
            headerOffset: offset,
            type: header.readU16(),
            entries: table(chunk, header.add(0x20).readU32(), header.add(8).readU16(), 4, p => p.readU32()),
            jumps: table(chunk, header.add(0x24).readU32(), header.add(0xA).readU16(), 4, p => p.readU32()),
            ints: table(chunk, header.add(0x18).readU32(), header.add(4).readU16(), 4, p => p.readS32())
        });
    }
    return { code, workers, instructions: decodeInstructions(code) };
}
