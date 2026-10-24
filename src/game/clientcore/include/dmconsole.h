#ifndef DMCONSOLE_H
#define DMCONSOLE_H

// The DM and client-debug console command set.

// Every command is a free function taking the raw console line and returning the string to print.
// The shared result buffer is filled by this translation unit's own variadic sprintf_s shim.
// The commands carry no ConF<name> registration object, so the registration block is compiled out
// of the release build and these bodies are dead code reachable only through the absent table.

char* spawnitem(char* sArgs);
char* spawnweapon(char* sArgs);
char* visualeffect(char* sArgs);
char* playerpathfindrule(char* sArgs);
char* dm_heal(char* sArgs);
char* makesafe(char* sArgs);
char* NetProfile(char* sArgs);
char* showeffects(char* sArgs);
char* spawnnasty(char* sArgs);
char* dm_setrace(char* sArgs);
char* verifyallcreatures(char* sArgs);
char* setphenotype(char* sArgs);
char* setgender(char* sArgs);
char* setpltcolor(char* sArgs);
char* setheadvariation(char* sArgs);
char* setalwaysrun(char* sArgs);
char* playanim(char* sArgs);
char* dm_gettime(char* sArgs);
char* DebugMode(char* sArgs);
char* setvideomode(char* sArgs);
char* displaystrref(char* sArgs);
char* RollDice(char* sArgs, int nSides);
char* d2(char* sArgs);
char* d3(char* sArgs);
char* d4(char* sArgs);
char* d6(char* sArgs);
char* d8(char* sArgs);
char* d10(char* sArgs);
char* d12(char* sArgs);
char* d20(char* sArgs);
char* d100(char* sArgs);
char* addpartymember(char* sArgs);
char* giveforcepowers(char* sArgs);

#endif // DMCONSOLE_H
