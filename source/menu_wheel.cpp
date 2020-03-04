#include <iostream>
#include <string>
#include <functional>
#include <nds.h>
#include <fat.h>
#include <sys/dir.h>
#include <font.h>
#include <song_frame.h>
#include <group_frame.h>
#include <song_font.h>
#include "render.h"
#include "menu_wheel.h"

using namespace std;

string fileext;

const int wheelview = 9; //amount of files visible at a single time on wheel
const int wheelviewchar = 7; //texto visible
int wheelsize = -1; //total amount of files

const int* wheelTiles[] {
	(const int[]){
		16, 17, 18, 19, 20, 21, 22,
		41, 42, 43, 44, 45,
		67, 68, -1
	},
	(const int[]){
		3, 4, 5, 6, 7, 8,
		25, 26, 27, 28, 29, 30, 31, 32, 33,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
		71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
		93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
		116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
		144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
		171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183,
		197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
		225, 225, 226, 227, 228, 229,
		251, 252, -1,
	},
	(const int[]){
		254, 255, 256, 257, 258, 259,
		277, 278, 279, 280, 281, 282, 283, 284, 285, 286,
		300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312,
		323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339,
		345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366,
		370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390,
		395, 396, 397, 398, 399, 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413,
		424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436,
		454, 455, 456, 457, 458, 459, -1
	},
	(const int[]){
		460, 461, 462, 463, 464,
		483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493,
		506, 507, 508, 509, 510, 511, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523,
		529, 530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551,
		552, 553, 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574,
		575, 576, 577, 578, 579, 580, 581, 582, 583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 594, 595, 596, 597,
		598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620,
		637, 638, 639, 640, 641, 642, 643, -1
	},
	(const int[]){
		644, 645, 646, 647, 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660,
		667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689,
		690, 691, 692, 693, 694, 695, 696, 697, 698, 699, 700, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 712, -1
	}
};

int wheelTilesLen[5];
int wheelTilesYOffset[5] = {-1, 3, 9, 13, 15};
int wheelTilesTotalLen = 0;

u8 songFontSprite[CHARSPRITES * 7];
u16* songFontGfx[CHARSPRITES * 7];
u8 songFrameColor[wheelview];
int wheelBg1;
int wheelBg2;

int wheelAnim = 0;
int wheelFrame = 0;

const int buffersize = 17;
int wheelcursor = 0;
int buffercursor = buffersize / 2;

wheelitem bufferitems[buffersize];
wheelitem wheelitems[wheelview];

void mw_setup() {
	cout << "aver";
	loadSongFontGfx();
	fillBuffer();
	loadFrameBg();
	updateFrameBg();
	playWheelAnim(-1);
}

void loadSongFontGfx() {
	for (int i = 0; i < CHARSPRITES * wheelviewchar; i++) {
		songFontSprite[i] = popSpriteSub();
	}
	for (int i = 0; i < CHARSPRITES * wheelviewchar; i++) {
		songFontGfx[i] = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_Bmp);
	}
}

void loadFrameBg() {
	wheelBg1 = bgInitSub(2, BgType_ExRotation, BgSize_ER_256x256, 0, 1);
	wheelBg2 = bgInitSub(3, BgType_ExRotation, BgSize_ER_256x256, 1, 1);
	int g = 1;
	//rojo negro
	for (int i = 0; i < 5; i++) {
		int t = 0;
		while (wheelTiles[i][t] != -1) {
			dmaCopy(song_frameTiles + 16 * wheelTiles[i][t], bgGetGfxPtr(wheelBg1) + 32 * g, 64);
			t++;
			g++;
		}
		wheelTilesLen[i] = t;
		wheelTilesTotalLen += t;
	}
	//verde
	for (int i = 0; i < 5; i++) {
		int t = 0;
		while (wheelTiles[i][t] != -1) {
			dmaCopy(group_frameTiles + 16 * wheelTiles[i][t], bgGetGfxPtr(wheelBg1) + 32 * g, 64);
			t++;
			g++;
		}
	}
	dmaCopy(song_framePal, &VRAM_H_EXT_PALETTE[2][0], song_framePalLen);
	dmaCopy(song_framePal, &VRAM_H_EXT_PALETTE[2][1], song_framePalLen);
	dmaCopy(group_framePal, &VRAM_H_EXT_PALETTE[2][2], group_framePalLen);
	dmaCopy(song_framePal, &VRAM_H_EXT_PALETTE[3][0], song_framePalLen);
	dmaCopy(song_framePal, &VRAM_H_EXT_PALETTE[3][1], song_framePalLen);
	dmaCopy(group_framePal, &VRAM_H_EXT_PALETTE[3][2], group_framePalLen);
}

