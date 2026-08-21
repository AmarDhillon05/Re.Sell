#include <iostream>
#include <chrono>
#include <random>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <limits>

#include "hmc.hpp"
#include "utils.hpp"

using namespace std;

#define PI 3.14159265358979323846

//Reference -> https://mc-stan.org/docs/reference-manual/mcmc.html#hamiltonian-monte-carlo


//Negative log density of an independent Normal(0, stdevs[i]) vector.
//Used as the kinetic energy of the momentum, so the sign is already flipped.
float neg_log_normal(const float* x, const float* stdevs, int size){
    float ans = 0;
    for(int i = 0; i < size; i ++){

        float a = x[i]; float sig = stdevs[i];
        if(sig <= 0.0f){ return numeric_limits<float>::infinity(); }

        float comp = -log(sig * sqrt(2 * PI)) - 0.5 * (a*a / (sig*sig));
        ans += comp;
    }

    return ans * -1;
}


//V(mu, sigma) = n*log(sigma*sqrt(2pi)) + sum((y - mu)^2) / (2 sigma^2)
float normal_potential(const float* data, int data_size, float mu, float sigma){

    //A non positive sigma is outside the support: infinite energy => always rejected
    if(!(sigma > 0.0f) || !isfinite(mu)){ return numeric_limits<float>::infinity(); }

    float V = data_size * log(sigma * sqrt(2 * PI));

    float sum = 0;
    for(int i = 0; i < data_size; i ++){
        float y = data[i];
        sum += (y - mu) * (y - mu);
    }

    V += (1 / ( 2 * sigma * sigma )) * sum;

    return isfinite(V) ? V : numeric_limits<float>::infinity();
}


//dV/dmu    = -sum(y - mu) / sigma^2
//dV/dsigma =  n / sigma - sum((y - mu)^2) / sigma^3
void normal_potential_grad(const float* data, int data_size, float mu, float sigma, float* grad){

    if(!(sigma > 0.0f)){ grad[0] = 0.0f; grad[1] = 0.0f; return; }

    float sum_nosq = 0;
    float sum_sq = 0;
    for(int i = 0; i < data_size; i ++){
        float diff = data[i] - mu;
        sum_nosq += diff; sum_sq += diff*diff;
    }

    //Divided by sigma^2, not multiplied by it
    grad[0] = -1 * sum_nosq / (sigma * sigma);
    grad[1] = ((float)data_size / sigma) - (sum_sq / (sigma * sigma * sigma));
}


