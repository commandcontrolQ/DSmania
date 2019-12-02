#include <nds.h>
#include <tap.h>
#include <iostream>
#include "main.h"
#include "parse.h"
#include "play.h"
#include <bitset>
#include <cmath>

//indice memoria
#define sprites ((spriteEntry*) OAM)
#define tiles_tap 0
#define pal_tap 0

//macros para acceso a memoria
#define tile2objram(t) (SPRITE_GFX + (t) * 16)
#define pal2objram(p) (SPRITE_PALETTE + (p) * 16)

#define NDSFREQ 32.7284 //khz
#define BPMFRAC 8 //bits
#define MINUTEFRAC 12

using namespace std;

static u8 notetype[] = {4, 8, 12, 16, 24, 32, 48, 64, 192};
static u32 beatfperiod = (1 << (BPMFRAC + MINUTEFRAC)) - 1; 
songdata song;
vector<step> steps;
bool freesprites[128];

void setup(songdata s){
	dmaCopyHalfWords(3, tapTiles, tile2objram(tiles_tap), tapTilesLen);
	dmaCopyHalfWords(3, tapPal, pal2objram(pal_tap), tapPalLen);
	setRotData();
	for (int i = 0; i < 128; i++) {
		pushSprite(i);
	}
	TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024;
	TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;
	song = s;
}

u32 millis() {
	return (timerTick(0) + (timerTick(1) << 16)) / NDSFREQ;
}

void loop(){
	while (1) {
		updateSteps();
		swiWaitForVBlank();
		renderSteps();
	}
}

u32 time;
u32 bpmf = 200 * pow(2, BPMFRAC);
u32 minutef;
u32 beatf;
int beat;					//beat global
int firstbeat = -1;			//primer beat del measure global
int count = 0; 				//beat relativo al primer beat de measure global
int sets;					//cantidad de sets en measure
int cursor = 0;
int measurecursor = -1;
int stepbeatf = 0;			//beat preciso en el cursor
int relbeatf = 0;			//beat preciso relativo al cursor a x set
u16 *set;
measure m;
void updateSteps() {
	time = millis();
	minutef = (time * (1 << MINUTEFRAC)) / 60000;
	beatf = (bpmf * minutef);
	beat = beatf >> (MINUTEFRAC + BPMFRAC);
	//cursor puede ir delante del beat global
	for (int i = cursor; i < beat + 4; i++) {
		if ((i / 4) > measurecursor) {
			cout << "\nmeasure " << i / 4;
			firstbeat = i;
			m = getMeasureAtBeat(i);
			sets = m.size();
			measurecursor = i / 4;
		}
		count = i - firstbeat;
		stepbeatf = i * beatfperiod;
		switch (sets) {
			case 1: //1 set, 1 linea por beat
				set = m.at(0);
				newSteps(set[count], stepbeatf, 0);
				break;
			case 2: //2 sets, 2 lineas por beat
				if ((count == 0) || (count == 1)) {
					set = m.at(0);
					newSteps(set[count * 2], stepbeatf, 0);
					newSteps(set[count * 2 + 1], stepbeatf + (beatfperiod / 2), 1);
				} else {
					set = m.at(1);
					newSteps(set[(count - 2) * 2], stepbeatf, 0);
					newSteps(set[(count - 2) * 2 + 1], stepbeatf + ((beatfperiod) / 2) , 1);
				}
				break;
			default: //sets sets, sets / 4 lineas por beat
				for (int k = 0; k < sets / 4; k++) {
					set = m.at(count * (sets / 4) + k);
					relbeatf = ((k * beatfperiod) / (sets / 4));
					for (int ii = 0; ii < 4; ii++) {
						newSteps(set[ii], stepbeatf + relbeatf + (ii * (beatfperiod / sets)), 0);
					}
				}
				break;
		}
		cursor = i + 1;
	}
	for (auto i = steps.begin(); i != steps.end(); i++) {
		i->y = ((i->beatf >> 13) - (beatf >> 13));
		if (i->beatf < beatf) {
			pushSprite(i->sprite);
			steps.erase(i--);
		}
	}
}

void newSteps(u16 data, u32 beatf, u8 type) {
	if (data == 0)
		return;
	//normal steps
	for (int i = 0; i < 4; i++) {
		if (data & normal[i]) {
			step s;
			s.x = (10 + 30 * i);
			s.y = 100;
			s.type = 0;
			s.beatf = beatf;
			s.sprite = popSprite();
			s.col = i;
			steps.push_back(s);
		}
	}
}

void renderSteps() {
	for (auto i = steps.begin(); i != steps.end(); i++) {
		if (i->y < 160) {
			sprites[i->sprite].attr0 = i->y | ATTR0_ROTSCALE_DOUBLE;
			sprites[i->sprite].attr1 = i->x | ATTR1_SIZE_32 | ATTR1_ROTDATA(i->col);
			sprites[i->sprite].attr2 = tiles_tap + (pal_tap << 12);
		} else {
			sprites[i->sprite].attr0 = ATTR0_DISABLED;
		}
	}
}

void setRotData() {
	s16 s;
	s16 c;
	u16* affine;
	//left
	s = sinLerp(degreesToAngle(270)) >> 4;
	c = cosLerp(degreesToAngle(270)) >> 4;
	affine = OAM + 3;
	affine[0] = c;
	affine[4] = s;
	affine[8] = -s;
	affine[12] = c;
	//up
	s = sinLerp(degreesToAngle(0)) >> 4;
	c = cosLerp(degreesToAngle(0)) >> 4;
	affine = OAM + 16 + 3;
	affine[0] = c;
	affine[4] = s;
	affine[8] = -s;
	affine[12] = c;
	//down
	s = sinLerp(degreesToAngle(180)) >> 4;
	c = cosLerp(degreesToAngle(180)) >> 4;
	affine = OAM + 32 + 3;
	affine[0] = c;
	affine[4] = s;
	affine[8] = -s;
	affine[12] = c;
	//right
	s = sinLerp(degreesToAngle(90)) >> 4;
	c = cosLerp(degreesToAngle(90)) >> 4;
	affine = OAM + 48 + 3;
	affine[0] = c;
	affine[4] = s;
	affine[8] = -s;
	affine[12] = c;
}

u8 popSprite() {
	for (u8 i = 0; i < 128; i++) {
		if (freesprites[i]) {
			freesprites[i] = FALSE;
			return i;
		}
	}
}

void pushSprite(u8 i) {
	freesprites[i] = TRUE;
	sprites[i].attr0 = ATTR0_DISABLED;
}

measure getMeasureAtBeat(u32 beat) {
	if (beat / 4 > song.notes.size() - 1) {
		sassert(0, "attempted to get nonexistant measure");
	}
	return song.notes.at(beat / 4);
}