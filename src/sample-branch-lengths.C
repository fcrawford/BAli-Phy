/*
   Copyright (C) 2004-2009 Benjamin Redelings

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

#include "rng.H"
#include "sample.H"
#include "mcmc.H"
#include "util.H"
#include "5way.H"
#include "substitution-cache.H"
#include "substitution-index.H"
#include "substitution.H"
#include "likelihood.H"
#include "proposals.H"
#include "distribution.H"
#include <gsl/gsl_cdf.h>

using MCMC::MoveStats;

bool do_MH_move(Parameters& P,const Parameters& P2,double rho) 
{
  bool success = accept_MH(P,P2,rho);

  if (success) {
    P=P2;
    //    std::cerr<<"accepted\n";
  }
  else {
    //    std::cerr<<"rejected\n";
  }

  return success;
}

double branch_twiddle(double& T,double sigma) {
  T += gaussian(0,sigma);
  return 1;
}

double branch_twiddle_positive(double& T,double sigma) {
  double ratio = branch_twiddle(T,sigma);
  T = std::abs(T);
  return ratio;
}

MCMC::Result change_branch_length_(Parameters& P,int b,double sigma,
				   double (*twiddle)(double&,double)) 
{
  MCMC::Result result(3);
  
  //------------ Propose new length -------------//
  const double length = P.T->branch(b).length();
  double newlength = length;

  double ratio = twiddle(newlength,sigma);
  
  //---------- Construct proposed Tree ----------//
  P.select_root(b);

  Parameters P2 = P;
  P2.setlength(b,newlength);

  //--------- Do the M-H step if OK--------------//
  if (do_MH_move(P,P2,ratio)) {
    result.totals[0] = 1;
    result.totals[1] = std::abs(length - newlength);
    result.totals[2] = std::abs(log(length/newlength));
  }

  return result;
}


void change_branch_length_flat(Parameters& P,MoveStats& Stats,int b,double sigma)
{
  const double L = P.T->branch(b).length();
  const double mu = P.branch_mean();

  MCMC::Result result = change_branch_length_(P, b, sigma*P.branch_mean(), branch_twiddle_positive);

  Stats.inc("branch-length *",result);
  if (L < mu/2.0)
    Stats.inc("branch-length 1",result);
  else if (L < mu)
    Stats.inc("branch-length 2",result);
  else if (L < mu*2)
    Stats.inc("branch-length 3",result);
  else 
    Stats.inc("branch-length 4",result);
}

void change_branch_length_log_scale(Parameters& P,MoveStats& Stats,int b,double sigma)
{
  const double L = P.T->branch(b).length();
  const double mu = P.branch_mean();

  MCMC::Result result = change_branch_length_(P, b, sigma, scale_gaussian );

  Stats.inc("branch-length (log) *",result);
  if (L < mu/2.0)
    Stats.inc("branch-length (log) 1",result);
  else if (L < mu)
    Stats.inc("branch-length (log) 2",result);
  else if (L < mu*2)
    Stats.inc("branch-length (log) 3",result);
  else 
    Stats.inc("branch-length (log) 4",result);
}

#include "slice-sampling.H"

void slice_sample_branch_length(Parameters& P,MoveStats& Stats,int b)
{
  const double L = P.T->branch(b).length();
  const double mu = P.branch_mean();

  P.select_root(b);

  MCMC::Result result(3);
  
  //------------- Find new length --------------//
  
  double sigma = loadvalue(P.keys,"slice_branch_sigma",1.5);
  // NOTE - it is OK to depend on L below.
  double w = sigma*(P.branch_mean()+L);
  branch_length_slice_function logp(P,b);
  double L2 = slice_sample(L,logp,w,100);

  //---------- Record Statistics - -------------//
  result.totals[0] = std::abs(L2 - L);
  result.totals[1] = std::abs(log(L2/L));
  result.totals[2] = logp.count;

  Stats.inc("branch-length (slice) *",result);
  if (L < mu/2.0)
    Stats.inc("branch-length (slice) 1",result);
  else if (L < mu)
    Stats.inc("branch-length (slice) 2",result);
  else if (L < mu*2)
    Stats.inc("branch-length (slice) 3",result);
  else 
    Stats.inc("branch-length (slice) 4",result);
}

void change_branch_length(Parameters& P,MoveStats& Stats,int b)
{
  if (myrandomf() < 0.5)
  {
    double sigma = loadvalue(P.keys,"log_branch_sigma",0.6);
    change_branch_length_log_scale(P, Stats, b, sigma);
  }
  else {
    double sigma = loadvalue(P.keys,"branch_sigma",0.6);
    change_branch_length_flat(P, Stats, b, sigma);
  }
}

void change_branch_length_multi(Parameters& P,MoveStats& Stats,int b) 
{
  const int n=3;

  for(int i=1;i<n;i++)
    change_branch_length(P,Stats,b);
}

void change_branch_length_and_T(Parameters& P,MoveStats& Stats,int b) 
{
  MCMC::Result result(5,0);

  result.counts[0] = 1;

  //------------- Propose new length --------------//
  const double length = P.T->branch(b).length();
  double newlength = length;
  double ratio = branch_twiddle(newlength,P.branch_mean()*0.6);

  //----- positive  =>  propose length change -----//
  if (newlength >= 0) 
  {
    result.counts[1] = 1;
    result.counts[3] = 1;

    //---------- Construct proposed Tree ----------//
    P.select_root(b);

    Parameters P2 = P;
    P2.setlength(b,newlength);

    //--------- Do the M-H step if OK--------------//
    if (do_MH_move(P,P2,ratio)) {
      result.totals[0] = 1;
      result.totals[1] = 1;
      result.totals[3] = std::abs(newlength - length);
    }
  }

  //----- negative  => propose topology ---------//
  else 
  {
    result.counts[2] = 1;
    result.counts[4] = 1;

    //----- Generate the Different Topologies ------//
    vector<Parameters> p(2,P);
    
    SequenceTree& T2 = *p[1].T;
    
    vector<int> nodes = A5::get_nodes_random(T2,b);
    int b1 = T2.directed_branch(nodes[4],nodes[1]);
    int b2 = T2.directed_branch(nodes[5],nodes[2]);
    exchange_subtrees(T2,b1,b2);

    p[1].invalidate_subA_index_branch(b);

    vector<efloat_t> rho(2,1);
    rho[1] = ratio;

    //------ Sample the Different Topologies ------//
    int C = two_way_topology_sample(p,rho,b);

    if (C != -1) {
      P = p[C];
    }

    if (C > 0) {
      result.totals[0] = 1;
      result.totals[2] = 1;
      result.totals[4] = std::abs(length - newlength);
    }
  }

  Stats.inc("change_branch_length_and_T",result);
}

double slide_node_no_expand_branch(vector<double>& lengths,double) 
{
  double L = lengths[0] + lengths[1];

  lengths[0] = L * uniform();
  lengths[1] = L - lengths[0];

  return 1;
}


double slide_node_expand_branch(vector<double>& lengths,double sigma) 
{
  double ratio = exp( gaussian(0,sigma) );

  double L = (lengths[0] + lengths[1]) * ratio;

  lengths[0] = L * uniform();
  lengths[1] = L - lengths[0];

  return ratio*ratio;
}

bool slide_node(Parameters& P,
		const vector<const_branchview>& b,
		double (*slide)(vector<double>&,double)
		) 
{
  // check that we've got three branches
  assert(b.size() == 3);

  // check that the last two are after the first one
  assert(b[0].target() == b[1].source() and
	 b[0].target() == b[2].source());

  //---------------- Propose new lengths ---------------//
  vector<double> lengths(2);
  lengths[0] = b[1].length();
  lengths[1] = b[2].length();

  double sigma = loadvalue(P.keys,"slide_node_sigma",0.3);
  double ratio = slide(lengths,sigma);

  //---------------- Propose new lengths ---------------//
  Parameters P2 = P;

  P2.setlength(b[1].undirected_name(), lengths[0]);
  P2.setlength(b[2].undirected_name(), lengths[1]);
    
  bool success = do_MH_move(P,P2,ratio);

  return success;
}


void slide_node(Parameters& P,MoveStats& Stats,int b0)
{
  vector<const_branchview> b;
  b.push_back( P.T->directed_branch(b0) );

  // choose branches to alter
  if (uniform() < 0.5)
    b[0] = b[0].reverse();
  if (b[0].target().is_leaf_node())
    b[0] = b[0].reverse();
  append(b[0].branches_after(),b);

  b0 = b[0].name();
  int b1 = b[1].undirected_name();
  int b2 = b[2].undirected_name();
  double L1a = P.T->branch(b1).length();
  double L2a = P.T->branch(b2).length();

  P.set_root(b[0].target());

  double p = loadvalue(P.keys,"branch_slice_fraction",0.9);
  if (uniform() < p)
  {
    slide_node_slice_function logp(P,b0);
    double w = logp.total * loadvalue(P.keys,"slide_branch_slice_window",0.3);
    double L1b = slice_sample(logp,w,100);
    
    MCMC::Result result(2);
    result.totals[0] = 2.0*std::abs(L1b-L1a);
    result.totals[1] = logp.count;
    Stats.inc("slide_node_slice",result);
  }
  else {
    bool success; string name;
    if (uniform() < 0.5) {
      success = slide_node(P, b, slide_node_no_expand_branch);
      name = "slide_node";
    }
    else {
      success = slide_node(P, b, slide_node_expand_branch);
      name = "slide_node_expand_branch";
    }
    double L1b = P.T->branch(b1).length();
    double L2b = P.T->branch(b2).length();

    MCMC::Result result(2);
    result.totals[0] = success?1:0;
    result.totals[1] = std::abs(L1b-L1a) + std::abs(L2b-L2a);
    Stats.inc(name,result);
  }
}

void check_caching(const Parameters& P1,Parameters& P2)
{
  efloat_t pi1 = P1.probability();
  efloat_t pi2 = P2.probability();
  
  double diff = std::abs(log(pi1)-log(pi2));
  if (diff > 1.0e-9) {
    std::cerr<<"scale_mean_only: probability diff = "<<diff<<std::endl;
    std::abort();
  }
    
  P2.recalc_smodels();

  pi1 = P1.probability();
  pi2 = P2.probability();
    
  diff = std::abs(log(pi1)-log(pi2));
  if (diff > 1.0e-9) {
    std::cerr<<"scale_mean_only: probability diff = "<<diff<<std::endl;
    std::abort();
  }

  P2.recalc_imodels();
  P2.recalc_smodels();

  pi1 = P1.probability();
  pi2 = P2.probability();
    
  diff = std::abs(log(pi1)-log(pi2));
  if (diff > 1.0e-9) {
    std::cerr<<"scale_mean_only: probability diff = "<<diff<<std::endl;
    std::abort();
  }
}

void scale_means_only(Parameters& P,MoveStats& Stats)
{
  // If any of the partition rates are fixed, then we're out of luck
  // FIXME - techincally, we could recompute likelihoods in just THOSE partitions :P
  //       - also, I suppose, if they are fixed, then there is no mixing problem.
  for(int i=0;i<P.n_branch_means();i++)
    if (P.fixed(i))
      return;

  MCMC::Result result(2);

  //------------ Propose scaling ratio ------------//
  const double sigma = loadvalue(P.keys,"log_branch_mean_sigma",0.6);
  double scale = exp( gaussian(0,sigma) );

  //-------- Change branch lengths and mean -------//
  Parameters P2 = P;

#ifndef NDEBUG
  {
    Parameters P3 = P2;
    check_caching(P,P3);
  }
#endif

  SequenceTree& T2 = *P2.T;
  for(int b=0;b<T2.n_branches();b++) {
    const double length = T2.branch(b).length();
    T2.branch(b).set_length(length/scale);
  }
  P2.tree_propagate();

  for(int i=0;i<P.n_branch_means();i++) 
    P2.branch_mean_tricky(i,P2.branch_mean(i)*scale);
  
#ifndef NDEBUG
  Parameters P3 = P2;
  P3.recalc_imodels();
  P3.recalc_smodels();
  efloat_t L1 =  P.likelihood();
  efloat_t L2 = P3.likelihood();
  double diff = std::abs(log(L1)-log(L2));
  if (diff > 1.0e-9) {
    std::cerr<<"scale_mean_only: likelihood diff = "<<diff<<std::endl;
    std::abort();
  }
#endif

  //--------- Compute proposal ratio ---------//
  efloat_t p_ratio = pow(efloat_t(scale),P2.n_data_partitions()-T2.n_branches());
  efloat_t a_ratio = P2.prior_no_alignment()/P.prior_no_alignment()*p_ratio;

#ifndef NDEBUG
  efloat_t a_ratio2 = P2.probability()/P.probability()*p_ratio;
  double diff2 = std::abs(log(a_ratio2)-log(a_ratio));
  if (diff2 > 1.0e-9) {
    std::cerr<<"scale_mean_only: a_ratio diff = "<<diff2<<std::endl;
    std::cerr<<"probability ratio = "<<log(P2.probability()/P.probability())<<std::endl;
    std::cerr<<"likelihood ratio = "<<log(P2.likelihood()/P.likelihood())<<std::endl;
    std::cerr<<"prior ratio       = "<<log(P2.prior()/P.prior())<<std::endl;
    std::cerr<<"prior ratio (no A)= "<<log(P2.prior_no_alignment()/P.prior_no_alignment())<<std::endl;
    std::cerr<<"prior ratio (   A)= "<<log(P2.prior_alignment()/P.prior_alignment())<<std::endl;
    std::cerr<<"    a ratio = "<<log(a_ratio)<<std::endl;
    std::abort();
  }
#endif
  
  if (uniform() < double(a_ratio)) {
    P=P2;
    result.totals[0] = 1;
    result.totals[1] = std::abs(log(scale));
  }

  Stats.inc("branch-means-only",result);
}

/// Propose three neighboring branch lengths all anti-correlated
void change_3_branch_lengths(Parameters& P,MoveStats& Stats,int n) 
{
  MCMC::Result result(2);

  const Tree& T = *P.T;
  if (not T[n].is_internal_node()) return;

  //-------------- Find branches ------------------//
  vector<const_branchview> branches;
  append(T[n].branches_out(),branches);
  int b1 = branches[0].undirected_name();
  int b2 = branches[1].undirected_name();
  int b3 = branches[2].undirected_name();

  //------------ Change coordinates ---------------//
  double T1 = T.branch(b1).length();
  double T2 = T.branch(b2).length();
  double T3 = T.branch(b3).length();

  double S12 = T1 + T2;
  double S23 = T2 + T3;
  double S31 = T3 + T1;

  //----------- Propose new distances -------------//
  double sigma = loadvalue(P.keys,"log_branch_sigma",0.6)/2.0;
  double ratio = 1.0;

  double T1_ = T1;
  double T2_ = T2;
  double T3_ = T3;

  for(int i=0;i<20;i++) 
  {
    double R12 = exp(gaussian(0,sigma));
    double R23 = exp(gaussian(0,sigma));
    double R31 = exp(gaussian(0,sigma));

    double S12_ = S12 * R12;
    double S23_ = S23 * R23;
    double S31_ = S31 * R31;

    //---------------- Change back ------------------//
    T1_ = (S12_ + S31_ - S23_)/2.0;
    T2_ = (S12_ + S23_ - S31_)/2.0;
    T3_ = (S23_ + S31_ - S12_)/2.0;

    ratio = R12 * R23 * R31;

    if (T1_ > 0.0 and T2_ > 0.0 and T3_ > 0.0) break;
  }
  if (T1_ <= 0.0 or T2_ <= 0.0 or T3_ <= 0.0) return;

  //----------- Construct proposed Tree -----------//
  P.set_root(n);
  
  Parameters P2 = P;
  P2.setlength(b1,T1_);
  P2.setlength(b2,T2_);
  P2.setlength(b3,T3_);
  
  //--------- Do the M-H step if OK--------------//
  if (do_MH_move(P,P2,ratio)) {
    result.totals[0] = 1;
    result.totals[1] = abs(T1_-T1) + abs(T2_-T2) + abs(T3_-T3);
  }

  Stats.inc("3-branch-lengths",result);
}
