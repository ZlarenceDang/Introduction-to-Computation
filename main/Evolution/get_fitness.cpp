#include "stdafx.h"
#include "get_fitness.h"

float mmax(float a, float b)
{
	return (a > b) ? a : b;
}

float clip(float x, float min, float max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

//singleton
S_terrain* S_terrain::m_instanceSingleton = NULL;
vector<ppoint> S_terrain::refresh()
{
	switch (mode)
	{
	case DISTANCE: case FANCY:
		terrain = perlin_terrain(terrain_nstep, perlin_para{});
		return terrain;
		break;

	case SPEED:
		terrain = flat_terrain(terrain_nstep);
		return terrain;
		break;

	case CLIMB:
		terrain = exp_terrain(terrain_nstep);
		return terrain;
		break;

	case JUMP:
		terrain = saw_terrain(terrain_nstep);
		return terrain;
		break;

	case ROCK_AND_ROLL:
		terrain = perlin_terrain(terrain_nstep, perlin_para{ 1,400,40,1,1,1.5,5 });
		return terrain;
		break;

	case _JUST_IGNORE_THIS:
		terrain = vector<ppoint>{};
		return terrain;
		break;

	case WELCOME:
		terrain = welcome_terrain(terrain_nstep);
		return terrain;
		break;

	default:
		cout << "No such mode!!!\nI'm angry!!!\n";
		throw(runtime_error("No such mode!!!\nI'm angry!!!\n"));
	}
}

void SetTerrainMode(fitness_judge mode)
{
	S_terrain *s;
	s->getInstance()->mode = mode;
	s->getInstance()->refresh();
}

vector<ppoint> FetchTerrain()
{
	S_terrain *s;
	return s->getInstance()->terrain;
}

vector<ppoint> TerrainRefresh()
{
	S_terrain *s;
	s->getInstance()->refresh();
	return s->getInstance()->terrain;
}

float get_terrain_height(const vector<ppoint> &terrain, float pos)
{
	if (terrain.size() == 0) return 0.0f;
	ppoint prev = terrain.front();
	if (prev.x > pos) return 0.0f;
	vector<ppoint>::const_iterator mitr = lower_bound(terrain.begin(), terrain.end(), ppoint{ pos,0.0f });
	vector<ppoint>::const_iterator nitr = mitr;
	if (mitr->x == pos)
	{
		++nitr;
		if (nitr == terrain.end()) return mitr->y;
		else  if (nitr->x == pos) return mmax(mitr->y, nitr->y);
		else return mitr->y;
	}
	--nitr;
	return nitr->y + (mitr->y - nitr->y) *(pos - nitr->x) / (mitr->x - nitr->x);
}

vector<b2Vec2> get_flag_position(const vector<ppoint> &terrain, float length, fitness_judge judge)
{
	vector<pair<float, float>> pos_dist;
	float final_dist = 500;

	switch (judge)
	{
	case SPEED: case DISTANCE:
		pos_dist.push_back(make_pair(200.0f, 20.0f));
		pos_dist.push_back(make_pair(500.0f, 50.0f));
		pos_dist.push_back(make_pair(2000.0f, 100.0f));
		pos_dist.push_back(make_pair(10000.0f, 200.0f));
		break;
	case FANCY: case JUMP: case CLIMB: case WELCOME: case ROCK_AND_ROLL:	
		pos_dist.push_back(make_pair(10000.0f, 50.0f));
		break;
	}

	float final_pos = terrain.back().x;

	vector<b2Vec2> flags{};
	float prev = 0.0f;
	bool end = false;
	for (int j = 0; j < pos_dist.size(); j++)
	{
		for (float i = prev; i < pos_dist.at(j).first; i += pos_dist.at(j).second)
		{
			if (i > length) { end = true; break; }
			flags.push_back(b2Vec2{ i,get_terrain_height(terrain,i) });
		}
		if (end) break;
		prev = pos_dist.at(j).first;
	}

	for (float i = pos_dist.back().first; i < length; i += final_dist) flags.push_back(b2Vec2{ i,get_terrain_height(terrain,i) });

	return flags;
}

inline float twist_score(int n)
{
	return 150.0f*floor(pow(n, 1.5));
}

inline float fly_score(float t)
{
	return 10.0f*pow(fabs(t), 2);
}

inline float jump_score(float h)
{
	return 20.0f*pow(mmax(0.0f, h) / 8.0f, 2.5);
}

const char * get_name(fitness_judge judge)
{
	static char DIS[] = "Distance Competition";
	static char SPD[] = "Speeding";
	static char FAN[] = "Fancy Racing";
	static char JMP[] = "Jumping Competition";
	static char CLM[] = "Hill Climbing";
	static char RAR[] = "Rock & Roll";
	static char NUL[] = "NULL";
	switch (judge)
	{
		case DISTANCE: return DIS;
		case SPEED: return SPD;
		case FANCY: return FAN;
		case JUMP: return JMP;
		case CLIMB: return CLM;
		case ROCK_AND_ROLL: return RAR;
		default: return NUL;
	}
}
bool getModeName(void*, int ind, const char** out)
{
	if (ind < 0 || ind > 5) return 0;
	*out = get_name((fitness_judge)ind);
	return 1;
}

//only in visualized version!!!
void evo_world::Step(Settings* settings)
{
	g_debugDraw.Flush();

	int ll = m_body.size();
	float xmax = -10.0f, y1 = 0.0f;
	if (settings->detailinfo)
	{
		g_debugDraw.DrawString(5, m_textLine, "frequency = %g hz, damping ratio = %g, judge mode = %s", m_hz, m_zeta, get_name(judge));
		m_textLine += DRAW_STRING_NEW_LINE;
	}
	for (int i = 0; i < ll; i++) if (m_body.at(i)->GetPosition().x > xmax)
	{
		xmax = m_body.at(i)->GetPosition().x;
		y1 = m_body.at(i)->GetPosition().y;
	}

	if (settings->fix_camera)
	{
		g_camera.m_center.x = xmax;
		g_camera.m_center.y = y1;
	}

	//dynamic printing and evaluation
	{
		int ll = m_body.size();

		//init variables
		pos = vector<b2Vec2>(ll, { 0,0 });
		vel = vector<b2Vec2>(ll, { 0,0 });

		//check if all dead
		bool one_alive = true;

		//evaluation
		for (int i = 0; i < ll; i++) if (m_is_alive.at(i))
		{
			//update step
			++m_tot_step.at(i);
			//reset motor torque
			//front
			m_spring_front.at(i)->SetMaxMotorTorque(
				speed_torque_const.at(i) / sqrt(max_speed.at(i)) /
				mmax(minangularv, std::fabs(m_spring_front.at(i)->GetJointLinearSpeed()))
			);
			//back
			m_spring_back.at(i)->SetMaxMotorTorque(
				speed_torque_const.at(i) / sqrt(max_speed.at(i)) /
				mmax(minangularv, std::fabs(m_spring_back.at(i)->GetJointLinearSpeed()))
			);

			//get car position & velocity
			pos.at(i) = m_body.at(i)->GetPosition();
			vel.at(i) = m_body.at(i)->GetLinearVelocity();
			//update position info
			m_xmax.at(i) = (pos.at(i).x < m_xmax.at(i)) ? m_xmax.at(i) : pos.at(i).x;
			//m_avg_vel.at(i) = mmax(0.0f, m_xmax.at(i)) * 60 / m_tot_step.at(i);

			//evaluate fitness
			switch (judge) {
			case ROCK_AND_ROLL:
				judge_SPEED(i, settings->hz, 0.5f);
				break;
			case DISTANCE:
				judge_DISTANCE(i, settings->hz);
				break;
			case SPEED:
				judge_SPEED(i, settings->hz);
				break;
			case FANCY:
				judge_FANCY(i, settings->hz);
				break;
			case JUMP:
				judge_JUMP(i, settings->hz);
				break;
			case CLIMB:
				judge_CLIMB(i, settings->hz);
				break;
			case WELCOME:
				judge_WELCOME_SCREEN(i, settings->hz);
				break;
			default:
				break;
			}

			//print
			if (settings->detailinfo)
			{
				g_debugDraw.DrawString(5, m_textLine,
					"car = %d, xmax = %g, life = %g, fitness = %g, fly(0=false) = %d",
					i, m_xmax.at(i), m_life.at(i), m_result.at(i), int(m_fly.at(i)));
				m_textLine += DRAW_STRING_NEW_LINE;
			}

			//check if dead
			if (m_is_alive.at(i) && m_life.at(i) <= 0.0f)
			{
				//m_avg_vel.at(i) = mmax(0.0f, m_xmax.at(i)) * 60 / m_tot_step.at(i);
				m_is_alive.at(i) = false;
			}
		}
		else {
			m_body.at(i)->SetActive(false);
			if (settings->detailinfo)
			{
				g_debugDraw.DrawString(5, m_textLine,
					"car = %d, xmax = %g, life = %s, fitness = %g",
					i, m_xmax.at(i), "RIP", m_result.at(i));
				m_textLine += DRAW_STRING_NEW_LINE;
			}
		}

		//check if all dead
		one_alive = false;
		for (bool i : m_is_alive) one_alive = (one_alive || i);

		//return result, this time average velocity and traveled distance both matters.
		//for (int i = 0; i < ll; i++) m_result.at(i) = mmax(0.00001f, m_xmax.at(i));

		//goodbye world 
		m_world_valid = one_alive;
	}
	Test::Step(settings);
}

void evo_world::judge_DISTANCE(int i, float hz)
{
	const float maxspeed = 7.5f;
	const float criticalspeed = 1.0f;
	m_result.at(i) = mmax(m_result.at(i), pos.at(i).x); 
	float killer = maxspeed*((2000 * (pow(e, -0.0005*m_tot_step.at(i)*60.0f / hz) - 1) + m_tot_step.at(i)*60.0f / hz) / 60.0f) - 120;
	if (killer > pos.at(i).x) { m_life.at(i) = -100.0f; }
	if (m_tot_step.at(i) > 400) {
		if (pos.at(i).x < m_xmax.at(i)) {
			m_life.at(i) -= ((vel.at(i).x) > criticalspeed) ? 0.3f : 1.5f;
		}
		else {
			m_life.at(i) += (vel.at(i).x < criticalspeed) ? 1.5f : 4.5f;
		}
	}
	m_life.at(i) = (m_life.at(i) > 300.0f) ? 300.0f : m_life.at(i);

	//show killer
	/*
	{
		b2Vec2 v1(killer, 1000.0f);
		b2Vec2 v2(killer, -1000.0f);
		b2EdgeShape edge;
		edge.Set(v1, v2);

		b2FixtureDef fd;
		fd.shape = &edge;
		fd.filter.groupIndex = -1;

		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0.0f, 0.0f);
		bd.angle = 0;
		if (m_whatever) m_world->DestroyBody(m_whatever);
		m_whatever = m_world->CreateBody(&bd);
		m_whatever->CreateFixture(&fd);
	}*/
	return;
}

void evo_world::judge_SPEED(int i, float hz, float speed)
{
	if (m_tot_step.at(i) > 400)
	{
		if (pos.at(i).x >= mmax(m_xmax.at(i), 10.0f)) m_life.at(i) += 0.4*clip(vel.at(i).x / speed - (1 + exp(double(m_tot_step.at(i) - 200) / 1500.0)), -1.0, 1.0) * 60 / hz;
		else m_life.at(i) -= 0.4 * 60 / hz;
	}
	m_life.at(i) = (m_life.at(i) > 300.0f) ? 300.0f : m_life.at(i);
	m_result.at(i) = mmax(0.00001f, m_xmax.at(i));
	return;
}

void evo_world::judge_FANCY(int i, float hz) 
{
	//judge whether the car is off the land
	bool contact{ false };
	{
		b2ContactEdge* ce = (m_body.at(i))->GetContactList();
		if (ce)	contact = true;
	}
	{
		b2ContactEdge* ce = (m_wheel_back.at(i))->GetContactList();
		if (ce)	contact = true;
	}
	{
		b2ContactEdge* ce = (m_wheel_front.at(i))->GetContactList();
		if (ce)	contact = true;
	}

	//update life & fitness
	//for safety
	if (!m_fly.at(i) && contact) {
		m_fly.at(i) = false;
		m_takeoff_step.at(i) = m_tot_step.at(i);
		m_angle.at(i) = m_body.at(i)->GetAngle();
		m_takeoff_height.at(i) = m_body.at(i)->GetPosition().y;
	}
	//is taking off
	else if (!m_fly.at(i) && !contact) {
		m_fly.at(i) = true;
		m_takeoff_step.at(i) = m_tot_step.at(i);
		m_angle.at(i) = m_body.at(i)->GetAngle();
		m_takeoff_height.at(i) = m_body.at(i)->GetPosition().y;
		m_max_height.at(i) = m_takeoff_height.at(i);
	}
	//is on the fly
	else if (m_fly.at(i) && !contact) {
		m_max_height.at(i) = (m_body.at(i)->GetPosition().y > m_max_height.at(i)) ? m_body.at(i)->GetPosition().y : m_max_height.at(i);
	}
	//first landing
	else if (m_fly.at(i) && contact && !m_authentic_fly.at(i)) {
		m_fly.at(i) = false;
		m_authentic_fly.at(i) = true;
	}
	//true landing
	else if (m_fly.at(i) && contact && m_authentic_fly.at(i) && (m_tot_step.at(i) - m_takeoff_step.at(i))>60) {
		m_fly.at(i) = false;
		int num_halfturns = floor(fabs((m_body.at(i)->GetAngle() - m_angle.at(i)) / (pi)));
		float flytime = fabs((m_tot_step.at(i) - m_takeoff_step.at(i))) / hz;
		float jumpheight = mmax(0.0f, m_max_height.at(i) - m_takeoff_height.at(i));
		m_life.at(i) += twist_score(num_halfturns)/5;
		m_life.at(i) += fly_score(flytime)/5;
		m_life.at(i) += jump_score(jumpheight)/5;

		if (num_halfturns > m_max_num_halfturns.at(i)) {
			m_result.at(i) -= twist_score(m_max_num_halfturns.at(i));
			m_max_num_halfturns.at(i) = num_halfturns;
			m_result.at(i) += twist_score(m_max_num_halfturns.at(i));
		}
		if (flytime > m_max_flytime.at(i)) {
			m_result.at(i) -= fly_score(m_max_flytime.at(i));
			m_max_flytime.at(i) = flytime;
			m_result.at(i) += fly_score(m_max_flytime.at(i));
		}
		if (jumpheight > m_max_jumpheight.at(i)) {
			m_result.at(i) -= jump_score(m_max_jumpheight.at(i));
			m_max_jumpheight.at(i) = jumpheight;
			m_result.at(i) += jump_score(m_max_jumpheight.at(i));
		}
	}

	m_life.at(i) += -0.3f*(1.0f - 1.0f*pow(e, -0.002*m_tot_step.at(i)*60.0f / hz));

	//max time = 120s
	m_life.at(i) = (m_tot_step.at(i) / hz > 120) ? -100.0f : m_life.at(i);
	m_life.at(i) = (m_life.at(i) > 300.0f) ? 300.0f : m_life.at(i);

	return;
}
void evo_world::judge_JUMP(int i, float hz) {
	//judge whether the car is off the land
	bool contact{ false };
	{
		b2ContactEdge* ce = (m_body.at(i))->GetContactList();
		if (ce)	contact = true;
	}
	{
		b2ContactEdge* ce = (m_wheel_back.at(i))->GetContactList();
		if (ce)	contact = true;
	}
	{
		b2ContactEdge* ce = (m_wheel_front.at(i))->GetContactList();
		if (ce)	contact = true;
	}
	//update life & fitness
	//for safety
	if (!m_fly.at(i) && contact) {
		m_fly.at(i) = false;
		m_takeoff_step.at(i) = m_tot_step.at(i);
		m_takeoff_height.at(i) = m_body.at(i)->GetPosition().y;
	}
	//is taking off
	else if (!m_fly.at(i) && !contact) {
		m_fly.at(i) = true;
		m_takeoff_step.at(i) = m_tot_step.at(i);
		m_takeoff_height.at(i) = m_body.at(i)->GetPosition().y;
		m_max_height.at(i) = m_takeoff_height.at(i);
	}
	//is on the fly
	else if (m_fly.at(i) && !contact) {
		m_max_height.at(i) = (m_body.at(i)->GetPosition().y > m_max_height.at(i)) ? m_body.at(i)->GetPosition().y : m_max_height.at(i);
	}
	//first landing
	else if (m_fly.at(i) && contact && !m_authentic_fly.at(i)) {
		m_fly.at(i) = false;
		m_authentic_fly.at(i) = true;
	}
	//true landing
	else if (m_fly.at(i) && contact && m_authentic_fly.at(i) && (m_tot_step.at(i) - m_takeoff_step.at(i))>60) {
		m_fly.at(i) = false;
		float jumpheight = mmax(m_max_height.at(i) - pos.at(i).y, m_max_height.at(i) - m_takeoff_height.at(i));
		m_life.at(i) += jump_score(jumpheight)/2;

		if (jumpheight > m_max_jumpheight.at(i)) {
			m_result.at(i) -= jump_score(m_max_jumpheight.at(i));
			m_max_jumpheight.at(i) = jumpheight;
			m_result.at(i) += jump_score(m_max_jumpheight.at(i));
		}
	}

	float criticalspeed = 7.5f;
	if (m_tot_step.at(i) > 400) {
		if (pos.at(i).x < m_xmax.at(i)) {
			m_life.at(i) -= ((vel.at(i).x) > criticalspeed) ? 0.3f : 1.0f;
		}
		else {
			m_life.at(i) += (vel.at(i).x < criticalspeed) ? -0.2f : 1.0f;
		}
	}
	m_life.at(i) += -0.03f*(1.0f - 1.0f*pow(e, -0.002*m_tot_step.at(i)*60.0f / hz));

	//max time = 120s
	m_life.at(i) = (m_tot_step.at(i) / hz > 120) ? -100.0f : m_life.at(i);
	m_life.at(i) = (m_life.at(i) > 300.0f) ? 300.0f : m_life.at(i);

	return;
}

void evo_world::judge_CLIMB(int i, float hz) 
{
	const float maxspeed = 0.2f;
	const float criticalspeed = 1.0f;
	if (m_xmax.at(i) < 50.0f) { m_result.at(i) = 0; }
	else{	m_result.at(i) = mmax(m_result.at(i), pow(pos.at(i).y,2)/50);	}
	float killer = maxspeed*((500 * (pow(e, -0.002*m_tot_step.at(i)*60.0f / hz) - 1) + m_tot_step.at(i)*60.0f / hz) / 60.0f) - 50;
	if (killer > pos.at(i).y) { m_life.at(i) = -100.0f; }
	if (m_tot_step.at(i) > 400) {
		if (pos.at(i).x < m_xmax.at(i)) {
			m_life.at(i) -= ((vel.at(i).x + vel.at(i).y) > criticalspeed) ? 0.1f : 0.3f;
		}
		else {
			m_life.at(i) += (vel.at(i).Length() < criticalspeed) ? 2.0f : 5.0f;
		}
	}
	m_life.at(i) = (m_life.at(i) > 300.0f) ? 300.0f : m_life.at(i);

	/*
	//show killer( HAS A SERIOUS BUD)
	{
		b2Vec2 v1(-2000.0f,killer);
		b2Vec2 v2(5000.0f,killer);
		b2EdgeShape edge;
		edge.Set(v1, v2);

		b2FixtureDef fd;
		fd.shape = &edge;
		fd.filter.groupIndex = -1;

		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0.0f, 0.0f);
		bd.angle = 0;
		if (m_whatever) m_world->DestroyBody(m_whatever);
		m_whatever = m_world->CreateBody(&bd);
		m_whatever->CreateFixture(&fd);
	}
	*/

	return;
}

void evo_world::judge_WELCOME_SCREEN(int i, float)
{
	if (m_tot_step.at(i) > 400 && m_life.at(i) > -100.0f)
	{
		float xMax = 0;
		for (int j = 0; j < m_xmax.size(); j++)
			if (m_xmax[j] > xMax) xMax = m_xmax[j];
		if (pos.at(i).x > xMax - 20 && (pos.at(i).x < m_xmax.at(i) - 0.01 || vel.at(i).x == 0)) m_life.at(i) -= 0.2f;
		else m_life.at(i) = 100.0f;
		if (pos.at(i).x > 6000.0f || m_life.at(i) < 0.0f)
			for (float & l : m_life) l = -100.0f;
	}
}

//get fitness directly
vector<float> evo_world::get_fitness()
{
	//init settings
	Settings settings;

	//evaluation
	while (m_world_valid) Step(&settings);

	return m_result;
}

void evo_world::set_gravity(float g)
{
	m_world->SetGravity(b2Vec2(0, -g));
}

//add cars to world
void evo_world::add_car(gene g)
{
	//init
	const int body_n = g.ngene_body;
	const int wheel_n = g.ngene_wheel;

	b2PolygonShape chassis;

	b2Vec2 vertices[3];
	for (int i = 0; i < 3; i++) { vertices[i].Set(0, 0); }

	//body def
	b2BodyDef bd;
	bd.type = b2_dynamicBody;
	bd.position.Set(0.0f, ini_height);
	m_body.push_back(m_world->CreateBody(&bd));

	//init fixture
	b2FixtureDef fd;
	fd.density = g.rho_body;
	fd.friction = 0.4f;
	//filter group -1, no inter-car collision
	fd.filter.groupIndex = -1;

	//add fixtures for body
	for (int i = 0; i < body_n; i++)
	{
		vertices[0].Set(0.0f, 0.0f);
		vertices[1].Set(
			g.gene_body.at(i).length * cos(g.gene_body.at(i).angle)*g.size_body + 0.05,
			g.gene_body.at(i).length * sin(g.gene_body.at(i).angle)*g.size_body + 0.05);
		if (i < body_n - 1) vertices[2].Set(
			g.gene_body.at(i + 1).length * cos(g.gene_body.at(i + 1).angle)*g.size_body + 0.05,
			g.gene_body.at(i + 1).length * sin(g.gene_body.at(i + 1).angle)*g.size_body + 0.05);
		else if (i == body_n - 1) vertices[2].Set(
			g.gene_body.at(0).length * cos(g.gene_body.at(0).angle)*g.size_body + 0.05,
			g.gene_body.at(0).length * sin(g.gene_body.at(0).angle)*g.size_body + 0.05);

		chassis.Set(vertices, 3);
		fd.shape = &chassis;
		m_body.back()->CreateFixture(&fd);
	}

	const int fr = 3;
	const int se = 7;

	//build front wheel(m_wheel_front)
	fd.density = g.rho_front;
	fd.friction = 1.2f;

	bd.position.Set(
		g.gene_body.at(fr).length * cos(g.gene_body.at(fr).angle)*g.size_body + 0.05,
		g.gene_body.at(fr).length * sin(g.gene_body.at(fr).angle)*g.size_body + 0.05 + ini_height);
	m_wheel_front.push_back(m_world->CreateBody(&bd));

	//add fixtures for front wheel
	for (int i = 0; i < wheel_n; i++)
	{
		vertices[0].Set(0.0f, 0.0f);
		vertices[1].Set(
			g.gene_wheel.at(i).length * cos(g.gene_wheel.at(i).angle)*g.size_front + 0.05,
			g.gene_wheel.at(i).length * sin(g.gene_wheel.at(i).angle)*g.size_front + 0.05);
		if (i < wheel_n - 1) vertices[2].Set(
			g.gene_wheel.at(i + 1).length * cos(g.gene_wheel.at(i + 1).angle)*g.size_front + 0.05,
			g.gene_wheel.at(i + 1).length * sin(g.gene_wheel.at(i + 1).angle)*g.size_front + 0.05);
		else if (i == wheel_n - 1) vertices[2].Set(
			g.gene_wheel.at(0).length * cos(g.gene_wheel.at(0).angle)*g.size_front + 0.05,
			g.gene_wheel.at(0).length * sin(g.gene_wheel.at(0).angle)*g.size_front + 0.05);

		chassis.Set(vertices, 3);
		fd.shape = &chassis;
		m_wheel_front.back()->CreateFixture(&fd);
	}


	//build back wheel(m_wheel_back)
	fd.density = g.rho_back;
	fd.friction = 1.2f;

	bd.position.Set(
		g.gene_body.at(se).length * cos(g.gene_body.at(se).angle)*g.size_body + 0.05,
		g.gene_body.at(se).length * sin(g.gene_body.at(se).angle)*g.size_body + 0.05 + ini_height);
	m_wheel_back.push_back(m_world->CreateBody(&bd));

	//add fixtures for back wheel
	for (int i = 0; i < wheel_n; i++)
	{
		vertices[0].Set(0.0f, 0.0f);
		vertices[1].Set(
			g.gene_wheel.at(i).length * cos(g.gene_wheel.at(i).angle)*g.size_back + 0.05,
			g.gene_wheel.at(i).length * sin(g.gene_wheel.at(i).angle)*g.size_back + 0.05);
		if (i < wheel_n - 1) vertices[2].Set(
			g.gene_wheel.at(i + 1).length * cos(g.gene_wheel.at(i + 1).angle)*g.size_back + 0.05,
			g.gene_wheel.at(i + 1).length * sin(g.gene_wheel.at(i + 1).angle)*g.size_back + 0.05);
		else if (i == wheel_n - 1) vertices[2].Set(
			g.gene_wheel.at(0).length * cos(g.gene_wheel.at(0).angle)*g.size_back + 0.05,
			g.gene_wheel.at(0).length * sin(g.gene_wheel.at(0).angle)*g.size_back + 0.05);

		chassis.Set(vertices, 3);
		fd.shape = &chassis;
		m_wheel_back.back()->CreateFixture(&fd);
	}


	//add joints
	b2WheelJointDef jd;
	b2Vec2 axis(0.0f, 1.0f);

	//front wheel
	jd.Initialize(m_body.back(), m_wheel_front.back(), m_wheel_front.back()->GetPosition(), axis);
	jd.motorSpeed = -g.max_speed;
	jd.maxMotorTorque = g.max_torque;
	jd.enableMotor = true;
	jd.frequencyHz = m_hz;
	jd.dampingRatio = m_zeta;
	m_spring_front.push_back((b2WheelJoint*)m_world->CreateJoint(&jd));

	//back wheel
	jd.Initialize(m_body.back(), m_wheel_back.back(), m_wheel_back.back()->GetPosition(), axis);
	jd.motorSpeed = -g.max_speed;
	jd.maxMotorTorque = g.max_torque;
	jd.enableMotor = true;
	jd.frequencyHz = m_hz;
	jd.dampingRatio = m_zeta;
	m_spring_back.push_back((b2WheelJoint*)m_world->CreateJoint(&jd));

	//speed torque const
	speed_torque_const.push_back(g.speed_torque_const);
	//max_speed
	max_speed.push_back(g.max_speed);

	m_xmax.push_back(-100.0f);
	m_life.push_back(100.0f);
	m_result.push_back(0.00001f);
	m_avg_vel.push_back(0.00001f);
	m_is_alive.push_back(true);
	m_num_of_oil.push_back(0);
	m_tot_step.push_back(1);
	pos.push_back(b2Vec2{ 0,0 });
	vel.push_back(b2Vec2{ 0,0 });
	m_fly.push_back(false);
	m_authentic_fly.push_back(false);
	m_angle.push_back(0);
	m_takeoff_step.push_back(1);
	m_takeoff_height.push_back(0);
	m_max_height.push_back(0);
	m_max_num_halfturns.push_back(0);
	m_max_jumpheight.push_back(0);
	m_max_flytime.push_back(0);

	//cout << "Car Created!\n";
}
void evo_world::add_car(vector<gene> gl)
{
	for (gene &g : gl) add_car(g);
}

//create terrain
void evo_world::create_terrain()
{
	vector<ppoint> t_pos = FetchTerrain();
	b2PolygonShape shape;
	b2FixtureDef fd;
	shape.SetAsBox(0.5f, 0.05f);
	fd.shape = &shape;
	fd.friction = 1.0f;

	for (int32 i = 1; i < t_pos.size(); i++)
	{
		b2Vec2 v1(t_pos[i - 1].x, t_pos[i - 1].y);
		b2Vec2 v2(t_pos[i].x, t_pos[i].y);
		b2EdgeShape edge;
		edge.Set(v1, v2);

		b2FixtureDef fd;
		fd.shape = &edge;
		fd.friction = 1.0f;
		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0, 0);
		bd.angle = 0;
		b2Body* block = m_world->CreateBody(&bd);
		block->CreateFixture(&fd);
	}
	
	//edge
	{
		b2Vec2 v1(-2000.0f, 0.0f);
		b2Vec2 v2(2.0f, 0.0f);
		b2EdgeShape edge;
		edge.Set(v1, v2);

		b2FixtureDef fd;
		fd.shape = &edge;
		fd.friction = 1.0f;
		b2BodyDef bd;
		bd.type = b2_staticBody;
		bd.position.Set(0.0f, 0.0f);
		bd.angle = 0;
		b2Body* block = m_world->CreateBody(&bd);
		block->CreateFixture(&fd);

		v1.Set(float(terrain_nstep) / 1.1, -10000.0f);
		v2.Set(float(terrain_nstep) / 1.1, 10000.0f);

		edge.Set(v1, v2);
		fd.shape = &edge;
		block->CreateFixture(&fd);

		v1.Set(float(terrain_xmax), -10000.0f);
		v2.Set(float(terrain_xmax), 10000.0f);

		edge.Set(v1, v2);
		fd.shape = &edge;
		block->CreateFixture(&fd);


		v1.Set(float(-1000.0f), -10000.0f);
		v2.Set(float(-1000.0f), 10000.0f);

		edge.Set(v1, v2);
		fd.shape = &edge;
		block->CreateFixture(&fd);
	}

	return;
}

//create world!!!!
evo_world::evo_world() :judge{ DISTANCE }
{
	create_terrain();
}
evo_world::evo_world(gene g, fitness_judge j) : judge{ j }
{
	create_terrain();
	add_car(g);
}
evo_world::evo_world(vector<gene> gl, fitness_judge j) : judge{ j }
{
	create_terrain();
	add_car(gl);
}