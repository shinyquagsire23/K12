// The client-side typed-console command table. Every command is a free function whose address the
// console's registration table takes.

#ifndef CONSOLE_H
#define CONSOLE_H

int  GetCheatsTurnedOn();
void heal();
void CheatSetSkill(int nSkill, char* sArgs);
void CheatSetAbility(int nAbility, char* sArgs);
void addexp(char* sArgs);

void setcomputeruse(char* sArgs);
void setdemolitions(char* sArgs);
void setstealth(char* sArgs);
void setawareness(char* sArgs);
void setpersuade(char* sArgs);
void setrepair(char* sArgs);
void setsecurity(char* sArgs);
void settreatinjury(char* sArgs);

void setstrength(char* sArgs);
void setdexterity(char* sArgs);
void setconstitution(char* sArgs);
void setintelligence(char* sArgs);
void setwisdom(char* sArgs);
void setcharisma(char* sArgs);

void turbo();
void invulnerability();
void restartminigame();
int  GetMiniGamePlayingType();
void bright();
void addlightside(int nAmount);
void adddarkside(int nAmount);
void revealmap();

void givecredits(char* sArgs);
void giveitem(char* sArgs);
void givemed(char* sArgs);
void giverepair(char* sArgs);
void givecomspikes(char* sArgs);
void givesecspikes(char* sArgs);
void givesitharmour();
void warp(char* sArgs);
void infiniteuses();
void whereami();
void addlevel();
void dancedancemalak();
char* fps(char* sArgs);

char* MacroRecord(char* sArgs);
char* MacroStop(char* sArgs);
char* MacroPlay(char* sArgs);
int   lockcamera(char* sArgs);
int   keyboardmode(char* sArgs);
int   memtotalallocated(char* sArgs);
char* PlayMovie(char* sArgs);
char* Binarize2DA(char* sArgs);
void  ToggleMovieNotification();
void  ToggleLiveContentIgnore();

#endif
