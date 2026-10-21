// Offline Frida identity prototype. No hooks, runtime learning or speech producer.
function navigationIdentity(authored, speaker, service, sprite, category, modelKind) {
    if (modelKind && modelKind !== "character" && category === "NPC") return sprite || "Object";
    if (authored) return authored.replace(/^(?:Talk (?:to|with)|Speak (?:to|with)|Interact with)\s+/i, '');
    if (speaker) return speaker;
    if (service) return service;
    if (sprite) return sprite;
    return category === 'NPC' ? 'NPC' : category === 'Object' ? 'Object' : 'Event';
}
function interactionPromptVisible(displayed, inRange, text) {
    return displayed && inRange ? text : '';
}

function navigationCategory(category, modelKind) {
    return category === "NPC" && modelKind !== "character" ? "Object" : category;
}
