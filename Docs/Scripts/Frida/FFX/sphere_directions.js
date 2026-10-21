// @desc Offline native Sphere Grid arrow selection and diagonal wording prototype
// Pure helpers only: no attachment, game access, speech or writes.
var gridArrows=[
    {x:0,y:-1,text:'up'}, {x:1,y:0,text:'right'},
    {x:0,y:1,text:'down'}, {x:-1,y:0,text:'left'},
    {x:-1,y:-1,text:'diagonally up and left'},
    {x:1,y:-1,text:'diagonally up and right'},
    {x:-1,y:1,text:'diagonally down and left'},
    {x:1,y:1,text:'diagonally down and right'}
];
function gridArrowChoices(nodes,at) {
    var choices=gridArrows.map(function() {return -1;}),best=gridArrows.map(function() {return 0;});
    nodes.forEach(function(node,index) {
        if(!node.valid) return;
        var x=node.x-nodes[at].x,y=node.y-nodes[at].y;
        var distance=Math.fround(Math.sqrt(x*x+y*y));
        if(!distance) return;
        gridArrows.forEach(function(arrow,key) {
            var dot=x*arrow.x+y*arrow.y;
            if(dot<=.5) return;
            var score=Math.fround(dot/(distance*distance));
            if(score>best[key]) {best[key]=score;choices[key]=index;}
        });
    });
    return choices;
}
function gridArrowWords(choices,goal) {
    var words=[];
    for(var key=0;key<8;++key) {
        if(key===4 && words.length) break;
        if(choices[key]===goal) words.push(gridArrows[key].text);
    }
    return words.join(' or ');
}
