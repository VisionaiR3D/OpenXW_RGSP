#ifndef XW_RGSP_COMBOS_H
#define XW_RGSP_COMBOS_H
#include "aeron/input.h"
#include "xw_runtime/input/actions.h"
#include "chord_engine.h"
typedef struct { int button; ChordBinding binding; } XwComboGroup;
enum { XW_COMBO_BUTTONS=8 };
const AeronInputSnapshot* XwCombos_Filter(const AeronInputSnapshot* input);
void XwCombos_Suspend(bool suspended);
const XwComboGroup* XwCombos_Groups(void);
bool XwCombos_SetGroup(int index,const XwComboGroup* group);
bool XwCombos_RestoreDefaults(void);
const char* XwCombos_Error(void);
const char* XwCombos_GroupName(int index);
const char* XwCombos_ButtonName(int index);
#endif

void XwControllerMapping_InjectAction(XwInputAction action,bool pressed);
bool XwCombos_Queue(int action);
int XwCombos_QueueCount(void);
int XwCombos_QueueLast(void);
void XwCombos_Undo(void);
void XwCombos_Clear(void);
void XwCombos_ResumeQueued(void);

#include "binding_engine.h"
int XwCombos_Count(void);
const FlightBinding* XwCombos_Bindings(void);
bool XwCombos_SetBinding(int i,unsigned mask,int action);
bool XwCombos_SetActionBinding(int action,unsigned mask);
bool XwCombos_AddBinding(void);
bool XwCombos_RemoveBinding(int i);
bool XwCombos_MoveBinding(int i,int delta);
const char* XwCombos_KeyName(int i);
void XwCombos_FormatMask(unsigned mask,char* out,int size);

bool XwCombos_AssistEnabled(void);
bool XwCombos_SetAssist(bool enabled);
bool XwCombos_AssistActive(void);

bool XwCombos_LeadEnabled(void);
bool XwCombos_SetLead(bool enabled);
bool XwCombos_TakeTargetListRequest(void);
