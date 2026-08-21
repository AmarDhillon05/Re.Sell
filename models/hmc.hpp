#pragma once

#include <vector>
#include <string>

using namespace std;

//Hamiltonian Monte Carlo over the two parameters of a Normal likelihood.
//Reference -> https://mc-stan.org/docs/reference-manual/mcmc.html#hamiltonian-monte-carlo

//Number of sampled parameters: (mu, sigma)
constexpr int HMC_NPARAMS = 2;

//Negative log density of an independent Normal(0, sigma_i) vector.
//This is an *energy* (lower = more probable), which is what the Hamiltonian wants.
float neg_log_normal(const float* x, const float* stdevs, int size);

//Potential energy V(mu, sigma) = -log p(data | mu, sigma).
//Returns +infinity for an invalid (non positive) sigma so the sampler rejects it.
float normal_potential(const float* data, int data_size, float mu, float sigma);

//Gradient of the potential wrt (mu, sigma), written into grad[HMC_NPARAMS].
void normal_potential_grad(const float* data, int data_size, float mu, float sigma, float* grad);


//Raw sampler. Returns a malloc'd array of (iters + 1) * HMC_NPARAMS floats:
//row 0 is the starting point, rows 1..iters are the chain. Caller frees.
//Pass init_mu/init_sigma <= 0 to start from the data's own moments.
float* hmc(const float* data,
           int data_size,
           int iters,
           float leapfrog_e,
           int leapfrog_steps,
           float init_mu = 0.0f,
           float init_sigma = -1.0f,
           unsigned int seed = 0);


//Fitted summary of the posterior, which is what the rest of the app consumes.
struct HMCResult {
    bool ok = false;
    string error;

    //Post burn-in draws, flattened row major as {mu, sigma} pairs
    vector<float> samples;
    int n_samples = 0;

    float mu_mean = 0.0f;
    float mu_sd = 0.0f;
    float sigma_mean = 0.0f;
    float sigma_sd = 0.0f;

    float accept_rate = 0.0f;
    float step_size = 0.0f;
};

//Convenience wrapper: burn-in, light step size adaptation and posterior summaries.
HMCResult hmc_fit_normal(const vector<float>& data,
                         int iters = 2000,
                         int burn_in = 500,
                         float leapfrog_e = 0.0f,
                         int leapfrog_steps = 25,
                         unsigned int seed = 0);
