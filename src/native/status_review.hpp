#pragma once
#include "events.hpp"
#include "text_plan.hpp"
namespace ffx {
struct StatusBatch {
    unsigned serial=0,batch=0,count=0,character=0,fixedRows=0,focusRow=0,nativeArrows=0;
    int page=0;
    bool valid=true,grid=false,reset=false,replaceContext=true,aeon=false,ability=false,present=false,help=false,battle=false,helpMenu=false,helpOpening=false;
};
// Row context[5] describes its native Help role; no text-content classification.
enum HelpRowRole : unsigned {HelpContent,HelpControls,HelpPrompt,HelpCharacter};
// Battle result row context[6] marks an earned total spoken even with Autodetail Off.
bool readHelpControls();
bool statusReviewAvailable();
void installStatusReview();
void resetStatusReview();
StatusBatch beginStatusRows(unsigned character,int page,bool grid,bool resetCursor,bool replaceContext=true);
void addStatusRow(StatusBatch&,TextPlan&,TextRef text,TextRef detail=0,bool skipEmpty=false);
bool finishStatusRows(StatusBatch&);
void closeGridStatus(int page);
void showAeonStatus(unsigned character,int page);
void closeHelpReview();
void closeBattleReview();
bool battleResultsReviewActive();
void refreshBattleResultReview();
void installGridStatus();
}
