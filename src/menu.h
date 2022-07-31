#include <LCDMenuLib.h>

extern const char version[];

void setupMenu();
void loopMenu();

void LCDML_BACK_setup(LCDML_BACKEND_control);
void LCDML_BACK_stable(LCDML_BACKEND_control);

void LCDML_CONTROL_setup();
void LCDML_CONTROL_loop();