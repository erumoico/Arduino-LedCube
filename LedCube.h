#ifndef _LED_CUBE_H
#define _LED_CUBE_H

// Create by: Jan Doležal, 2020

#include <VariableTimedAction.h>

class LedCube;
class LedCubeRefresher;
class LedCubeSequence;


class LedCubeRefresher : public VariableTimedAction
{
private:
	LedCube * _led_cube;
	
	unsigned long run();

public:
	LedCubeRefresher(LedCube * led_cube);
};


class LedCube
{
private:
	int _size;
	int ** _led_cube_map;
	int * _layer;
	int * _column;
	int _num_layers;
	int _num_columns;
	bool _is_splitted;
	int _num_splits;
	int _freq;
	int _time_for_layer; // [ms]
	LedCubeRefresher _led_cube_refresher;
	LedCubeSequence * _current_sequence;
	
	int _last_x = 0, _last_y = 0, _last_z = 0, _last_layer = -1;
	
	void _modulo(int &x, int &y, int &z);
	
	void _turnDirect(int x, int y, int z, int state);
	
	void _turnThroughMap(int x, int y, int z, int state);
	
	void _turn(int x, int y, int z, int state);
	
	void _initMap();
	
public:
	LedCube(int ** led_cube_map, int * layer, int * column, int num_layers, int num_columns, int size, int freq);
	
	void initCube();
	
	void update();
	
	void updateNextLayer();
	
	void turnOn(int x, int y, int z);
	
	void turnOff(int x, int y, int z);
	
	void switchTo(int x, int y, int z);
	
	// TODO: void move(axis={x,y,z}, distance=<int>, zero/rotate=<bool>)
	// TODO: void rotate(axis={x,y,z}, angle=+/-{45,90,135,180}, center=<coord>)
	// TODO: void scale(axis={x,y,z}, value=<int>)
	// TODO: void mirror(axis={x,y,z}, clone=<bool>)
	
	void test(int speed=1000);
	
	void turnEverythingOff();
	
	void turnEverythingOn();
	
	void setSequence(LedCubeSequence * new_sequence);
	
	unsigned long nextFrameOfSequence();
	
	void stopCurrentSequence();
	
	bool isSequenceRunning() { return _current_sequence != nullptr; }
	
	int getRefreshFrequency() { return _freq; }
	
	int getTimeForLayer() { return _time_for_layer; }
	
	int getSize() { return _size; }

};


class LedCubeSequence
{
protected:
	LedCube * _led_cube;
	int _state;
public:
	LedCubeSequence(LedCube * led_cube)
		: _led_cube(led_cube), _state(0)
	{}
	
	virtual unsigned long operator()() = 0;
};

namespace sequences {
	class TurnEverythingOff : public LedCubeSequence
	{
	public:
		TurnEverythingOff(LedCube * led_cube)
			: LedCubeSequence(led_cube)
		{}
		
		unsigned long operator()() { _led_cube->turnEverythingOff(); return 0; }
	};

	class TurnEverythingOn : public LedCubeSequence
	{
	public:
		TurnEverythingOn(LedCube * led_cube)
			: LedCubeSequence(led_cube)
		{}
		
		unsigned long operator()() { _led_cube->turnEverythingOn(); return 0; }
	};

	class FlickerOn : public LedCubeSequence
	{
	protected:
		const unsigned long _max_wait; // [ms]
		unsigned long _wait; // [ms]
		const unsigned long _step; // [ms]
	public:
		FlickerOn(LedCube * led_cube, unsigned long max_wait=150, unsigned long step=5)
			: LedCubeSequence(led_cube), _max_wait(max_wait), _step(step)
		{
			_wait = _max_wait;
		}
		
		unsigned long operator()();
	};

	class FlickerOff : public FlickerOn
	{
	public:
		FlickerOff(LedCube * led_cube, unsigned long max_wait=150, unsigned long step=5)
			: FlickerOn(led_cube, max_wait, step)
		{
			_wait = _step;
		}
		
		unsigned long operator()();
	};

	class TurnOnAndOffAllByLayerUpAndDown : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		int _layer;
		const int _max_cycles;
		int _cycles_cnt;
	public:
		TurnOnAndOffAllByLayerUpAndDown(LedCube * led_cube, unsigned long wait=75, int max_cycles=5)
			: LedCubeSequence(led_cube), _wait(wait), _layer(0), _max_cycles(max_cycles), _cycles_cnt(0)
		{}
		
