#pragma once
#include "message_text.hpp"
#include "atel_resources.hpp"
#include <array>
namespace ffx {
struct BlitzSnapshot {
    unsigned epoch=0,revision=0;
    AtelSpan span;
    unsigned ticket=64,ticketSerial=0;
    bool valid=false,lobby=false,statsValid=false;
    std::array<int,9> stats{};
    unsigned statsCount=0;
    int position=-1;
};
AtelPreparation blitzSnapshotPreparation(const BlitzSnapshot&);
void releaseBlitzSnapshot(const BlitzSnapshot&);
BlitzSnapshot captureBlitzSnapshot(bool statistics=false,bool position=false);
bool observeBlitzMessageOwned(unsigned slot,const MessagePage&,const BlitzSnapshot&);
void installBlitzReview();
bool blitzReviewAvailable();
bool blitzPositionAvailable();
bool blitzCapturesKey(unsigned vk,bool control);
bool dispatchBlitzKey(unsigned vk,bool control);
void resetBlitzReview();
}
