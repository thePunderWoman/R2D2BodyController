// vocalizer.h — HCR Vocalizer audio + emotion API wrapper.
#pragma once

#include <hcr.h>
#include "bus.h"

extern HCRVocalizer HCR;

void playScream();
void playLeia();
void playLeiaFull();
void playSWTheme();
void playSWThemeFull();
void playCantina();
void playCantinaFull();
void playVader();
void playVaderFull();
void playDuel();
void playThrone();
void playClones();
void playLukeJabba();
void playHello();
void playImperialAlarm();
void playBattleAlarm();
void playLove();
void playRockMarch();
void playDisco();
void playStepBack();

void enableMuse();
void disableMuse();
void resetVocalizer();
void overloadEmote();
