/*
   Copyright (C) 2004-2006,2009 Benjamin Redelings

This file is part of BAli-Phy.

BAli-Phy is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

BAli-Phy is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with BAli-Phy; see the file COPYING.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef RNG_H
#define RNG_H

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <valarray>
#include <cassert>

unsigned long myrand_init();
unsigned long myrand_init(unsigned long);
 
// returns a value in [0,1)
unsigned long uniform_unsigned_long();

// synonym for 'uniform'
double myrandomf();

// returns a value in [0,1)
double uniform();

// return the log of a variable that is uniform on [0,1)
double log_unif();

// return a normal random variate
double gaussian(double mu,double sigma);

// return a shifted laplacian random variate
double laplace(double mu,double sigma);

// return a shifted laplacian random variate
double cauchy(double l=0.0,double s=1.0);

// return an exponentially distributed variable
double exponential(double mu);

// return an gamma-distributed variable
double gamma(double a, double b);

// return a poisson random variate
unsigned poisson(double mu);

// return a geometric random variate
unsigned geometric(double p);

// return a dirichlet random vector
std::valarray<double> dirichlet(const std::valarray<double>& n);

namespace rng {

  unsigned long get_random_seed();

  typedef int amount_t;
  typedef std::valarray<amount_t> tuple;

  class RNG {

  protected:
    gsl_rng* generator;
    

  public:
    unsigned long int seed(unsigned long int);
    unsigned long int seed();
    
    unsigned long min() const { return gsl_rng_min(generator); }
    unsigned long max() const { return gsl_rng_max(generator); }

    unsigned long get() { return gsl_rng_get(generator) ;}

    double uniform() { return gsl_rng_uniform(generator); }
    
    double uniform_int(unsigned long int n) { return gsl_rng_uniform_int(generator,n); }

    double log_unif() {return -gsl_ran_exponential(generator,1.0);}

    double laplace(double mu,double sigma) {return gsl_ran_laplace(generator,sigma)+mu;}

    double cauchy(double l=0.0,double s=1.0) {return gsl_ran_cauchy(generator,s)+l;}

    double gaussian(double mu,double sigma) {return gsl_ran_gaussian(generator,sigma/sqrt(2))+mu;}

    double exponential(double mu) {return gsl_ran_exponential(generator,mu);}
    double gamma(double a,double b) {return gsl_ran_gamma(generator,a,b);}

    int geometric (double p) {return gsl_ran_geometric(generator,p);}

    unsigned poisson(double mu) {return gsl_ran_poisson(generator,mu);}

    std::valarray<double> dirichlet(const std::valarray<double>& n);

    RNG();
    ~RNG();
  };


  class Binomial:public RNG {
  public:
    unsigned operator()(double p,unsigned long n);
  };
  
  
  class Poisson:public RNG {
  public:
    unsigned operator()(double lambda);
  };
  

  class Multinomial {
    Binomial Bin;
  public:
    tuple operator()(const std::valarray<double>& p,unsigned long n);
  };

  class Exponential: public RNG {
    double mu;
  public:
    double operator()();

    Exponential(double d):mu(d) { assert(mu>0.0); }
  };

  void init();

  extern RNG* standard;
}

/// returns a value in [0,max-1]
inline unsigned long myrandom(unsigned long max) {
  return (unsigned long)rng::standard->uniform_int(max);
} 

inline long myrandom(long min,long max) {
  unsigned long diff = max - min;
  return myrandom(diff)+min;
}

namespace dng {
  
  typedef double amount_t;
  typedef std::valarray<amount_t> tuple;
  
  class Binomial {
  public:
    double operator()(double p,double n) { return p*n;}
  };
  
  
  class Poisson {
  public:
    double operator()(double lambda) {return lambda;}
  };
  
  
  class Multinomial {
  public:
    tuple operator()(const std::valarray<double>& p,double n) { return p*n;}
  };


  void init();
}

#endif
