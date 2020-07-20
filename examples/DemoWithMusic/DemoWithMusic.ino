// Create by: Jan Doležal, 2020
// It is "Demo.ino" mixed with <https://github.com/dzlonline/the_synth/blob/master/examples/song/song.ino>

#include <synth.h> // use original TheSynth library from <https://github.com/dzlonline/the_synth>
synth synthesizer;

#include "LedCube.h"
#include "notes.h"
#include "song.h"

#define SIZE 4 // 4x4x4 => 4, 8x8x8 => 8
#define NUM_LAYERS 8
#define NUM_COLUMNS 8

int led_cube_map[NUM_LAYERS][NUM_COLUMNS];
int * p_led_cube_map[NUM_LAYERS];
int layer[NUM_LAYERS] = {A2,A3,A4,A5,12,13,A0,A1}; // initializing and declaring led layers
int column[NUM_COLUMNS] = {2,6,10,8,4,5,9,7}; // initializing and declaring led rows

LedCube led_cube(p_led_cube_map, layer, column, NUM_LAYERS, NUM_COLUMNS, SIZE, 60);

class LedCubeManager : public VariableTimedAction
{
private:
	LedCube * _led_cube;
	int _state;
	
	unsigned long run() {
		/*
		 * Volá funkce zobrazující zajímavé sekvence.
		 * - vrátí-li funkce číslo > 0, pak se jedná o pauzu mezi dvěma obrázky sekvence
		 * - jinak sekvence již skončila, vyčká se nějaký čas a pak začne další funkce zobrazující nějakou jinou sekvenci
		 * - metody kostky obsahující sekvence začínají slovem "yield"
		 */
		unsigned long wait = 0;
		
		if (_led_cube->isSequenceRunning()) {
			wait = _led_cube->nextFrameOfSequence();
		} else {
			do {
				_state += 1;
				switch(_state) {
					case 1:
 						_led_cube->setSequence(new sequences::Demo(_led_cube));
						wait = 500;
						break;
					default:
						_state = 0;
				}
			} while (_state == 0);
		}
		
		return wait;
	}

public:
	LedCubeManager(LedCube * led_cube)
		: _led_cube(led_cube), _state(0)
	{
		start(150);
	}
} led_cube_manager(&led_cube);


class MusicManager : public VariableTimedAction
{
private:
	int thisNote = 0;
	unsigned long run() {
		int bpm = 120; // beats per minute
		int noteDuration = 60000 / bpm * 4 / noteDurations[thisNote];
		
		if(melody[thisNote]<=NOTE_E4) {
			synthesizer.mTrigger(1, melody[thisNote]+32);
		} else {
			synthesizer.mTrigger(0, melody[thisNote]+32);
		}
		
		thisNote += 1;
		if (thisNote >= melody_length) {
			thisNote = 0;
		}
		
		return noteDuration;
	}

public:
	MusicManager()
	{
		start(100);
	}
} music_manager;



void setup()
{
	synthesizer.begin(DIFF);
	synthesizer.setupVoice(0, SQUARE, 60, ENVELOPE0, 80, 64);
	synthesizer.setupVoice(1, SQUARE, 62, ENVELOPE0, 100, 64);
	
	for (int l = 0; l < NUM_LAYERS; ++l) {
		p_led_cube_map[l] = led_cube_map[l];
	}
	randomSeed(analogRead(10)); // seeding random for random pattern
}

void loop()
{
	VariableTimedAction::updateActions();
}

// EOF
