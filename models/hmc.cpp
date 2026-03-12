#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>
#include <utils.hpp>

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
    const int n_params = 2;
    float M[n_params * n_params] = {1, 0, 0, 1};

    //Params[-1] represents the most recent generated parameter set
    float *params = (float*)malloc((iters+1) * n_params * sizeof(float));
    params[0] = 0; params[1] = 1;

    int i = 0;
    while(i < iters){

        //Sampling momentum
        normal_distribution<float> mu_sampler(0, M[0]); 
        normal_distribution<float> theta_sampler(0, M[3]);
        float sample_mu = mu_sampler(rng);
        float sample_theta = theta_sampler(rng);
        float rho[n_params] = { sample_mu, sample_theta };

        //Leapfrog iterations
        float current_params[n_params] = { params[n_params * (i)], params[n_params * (i) + 1] };
        cout << "Current Params: " << current_params[0] << ", " << current_params[1] << endl;

        //Starting Hamilonian 
        float T = 0;
        float V = -log(prior_func_term(current_params));
        float h_start = T + V;

        for(int j = 0; j < leapfrog_steps; j ++){

            //CPU impl of calculating dV/dt
            float dv_dt = 0;
            for(int k = 0; k < data_size; k ++){
                dv_dt += pow((data[k] - current_params[0]), 2);
            }
            dv_dt *= current_params[1];
            V = prior_func_term(current_params);
            dv_dt -= data_size * V;

            //Actual leapfrog steps
            for(int k = 0; k < n_params; k ++){ rho[k] -= (leapfrog_e/2)*dv_dt; }

            float* m_trans = transpose(M, n_params, n_params);
            float* m_trans_rho = mmul(m_trans, rho, n_params, n_params, n_params, 1);
            for(int k = 0; k < n_params; k ++){ m_trans_rho[k] *= leapfrog_e; }
            float* pnew = madd(m_trans_rho, current_params, 1, n_params);
            memcpy(pnew, current_params, sizeof(float) * n_params);
            free(m_trans); free(m_trans_rho); free(pnew);

            for(int k = 0; k < n_params; k ++){ rho[k] -= (leapfrog_e/2)*dv_dt; }
         

        }

        //Assign new params and new rho w metropolis prob
        float h_end = T + V;
        float chance = exp(h_end - h_start);
        if(chance > 1){ chance = 1; }

        cout << "Accepting w chance " << chance << endl;

        uniform_real_distribution<float> metro_sampler(0, 1);
        if(metro_sampler(rng) < chance){
            for(int offset = 0; offset < n_params; offset ++){
                params[n_params * (i+1) + offset] = current_params[offset];
            }
        }

        i++;
        
    }

    return params; //+ (2 * sizeof(float)); //Free when done 
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

    random_device rd;
    mt19937 rng(rd());
    normal_distribution<float> nsampler(0, 0.025);

    float data[DSIZE];
    float start = 0.01;
    for(int i = 0; i < DSIZE; i++){
        data[i] = start;
        start += 0.1 + nsampler(rng);
    }

    cout << "Real mu + theta: " << mean(data, DSIZE) << ", " << stdev(data, DSIZE) << endl;

    auto startt = std::chrono::high_resolution_clock::now();

    int n_new = 5;
    float* new_samples = hmc(data, DSIZE, n_new, 0.05, 25, *prior_func_term_stdn);
    float* new_samples_correct = new_samples + 2 * sizeof(float);
    matprint(new_samples_correct, n_new, 2);
    free(new_samples);

    auto endt = std::chrono::high_resolution_clock::now();


    chrono::duration<double, milli> duration_ms = endt - startt;
    cout << "Took " << duration_ms.count() << " ms for " << n_new << " new entries" << endl;



    return 0;

}