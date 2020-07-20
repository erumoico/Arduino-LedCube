// Create by: Jan Doležal, 2020

#include "Arduino.h"
#include "LedCube.h"

LedCubeRefresher::LedCubeRefresher(LedCube * led_cube)
	: _led_cube(led_cube)
{
	start(1000 / _led_cube->getRefreshFrequency());
}

unsigned long LedCubeRefresher::run() {
	_led_cube->update();
	return 0;
}




// TODO: led_cube_map nahradit pomocí "new" při konstruktoru a "delete" při destruktoru - https://forum.arduino.cc/index.php?topic=351930.0
LedCube::LedCube(int ** led_cube_map, int * layer, int * column, int num_layers, int num_columns, int size, int freq)
	: _led_cube_map(led_cube_map), _layer(layer), _column(column), _num_layers(num_layers), _num_columns(num_columns), _size(size), _freq(freq), _led_cube_refresher(this), _current_sequence(nullptr)
{
	_is_splitted = _size != _num_layers; // když neodpovídá počet vrstev výšce kostky, pak je kostka rozdělena
// 	_is_splitted = _size * _size != _num_columns; // druhá (výpočetně složitější) možnost

	if (_is_splitted) {
		_num_splits = _size / (_num_columns / _size);
	}
	
	_time_for_layer = 1000 / _freq / _num_layers;
	
	initCube();
}

void LedCube::_modulo(int &x, int &y, int &z)
{
	x = x % _size;
	y = y % _size;
	z = z % _size;
}

void LedCube::_turnDirect(int x, int y, int z, int state)
{
	// state = HIGH / LOW
	
	//x < _size, y < _size, z < _size;
	_modulo(x, y, z);
	
	/* Note:
	 * ----
	 * if led_cube is splitted
	 *   x and y is column mod _num_columns
	 *   z are two layers
	 * else
	 *   x and y is column
	 *   z is layer
	 */
	
	if (_is_splitted) {
		digitalWrite(_layer[(z + y / _num_splits * _size) % _num_layers], state);
	} else {
		digitalWrite(_layer[z % _num_layers], state);
	}
	digitalWrite(_column[(x + y * _size) % _num_columns], state);
}

void LedCube::_turnThroughMap(int x, int y, int z, int state)
{
	// nastaví, že při vykreslování odpovídající vrstvy, má svítit odpovídající LEDka
	int layer;
	int column;
	
	if (_is_splitted) {
		layer = (z + y / _num_splits * _size) % _num_layers;
	} else {
		layer = z % _num_layers;
	}
	column = (x + y * _size) % _num_columns;
	
	_led_cube_map[layer][column] = state;
}

void LedCube::_turn(int x, int y, int z, int state)
{
	_turnThroughMap(x, y, z, state);
}

void LedCube::initCube()
{
	for (int i = 0; i < _num_layers; ++i)
	{
		pinMode(_layer[i], OUTPUT);  //setting layers to output
		digitalWrite(_layer[i], LOW);
	}
	
	for (int i = 0; i < _num_columns; ++i)
	{
		pinMode(_column[i], OUTPUT);  //setting rows to ouput
		digitalWrite(_column[i], LOW);
	}
	
	_initMap();
}

void LedCube::_initMap()
{
	turnEverythingOff();
}

void LedCube::update()
{
	// TODO: optimalizace každý obraz sekvence příkazů
	for (int layer = 0; layer < _num_layers; ++layer) {
		// nejprve musíme nastavit sloupce a poté je nechat rozsvítit v dané vrstvě (kdybychom to udělali naopak, tak by chvilku svítily dle předchozího nastavení)
		for (int column = 0; column < _num_columns; ++column) {
			digitalWrite(_column[column], _led_cube_map[layer][column]);
		}
		digitalWrite(_layer[layer], HIGH);
		delay(_time_for_layer);
		digitalWrite(_layer[layer], LOW);
	}
}

void LedCube::updateNextLayer() // BUG: Bliká to
{
	if (_last_layer >= 0) {
		digitalWrite(_layer[_last_layer], LOW);
	}
	
	_last_layer += 1;
	if (_last_layer >= _num_layers || _last_layer < 0) {
		_last_layer = 0;
	}
	
	// nejprve musíme nastavit sloupce a poté je nechat rozsvítit v dané vrstvě (kdybychom to udělali naopak, tak by chvilku svítily dle předchozího nastavení)
	for (int column = 0; column < _num_columns; ++column) {
		digitalWrite(_column[column], _led_cube_map[_last_layer][column]);
	}
	
	digitalWrite(_layer[_last_layer], HIGH);
}

