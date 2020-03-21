#include <nds.h>
#include <iostream>
#include "parse.h"
#include "main.h"
#include "sound.h"
#include <hold.h>
#include <bitset>
#include <cmath>
#include <maxmod9.h>
#include <vector>
#include "play.h"
#include "play_render.h"
#include "play_input.h"
#include "play_score.h"
#include "render.h"

using namespace std;

u32 beatfperiod = (1 << (BPMFRAC + MINUTEFRAC));

Play::Play(songdata* song){
	cout << "\n this song";
	this->song = song;
	render = new PlayRender(this);
	score = new PlayScore(this);
	input = new PlayInput(this);
	vramSetBankF(VRAM_F_BG_EXT_PALETTE_SLOT01);
	vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
}

Play::~Play() {
	delete render;
	delete score;
	delete input;
}

void Play::loop(){
	//s_play();
	TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1024;
	TIMER1_CR = TIMER_ENABLE | TIMER_CASCADE;
	while (1) {
		updateBeat();
		scanKeys();
		input->update();
		mmStreamUpdate();
		swiWaitForVBlank();
		updateSteps();
		render->update();
		oamUpdate(&oamMain);
		oamUpdate(&oamSub);
		if (state != 1) {
			return;
		}
	}
}

int Play::getNoteType(u32 row) {
	u8 notetype = 9; //192
	u32 rowf = row << 8;
	u32 rpmf = rowspermeasure << 8;
	if (rowf % (rpmf / 4) == 0) {notetype = 1;}
	else if (rowf % (rpmf / 8) == 0) {notetype = 2;}
	else if (rowf % (rpmf / 12) == 0) {notetype = 3;}
	else if (rowf % (rpmf / 16) == 0) {notetype = 4;}
	else if (rowf % (rpmf / 24) == 0) {notetype = 5;}
	else if (rowf % (rpmf / 32) == 0) {notetype = 6;}
	else if (rowf % (rpmf / 48) == 0) {notetype = 7;}
	else if (rowf % (rpmf / 64) == 0) {notetype = 8;}
	return notetype;
}

void Play::updateBeat() {
	time = millis();
	minutef = (time * (1 << MINUTEFRAC)) / 60000;
	beatf = 0;
	minutefsum = 0;
	for (uint i = 0; i < song->bpms.size(); i++) {
		lostbeatsbpm = FALSE;
		if (i < song->bpms.size() - 1) {
			minutefbpm = (song->bpms[i + 1].beatf - song->bpms[i].beatf) / song->bpms[i].bpmf;
			for (uint s = 0; s < song->stops.size(); s++) {
				if ((song->stops[s].beatf > song->bpms[i].beatf) && (song->stops[s].beatf < song->bpms[i + 1].beatf)) {
					minutefbpm = minutefbpm + song->stops[s].bpmf;
					lostbeatsbpm = TRUE;
				}
			}
		} else {
			minutefbpm = 0;
		}
		if ((minutefbpm > 0) && ((minutefsum + minutefbpm) < minutef)) {
			beatf = song->bpms[i + 1].beatf - song->bpms[i].beatf + beatf;
			minutefsum = minutefsum + minutefbpm;
		} else {
			beatf = beatf + ((minutef - minutefsum) * song->bpms[i].bpmf);
			if (bpmf != song->bpms[i].bpmf) {
				bpmf = song->bpms[i].bpmf;
				score->updateJudgesWindow();
			}
			if (lostbeatsbpm) {
				for (uint s = 0; s < song->stops.size(); s++) {
					if ((song->stops[s].beatf > song->bpms[i].beatf) && (song->stops[s].beatf < song->bpms[i + 1].beatf)) {
						if (beatf > song->stops[s].beatf) {
							u32 offset = song->stops[s].bpmf * song->bpms[i].bpmf;
							if (beatf - song->stops[s].beatf < offset){
								beatf = song->stops[s].beatf;
							} else {
								beatf = beatf - offset;
							}
						}
					}
				}
			}
			break;
		}
	}
	beat = beatf >> (MINUTEFRAC + BPMFRAC);
}

