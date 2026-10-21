// Offline Frida prototype model. Native production has one producer.
(function(root) {
  'use strict';
  function pickup(player, target, angle, radius, height) {
    if (![player.x,player.y,player.z,target.x,target.y,target.z,angle,radius,height].every(Number.isFinite) || radius<=.1 || height<=0) return null;
    const x=target.x-player.x,z=target.z-player.z;
    const ahead=x*Math.cos(angle)+z*Math.sin(angle), lateral=x*Math.sin(angle)-z*Math.cos(angle);
    if (ahead < -radius || Math.abs(target.y-player.y)>=height) return null;
    return {ahead,lateral,pan:Math.max(-1,Math.min(1,lateral/radius)),
      proximity:Math.max(.2,1-Math.max(0,ahead)/450),aligned:Math.abs(lateral)<radius-.1};
  }
  function carrier(base,slot) { return (base===2 || base===10) && slot>=0 && slot<6 ? base+slot : -1; }
  function team(worker) { return worker>=2 && worker<=7 ? 0 : worker>=10 && worker<=15 ? 1 : -1; }
  function possession(previous,next) { return team(previous)>=0 && team(next)>=0 && team(previous)!==team(next); }
  function goal(previous,next) { return previous>=0 && next>previous; }
  const model={pickup,carrier,team,possession,goal};
  if (typeof module!=='undefined') module.exports=model; else root.MinigameSpatial=model;
})(globalThis);
