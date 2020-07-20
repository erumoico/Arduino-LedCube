// Create by: Jan Doležal, 2020

#include "LedCube.h"

#define SIZE 4 // 4x4x4 => 4, 8x8x8 => 8
#define NUM_LAYERS 8
#define NUM_COLUMNS 8

int led_cube_map[NUM_LAYERS][NUM_COLUMNS];
int * p_led_cube_map[NUM_LAYERS];
int layer[NUM_LAYERS] = {A2,A3,A4,A5,12,13,A0,A1}; // initializing and declaring led layers
int column[NUM_COLUMNS] = {2,6,10,8,4,5,9,7}; // initializing and declaring led rows

LedCube led_cube(p_led_cube_map, layer, column, NUM_LAYERS, NUM_COLUMNS, SIZE, 60);

void setup()
{
	for (int l = 0; l < NUM_LAYERS; ++l) {
		p_led_cube_map[l] = led_cube_map[l];
	}
	randomSeed(analogRead(10)); // seeding random for random pattern
}

void loop()
{
	led_cube.test();
}

// EOF
