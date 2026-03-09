#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>

using namespace std;

//Reference -> https://mc-stan.org/docs/reference-manual/mcmc.html#hamiltonian-monte-carlo

typedef float (*Float2Float)(float*);

//HMC impl
float* hmc(float* data, int data_size, int iters, float leapfrog_e, float leapfrog_steps, 
            Float2Float prior_func_term
        ){
    random_device rd;
    mt19937 rng(rd());

    //Sampling from a modified normal distribution (mu, theta)
    float M[2][2] = {{1, 0}, {0, 1}};

    //Params[-1] represents the most recent generated parameter set
    float params[iters+1][2];
    float tmp[2] = {0.0f, 1.0f};
    memcpy(params[0], tmp, 2 * sizeof(float));

    int i = 0;
    while(i < iters){

        //Sampling momentum
        normal_distribution<float> mu_sampler(0, M[0][0]); 
        normal_distribution<float> theta_sampler(0, M[1][1]);
        float sample_mu = mu_sampler(rng);
        float sample_theta = theta_sampler(rng);
        float rho[2] = { sample_mu, sample_theta };

        //Leapfrog iterations
        float current_params[2] = { params[i+1][0], params[i+1][1] };

        for(int j = 0; j < leapfrog_steps; j ++){

            //CPU impl of calculating dV/dt
            float dv_dt = 0;
            for(int k = 0; k < data_size; k ++){
                dv_dt += pow((data[k] - current_params[0]), 2);
            }
            dv_dt *= current_params[1];
            dv_dt -= data_size * prior_func_term(current_params);

            //Actual leapfrog steps

        }

        i++;
        
    }

    return *params + (2 * sizeof(float));
}


//Simple std Normal prior func eval 
float prior_func_term_stdn(float* rho){
    const double PI = 3.14159265358979323846;

    float p_mu = (1/sqrt(2 * PI)) * exp(-0.5 * pow(rho[0], 2));
    float p_theta = (1/sqrt(2 * PI)) * exp(-0.5 * pow(rho[1], 2));
    
    return p_mu * p_theta;
}




int main(){

    const int DSIZE = 128;

    float data[DSIZE];
    float* new_samples = hmc(data, DSIZE, 5, 0.1, 5, *prior_func_term_stdn);

    return 0;

}