// @desc Offline read-only standard-section guidance and owned-key routing prototype
// No attachment, game reads, speech or writes. Native C++ is the sole producer.
function sectionChoices(meta, links, at, forward) {
    var here=meta[at];
    if(!here || here.section<0) return [];
    return links.map(function(edge) { return edge.a===at?edge.b:edge.b===at?edge.a:-1; })
        .filter(function(next) {
            var there=meta[next];
            if(!there || there.section!==here.section) return false;
            var ahead=there.depth>here.depth || (there.depth===here.depth && next>at);
            return ahead===forward;
        }).sort(function(a,b) { return a-b; });
}
function nearestSection(grid,meta,start,section,character,stock) {
    if(!grid.nodes[start] || character<0 || character>6) return null;
    var labels=[{at:start,cost:0,spent:{},path:[start]}], pending=[0];
    while(pending.length) {
        pending.sort(function(a,b) { return labels[b].cost-labels[a].cost; });
        var state=labels[pending.pop()];
        if(meta[state.at].section===section && !grid.nodes[state.at].locked) return state.path;
        grid.links.forEach(function(edge) {
            var next=edge.a===state.at?edge.b:edge.b===state.at?edge.a:-1;
            if(next<0 || !grid.nodes[next].valid) return;
            var spent=Object.assign({},state.spent), node=grid.nodes[next];
            if(node.locked) {
                if(!node.sphere || (spent[node.sphere]||0)>=(stock[node.sphere]||0)) return;
                spent[node.sphere]=(spent[node.sphere]||0)+1;
            }
            var cost=(node.locked?Math.ceil(state.cost/4)*4:state.cost)+((edge.travelled&(1<<character))?1:4);
            var dominated=labels.some(function(old) {
                return old.at===next && old.cost<=cost && Object.keys(old.spent).every(function(id) { return old.spent[id]<=(spent[id]||0); });
            });
            if(dominated) return;
            labels.push({at:next,cost:cost,spent:spent,path:state.path.concat(next)}); pending.push(labels.length-1);
        });
    }
    return null;
}
function visibleGear(ids,slots,nativeName) {
    var names=ids.filter(function(id) { return id && id!==255 && id!==65535; }).map(nativeName);
    return {names:names,empty:Math.max(slots,names.length)-names.length};
}
function statGoals(grid,stat,character) {
    return grid.nodes.map(function(node) {
        return node.valid && !node.locked && !node.ability && (node.flags&(1<<stat)) && !(node.activated&(1<<character))?0:-1;
    });
}
function genericPathDepth(grid,anchor) {
    var depth=grid.nodes.map(function() {return -1;}),pending=[anchor];depth[anchor]=0;
    for(var i=0;i<pending.length;++i) {
        var at=pending[i];
        grid.links.forEach(function(edge) {
            var next=edge.a===at?edge.b:edge.b===at?edge.a:-1;
            if(next>=0 && grid.nodes[next].valid && depth[next]<0) {depth[next]=depth[at]+1;pending.push(next);}
        });
    }
    return depth;
}