void LedCube::turnOn(int x, int y, int z)
{
	_turn(x, y, z, HIGH);
	_last_x = x;
	_last_y = y;
	_last_z = z;
}

void LedCube::turnOff(int x, int y, int z)
{
	_turn(x, y, z, LOW);
}

void LedCube::switchTo(int x, int y, int z)
{
	turnOff(_last_x, _last_y, _last_z);
	turnOn(x, y, z);
}

void LedCube::test(int speed=1000)
{
	for (int z = 0; z < _size; ++z) {
		for (int y = 0; y < _size; ++y) {
			for (int x = 0; x < _size; ++x) {
				_turnDirect(x, y, z, HIGH);
				delay(speed);
				_turnDirect(x, y, z, LOW);
			}
		}
	}
}

void LedCube::turnEverythingOff()
{
	for (int layer = 0; layer < _num_layers; ++layer) {
		for (int column = 0; column < _num_columns; ++column) {
			_led_cube_map[layer][column] = LOW;
		}
	}
}

void LedCube::turnEverythingOn()
{
	for (int layer = 0; layer < _num_layers; ++layer) {
		for (int column = 0; column < _num_columns; ++column) {
			_led_cube_map[layer][column] = HIGH;
		}
	}
}

void LedCube::setSequence(LedCubeSequence * new_sequence)
{
	stopCurrentSequence();
	_current_sequence = new_sequence;
}

unsigned long LedCube::nextFrameOfSequence()
{
	unsigned long wait = 0;
	
	if (isSequenceRunning()) {
		wait = (*_current_sequence)();
		if (wait == 0) {
			stopCurrentSequence();
		}
	} else {
		wait = 0;
	}
	
	return wait;
}

void LedCube::stopCurrentSequence()
{
	if (isSequenceRunning()) {
		delete _current_sequence;
		_current_sequence = nullptr;
	}
}

namespace sequences {
	unsigned long FlickerOn::operator()()
	{
		do {
			_state += 1;
			switch(_state) {
				case 1:
					_led_cube->turnEverythingOn(); return _wait;
					break;
				case 2:
					_led_cube->turnEverythingOff(); return _wait;
					break;
				default:
					_state = 0;
					if (_wait > _step) {
						_wait -= _step;
					} else {
						return 0;
					}
			}
		} while (_state == 0);
	}

	unsigned long FlickerOff::operator()()
	{
		do {
			_state += 1;
			switch(_state) {
				case 1:
					_led_cube->turnEverythingOff(); return _wait+50;
					break;
				case 2:
					_led_cube->turnEverythingOn(); return _wait;
					break;
				default:
					_state = 0;
					if (_wait < _max_wait) {
						_wait += _step;
					} else {
						return 0;
					}
			}
		} while (_state == 0);
	}