//HMC impl
float* hmc(const float* data,
           int data_size,
           int iters,
           float leapfrog_e,
           int leapfrog_steps,
           float init_mu,
           float init_sigma,
           unsigned int seed){

    if(iters < 0){ iters = 0; }
    if(leapfrog_steps < 1){ leapfrog_steps = 1; }

    mt19937 rng;
    if(seed == 0){
        random_device rd;
        rng.seed(rd());
    } else {
        rng.seed(seed);
    }

    //Sampling from a modified normal distribution (mu, sigma)
    const int n_params = HMC_NPARAMS;

    //Mass matrix. Entries are variances, so momenta are drawn with stdev sqrt(M_ii).
    float M[n_params * n_params] = {1, 0, 0, 1};
    float M_sd[n_params] = { sqrt(M[0]), sqrt(M[3]) };

    float *params = (float*)malloc((size_t)(iters + 1) * n_params * sizeof(float));

    //Start from the data's own moments unless the caller pinned a starting point,
    //otherwise a chain on (say) sneaker prices has to walk in from mu = 0.
    if(!(init_sigma > 0.0f)){
        float mu0 = mean(const_cast<float*>(data), data_size);
        float sd0 = stdev(const_cast<float*>(data), data_size);
        if(!(sd0 > 0.0f)){ sd0 = 1.0f; }
        params[0] = mu0; params[1] = sd0;
    } else {
        params[0] = init_mu; params[1] = init_sigma;
    }

    uniform_real_distribution<float> metro_sampler(0, 1);
    normal_distribution<float> momentum_sampler(0, 1);

    for(int i = 0; i < iters; i ++){

        //Current position q, and a working copy the leapfrog is allowed to move
        float q[n_params] = { params[n_params * i], params[n_params * i + 1] };

        //Sampling momentum ~ N(0, M)
        float rho[n_params];
        for(int d = 0; d < n_params; d ++){ rho[d] = momentum_sampler(rng) * M_sd[d]; }

        //Starting Hamiltonian, measured at the *current* position
        float h_start = normal_potential(data, data_size, q[0], q[1])
                      + neg_log_normal(rho, M_sd, n_params);

        //Leapfrog iterations
        float grad[n_params];
        bool diverged = false;

        for(int j = 0; j < leapfrog_steps; j ++){

            //Half step on the momentum
            normal_potential_grad(data, data_size, q[0], q[1], grad);
            for(int d = 0; d < n_params; d ++){ rho[d] -= 0.5f * leapfrog_e * grad[d]; }

            //Full step on the position: q += e * M^-1 rho
            float* m_inv = twodinv(M);
            float* m_inv_p = mmul(m_inv, rho, n_params, n_params, n_params, 1);
            emul(m_inv_p, n_params, leapfrog_e);
            for(int d = 0; d < n_params; d ++){ q[d] += m_inv_p[d]; }
            free(m_inv); free(m_inv_p);

            //Leaving the support (sigma <= 0) or blowing up ends the trajectory early;
            //the proposal is then rejected by the infinite energy below.
            if(!(q[1] > 0.0f) || !isfinite(q[0]) || !isfinite(q[1])){ diverged = true; break; }

            //Another half step on the momentum, now at the new position
            normal_potential_grad(data, data_size, q[0], q[1], grad);
            for(int d = 0; d < n_params; d ++){ rho[d] -= 0.5f * leapfrog_e * grad[d]; }
        }

        //Metropolis correction. H is an energy, so the ratio is exp(H_start - H_end).
        float h_end = diverged
            ? numeric_limits<float>::infinity()
            : normal_potential(data, data_size, q[0], q[1]) + neg_log_normal(rho, M_sd, n_params);

        float chance = 0.0f;
        if(isfinite(h_end)){
            chance = exp(h_start - h_end);
            if(chance > 1){ chance = 1; }
        }

        //Accept the *evolved* position; on rejection the chain repeats the current one
        //(leaving the slot unwritten would hand back uninitialised memory).
        const float* keep = (metro_sampler(rng) < chance) ? q : params + n_params * i;
        for(int offset = 0; offset < n_params; offset ++){
            params[n_params * (i+1) + offset] = keep[offset];
        }
    }

    return params; //Caller frees
}