void Play::updateSteps() {
	//crear nuevos steps
	for (int i = cursor; i < beat + parseaheadbeats; i++) {
		if ((i / 4) > measurecursor) {
			firstbeat = i;
			m = getMeasureAtBeat(i);
			cout << "\nmeasure " << (i / 4);
			sets = m.size();
			measurecursor = i / 4;
			rowspermeasure = sets * 4;
		}
		count = i - firstbeat;
		stepbeatf = i * beatfperiod;
		switch (sets) {
			case 1: //1 set, 1 linea por beat
				set = m.at(0);
				newSteps(set[count], stepbeatf, 1);
				break;
			case 2: //2 sets, 2 lineas por beat
				if ((count == 0) || (count == 1)) {
					set = m.at(0);
					newSteps(set[count * 2], stepbeatf, 1);
					newSteps(set[count * 2 + 1], stepbeatf + (beatfperiod / 2), 2);
				} else {
					set = m.at(1);
					newSteps(set[(count - 2) * 2], stepbeatf, 1);
					newSteps(set[(count - 2) * 2 + 1], stepbeatf + ((beatfperiod) / 2), 2);
				}
				break;
			case 3: //3 sets, 3 lineas por beat
				switch (count) {
					case 0:
						set = m.at(0);
						newSteps(set[0], stepbeatf, 1);
						newSteps(set[1], stepbeatf + (beatfperiod / 3), 3);
						newSteps(set[2], stepbeatf + (beatfperiod / 3) * 2, 3);
						break;
					case 1:
						set = m.at(0);
						newSteps(set[3], stepbeatf, 1);
						set = m.at(1);
						newSteps(set[0], stepbeatf + (beatfperiod / 3), 3);
						newSteps(set[1], stepbeatf + (beatfperiod / 3) * 2, 3);
						break;
					case 2:
						set = m.at(1);
						newSteps(set[2], stepbeatf, 1);
						newSteps(set[3], stepbeatf + (beatfperiod / 3), 3);
						set = m.at(2);
						newSteps(set[0], stepbeatf + (beatfperiod / 3) * 2, 3);
						break;
					case 3:
						set = m.at(2);
						newSteps(set[1], stepbeatf, 1);
						newSteps(set[2], stepbeatf + (beatfperiod / 3), 3);
						newSteps(set[3], stepbeatf + (beatfperiod / 3) * 2, 3);
						break;

				}
				break;
			default: //sets sets, sets lineas por beat
				for (int k = 0; k < sets / 4; k++) {
					int seti = count * (sets / 4) + k;
					set = m.at(seti);
					relbeatf = ((k * beatfperiod) / (sets / 4));
					for (int ii = 0; ii < 4; ii++) {
						newSteps(set[ii], stepbeatf + relbeatf + (ii * (beatfperiod / sets)), getNoteType(seti * 4 + ii));
					}
				}
				break;
		}
		cursor = i + 1;
	}
	//crear steps por holds
	int ystart;
	int yend;
	int pos = 0;
	u8 push[4];
	vector<step>::iterator it;
	u32 height;
	step s;
	for (auto h = holds.begin(); h != holds.end(); h++) {
		ystart = ((h->startbeatf >> BEATFSCREENYFRAC) - (beatf >> BEATFSCREENYFRAC)) + 16;
		if (ystart > NDSHEIGHT) {
			continue;
		}
		for (auto i = steps.begin(); i != steps.end(); i++) {
			if (((i->col == h->col) && (i->beatf == h->startbeatf)) && ((i->type == 2) || (i->type == 5))) {
				pos = distance(steps.begin(), i) + 1;
			} 
		}
		height = NDSHEIGHT - ystart;
		if (h->endbeatf > 0) {
			yend = ((h->endbeatf >> BEATFSCREENYFRAC) - (beatf >> BEATFSCREENYFRAC)) + 16;
			height = yend - ystart;
		}
		while (((height + 32 - 1) / 32) > h->stepcount) {
			it = steps.begin();
			advance(it, pos);
			h->stepcount = h->stepcount + 1;
			s.type = 5;
			s.x = (HITXOFFSET + 32 * h->col);
			s.y = 0;
			s.col = h->col;
			s.sprite = popSprite();
			s.beatf = h->startbeatf;
			s.stepcount = h->stepcount - 1;
			for (int c = 0; c < 4; c++) {
				push[c] = 0;
				if (input->holdCol[c] >= it) {
					push[c] = distance(steps.begin(), input->holdCol[c]) + 1;
				}
			}
			steps.insert(it, s);
			for (int c = 0; c < 4; c++) {
				if (push[c] > 0) {
					input->holdCol[c] = steps.begin();
					advance(input->holdCol[c], push[c]);
				}
			}
			pos++;
		}
		//cortar sprite de ultimo hold
		if (h->endbeatf > 0) {
			u8 mod = height % 32;
			if (mod > 0) {
				for (auto n = steps.end() - 1; n != steps.begin() - 1; n--) {
					if ((n->col == h->col) && (n->type == 5) && (n->beatf == h->startbeatf)) {
						n->gfx = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_Bmp);
						dmaCopyHalfWords(3, holdBitmap, n->gfx, holdBitmapLen / 32 * mod);
						break;
					}
				}
			}
			holds.erase(h--);
		}
	}
	//actualizar posicion
	for (auto i = steps.begin(); i != steps.end(); i++) {
		if (((i->type == 1) || (i->type == 2)) && !i->disabled && (i->beatf < beatf)) {
			u32 beatfdiff = beatf - i->beatf;
			if (beatfdiff > score->judgesWindow[4]) {
				//se paso una nota
				render->playJudgmentAnim(11);
				score->addDPTotal();
				if (i->type == 2) {
					score->addDPTotal(); //la cola del hold igual vale 2dp
				}
				score->dropCombo();
				i->disabled = true;
			}
		}
		i->y = ((i->beatf >> BEATFSCREENYFRAC) - (beatf >> BEATFSCREENYFRAC)) + HITYOFFSET;
		if (i->type == 5) { //holds
			i->y += 16 + 32 * i->stepcount;
		}
		if (i->y < -32) {
			removeStep(&i);
		}
	}
}