	unsigned long TurnOnAndOffAllByLayerUpAndDown::operator()()
	{
		int x = 0;
		int y = 0;
		int z = 0;
		
		while (true) {
			switch(_state) {
				// TODO: Místo čísel použít enum (viz https://en.cppreference.com/w/cpp/language/enum )
				case 0:
					_led_cube->turnEverythingOn();
					_state += 1;
					return _wait;
				case 1:
					_layer = _led_cube->getSize()-1;
					_state += 1;
				case 2:
					// Turn off by layer from the top to the bottom
					z = _layer;
					for (x = 0; x < _led_cube->getSize(); ++x) {
						for (y = 0; y < _led_cube->getSize(); ++y) {
							_led_cube->turnOff(x, y, z);
						}
					}
					_layer -= 1;
					if (_layer < 0) {
						_layer = 0;
						_state += 1;
					}
					return _wait;
				case 3:
					// Turn on by layer from the bottom to the top
					z = _layer;
					for (x = 0; x < _led_cube->getSize(); ++x) {
						for (y = 0; y < _led_cube->getSize(); ++y) {
							_led_cube->turnOn(x, y, z);
						}
					}
					_layer += 1;
					if (_layer >= _led_cube->getSize()) {
						_layer = 0;
						_state += 1;
					}
					return _wait;
				case 4:
					// Turn off by layer from the bottom to the top
					z = _layer;
					for (x = 0; x < _led_cube->getSize(); ++x) {
						for (y = 0; y < _led_cube->getSize(); ++y) {
							_led_cube->turnOff(x, y, z);
						}
					}
					_layer += 1;
					if (_layer >= _led_cube->getSize()) {
						_layer = _led_cube->getSize()-1;
						_state += 1;
					}
					return _wait;
				case 5:
					// Turn on by layer from the top to the bottom
					z = _layer;
					for (x = 0; x < _led_cube->getSize(); ++x) {
						for (y = 0; y < _led_cube->getSize(); ++y) {
							_led_cube->turnOn(x, y, z);
						}
					}
					_layer -= 1;
					if (_layer < 0) {
						_layer = 0;
						_state += 1;
					}
					return _wait;
				case 6:
					if (_cycles_cnt < _max_cycles) {
						_cycles_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long TurnOnAndOffAllByLayerSideways::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
					return _wait;
				case 1:
					_layer = 0;
					_state += 1;
				case 2:
					// Turn on by layer from the front to the back
					for (int i = 0; i < _led_cube->getSize(); ++i) {
						for (int j = 0; j < _led_cube->getSize(); ++j) {
							_led_cube->turnOn(i, _layer, j);
						}
					}
					_layer += 1;
					if (_layer >= _led_cube->getSize()) {
						_layer = 0;
						_state += 1;
					}
					return _wait;
				case 3:
					// Turn off by layer from the front to the back
					for (int i = 0; i < _led_cube->getSize(); ++i) {
						for (int j = 0; j < _led_cube->getSize(); ++j) {
							_led_cube->turnOff(i, _layer, j);
						}
					}
					_layer += 1;
					if (_layer >= _led_cube->getSize()) {
						_layer = _led_cube->getSize()-1;
						_state += 1;
					}
					return _wait;
				case 4:
					// Turn on by layer from the back to the front
					for (int i = 0; i < _led_cube->getSize(); ++i) {
						for (int j = 0; j < _led_cube->getSize(); ++j) {
							_led_cube->turnOn(i, _layer, j);
						}
					}
					_layer -= 1;
					if (_layer < 0) {
						_layer = _led_cube->getSize()-1;
						_state += 1;
					}
					return _wait;
				case 5:
					// Turn off by layer from the back to the front
					for (int i = 0; i < _led_cube->getSize(); ++i) {
						for (int j = 0; j < _led_cube->getSize(); ++j) {
							_led_cube->turnOff(i, _layer, j);
						}
					}
					_layer -= 1;
					if (_layer < 0) {
						_layer = 0;
						_state += 1;
					}
					return _wait;
				case 6:
					if (_cycles_cnt < _max_cycles) {
						_cycles_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long LayerStompUpAndDown::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
					return _wait;
				case 1:
					_layer = 0;
					_state += 1;
				case 2:
					// Turn on one layer at the bottom
					for (int i = 0; i < _led_cube->getSize(); ++i) {
						for (int j = 0; j < _led_cube->getSize(); ++j) {
							_led_cube->turnOn(i, j, _layer);
						}
					}
					_state += 1;
					return _wait;
				case 3:
					// Move the layer from the bottom to the top
					if (_layer < _led_cube->getSize()-1) {
						for (int i = 0; i < _led_cube->getSize(); ++i) {
							for (int j = 0; j < _led_cube->getSize(); ++j) {
								_led_cube->turnOff(i, j, _layer);
								_led_cube->turnOn(i, j, _layer+1);
							}
						}
						_layer += 1;
					} else {
						_state += 1;
					}
					return _wait;
				case 4:
					// Move the layer from the top to the bottom
					if (_layer > 0) {
						for (int i = 0; i < _led_cube->getSize(); ++i) {
							for (int j = 0; j < _led_cube->getSize(); ++j) {
								_led_cube->turnOff(i, j, _layer);
								_led_cube->turnOn(i, j, _layer-1);
							}
						}
						_layer -= 1;
					} else if (_inner_repeats_cnt < _max_inner_repeats) {
						_inner_repeats_cnt += 1;
						_state -= 1;
					} else {
						_inner_repeats_cnt = 1;
						_state += 1;
					}
					return _wait;
				case 5:
					// Expand the bottom layer to the top
					if (_layer < _led_cube->getSize()-1) {
						for (int i = 0; i < _led_cube->getSize(); ++i) {
							for (int j = 0; j < _led_cube->getSize(); ++j) {
								_led_cube->turnOn(i, j, _layer+1);
							}
						}
						_layer += 1;
					} else {
						_state += 1;
					}
					return _wait;
				case 6:
					// Shrink the bottom layer
					if (_layer >= 0) {
						for (int i = 0; i < _led_cube->getSize(); ++i) {
							for (int j = 0; j < _led_cube->getSize(); ++j) {
								_led_cube->turnOff(i, j, _layer);
							}
						}
						_layer -= 1;
					} else {
						_state += 1;
					}
					return _wait;
				case 7:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	void AroundEdgeDown::_createTrace()
	{
		_trace_len = 0;
		
		int last_x = 0;
		int last_y = 0;
		
		int low = 0;
		int high = _led_cube->getSize()-1;
		if (low < high) {
			for (int i = low; i < high; ++i) {
				_trace[_trace_len++] = {.x = last_x, .y = last_y++};
			}
			for (int i = low; i < high; ++i) {
				_trace[_trace_len++] = {.x = last_x++, .y = last_y};
			}
			for (int i = low; i < high; ++i) {
				_trace[_trace_len++] = {.x = last_x, .y = last_y--};
			}
			for (int i = low; i < high; ++i) {
				_trace[_trace_len++] = {.x = last_x--, .y = last_y};
			}
		} else if (low == high) {
			_trace[_trace_len++] = {.x = last_x, .y = last_y};
		}
	}

	unsigned long AroundEdgeDown::operator()()
	{
		Coord * coord = nullptr;
		int middle = _led_cube->getSize()/2 - 1;
		
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_layer = _led_cube->getSize()-1;
					_state += 1;
				case 1:
					_trace_step = 0;
					for (int x = middle; x < _led_cube->getSize() - middle; ++x) {
						for (int y = middle; y < _led_cube->getSize() - middle; ++y) {
							_led_cube->turnOn(x, y, _layer);
						}
					}
					_state += 1;
				case 2:
					if (_trace_step < _trace_len) {
						_state += 1;
					} else {
						_state = 5;
					}
					break;
				case 3:
					coord = &(_trace[_trace_step]);
					_led_cube->turnOn(coord->x, coord->y, _layer);
					_state += 1;
					return _wait;
				case 4:
					coord = &(_trace[_trace_step]);
					_led_cube->turnOff(coord->x, coord->y, _layer);
					_trace_step += 1;
					_state = 2;
					break;
				case 5:
					if (_layer > 0) {
						_layer -= 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				case 6:
					if (_wait > _time_step) {
						_wait -= _time_step;
						_state = 0;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long RandomFlicker::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
				case 1:
					_last_x = random(_led_cube->getSize());
					_last_y = random(_led_cube->getSize());
					_last_z = random(_led_cube->getSize());
					
					_led_cube->turnOn(_last_x, _last_y, _last_z);
					_state += 1;
					return _wait;
				case 2:
					_led_cube->turnOff(_last_x, _last_y, _last_z);
					_state += 1;
					return _wait;
				case 3:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long RandomRain::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
				case 1:
					_last_x = random(_led_cube->getSize());
					_last_y = random(_led_cube->getSize());
					_layer = _led_cube->getSize()-1;
					_state += 1;
				case 2:
					_led_cube->turnOn(_last_x, _last_y, _layer);
					_state += 1;
					if (_layer == _led_cube->getSize()-1 || _layer == 0) {
						return _wait + _wait/2;
					} else {
						return _wait;
					}
				case 3:
					_led_cube->turnOff(_last_x, _last_y, _layer);
					_state += 1;
					break;
				case 4:
					if (_layer > 0) {
						_layer -= 1;
						_state = 2;
					} else {
						_state += 1;
					}
					break;
				case 5:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long MatrixRain::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
				case 1:
					for (int i = 0; i < _max_drops; ++i) {
						if (_drops[i].enable == false && random(6) < 2) {
							_drops[i].enable = true;
							_drops[i].redraw = true;
							_drops[i].x = random(_led_cube->getSize());
							_drops[i].y = random(_led_cube->getSize());
							_drops[i].layer = _led_cube->getSize()-1;
							_drops[i].sublayer = 0;
							_drops[i].slowness = random(_led_cube->getSize()*2);
						}
					}
					_state += 1;
				case 2:
					for (int i = 0; i < _max_drops; ++i) {
						if (_drops[i].enable && _drops[i].redraw) {
							_drops[i].redraw = false;
							_led_cube->turnOn(_drops[i].x, _drops[i].y, _drops[i].layer);
						}
					}
					_state += 1;
					return _wait;
				case 3:
					for (int i = 0; i < _max_drops; ++i) {
						if (_drops[i].enable) {
// 							if (random(100) > 0) { // BUG: Při 8 kapkách už to bliká. Není to generátorem pseudonáhodných čísel? Zdá se, že ano.
							if (true) {
// 								if (_drops[i].layer < _led_cube->getSize()-1 && random(100) < 1) {
								if (false) {
									_led_cube->turnOff(_drops[i].x, _drops[i].y, _drops[i].layer);
									_drops[i].layer += 1;
									_drops[i].redraw = true;
								} else if (_drops[i].sublayer < _drops[i].slowness) {
									_drops[i].sublayer += 1;
								} else {
									_led_cube->turnOff(_drops[i].x, _drops[i].y, _drops[i].layer);
									_drops[i].sublayer = 0;
									if (_drops[i].layer > 0) {
										_drops[i].layer -= 1;
										_drops[i].redraw = true;
									} else {
										_drops[i].enable = false;
									}
								}
							}
						}
					}
					_state += 1;
					break;
				case 4:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						bool any_enabled = false;
						for (int i = 0; i < _max_drops; ++i) {
							if (_drops[i].enable) {
								any_enabled = true;
								break;
							}
						}
						if (any_enabled) {
							_state = 2;
						} else {
							_state += 1;
						}
					}
					break;
				default:
					return 0;
			}
		}
	}

	void DiagonalRectangle::_topLeftOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j, 2);
				_led_cube->turnOn(i, j, 3);
			}
		}
	}

