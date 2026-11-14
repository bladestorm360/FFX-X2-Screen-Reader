#pragma once
#include "text_plan.hpp"
namespace ffx {
struct SphereNodePlan {TextRef name=0,description=0;unsigned flags=0,activated=0,sphere=0;int type=-1;bool valid=false;};
void prepareSphereText();
void invalidateSphereText();
SphereNodePlan planSphereNode(TextPlan&,int,bool materials=false);
TextRef planSphereItemDescription(TextPlan&,unsigned);
}
