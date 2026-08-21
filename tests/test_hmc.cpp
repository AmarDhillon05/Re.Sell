#include "harness.hpp"
#include "models/hmc.hpp"
#include "models/utils.hpp"

#include <cmath>
#include <cstdlib>
#include <random>
#include <vector>

using namespace std;

namespace {

    //Deterministic sample from N(mu, sigma)
    vector<float> normal_sample(int n, float mu, float sigma, unsigned seed){
        mt19937 rng(seed);
        normal_distribution<float> d(mu, sigma);
        vector<float> out(n);
        for(float& v : out){ v = d(rng); }
        return out;
    }

}


// ---- matrix utilities the sampler is built on ----

TEST(matrix, transpose_handles_non_square){
    float m[] = {2, 0, 1,
                 3, 5, 6};              //2 rows, 3 cols
    float* t = transpose(m, 2, 3);      //-> 3 rows, 2 cols

    float expect[] = {2, 3, 0, 5, 1, 6};
    for(int i = 0; i < 6; i ++){ CHECK_NEAR(t[i], expect[i], 1e-5); }
    free(t);
}


TEST(matrix, multiply_handles_non_square){
    float a[] = {2, 0, 1,
                 3, 5, 6};              //2x3
    float b[] = {2, 3,
                 0, 5,
                 1, 6};                 //3x2
    float* p = mmul(a, b, 2, 3, 3, 2);  //2x2

    CHECK_NEAR(p[0], 5, 1e-4);
    CHECK_NEAR(p[1], 12, 1e-4);
    CHECK_NEAR(p[2], 12, 1e-4);
    CHECK_NEAR(p[3], 70, 1e-4);
    free(p);
}


TEST(matrix, multiply_rejects_mismatched_dimensions){
    float a[] = {1, 2, 3, 4};
    bool threw = false;
    try { float* p = mmul(a, a, 2, 2, 3, 3); free(p); }
    catch(const exception&){ threw = true; }
    CHECK(threw);
}


TEST(matrix, add_and_inverse){
    float a[] = {1, 2, 3, 4};
    float b[] = {10, 20, 30, 40};
    float* s = madd(a, b, 2, 2);
    CHECK_NEAR(s[0], 11, 1e-4); CHECK_NEAR(s[3], 44, 1e-4);
    free(s);

    float m[] = {2, -1, 9, -4};          //det = -8 + 9 = 1
    float* inv = twodinv(m);
    CHECK_NEAR(inv[0], -4, 1e-4);
    CHECK_NEAR(inv[1], 1, 1e-4);
    CHECK_NEAR(inv[2], -9, 1e-4);
    CHECK_NEAR(inv[3], 2, 1e-4);
    free(inv);

    //Singular input must not return silent garbage
    float sing[] = {1, 2, 2, 4};
    bool threw = false;
    try { float* bad = twodinv(sing); free(bad); }
    catch(const exception&){ threw = true; }
    CHECK(threw);
}


TEST(matrix, mean_and_stdev){
    float d[] = {1, 2, 3, 4, 5};
    CHECK_NEAR(mean(d, 5), 3.0, 1e-5);
    CHECK_NEAR(stdev(d, 5), 1.5811388, 1e-5);   //Sample (n-1) standard deviation

    float one[] = {42.0f};
    CHECK_NEAR(mean(one, 1), 42.0, 1e-5);
    CHECK_NEAR(stdev(one, 1), 0.0, 1e-5);
}


// ---- the model's energy function ----

TEST(hmc, potential_rejects_invalid_sigma){
    vector<float> d = normal_sample(20, 5.0f, 1.0f, 1);

    CHECK(isfinite(normal_potential(d.data(), 20, 5.0f, 1.0f)));
    CHECK(isinf(normal_potential(d.data(), 20, 5.0f, 0.0f)));
    CHECK(isinf(normal_potential(d.data(), 20, 5.0f, -1.0f)));
}


TEST(hmc, potential_is_minimised_at_the_mle){
    vector<float> d = normal_sample(200, 5.0f, 2.0f, 3);
    float mu = mean(d.data(), 200);
    float sd = stdev(d.data(), 200);

    float best = normal_potential(d.data(), 200, mu, sd);

    //Any nudge away from the fitted values must cost energy
    CHECK(normal_potential(d.data(), 200, mu + 0.5f, sd) > best);
    CHECK(normal_potential(d.data(), 200, mu - 0.5f, sd) > best);
    CHECK(normal_potential(d.data(), 200, mu, sd * 1.3f) > best);
    CHECK(normal_potential(d.data(), 200, mu, sd * 0.7f) > best);
}


TEST(hmc, gradient_matches_finite_differences){
    vector<float> d = normal_sample(100, 12.0f, 3.0f, 5);

    const float mu = 11.0f;
    const float sigma = 2.5f;
    const float h = 1e-3f;

    float grad[HMC_NPARAMS];
    normal_potential_grad(d.data(), 100, mu, sigma, grad);

    //Central differences on the same potential the sampler uses
    double num_mu = (normal_potential(d.data(), 100, mu + h, sigma) -
                     normal_potential(d.data(), 100, mu - h, sigma)) / (2 * h);
    double num_sigma = (normal_potential(d.data(), 100, mu, sigma + h) -
                        normal_potential(d.data(), 100, mu, sigma - h)) / (2 * h);

    CHECK_REL(grad[0], num_mu, 0.02);
    CHECK_REL(grad[1], num_sigma, 0.02);
}


// ---- sampling behaviour ----

