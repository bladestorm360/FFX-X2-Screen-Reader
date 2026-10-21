// @desc Offline native interaction flags, entry mappings and counter-zone prototype
// No attachment, game reads, speech or writes; C++ owns production events.
function activeInteraction(flags,entries,entryMask) {
    for(var channel=0;channel<7;++channel) if((flags&(1<<channel)) && entries[channel]<32 && (entryMask&(1<<entries[channel]))) return true;
    return false;
}
function zoneDistance(zone,point) {
    var x=point.x-zone.x,z=point.z-zone.z;
    if(zone.shape===3) {
        var c=Math.cos(zone.angle),s=Math.sin(zone.angle),u=x*c-z*s,v=x*s+z*c;
        return Math.hypot(Math.max(0,Math.abs(u)-zone.width),Math.max(0,Math.abs(v)-zone.depth));
    }
    if(zone.shape===2) {
        var dx=zone.endX-zone.x,dz=zone.endZ-zone.z,length=dx*dx+dz*dz;
        var t=length?Math.max(0,Math.min(1,(x*dx+z*dz)/length)):0;
        return Math.hypot(x-t*dx,z-t*dz);
    }
    return Math.hypot(x,z);
}