	void DiagonalRectangle::_topMiddleOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j+1, 2);
				_led_cube->turnOn(i, j+1, 3);
			}
		}
	}

	void DiagonalRectangle::_topRightOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j+2, 2);
				_led_cube->turnOn(i, j+2, 3);
			}
		}
	}

	void DiagonalRectangle::_middleMiddleOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j+1, 1);
				_led_cube->turnOn(i, j+1, 2);
			}
		}
	}

	void DiagonalRectangle::_bottomLeftOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j, 0);
				_led_cube->turnOn(i, j, 1);
			}
		}
	}

	void DiagonalRectangle::_bottomMiddleOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j+1, 0);
				_led_cube->turnOn(i, j+1, 1);
			}
		}
	}

	void DiagonalRectangle::_bottomRightOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			for (int j = 0; j < _led_cube->getSize()/2; ++j) {
				_led_cube->turnOn(i, j+2, 0);
				_led_cube->turnOn(i, j+2, 1);
			}
		}
	}

	unsigned long DiagonalRectangle::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_topLeftOn();
					_state += 1;
					return _wait;
				case 1:
					_led_cube->turnEverythingOff();
					_middleMiddleOn();
					_state += 1;
					return _wait;
				case 2:
					_led_cube->turnEverythingOff();
					_bottomRightOn();
					_state += 1;
					return _wait;
				case 3:
					_led_cube->turnEverythingOff();
					_bottomMiddleOn();
					_state += 1;
					return _wait;
				case 4:
					_led_cube->turnEverythingOff();
					_bottomLeftOn();
					_state += 1;
					return _wait;
				case 5:
					_led_cube->turnEverythingOff();
					_middleMiddleOn();
					_state += 1;
					return _wait;
				case 6:
					_led_cube->turnEverythingOff();
					_topRightOn();
					_state += 1;
					return _wait;
				case 7:
					_led_cube->turnEverythingOff();
					_topMiddleOn();
					_state += 1;
					return _wait;
				case 8:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 0;
					} else {
						_led_cube->turnEverythingOff();
						_topLeftOn();
						_state += 1;
						return _wait;
					}
					break;
				case 9:
					_led_cube->turnEverythingOff();
					_state += 1;
					break;
				default:
					return 0;
			}
		}
	}

	void Propeller::_diagonalLeftToRightOn()
	{
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			_led_cube->turnOn(i, i, _layer);
		}
	}

	void Propeller::_diagonalRightToLeftOn()
	{
		int high = _led_cube->getSize()-1;
		
		for (int i = 0; i < _led_cube->getSize(); ++i) {
			_led_cube->turnOn(high - i, i, _layer);
		}
	}

	void Propeller::_sLeftToRightOn()
	{
		int high = _led_cube->getSize()-1;
		int middle = _led_cube->getSize()/2 + _led_cube->getSize()%2 - 1;
		int y = middle;
		
		for (int x = 0; x < _led_cube->getSize()/2; ++x) {
			_led_cube->turnOn(x, y, _layer);
			_led_cube->turnOn(high - x, high - y, _layer);
		}
	}

	void Propeller::_sFrontToBackOn()
	{
		int high = _led_cube->getSize()-1;
		int middle = _led_cube->getSize()/2 + _led_cube->getSize()%2 - 1;
		int x = middle;
		
		for (int y = 0; y < _led_cube->getSize()/2; ++y) {
			_led_cube->turnOn(x, high - y, _layer);
			_led_cube->turnOn(high - x, y, _layer);
		}
	}

	void Propeller::_zRightToLeftOn()
	{
		int high = _led_cube->getSize()-1;
		int middle = _led_cube->getSize()/2 + _led_cube->getSize()%2 - 1;
		int y = middle;
		
		for (int x = 0; x < _led_cube->getSize()/2; ++x) {
			_led_cube->turnOn(high - x, y, _layer);
			_led_cube->turnOn(x, high - y, _layer);
		}
	}

	void Propeller::_zBackToFrontOn()
	{
		int high = _led_cube->getSize()-1;
		int middle = _led_cube->getSize()/2 + _led_cube->getSize()%2 - 1;
		int x = middle;
		
		for (int y = 0; y < _led_cube->getSize()/2; ++y) {
			_led_cube->turnOn(x, y, _layer);
			_led_cube->turnOn(high - x, high - y, _layer);
		}
	}

	unsigned long Propeller::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_diagonalLeftToRightOn();
					_state += 1;
					return _wait;
				case 1:
					_led_cube->turnEverythingOff();
					_sLeftToRightOn();
					_state += 1;
					return _wait;
				case 2:
					_led_cube->turnEverythingOff();
					_zRightToLeftOn();
					_state += 1;
					return _wait;
				case 3:
					_led_cube->turnEverythingOff();
					_diagonalRightToLeftOn();
					_state += 1;
					return _wait;
				case 4:
					_led_cube->turnEverythingOff();
					_sFrontToBackOn();
					_state += 1;
					return _wait;
				case 5:
					_led_cube->turnEverythingOff();
					_zBackToFrontOn();
					_state += 1;
					return _wait;
				case 6:
					if (_inner_repeats_cnt < _max_inner_repeats) {
						_inner_repeats_cnt += 1;
						_state = 0;
					} else {
						_inner_repeats_cnt = 1;
						_state += 1;
					}
					break;
				case 7:
					if (_layer > 0) {
						_layer -= 1;
						_state = 0;
					} else {
						_state += 1;
					}
					break;
				case 8:
					_led_cube->turnEverythingOff();
