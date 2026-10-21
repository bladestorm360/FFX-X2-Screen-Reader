#pragma once
#include "navigation.hpp"
#include "speech_capture.hpp"
#include <atomic>
namespace ffx {
enum class Setting : unsigned;
namespace nav {struct Scene;}
struct ButterflyCue;
inline std::atomic<unsigned> navAudioContextRevision{1};
inline unsigned navAudioContextSerial() noexcept {return navAudioContextRevision.load(std::memory_order_acquire);}
// Native gate/resource events retire synthetic movement immediately. A copied
// scene carries this serial through route computation and native dispatch; an
// old route cannot become current again when a gate closes and reopens.
inline std::atomic<unsigned> navCollisionRevision{1};
inline unsigned navCollisionSerial() noexcept {return navCollisionRevision.load(std::memory_order_acquire);}
struct NavAudioState {unsigned contextSerial=navAudioContextSerial(); float pan=0,front=1,steps=0; unsigned walls=0; bool beacon=false,active=false; };
struct NavLeg { Point start,goal,final; NativeRouteTarget target; unsigned epoch=0; bool arrived=false;
    unsigned collisionSerial=navCollisionSerial();bool pendingCollision=false; };
bool navLeg(NavLeg&);
void publishNavAudio(const NavAudioState&);
void pauseNavAudio();
// Native owner grants ordinary field/minigame continuous audio only in its
// actual context. Previews and independent timing events keep their own gates.
// The mixer reads copied atomics, never native map/script pointers.
void setFieldAudioAllowed(bool);
void pauseNavRouteAudio();
void stopNavPreview();
void previewNavAudio(bool walls);
void startNavAudio();
void navAudioArrival();
void navAudioLightningWarning(unsigned epoch);
void navAudioCancelLightningWarning();
void navAudioOverdriveBeep(float frequency,unsigned epoch);
void navAudioTimingBeep(float frequency,unsigned epoch,Setting gate);
void navAudioCancelGagazetBeep();
void navAudioCancelOverdriveBeep();
void routeNavAudio(const std::vector<Point>&,const Entity&);
void updateNavAudio(Point,bool moving);
void resetNavAudio();
void clearNavRoute();
void refreshNavCollision();
bool navRouteKnown();
void refreshSelectedBeacon();
void publishPositionalCue(float pan,float proximity,bool active,unsigned epoch);
void publishButterflyCue(const ButterflyCue&);
void updateNavTarget(const Entity&);
// Sole preparation owner supplies copied state; no Native getter is entered.
void updateOwnedNavAudio(const nav::Scene&,const Mesh&,const std::vector<Entity>&,const speechq::Header&,const std::wstring& pedestal);
}
