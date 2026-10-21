// Offline prototype from archived native bodies; no live hook/probe producer.
// MsNumberRegist(3) follows a confirmed hit-test failure result, not zero HP.
// MsDamageCheckDeath commits +dcc/606 before deactivating killed monsters.
(function(root) {
    const api={
        graphic(kind,value,name) {
            if(kind===3) return name+': Miss';
            if(kind>1 || !value) return '';
            if(kind===1) return name+': '+(value<0?'restored '+(-value)+' MP':value+' MP damage');
            return name+': '+(value<0?'healed for '+(-value):value+' damage');
        },
        death(before,after,name) { return !before && after?name+': KO':''; },
        formation(slots) {
            const groups=new Map();
            for(const s of slots.slice(0,8)) if(s.active && s.visible && !s.dead && s.name) groups.set(s.name,(groups.get(s.name)||0)+1);
            return groups.size?'Enemies: '+[...groups].map(([n,c])=>n+(c>1?' x'+c:'')).join(', '):'';
        }
    };
    if(typeof module!=='undefined') module.exports=api; else root.FFXBattleOutcomes=api;
})(globalThis);
