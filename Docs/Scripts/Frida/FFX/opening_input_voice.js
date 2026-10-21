// Offline event-order prototype. Never installed as a runtime producer.
(function(root) {
    class Opening {
        constructor() { this.read=false; this.messages=[]; }
        appear(slots) {
            if(this.read || slots.slice(0,8).some(s=>s.active && !s.dead && (!s.visible || !s.actor || !s.name))) return [];
            const names=slots.slice(0,8).filter(s=>s.active && !s.dead && s.visible && s.actor).map(s=>s.name);
            if(!names.length) return [];
            this.read=true; const out=['Enemies: '+names.join(', '),...this.messages]; this.messages=[]; return out;
        }
        message(text) { if(this.read) return [text]; this.messages.push(text); return []; }
    }
    function pad(held,reserved) { return held.filter(control=>!reserved.includes(control)); }
    function dialogue(prepared,playing,text) { return prepared && !playing?{pending:text}:{text,voiced:playing}; }
    function walkable(blocked) { return blocked===0; }
    class Guide {
        constructor(count) { this.selected=0; this.expected=0; this.count=count; }
        move(delta) { this.selected=Math.max(0,Math.min(this.count-1,this.selected+delta)); return this.selected; }
        checkpoint(index) { this.expected=Math.max(this.expected,Math.min(this.count-1,index+1)); return this.expected; }
    }
    const api={Opening,pad,dialogue,walkable,Guide};
    if(typeof module!=='undefined') module.exports=api; else root.FFXOpeningInputVoice=api;
})(globalThis);