		unsigned long operator()();
	};

	class TurnOnAndOffAllByLayerSideways : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		int _layer;
		const int _max_cycles;
		int _cycles_cnt;
	public:
		TurnOnAndOffAllByLayerSideways(LedCube * led_cube, unsigned long wait=75, int max_cycles=5)
			: LedCubeSequence(led_cube), _wait(wait), _layer(0), _max_cycles(max_cycles), _cycles_cnt(1)
		{}
		
		unsigned long operator()();
	};

	class LayerStompUpAndDown : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		int _layer;
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		const int _max_inner_repeats;
		int _inner_repeats_cnt;
	public:
		LayerStompUpAndDown(LedCube * led_cube, unsigned long wait=75, int max_whole_repeats=5, int max_inner_repeats=2)
			: LedCubeSequence(led_cube), _wait(wait), _layer(0), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1), _max_inner_repeats(max_inner_repeats), _inner_repeats_cnt(1)
		{}
		
		unsigned long operator()();
	};

	class AroundEdgeDown : public LedCubeSequence
	{
	protected:
		const unsigned long _max_wait; // [ms]
		unsigned long _wait; // [ms]
		const unsigned long _time_step; // [ms]
		int _layer;
		
		int _trace_step;
		static const int _trace_capacity = 32;
		int _trace_len;
		struct Coord {
			int x;
			int y;
		};
		Coord _trace[_trace_capacity];
		
		void _createTrace();
	public:
		AroundEdgeDown(LedCube * led_cube, unsigned long max_wait=200, unsigned long step=50)
			: LedCubeSequence(led_cube), _max_wait(max_wait), _time_step(step), _layer(0)
		{
			_wait = _max_wait;
			_createTrace();
		}
		
		unsigned long operator()();
	};

	class RandomFlicker : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		int _last_x, _last_y, _last_z;
	public:
		RandomFlicker(LedCube * led_cube, unsigned long wait=20, int max_whole_repeats=750/2)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1), _last_x(0), _last_y(0), _last_z(0)
		{}
		
		unsigned long operator()();
	};

	class RandomRain : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		int _last_x, _last_y, _layer;
	public:
		RandomRain(LedCube * led_cube, unsigned long wait=100, int max_whole_repeats=60/2)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1), _last_x(0), _last_y(0), _layer(0)
		{}
		
		unsigned long operator()();
	};

	class MatrixRain : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		static const int _max_drops = 16;
		struct drop {
			bool enable;
			int x;
			int y;
			int layer;
			int sublayer;
			int slowness;
			bool redraw;
		} _drops[_max_drops];
	public:
		MatrixRain(LedCube * led_cube, unsigned long wait=100, int max_whole_repeats=500)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1)
		{
			for (int i = 0; i < _max_drops; ++i) {
				_drops[i].enable = false;
				_drops[i].x = 0;
				_drops[i].y = 0;
				_drops[i].layer = 0;
				_drops[i].sublayer = 0;
				_drops[i].slowness = 0;
			}
		}
		
		unsigned long operator()();
	};

	class DiagonalRectangle : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		
		void _topLeftOn();
		void _topMiddleOn();
		void _topRightOn();
		void _middleMiddleOn();
		void _bottomLeftOn();
		void _bottomMiddleOn();
		void _bottomRightOn();
	public:
		DiagonalRectangle(LedCube * led_cube, unsigned long wait=350, int max_whole_repeats=5)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1)
		{}
		
		unsigned long operator()();
	};

	class Propeller : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_inner_repeats;
		int _inner_repeats_cnt;
		int _layer;
		
		void _diagonalLeftToRightOn();
		void _sLeftToRightOn();
		void _zRightToLeftOn();
		void _diagonalRightToLeftOn();
		void _sFrontToBackOn();
		void _zBackToFrontOn();
	public:
		Propeller(LedCube * led_cube, unsigned long wait=90, int max_inner_repeats=6)
			: LedCubeSequence(led_cube), _wait(wait), _max_inner_repeats(max_inner_repeats), _inner_repeats_cnt(1)
		{
			_layer = _led_cube->getSize()-1;
		}
		
		unsigned long operator()();
	};

	class SpiralInAndOut : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		int _column;
		static const int _max_columns = 16;
		struct Column {
			int x;
			int y;
		};
		Column _spiral_in_clockwise[_max_columns];
		Column _spiral_in_counter_clockwise[_max_columns];
		
		void _createMapForSpiralInClockwise();
		void _createMapForSpiralInCounterClockwise();
		void _turnOnColumn(Column column);
		void _turnOffColumn(Column column);
	public:
		SpiralInAndOut(LedCube * led_cube, unsigned long wait=60, int max_whole_repeats=6)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1), _column(0)
		{
			_createMapForSpiralInClockwise();
			_createMapForSpiralInCounterClockwise();
		}
		
		unsigned long operator()();
	};

	class GoThroughAllLedsOneAtATime : public LedCubeSequence
	{
	protected:
		const unsigned long _wait; // [ms]
		const int _max_whole_repeats;
		int _whole_repeats_cnt;
		struct Coord {
			int x;
			int y;
			int z;
		};
		static const int _trace_len = 64;
		Coord _trace[_trace_len];
		int _step;
		
		void _createTrace();
	public:
		GoThroughAllLedsOneAtATime(LedCube * led_cube, unsigned long wait=20, int max_whole_repeats=5)
			: LedCubeSequence(led_cube), _wait(wait), _max_whole_repeats(max_whole_repeats), _whole_repeats_cnt(1)
		{
			_createTrace();
		}
		
		unsigned long operator()();
	};




	class Demo : public LedCubeSequence
	{
	protected:
		LedCubeSequence * _current_sequence;
		
		void _setSequence(LedCubeSequence * new_sequence);
		
		unsigned long _nextFrameOfSequence();
		
		void _stopCurrentSequence();
		
		bool _isSequenceRunning() { return _current_sequence != nullptr; }
	public:
		Demo(LedCube * led_cube)
			: LedCubeSequence(led_cube), _current_sequence(nullptr)
		{}
		
		unsigned long operator()();
	};
}

#endif // _LED_CUBE_H
