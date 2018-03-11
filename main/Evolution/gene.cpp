#include "stdafx.h"
#include "gene.h"
#include<string>

gene::gene(vector<point> in_gene_body, vector<point> in_gene_wheel, float in_size_body, float in_size_front, float in_size_back, float in_rho_body, float in_rho_front,float in_rho_back, float in_max_speed) :
	gene_body(in_gene_body), 
	gene_wheel(in_gene_wheel),
	size_body(in_size_body),
	size_front(in_size_front),
	size_back(in_size_back), 
	rho_body(in_rho_body), 
	rho_front(in_rho_front),
	rho_back(in_rho_back), 
	max_speed(in_max_speed), 
	max_torque(torque(in_max_speed))
{
	gene_normalize();
}

float gene::torque(float speed)
{
	return 1000 * tanh(speed_torque_const / speed / 1000);
}

void gene::gene_normalize()
{
	//normalize all sizes
	normalize_area(gene_body);
	normalize_area(gene_wheel);

	//normalize area
	//all genes should be valid at all times!!!
	if (!valid_gene(*this))
	{
		float scale = (size_body*eu_dist(gene_body.at(3), gene_body.at(7))) / ((size_front + size_back)*l_max(gene_wheel)); 
		size_front *= scale, size_back *= scale;
	}

	float temp_tot = size_body*size_body + size_front*size_front + size_back*size_back;
	temp_tot = sqrt(temp_tot/area_tot);
	size_body /= temp_tot;
	size_front /= temp_tot;
	size_back /= temp_tot;

	//normalize mass
	float mm_body = size_body * size_body*rho_body, mm_front = size_front * size_front*rho_front, mm_back = size_back * size_back*rho_back;
	temp_tot = mm_body + mm_front + mm_back;
	mm_body /= temp_tot; mm_front /= temp_tot; mm_back /= temp_tot;
	vector<float*> mm_p{ &mm_body,&mm_front,&mm_back };
	sort(mm_p.begin(), mm_p.end(), [](float* a, float*b) {return *a < *b; });
	float *p1 = mm_p[0], *p2 = mm_p[1], *p3 = mm_p[2];

	if (*p1 < min_mass_ratio) *p1 = min_mass_ratio;
	if (*p2 < min_mass_ratio) *p2 = min_mass_ratio;
	*p3 = 1 - *p1 - *p2;

	rho_body = mass_tot * mm_body / size_body / size_body;
	rho_front = mass_tot * mm_front / size_front / size_front;
	rho_back = mass_tot * mm_back / size_back / size_back;
}

void gene::mutate(float chance, float size)
{
	//mutation!
	for (point& i : gene_body) mutate_point(i, chance, size/ngene_body);
	for (point& i : gene_wheel) mutate_point(i, chance, size/ngene_wheel);
	if (rand_chance(chance)) size_body *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) size_front *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) size_back *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) rho_body *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) rho_front *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) rho_back *= (1 + size*rand_clip_normal());
	if (rand_chance(chance)) max_speed *= (1 + size*rand_clip_normal());

	//normalize again!
	gene_normalize();
}

//just for simplicity
#define help_func_in_gene_crossover(cont) cont = (rand_zero_one()) ? gene1.cont : gene2.cont ;

void gene::from_crossover(const gene& gene1, const gene& gene2)
{
	help_func_in_gene_crossover(gene_body)
	help_func_in_gene_crossover(gene_wheel)
	help_func_in_gene_crossover(size_body)
	help_func_in_gene_crossover(size_front)
	help_func_in_gene_crossover(size_back)
	help_func_in_gene_crossover(rho_body)
	help_func_in_gene_crossover(rho_front)
	help_func_in_gene_crossover(rho_back)
	help_func_in_gene_crossover(max_speed)
	gene_normalize();
	return;
}

bool valid_gene(gene g)
{
	return (g.size_body*eu_dist(g.gene_body.at(3), g.gene_body.at(7))) > (g.size_front + g.size_back)*l_max(g.gene_wheel);
}

gene rand_gene()
{
	gene g{ rand_point(gene::ngene_body),rand_point(gene::ngene_wheel),1.5f + rand_zero2one(),.5f + .5f*rand_zero2one() ,.5f + .5f*rand_zero2one() ,.5f + rand_zero2one() ,.5f + rand_zero2one() ,.5f + rand_zero2one() ,.5f + 40 * rand_zero2one() };
	while (!valid_gene(g)) { g = rand_gene(); }
	return g;
}

vector<gene> rand_gene(const unsigned int n)
{
	vector<gene> temp(n, gene{});
	for (gene & i:temp) i=rand_gene();
	return temp;
}

ostream & operator << (ostream &ost, const gene & ge)
{
	ost << "body parameters" << endl;
	for (int i = 0; i < gene::ngene_body; i++)
		ost << ge.gene_body[i].angle << ' ' << ge.gene_body[i].length << endl;
	ost << "wheel parameters" << endl;
	for (int i = 0; i < gene::ngene_wheel; i++)
		ost << ge.gene_wheel[i].angle << ' ' << ge.gene_wheel[i].length << endl;
	ost << "body size = " << ge.size_body << endl;
	ost << "front wheel size = " << ge.size_front << endl;
	ost << "rear wheel size = " << ge.size_back << endl;
	ost << "body density = " << ge.rho_body << endl;
	ost << "front wheel density = " << ge.rho_front << endl;
	ost << "rear wheel density = " << ge.rho_back << endl;
	ost << "max speed = " << ge.max_speed << endl;
	ost << "max torque = " << ge.max_torque << endl;
	return ost;
}

istream& operator >> (istream &ist, gene &ge)
{
	std::string s;
	while (std::getline(ist, s) && s != "body parameters");
	float x, y;
	ge.gene_body.resize(gene::ngene_body);
	for (int i = 0; i < gene::ngene_body; i++)
	{
		ist >> x >> y;
		ge.gene_body.at(i) = point{ x,y };
	}
	while (std::getline(ist, s) && s != "wheel parameters");
	ge.gene_wheel.resize(gene::ngene_wheel);
	for (int i = 0; i < gene::ngene_wheel; i++)
	{
		ist >> x >> y;
		ge.gene_wheel.at(i) = point{ x,y };
	}
	ist >> s >> s >> s >> ge.size_body;
	ist >> s >> s >> s >> s >> ge.size_front;
	ist >> s >> s >> s >> s >> ge.size_back;
	ist >> s >> s >> s >> ge.rho_body;
	ist >> s >> s >> s >> s >> ge.rho_front;
	ist >> s >> s >> s >> s >> ge.rho_back;
	ist >> s >> s >> s >> ge.max_speed;
	ist >> s >> s >> s >> ge.max_torque;
	return ist;
}

istream& operator >> (istream &ist, vector<gene>&gp)
{
	if (!ist) return ist;
	gp.clear();
	gene ge;
	while (ist >> ge) gp.push_back(ge);
	return ist;
}