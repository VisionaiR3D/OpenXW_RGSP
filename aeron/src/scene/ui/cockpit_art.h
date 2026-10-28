#ifndef COMMAND_COCKPIT_ART_H
#define COMMAND_COCKPIT_ART_H
int AeronUi_CockpitBegin(AeronUiContext*,const char*,int,int);
int AeronUi_CockpitMenuButton(AeronUiContext*,const char*,int,int,int);
int AeronUi_CockpitButton(AeronUiContext*,const char*,float,float,float,float,int,int);
void AeronUi_CockpitText(AeronUiContext*,const char*,float,float,float,float,float,int,int);
void AeronUi_CockpitMeter(AeronUiContext*,const char*,int,int,float,float,float,int);
void AeronUi_CockpitStatus(AeronUiContext*,int,int,int,int);
void AeronUi_CockpitBlueprint(AeronUiContext*,const char*,float,float,float,float);
#endif

void AeronUi_CockpitPortrait(AeronUiContext*,int,float,float,float,float);

int AeronUi_TacticalBegin(AeronUiContext*);
void AeronUi_TacticalPanel(AeronUiContext*,float,float,float,float,int);
void AeronUi_TacticalScope(AeronUiContext*,float,float,float,float);

void AeronUi_TacticalMeter(AeronUiContext*,const char*,int,float,float,float,int);
