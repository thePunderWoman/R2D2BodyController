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
  HCR.PlayWAV(CH_A, "0000");
}

void playSWTheme() {
  HCR.PlayWAV(CH_A, "0001");
}

void playSWThemeFull() {
  HCR.PlayWAV(CH_A, "0002");
}

void playCantina() {
  HCR.PlayWAV(CH_A, "0003");
}

void playCantinaFull() {
  HCR.PlayWAV(CH_A, "0004");
}

void playVader() {
  HCR.PlayWAV(CH_A, "0005");
}

void playVaderFull() {
  HCR.PlayWAV(CH_A, "0006");
}

void playDuel() {
  HCR.PlayWAV(CH_A, "0007");
}

void playThrone() {
  HCR.PlayWAV(CH_A, "0008");
}

void playClones() {
  HCR.PlayWAV(CH_A, "0009");
}

void playLukeJabba() {
  HCR.PlayWAV(CH_A, "0010");
}

void playHello() {
  HCR.PlayWAV(CH_A, "0011");
}

void playImperialAlarm() {
  HCR.PlayWAV(CH_A, "0012");
}

void playBattleAlarm() {
  HCR.PlayWAV(CH_A, "0013");
}

void playLove() {
  HCR.PlayWAV(CH_A, "0014");
}

void playRockMarch() {
  HCR.PlayWAV(CH_A, "0015");
}

void playDisco() {
  HCR.PlayWAV(CH_A, "0016");
}

void playStepBack() {
  HCR.PlayWAV(CH_A, "0017");
}

void playLeiaFull() {
  HCR.PlayWAV(CH_A, "0018");
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
