#pragma once
#include "gene.h"
#include "get_fitness.h"

//tracking crucial properties in an evolution process
typedef struct track_info
{
	float max;
	float mean;
	float std;
};

ostream & operator << (ostream &ost, const track_info&);

class evolution
{
public:
	vector<gene> gene_pool;
	vector<float> fitnessl;
	//how much best ones to directly move to the next step
	int best_number = 0;

	int gene_count = 0;
	int step = 0;
	//if converged, 1, stop evaluation. normally should be 0
	bool converged = 0;

	//tracked properties;
	vector<track_info> fit_info;

	evolution(vector<gene>);

	//evaluate fitness for gene and modify them
	void eval_fitness();
	//selection in gene_pool according to fitness
	void select();
	//cross-over
	void crossover();
	//mutation
	void mutate();
	//all in all
	void next_gen();
	void echo(ostream & ost, const int &ind) const;
};