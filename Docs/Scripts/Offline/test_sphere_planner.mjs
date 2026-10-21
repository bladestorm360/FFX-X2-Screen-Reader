import { readFileSync } from 'node:fs';
import { createContext, runInContext } from 'node:vm';
import assert from 'node:assert/strict';
const context=createContext({});
runInContext(readFileSync(new URL('../Frida/FFX/sphere_planner.js',import.meta.url),'utf8'),context);
const grid={nodes:Array.from({length:5},()=>({valid:true})),links:[
    {a:0,b:1,travelled:1},{a:1,b:2,travelled:1},{a:0,b:3,travelled:0},{a:3,b:2,travelled:0}
]};
const route=(from,to,character=0)=>context.sphereRoute(grid,from,to,character);
assert.deepEqual(Array.from(route(0,2)),[0,1,2]);
grid.nodes[1].locked=true;
assert.deepEqual(Array.from(route(0,2)),[0,3,2]);
assert.equal(route(0,4),null);
assert.equal(route(-1,2),null);
assert.equal(route(0,2,7),null);
grid.nodes[2].ability=7;
assert.deepEqual(Array.from(route(3,2)),[3,2]);
assert.deepEqual(Array.from(route(2,2)),[2]);
console.log('PASS: offline Frida route prototype, travel history, lock preference, invalid/disconnected destinations and adjacent ability activation.');
