#pragma once
#include "gene.h"
#include "terrain.h"

//#include "terrain_generation.h"
//#include "my_test.h"

//only in GUI mode
#include "..\Framework\Test.h"

//terrain properties for all worlds
constexpr int terrain_nstep{ 8200 };
constexpr int terrain_xmax{ 8000 };

//_JUST_IGNORE_THIS is for internal usage in singleton creation.
enum fitness_judge
{
	DISTANCE = 0, SPEED, FANCY, JUMP, CLIMB, ROCK_AND_ROLL, WELCOME, _JUST_IGNORE_THIS
};

const char * get_name(fitness_judge judge);
bool getModeName(void*, int, const char**);

float mmax(float, float);

//singleton class definition
class S_terrain
{
public:
	static S_terrain* getInstance()
	{
		if (m_instanceSingleton == NULL)
		{
			//initialization
			m_instanceSingleton = new S_terrain;
			m_instanceSingleton->mode = _JUST_IGNORE_THIS;
			m_instanceSingleton->refresh();
		}
		return m_instanceSingleton;
	}
	//generation mode
	fitness_judge mode;
	//random number
	vector<ppoint> terrain;
	vector<ppoint> refresh();
private:
	S_terrain() {}
	S_terrain(S_terrain const&) {}
	S_terrain& operator=(S_terrain const&) {}
	static S_terrain *m_instanceSingleton;
};
void SetTerrainMode(fitness_judge);
vector<ppoint> FetchTerrain();
vector<ppoint> TerrainRefresh();
float get_terrain_height(const vector<ppoint> &terrain, float pos);
vector<b2Vec2> get_flag_position(const vector<ppoint> &terrain, float length, fitness_judge judge);

//input: the number of turns
inline float twist_score(int);
//input: the flying time (step / hz)
inline float fly_score(float);
//input: jump height
inline float jump_score(float);

//evolution world definition
class evo_world: public Test
{
public:
	//choose how to evaluate fitness
	//DISTANCE = 1, SPEED, FANCY, JUMP, CLIMB
	fitness_judge judge;
	//basic settings for world
	float ini_height{ 10.0f };
	float minangularv{ 6.0f };
	////running world
	//b2World* m_world;

	//basic settings for cars
	float32 m_hz{ 20.0f };
	float32 m_zeta{ 1.0f };
	float32 m_speed;
	//car properties
	b2Body* m_whatever;
	vector<b2Body*> m_body;
	vector<b2Body*> m_wheel_front;
	vector<b2Body*> m_wheel_back;
	vector<b2WheelJoint*> m_spring_front;
	vector<b2WheelJoint*> m_spring_back;
	vector<float> speed_torque_const;
	vector<float> max_speed;

	vector<float> m_xmax;
	vector<float> m_life;
	vector<float> m_result;
	vector<float> m_avg_vel;
	vector<bool> m_is_alive;
	vector<int> m_num_of_oil;
	vector<float> oil_position;
	vector<int> m_tot_step;

	//not valid = all cars are dead
	bool m_world_valid = true;
	bool is_valid() { return m_world_valid; }
	bool is_valid() const { return m_world_valid; }

	virtual void Step(Settings*);

	void judge_DISTANCE(int, float);
	void judge_SPEED(int, float, float speed = 1.0f);
	void judge_FANCY(int, float);
	void judge_JUMP(int, float);
	void judge_CLIMB(int, float);
	void judge_WELCOME_SCREEN(int, float);

	evo_world();
	evo_world(gene, fitness_judge);
	evo_world(vector<gene>, fitness_judge);

	void create_terrain();
	void add_car(gene);
	void add_car(vector<gene>);

	vector<float> get_fitness();
	void set_gravity(float);

private:
	vector<b2Vec2> pos, vel;
	vector<bool> m_fly;
	vector<float> m_angle;
	vector<int> m_takeoff_step;
	vector<float> m_takeoff_height;
	//temp max height
	vector<float> m_max_height;
	vector<bool> m_authentic_fly;
	vector<int> m_max_num_halfturns;
	//max height in all time
	vector<float> m_max_jumpheight;
	//maxstep / hz
	vector<float> m_max_flytime;
};