void wheelNext() {
	buffercursor++;
	if ((buffercursor - buffersize / 2) >= (wheelsize - wheelview / 2)) {
		buffercursor -= wheelsize;
	}
	else if (buffercursor > (buffersize - wheelview / 2)) {
		wheelcursor = wheelcursor + buffercursor - buffersize / 2;
		if (wheelcursor >= wheelsize) {
			wheelcursor -= wheelsize;
		}
		buffercursor = buffersize / 2;
		//mover bufferitems
		for (int y = 0; y < buffersize / 2 + wheelview / 2 - 1; y++) {
			bufferitems[y] = bufferitems[y + buffersize / 2 - wheelview / 2 + 2];
		}
		for (int i = buffercursor + wheelview / 2 - 1; i < buffersize; i++) {
			bufferitems[i].type = -1;
		}
		fillBuffer();
	}
	/*
	//mover texto
	u16* tempFontGfx[CHARSPRITES];
	for (int i = 0; i < CHARSPRITES; i++) {
		tempFontGfx[i] = songFontGfx[i];
	}
	for (int y = 0; y < wheelviewchar - 1; y++) {
		for (int i = 0; i < CHARSPRITES; i++) {
			songFontGfx[y * CHARSPRITES + i] = songFontGfx[(y + 1) * CHARSPRITES + i];
		}
	}
	for (int i = 0; i < CHARSPRITES; i++) {
		songFontGfx[6 * CHARSPRITES + i] = tempFontGfx[i];
	}
	*/
	//rellenar espacio nuevo
	//printToBitmap(6 * CHARSPRITES, wheelitems[wheelview - 2].name);
	//fillWheelEmpty();
	//updateFrameBg();
}

void wheelPrev() {
	buffercursor--;
	if ((buffercursor - buffersize / 2) <= (wheelview / 2 - wheelsize)) {
		buffercursor += wheelsize;
	}
	else if (buffercursor < (wheelview / 2 - 1)) {
		wheelcursor = wheelcursor - (buffersize / 2 - wheelview / 2 + 2);
		if (wheelcursor < 0) {
			wheelcursor += wheelsize;
		}
		buffercursor = buffersize / 2;
		//mover bufferitems
		for (int i = buffersize - 1; i > buffersize / 2 - wheelview / 2 + 1; i--) {
			bufferitems[i] = bufferitems[i - buffersize / 2 + wheelview / 2 - 2];
			bufferitems[i].type = 3;
		}
		for (int i = 0; i <= (buffercursor - wheelview / 2 + 1); i++) {
			bufferitems[i].type = -1;
		}
		fillBuffer();
	}
	/*
	//mover texto
	u16* tempFontGfx[CHARSPRITES];
	for (int i = 0; i < CHARSPRITES; i++) {
		tempFontGfx[i] = songFontGfx[6 * CHARSPRITES + i];
	}
	for (int y = wheelviewchar - 1; y > 0; y--) {
		for (int i = 0; i < CHARSPRITES; i++) {
			songFontGfx[y * CHARSPRITES + i] = songFontGfx[(y - 1) * CHARSPRITES + i];
		}
	}
	for (int i = 0; i < CHARSPRITES; i++) {
		songFontGfx[i] = tempFontGfx[i];
	}
	printToBitmap(0, wheelitems[1].name);
	fillWheelEmpty();
	updateFrameBg();*/
}

