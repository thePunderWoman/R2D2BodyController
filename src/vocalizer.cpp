#include "vocalizer.h"
#include "config.h"

HCRVocalizer HCR(&WCBSerial, WCB_BAUD);

void playScream() {
  HCR.Stimulate(SCARED, EMOTE_STRONG);
}

/**
 * @brief Plays the Leia audio
 *
 */
void playLeia() {
  HCR.PlayWAV(CH_A, 0);
}

void playSWTheme() {
  HCR.PlayWAV(CH_A, 1);
}

void playSWThemeFull() {
  HCR.PlayWAV(CH_A, 2);
}

void playCantina() {
  HCR.PlayWAV(CH_A, 3);
}

void playCantinaFull() {
  HCR.PlayWAV(CH_A, 4);
}

void playVader() {
  HCR.PlayWAV(CH_A, 5);
}

void playVaderFull() {
  HCR.PlayWAV(CH_A, 6);
}

void playDuel() {
  HCR.PlayWAV(CH_A, 7);
}

void playThrone() {
  HCR.PlayWAV(CH_A, 8);
}

void playClones() {
  HCR.PlayWAV(CH_A, 9);
}

void playLukeJabba() {
  HCR.PlayWAV(CH_A, 10);
}

void playHello() {
  HCR.PlayWAV(CH_A, 11);
}

void playImperialAlarm() {
  HCR.PlayWAV(CH_A, 12);
}

void playBattleAlarm() {
  HCR.PlayWAV(CH_A, 13);
}

void playLove() {
  HCR.PlayWAV(CH_A, 14);
}

void playRockMarch() {
  HCR.PlayWAV(CH_A, 15);
}

void playDisco() {
  HCR.PlayWAV(CH_A, 16);
}

void playStepBack() {
  HCR.PlayWAV(CH_A, 17);
}

void playLeiaFull() {
  HCR.PlayWAV(CH_A, 18);
}

// Emote Events

void enableMuse() {
  HCR.Muse(20,45);
  HCR.SetMuse(1);
}

void disableMuse() {
  HCR.SetMuse(0);
}

void resetVocalizer() {
  DEBUG_PRINT_LN(F("RESETTING VOCALIZER"));
  HCR.ResetEmotions();
  HCR.StopWAV(CH_A);
  enableMuse();
}

void overloadEmote() {
  HCR.Overload();
}
