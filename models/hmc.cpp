#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cmath>
#include <utils.hpp>

using namespace std;

#define PI 3.14159265358979323846

//Reference -> https://mc-stan.org/docs/reference-manual/mcmc.html#hamiltonian-monte-carlo


//Simple std Normal 
float log_pmf_normal(float* x, float* stdevs, int size){
    float ans = 0;
    for(int i = 0; i < size; i ++){

        float a = x[i]; float sig = stdevs[i];
        float comp = -log(sig * sqrt(2 * PI)) - 0.5 * (a*a / (sig*sig));
        ans += comp;
    }

    return ans * -1;
}


//HMC impl
float* hmc(float* data, int data_size, int iters, float leapfrog_e, float leapfrog_steps){
    random_device rd;
    mt19937 rng(rd());

    //Sampling from a modified normal distribution (mu, theta)
    const int n_params = 2;
    float M[n_params * n_params] = {1, 0, 0, 1};

    //Params[-1] represents the most recent generated parameter set
    float *params = (float*)malloc((iters+1) * n_params * sizeof(float));
    params[0] = 0; params[1] = 1;



    //Helper lambda to calculate current Hamiltonian
    auto hamiltonian = [](float mu, float theta, int data_size, float* data, float* M, float* rho){
        float M_diag[n_params] = { M[0], M[3] };
        float T = log_pmf_normal(rho, M, n_params);

        float V = data_size * log(theta * sqrt(2 * PI));

        float sum = 0;
        for(int i = 0; i < data_size; i ++){
            float y = data[i];
            sum += (y - mu) * (y - mu);
        }

        V += (1 / ( 2 * theta  * theta )) * sum;

        float h = T + V;
        return h;
    };


    //Helper to calculate dV/dt
    //TODO: Maybe verify these formulas idk
    auto dvdt = [](float* data, int dsize, float mu, float theta, float* tocopy){

        float sum_nosq = 0;
        float sum_sq = 0;
        for(int i = 0; i < dsize; i ++){
            float sum = data[i] - mu;
            sum_nosq += sum; sum_sq += sum*sum;
        }

        float dmu = -1 * (theta * theta) * sum_nosq;
        float dtheta = (dsize / theta) + (-1 / (theta*theta*theta)) * sum_sq;
                
        float dv[2] = { dmu, dtheta };
        memcpy(tocopy, dv, sizeof(dv));
    };



    //For use in funcs
    float ones[n_params];
    for(int i = 0; i < n_params; i ++){ ones[i] = 1; }

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

        //Starting Hamilonian 
        float h_start = hamiltonian(current_params[0], current_params[1], data_size, data, M, rho);

        for(int j = 0; j < leapfrog_steps; j ++){

            //Half step
            float dv[n_params]; dvdt(data, data_size, params[0], params[1], dv);
            emul(dv, n_params, -1 * leapfrog_e / 2);
            float* rho_new = madd(rho, dv, 1, n_params);
            memcpy(rho, rho_new, n_params * sizeof(float)); free(rho_new);

            //cout << "Completed half step" << endl;

            //Full step (TODO: Simply this if possible)
            float* m_inv = twodinv(M);
            float* m_inv_p = mmul(m_inv, rho, n_params, n_params, n_params, 1);
            emul(m_inv_p, n_params, leapfrog_e); 
            float* params_new = madd(params, m_inv_p, 1, n_params);
            memcpy(params, params_new, n_params * sizeof(float));
            free(m_inv); free(m_inv_p); free(params_new);

            //cout << "Completed full step" << endl;

            //Another half step
            dvdt(data, data_size, params[0], params[1], dv);
            emul(dv, n_params, -1 * leapfrog_e / 2);
            rho_new = madd(rho, dv, 1, n_params);
            memcpy(rho, rho_new, n_params * sizeof(float)); free(rho_new);
           

            //cout << "Completed leapfrog" << endl;

        }

        //Assign new params and new rho w metropolis prob
        float h_end = hamiltonian(current_params[0], current_params[1], data_size, data, M, rho);
        float chance = exp(h_end - h_start);
        if(chance > 1){ chance = 1; }

        //cout << "Accepting w chance " << chance << endl;

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


    auto startt = std::chrono::high_resolution_clock::now();

    int n_new = 12;
    float* new_samples = hmc(data, DSIZE, n_new, 0.05, 25);
    float* new_samples_correct = new_samples + 2 * sizeof(float);
    matprint(new_samples_correct, n_new, 2);
    free(new_samples);

    auto endt = std::chrono::high_resolution_clock::now();


    chrono::duration<double, milli> duration_ms = endt - startt;
    cout << "Took " << duration_ms.count() << " ms for " << n_new << " new entries" << endl;



    return 0;

}