void printToBitmap(u8 gfx, string str) {
	int c;
	int x;
	int s;
	for (uint i = 0; i < str.length(); i++) {
		c = int(str[i]) - ASCIIOFFSET;
		x = i * CHARWIDTH;
		s = x / 64;
		if ((x + CHARWIDTH) > (CHARSPRITES * 64)) {
			break;
		}
		for (int y = 0; y < 16; y++) {
			if ((x % 64) > (64 - CHARWIDTH)) {
				dmaCopy(song_fontBitmap + 16 * y + 256 * c + CHAROFFSET, songFontGfx[gfx + s] + (y + 8) * 64 + i * CHARWIDTH - (s * 64), (((s + 1) * 64) - x) * 2);
				dmaCopy(song_fontBitmap + 16 * y + 256 * c + CHAROFFSET + (((s + 1) * 64) - x), songFontGfx[gfx + s + 1] + (y + 8) * 64 + i * CHARWIDTH + (((s + 1) * 64) - x) - ((s + 1) * 64), (CHARWIDTH * 2) - ((((s + 1) * 64) - x) * 2));
			}
			else {
				dmaCopy(song_fontBitmap + 16 * y + 256 * c + CHAROFFSET, songFontGfx[gfx + s] + (y + 8) * 64 + i * CHARWIDTH - (s * 64), CHARWIDTH * 2);
			}
		}
	}
}

void fillBuffer() {
	int dircount = -1;
	int buffercount = 0;
	//lectura recursiva de directorios
	function<bool(string)> parse = [&](string dir) -> bool {
		int pos;
		DIR *pdir;
		struct dirent *pent;
		bool isgroup = false;
		pdir = opendir(dir.c_str());
		if (pdir){
			while ((pent = readdir(pdir)) != NULL) {
				fileext = "";
	    		if ((strcmp(".", pent->d_name) == 0) || (strcmp("..", pent->d_name) == 0)) {
	        		continue;
	    		}
	    		if (pent->d_type == DT_DIR) {
	    			dircount++;
	    			isgroup = true;
	    			if (wheelsize != -1) {
	    				pos = dircountToBuffer(dircount);
	        			if ((pos != -1) && (bufferitems[pos].type == -1)) {
	        				wheelitem group;
	        				group.type = 0;
	        				group.name = pent->d_name;
	        				group.path = dir + '/' + pent->d_name;
	        				bufferitems[pos] = group;
	        				buffercount++;
	        			}
	        		}
	        		if (parse(dir + '/' + pent->d_name)) {
	        			return true;
	        		}
	    			if (buffercount > buffersize) {
	    				return true;
	    			}
	    		}
	    		else if (!isgroup) {
	        		for (int i = 0; pent->d_name[i] != '\0'; i++) {
	        			fileext += pent->d_name[i];
	        			if (pent->d_name[i] == '.') {
	        				fileext = "";
	        			}
	        		}
	        		if (fileext == "sm") {
	        			if (wheelsize != -1) {
	        				pos = dircountToBuffer(dircount);
		        			if ((pos != -1) && (bufferitems[pos].type == 0)) {
		        				wheelitem* song = &bufferitems[pos];
		        				song->type = 1;
		        				song->smpath = dir + '/' + pent->d_name;
		        				return false;
		        			}
	        			}
	        		}
	    		}
			}
			closedir(pdir);
		}
		return false;
	};
	//encontrar total de elementos
	if (wheelsize == -1) {
		parse("/ddr");
		wheelsize = dircount + 1;
		cout << "\nwheelsize " << wheelsize;
		dircount = -1;
	}
	//popular rueda
	parse("/ddr");
	//llenar espacios que faltan
	if (wheelsize < buffersize) {
		cout << "\nfill missing";
		for (int i = 0; i < buffersize; i++) {
			if (bufferitems[i].type == -1) {
				int pos = bufferToFile(i);
				bufferitems[i] = bufferitems[dircountToBuffer(pos)];
			}
		}
	}
	//for (int i = 0; i < wheelviewchar; i++) {
	//	printToBitmap(i * 3, wheelitems[i + 1].name);
	//}
}

void updateWheelColor() {
	for (int i = 0; i < wheelview; i++) {
		switch (wheelitems[i].type) {
			case 0:
				songFrameColor[i] = 2;
				break;
			case 1:
				songFrameColor[i] = 1;
				break;
		}
	}
}

int bufferToFile(int i) {
	int pos = wheelcursor - (buffersize / 2) + i;
	while (pos >= wheelsize) {
		pos = pos - wheelsize;
	}
	while (pos < 0) {
		pos = pos + wheelsize;
	}
	return pos;
}