TEST(hmc, recovers_a_known_distribution){
    vector<float> d = normal_sample(400, 250.0f, 40.0f, 11);
    float sample_mu = mean(d.data(), 400);
    float sample_sd = stdev(d.data(), 400);

    HMCResult r = hmc_fit_normal(d, 2000, 500, 0.0f, 25, 4242);
    CHECK(r.ok);

    //The posterior must sit on the data's own moments
    CHECK_REL(r.mu_mean, sample_mu, 0.01);
    CHECK_REL(r.sigma_mean, sample_sd, 0.05);

    //And near the truth it was generated from
    CHECK_REL(r.mu_mean, 250.0, 0.05);
    CHECK_REL(r.sigma_mean, 40.0, 0.15);
}


TEST(hmc, posterior_spread_tracks_the_analytic_standard_error){
    const int n = 400;
    vector<float> d = normal_sample(n, 100.0f, 20.0f, 13);
    float sd = stdev(d.data(), n);

    HMCResult r = hmc_fit_normal(d, 3000, 800, 0.0f, 25, 909);
    CHECK(r.ok);

    //se(mu) = sigma/sqrt(n), se(sigma) = sigma/sqrt(2n)
    CHECK_REL(r.mu_sd, sd / sqrt((double)n), 0.25);
    CHECK_REL(r.sigma_sd, sd / sqrt(2.0 * n), 0.25);
}


TEST(hmc, works_across_wide_data_scales){
    //A fixed step size cannot serve cent-scale and six-figure data alike,
    //so this pins the step size adaptation.
    for(double scale : {0.01, 1.0, 100.0, 100000.0}){
        vector<float> d = normal_sample(300, (float)scale, (float)(scale * 0.2), 17);
        float sample_mu = mean(d.data(), 300);
        float sample_sd = stdev(d.data(), 300);

        HMCResult r = hmc_fit_normal(d, 1500, 400, 0.0f, 25, 55);
        CHECK(r.ok);
        CHECK_REL(r.mu_mean, sample_mu, 0.02);
        CHECK_REL(r.sigma_mean, sample_sd, 0.10);

        //A frozen chain would report a near zero acceptance rate
        CHECK(r.accept_rate > 0.15f);
    }
}


TEST(hmc, chain_actually_moves){
    vector<float> d = normal_sample(200, 300.0f, 25.0f, 19);
    HMCResult r = hmc_fit_normal(d, 1500, 400, 0.0f, 25, 31);
    CHECK(r.ok);

    CHECK(r.accept_rate > 0.15f);
    CHECK(r.accept_rate <= 1.0f);
    CHECK(r.step_size > 0.0f);

    //Distinct draws, and a spread that is not collapsed to a point
    int distinct = 0;
    for(int i = 1; i < r.n_samples; i ++){
        if(r.samples[HMC_NPARAMS*i] != r.samples[HMC_NPARAMS*(i-1)]){ distinct ++; }
    }
    CHECK(distinct > r.n_samples / 10);
    CHECK(r.mu_sd > 0.0f);
    CHECK(r.sigma_sd > 0.0f);
}


TEST(hmc, every_draw_stays_in_the_support){
    vector<float> d = normal_sample(150, 50.0f, 30.0f, 23);
    HMCResult r = hmc_fit_normal(d, 2000, 500, 0.0f, 25, 77);
    CHECK(r.ok);

    //sigma must never go negative and nothing may be NaN
    int bad = 0;
    for(int i = 0; i < r.n_samples; i ++){
        float mu = r.samples[HMC_NPARAMS*i];
        float sg = r.samples[HMC_NPARAMS*i + 1];
        if(!(sg > 0.0f) || !isfinite(sg) || !isfinite(mu)){ bad ++; }
    }
    CHECK_EQ(bad, 0);
}


TEST(hmc, is_reproducible_with_a_fixed_seed){
    vector<float> d = normal_sample(200, 75.0f, 12.0f, 29);

    HMCResult a = hmc_fit_normal(d, 800, 200, 0.0f, 25, 12345);
    HMCResult b = hmc_fit_normal(d, 800, 200, 0.0f, 25, 12345);

    CHECK(a.ok && b.ok);
    CHECK_EQ(a.n_samples, b.n_samples);
    CHECK_NEAR(a.mu_mean, b.mu_mean, 1e-6);
    CHECK_NEAR(a.sigma_mean, b.sigma_mean, 1e-6);
}


TEST(hmc, rejects_input_that_is_too_small){
    vector<float> one{5.0f};
    HMCResult r = hmc_fit_normal(one);
    CHECK(!r.ok);
    CHECK(!r.error.empty());

    HMCResult empty = hmc_fit_normal({});
    CHECK(!empty.ok);
}


TEST(hmc, handles_identical_observations){
    //Zero variance data has no posterior spread to explore: must not hang or NaN
    vector<float> flat(30, 7.5f);
    HMCResult r = hmc_fit_normal(flat);

    CHECK(r.ok);
    CHECK_NEAR(r.mu_mean, 7.5, 1e-3);
    CHECK_NEAR(r.sigma_mean, 0.0, 1e-3);
}


TEST(hmc, raw_chain_includes_the_seed_row){
    vector<float> d = normal_sample(50, 10.0f, 2.0f, 37);

    const int iters = 100;
    float* chain = hmc(d.data(), 50, iters, 0.05f, 20, 10.0f, 2.0f, 8);
    CHECK(chain != nullptr);

    //Row 0 is the starting point the caller asked for
    CHECK_NEAR(chain[0], 10.0, 1e-5);
    CHECK_NEAR(chain[1], 2.0, 1e-5);

    //Rejections repeat the previous row rather than leaving it uninitialised
    int bad = 0;
    for(int i = 0; i <= iters; i ++){
        if(!isfinite(chain[HMC_NPARAMS*i]) || !(chain[HMC_NPARAMS*i + 1] > 0.0f)){ bad ++; }
    }
    CHECK_EQ(bad, 0);
    free(chain);
}