void Play::newSteps(u16 data, u32 beatf, u8 notetype) {
	if (data == 0)
		return;
	bool newstep = false;
	bool push[4] = {false};
	//normal steps
	step s;
	for (int i = 0; i < 4; i++) {
		newstep = false;
		if (data & normal[i]) {
			s.type = 1;
			newstep = true;
		}
		if (data & holdhead[i]) {
			s.type = 2;
			newstep = true;
			hold h;
			h.col = i;
			h.startbeatf = beatf;
			h.endbeatf = 0;
			h.stepcount = 0;
			holds.push_back(h);
		}
		if (data & holdtail[i]) {
			s.type = 3;
			newstep = true;
			for (auto k = holds.begin(); k != holds.end(); k++) {
				if ((k->col == i) && (k->endbeatf == 0)) {
					k->endbeatf = beatf;
					break;
				}
			}
		}
		if (newstep) {
			for (int c = 0; c < 4; c++) {
				if (input->holdCol[c] == steps.end()) {
					push[c] = true;
				}
			}
			s.x = (HITXOFFSET + 32 * i);
			s.y = 100;
			s.col = i;
			s.sprite = popSprite();
			s.beatf = beatf;
			s.notetype = notetype;
			steps.push_back(s);
			for (int c = 0; c < 4; c++) {
				if (push[c]) {
					input->holdCol[c] = steps.end();
				}
			}
		}
	}
}

void Play::removeStep(vector<step>::iterator* s) {
	pushSprite((*s)->sprite);
	if ((*s)->gfx != NULL) {
		oamFreeGfx(&oamMain, (*s)->gfx);
	}
	for (int i = 0; i < 4; i++) {
		if (input->holdCol[i] > (*s)) {
			input->holdCol[i]--;
		}
	}
	steps.erase((*s));
	(*s)--;
}

measure Play::getMeasureAtBeat(u32 beat) {
	if (beat / 4 > song->notes.size() - 1) {
		while (1) {
			swiWaitForVBlank();
		}
		sassert(0, "attempted to get nonexistant measure");
	}
	return song->notes.at(beat / 4);
}

u32 Play::millis() {
	return (timerTick(0) + (timerTick(1) << 16)) / NDSFREQ;
}