int dircountToBuffer(int i) {
	if (abs(wheelcursor - i) <= (buffersize / 2)) {
		return (i - wheelcursor + (buffersize / 2));
	}
	else if (abs(wheelcursor + wheelsize - i) <= (buffersize / 2)) {
		return (i - wheelsize - wheelcursor + (buffersize / 2));
	}
	else if (abs(wheelcursor - wheelsize - i) <= (buffersize / 2)) {
		return (i + wheelsize - wheelcursor + (buffersize / 2));
	}
	return -1;
}

void renderWheel() {
	int angle = 0;
	if (wheelFrame > 22) {
		angle = 12 * (45 - wheelFrame) * wheelAnim;
	}
	else if (wheelFrame > 0) {
		angle = 12 * (wheelFrame) * -wheelAnim;
	}
	int rx = 520 << 8;
	int ry = 96 << 8;
	int sx = 440 << 8;
	int sy = 128 << 8;
	bgSet(wheelBg1, angle, 1 << 8, 1 << 8, sx, sy, rx, ry);
	bgSet(wheelBg2, angle, 1 << 8, 1 << 8, sx, sy, rx, ry);
	if (wheelFrame == 22) {
		wheelNext();
		cout << "\n-- " << wheelcursor;
		for (int i = 0; i < buffersize; i++) {
			cout << "\n" << i << " " << bufferitems[i].type << " " << bufferitems[i].name;
			if (buffercursor == i) {
				cout << " ---";
			}
		}
	}
	bgUpdate();
	renderWheelChar(angle);
	if (wheelFrame > 0) {
		wheelFrame--;
	}
	else {
		playWheelAnim(-1);
	}
}

void playWheelAnim(int anim) {
	wheelAnim = anim;
	wheelFrame = 44;
}

void renderWheelChar(int angle) {
	int scale = 256;
	int o = ((scale - 256) * 64) / 256;
	for (int i = -3; i <= 3; i++) {
		for (int c = 0; c < CHARSPRITES; c++) {
			int x = (((370 - (63 * c) - (o * (c * 2 + 1) / 2)) * cosLerp((180 + i * -WHEELANGLE) * 32768 / 360 - angle)) >> 12) + 60;
			int y = (((370 - (63 * c) - (o * (c * 2 + 1) / 2)) * sinLerp((180 + i * -WHEELANGLE) * 32768 / 360 - angle)) >> 12) + 32;
			oamSet(&oamSub, songFontSprite[CHARSPRITES * (-i + 3) + c], x - 85, y + 48, 0, 15, SpriteSize_64x32, SpriteColorFormat_Bmp, songFontGfx[CHARSPRITES * (i + 3) + c], i + 3 + 7, false, false, false, false, false);
		}
		oamRotateScale(&oamSub, i + 3 + 7, (i * WHEELANGLE) * 32768 / 360 + angle, (1 << 16) / scale, 256);
	}
}

void updateFrameBg() {
	updateWheelColor();
	int tileOffset = 1;
	int t = 0;
	u16* bgMap1 = bgGetMapPtr(wheelBg1);
	u16* bgMap2 = bgGetMapPtr(wheelBg2);
	u16* bgMap = bgMap1;
	for (int i = 0; i < 5; i++) {
		while (wheelTiles[i][t] != -1) {
			int pos = wheelTiles[i][t] + (wheelTiles[i][t] / 23 * 9);
			int x = pos % 32;
			int y = pos / 32;
			if (y >= wheelTilesYOffset[i]) {
				y -= wheelTilesYOffset[i];
				int tile1 = (t + tileOffset) | TILE_PALETTE(songFrameColor[i]);
				int tile2 = (t + tileOffset) | TILE_PALETTE(songFrameColor[8 - i]) | TILE_FLIP_V;
				if (songFrameColor[i] == 2) {tile1 += wheelTilesTotalLen;}
				if (songFrameColor[8 - i] == 2) {tile2 += wheelTilesTotalLen;}
				bgMap[y * 32 + x] = tile1;
				bgMap[(31 - y) * 32 + x] = tile2;
			}
			t++;
		}
		if (bgMap == bgMap1) {bgMap = bgMap2;}
		else {bgMap = bgMap1;}
		tileOffset += wheelTilesLen[i];
		t = 0;
	}
}