#ifndef GENETIC_GENE_H
#define GENETIC_GENE_H

#include "random_convenience.h"
#include "geometry.h"

class gene
{
public:
    gene() {}
    gene(vector<point>, vector<point>, float, float, float, float, float, float, float);
        //(body_paras,wheel_paras,size_body,size_front,size_back,rho_body,rho_front,rho_back,max_speed)

    vector<point> gene_body {};
        // body shape normalized to surface area 1
    vector<point> gene_wheel {};
        // wheel shape normalized to surface area 1
   	float size_body = 1, size_front = 1, size_back = 1;
        //  linear coefficient 
        //  * = size_* * gene_wheel
    float rho_body = 1.0f, rho_front = 1.0f, rho_back = 1.0f;
        // density
	float max_speed = 40.0f, max_torque = 150.0f;
        // max speed and torque specified in torque()

    void gene_normalize();
    void mutate(float,float);


	static constexpr int ngene_body = 8, ngene_wheel = 8;
		//size of gene
	static constexpr float area_tot = 10;
		//body size + wheel size
	static constexpr float mass_tot = 100;
		//minimum mass ratio for each part;
	static constexpr float min_mass_ratio = .1;
		//body mass + wheel mass
	static constexpr float speed_torque_const = 20000;
		//max_speed * max_torque;
	void from_crossover(const gene&, const gene&);

private:
	inline float torque(float speed);
};

ostream & operator << (ostream &, const gene &);

//generate random gene
bool valid_gene(gene);
gene rand_gene();
vector<gene> rand_gene(const unsigned int n);

istream & operator >> (istream &, gene&);
istream & operator >> (istream &, vector<gene>&);
#endif 