// 					_diagonalLeftToRightOn();
					_state += 1;
					return _wait;
				default:
					return 0;
			}
		}
	}

	void SpiralInAndOut::_createMapForSpiralInClockwise()
	{
		int last_x = 0;
		int last_y = 0;
		
		int column = 0;
		int low = 0;
		int high = _led_cube->getSize()-1;
		while (low < high) {
			for (int i = low; i < high; ++i) {
				_spiral_in_clockwise[column++] = {.x = last_x, .y = last_y++};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_clockwise[column++] = {.x = last_x++, .y = last_y};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_clockwise[column++] = {.x = last_x, .y = last_y--};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_clockwise[column++] = {.x = last_x--, .y = last_y};
			}
			low += 1;
			high -= 1;
			
			last_x = last_y = low;
		}
		if (low == high) {
			_spiral_in_clockwise[column++] = {.x = last_x, .y = last_y};
		}
	}

	void SpiralInAndOut::_createMapForSpiralInCounterClockwise()
	{
		int last_x = 0;
		int last_y = 0;
		
		int column = 0;
		int low = 0;
		int high = _led_cube->getSize()-1;
		while (low < high) {
			for (int i = low; i < high; ++i) {
				_spiral_in_counter_clockwise[column++] = {.x = last_x++, .y = last_y};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_counter_clockwise[column++] = {.x = last_x, .y = last_y++};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_counter_clockwise[column++] = {.x = last_x--, .y = last_y};
			}
			for (int i = low; i < high; ++i) {
				_spiral_in_counter_clockwise[column++] = {.x = last_x, .y = last_y--};
			}
			low += 1;
			high -= 1;
			
			last_x = last_y = low;
		}
		if (low == high) {
			_spiral_in_counter_clockwise[column++] = {.x = last_x, .y = last_y};
		}
	}

	void SpiralInAndOut::_turnOnColumn(Column column)
	{
		for (int z = 0; z < _led_cube->getSize(); ++z) {
			_led_cube->turnOn(column.x, column.y, z);
		}
	}

	void SpiralInAndOut::_turnOffColumn(Column column)
	{
		for (int z = 0; z < _led_cube->getSize(); ++z) {
			_led_cube->turnOff(column.x, column.y, z);
		}
	}

	unsigned long SpiralInAndOut::operator()()
	{
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOn();
					_column = 0;
					_state += 1;
					return _wait;
				case 1:
					//spiral in clockwise
					if (_column < _max_columns) {
						_turnOffColumn(_spiral_in_clockwise[_column]);
						_column += 1;
						return _wait;
					} else {
						_column = _max_columns-1;
						_state += 1;
					}
					break;
				case 2:
					//spiral out counter clockwise
					if (_column >= 0) {
						_turnOnColumn(_spiral_in_clockwise[_column]);
						_column -= 1;
						return _wait;
					} else {
						_column = 0;
						_state += 1;
					}
					break;
				case 3:
					//spiral in counter clockwise
					if (_column < _max_columns) {
						_turnOffColumn(_spiral_in_counter_clockwise[_column]);
						_column += 1;
						return _wait;
					} else {
						_column = _max_columns-1;
						_state += 1;
					}
					break;
				case 4:
					//spiral out clockwise
					if (_column >= 0) {
						_turnOnColumn(_spiral_in_counter_clockwise[_column]);
						_column -= 1;
						return _wait;
					} else {
						_column = 0;
						_state += 1;
					}
					break;
				case 5:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				case 6:
					_led_cube->turnEverythingOff();
					_state += 1;
					return _wait;
				default:
					return 0;
			}
		}
	}

	void GoThroughAllLedsOneAtATime::_createTrace()
	{
		int step = 0;
		int high = _led_cube->getSize()-1;
		bool go_up = false;
		
		for (int y = 0; y < _led_cube->getSize(); ++y) {
			for (int z = 0; z < _led_cube->getSize(); ++z) {
				for (int x = 0; x < _led_cube->getSize(); ++x) {
					_trace[step++] = {.x = x, .y = y, .z = (go_up)?(z):(high - z)};
				}
			}
			go_up = !go_up;
		}
	}

	unsigned long GoThroughAllLedsOneAtATime::operator()()
	{
		Coord * coord = nullptr;
		
		while (true) {
			switch(_state) {
				case 0:
					_led_cube->turnEverythingOff();
					_state += 1;
				case 1:
					_step = 0;
					_state += 1;
				case 2:
					if (_step < _trace_len) {
						_state += 1;
					} else {
						_state = 5;
					}
					break;
				case 3:
					coord = &(_trace[_step]);
					_led_cube->turnOn(coord->x, coord->y, coord->z);
					_state += 1;
					return _wait;
				case 4:
					coord = &(_trace[_step]);
					_led_cube->turnOff(coord->x, coord->y, coord->z);
					_step += 1;
					_state = 2;
					return _wait;
				case 5:
					if (_whole_repeats_cnt < _max_whole_repeats) {
						_whole_repeats_cnt += 1;
						_state = 1;
					} else {
						_state += 1;
					}
					break;
				default:
					return 0;
			}
		}
	}

	unsigned long Demo::operator()()
	{
		unsigned long wait = 0;
		const unsigned long wait_between_sequences = 50;
		
		while (true) {
			if (_isSequenceRunning()) {
				wait = _nextFrameOfSequence();
				if (wait > 0) {
					return wait;
				}
			} else {
				_state += 1;
				switch(_state) {
					case 1:
						_setSequence(new TurnEverythingOff(_led_cube));
						return wait_between_sequences;
					case 2:
						_setSequence(new FlickerOn(_led_cube));
						return wait_between_sequences;
					case 3:
						_setSequence(new TurnEverythingOn(_led_cube));
						return wait_between_sequences;
					case 4:
						_setSequence(new TurnOnAndOffAllByLayerUpAndDown(_led_cube));
						return 250;
					case 5:
						_setSequence(new LayerStompUpAndDown(_led_cube));
						return wait_between_sequences;
					case 6:
						_setSequence(new SpiralInAndOut(_led_cube));
						return wait_between_sequences;
					case 7:
						_setSequence(new TurnOnAndOffAllByLayerSideways(_led_cube));
						return wait_between_sequences;
					case 8:
						_setSequence(new AroundEdgeDown(_led_cube));
						return 250;
					case 9:
						_setSequence(new TurnEverythingOff(_led_cube));
						return wait_between_sequences;
					case 10:
						_setSequence(new RandomFlicker(_led_cube));
						return wait_between_sequences;
					case 11:
						_setSequence(new RandomRain(_led_cube));
						return wait_between_sequences;
					case 12:
						_setSequence(new MatrixRain(_led_cube));
						return wait_between_sequences;
					case 13:
						_setSequence(new DiagonalRectangle(_led_cube));
						return wait_between_sequences;
					case 14:
						_setSequence(new GoThroughAllLedsOneAtATime(_led_cube));
						return wait_between_sequences;
					case 15:
						_setSequence(new Propeller(_led_cube));
						return wait_between_sequences;
					case 16:
						_setSequence(new SpiralInAndOut(_led_cube));
						return wait_between_sequences;
					case 17:
						_setSequence(new FlickerOff(_led_cube));
						return wait_between_sequences;
					case 18:
						_setSequence(new TurnEverythingOff(_led_cube));
						return wait_between_sequences;
					case 19:
						return 2000;
					default:
						return 0;
				}
			}
		}
		
		return 0;
	}

	void Demo::_setSequence(LedCubeSequence * new_sequence)
	{
		_stopCurrentSequence();
		_current_sequence = new_sequence;
	}

	unsigned long Demo::_nextFrameOfSequence()
	{
		unsigned long wait = 0;
		
		if (_isSequenceRunning()) {
			wait = (*_current_sequence)();
			if (wait == 0) {
				_stopCurrentSequence();
			}
		} else {
			wait = 0;
		}
		
		return wait;
	}

	void Demo::_stopCurrentSequence()
	{
		if (_isSequenceRunning()) {
			delete _current_sequence;
			_current_sequence = nullptr;
		}
	}
}


