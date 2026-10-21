// @desc Native message page/choice parser prototype (offline; no speech owner)
// Native token widths: 008DFD70; page stops: 008B7EF0; rows: 008B9A10.
function interactionPage(bytes, resolve, depth = 0, budget = { remaining: 0x4000 }) {
    const page = { prompt: '', choices: Array(32).fill(''), selector: false };
    let row = -1;
    function parse(data, level) {
        if (level >= 8) return;
        for (let i = 0; i < data.length && i < 0x1000 && budget.remaining;) {
            const code = data[i], payload = data[i + 1];
            if (code <= 2) break;
            const width = code === 4 ? (payload < 0x30 ? 3 : 2) :
                ((code >= 6 && code <= 14) || code === 16 || (code >= 18 && code <= 47)) ? 2 : 1;
            if (i + width > data.length || width > budget.remaining) break;
            budget.remaining -= width;
            let text = '';
            if (code === 3) text = ' ';
            else if (code === 16 && payload >= 0x30 && payload < 0x50) { page.selector = true; row = payload - 0x30; }
            else if (code === 16 && payload === 0xff) row = -1;
            else if (code >= 18 && code <= 35 && ![20, 21, 24].includes(code)) {
                const expanded = resolve && resolve(code, payload);
                if (expanded && expanded !== data) parse(expanded, level + 1);
            } else if (code >= 0x30) {
                const charset = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~';
                text = charset[code - 0x30] || '';
            }
            if (row < 0) page.prompt += text; else page.choices[row] += text;
            i += width;
        }
    }
    parse(bytes, depth);
    page.prompt = page.prompt.trim(); page.choices = page.choices.map(s => s.trim());
    return page;
}
if (typeof module !== 'undefined') module.exports = { interactionPage };
else throw Error('Offline prototype only; the native reader owns message speech.');