HMCResult hmc_fit_normal(const vector<float>& data,
                         int iters,
                         int burn_in,
                         float leapfrog_e,
                         int leapfrog_steps,
                         unsigned int seed){

    HMCResult out;

    if((int)data.size() < 2){
        out.error = "need at least 2 observations to fit";
        return out;
    }
    if(iters < 1){
        out.error = "iters must be >= 1";
        return out;
    }
    if(burn_in < 0){ burn_in = 0; }
    if(burn_in >= iters){ burn_in = iters / 2; }

    const int n = (int)data.size();
    float* raw = const_cast<float*>(data.data());

    float sd0 = stdev(raw, n);
    if(!(sd0 > 0.0f)){
        //Degenerate (all identical) data: the posterior has no spread to explore
        out.ok = true;
        out.n_samples = 1;
        out.mu_mean = mean(raw, n);
        out.sigma_mean = 0.0f;
        out.samples = { out.mu_mean, 0.0f };
        out.accept_rate = 1.0f;
        return out;
    }

    //Scale the default step size to the data, otherwise a fixed epsilon either
    //crawls (prices in the hundreds) or diverges (returns near zero).
    if(!(leapfrog_e > 0.0f)){ leapfrog_e = 0.1f * sd0 / sqrt((float)n); }
    if(leapfrog_steps < 1){ leapfrog_steps = 1; }

    //Burn-in with a crude step size adaptation towards a ~0.65 acceptance rate,
    //run in short blocks so each block restarts from the last accepted draw.
    float mu_cur = mean(raw, n);
    float sigma_cur = sd0;

    if(burn_in > 0){
        const int blocks = 20;
        const int per_block = max(1, burn_in / blocks);
        const float target = 0.60f;

        //Acceptance against step size is close to a cliff: near 1 while the leapfrog
        //is stable, then abruptly 0 once it diverges. Multiplying up and down around
        //a target just oscillates across that edge, so bracket it instead and keep
        //the largest step size actually measured as good.
        float e_good = 0.0f;                              //Largest known stable
        float e_bad = numeric_limits<float>::infinity();  //Smallest known divergent

        for(int b = 0; b < blocks; b ++){
            float* chain = hmc(raw, n, per_block, leapfrog_e, leapfrog_steps,
                               mu_cur, sigma_cur, seed == 0 ? 0 : seed + b + 1);

            int moved = 0;
            for(int i = 1; i <= per_block; i ++){
                if(chain[HMC_NPARAMS*i] != chain[HMC_NPARAMS*(i-1)] ||
                   chain[HMC_NPARAMS*i + 1] != chain[HMC_NPARAMS*(i-1) + 1]){ moved ++; }
            }
            float rate = (float)moved / (float)per_block;

            //A stuck block leaves the position untouched, which is harmless to carry
            mu_cur = chain[HMC_NPARAMS * per_block];
            sigma_cur = chain[HMC_NPARAMS * per_block + 1];
            free(chain);

            if(rate >= target){ e_good = max(e_good, leapfrog_e); }
            else { e_bad = min(e_bad, leapfrog_e); }

            //Once the edge is bracketed, bisect geometrically; before that, hunt for it
            if(e_good > 0.0f && isfinite(e_bad)){
                leapfrog_e = sqrt(e_good * e_bad);
            } else if(rate >= target){
                leapfrog_e *= 2.0f;
            } else {
                leapfrog_e *= 0.5f;
            }
        }

        //Hand the main run a step size that was measured, never a freshly
        //multiplied one, and back off slightly to stay inside the stable region.
        if(e_good > 0.0f){ leapfrog_e = e_good * 0.9f; }
    }

    //Main run
    float* chain = hmc(raw, n, iters, leapfrog_e, leapfrog_steps,
                       mu_cur, sigma_cur, seed);

    int moved = 0;
    for(int i = 1; i <= iters; i ++){
        if(chain[HMC_NPARAMS*i] != chain[HMC_NPARAMS*(i-1)]){ moved ++; }
    }

    //Drop the seeded row 0 and keep the whole main run
    out.samples.assign(chain + HMC_NPARAMS, chain + HMC_NPARAMS * (iters + 1));
    out.n_samples = iters;
    free(chain);

    vector<float> mus(out.n_samples);
    vector<float> sigmas(out.n_samples);
    for(int i = 0; i < out.n_samples; i ++){
        mus[i] = out.samples[HMC_NPARAMS*i];
        sigmas[i] = out.samples[HMC_NPARAMS*i + 1];
    }

    out.mu_mean = mean(mus.data(), out.n_samples);
    out.mu_sd = stdev(mus.data(), out.n_samples);
    out.sigma_mean = mean(sigmas.data(), out.n_samples);
    out.sigma_sd = stdev(sigmas.data(), out.n_samples);
    out.accept_rate = (float)moved / (float)iters;
    out.step_size = leapfrog_e;
    out.ok = true;

    return out;
}


#ifdef HMC_DEMO
int main(){

    const int DSIZE = 128;

    random_device rd;
    mt19937 rng(rd());
    normal_distribution<float> nsampler(6.0f, 1.5f);

    vector<float> data(DSIZE);
    for(int i = 0; i < DSIZE; i++){ data[i] = nsampler(rng); }

    auto startt = chrono::high_resolution_clock::now();

    HMCResult res = hmc_fit_normal(data, 2000, 500);

    auto endt = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration_ms = endt - startt;

    cout << "truth      mu = 6.000  sigma = 1.500" << endl;
    cout << "posterior  mu = " << res.mu_mean << " +/- " << res.mu_sd
         << "  sigma = " << res.sigma_mean << " +/- " << res.sigma_sd << endl;
    cout << "accept rate " << res.accept_rate << " at step size " << res.step_size << endl;
    cout << "Took " << duration_ms.count() << " ms for " << res.n_samples << " draws" << endl;

    return 0;
}
#endif