void center(LedCube * _led_cube) {
	int low = _led_cube->getSize()/2 - 1;
	int high = _led_cube->getSize()-1 - low;
	
	for (int z = low; z <= high; ++z) {
		for (int y = low; y <= high; ++y) {
			for (int x = low; x <= high; ++x) {
				_led_cube->turnOn(x, y, z);
			}
		}
	}
}

void corners(LedCube * _led_cube) {
	int low = 0;
	int high = _led_cube->getSize()-1;
	
	for (int z = low; ; z = high) {
		for (int y = low; ; y = high) {
			for (int x = low; ; x = high) {
				
				_led_cube->turnOn(x, y, z);
				
				if (x == high) break;
			}
			if (y == high) break;
		}
		if (z == high) break;
	}
}

void sides(LedCube * _led_cube) {
	int low = 0;
	int high = _led_cube->getSize()-1;
	
	for (int i = 0; i <= high; ++i) {
		for (int j = 0; j <= high; ++j) {
			_led_cube->turnOn(i, j, low);
			_led_cube->turnOn(i, j, high);
			_led_cube->turnOn(i, low, j);
			_led_cube->turnOn(i, high, j);
			_led_cube->turnOn(low, i, j);
			_led_cube->turnOn(high, i, j);
		}
	}
}

// EOF
