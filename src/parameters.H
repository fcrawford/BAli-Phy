#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "alphabet.H"
#include "mytypes.H"
#include "sequencetree.H"
#include <valarray>
#include "smodel.H"
#include "imodel.H"
#include "alignment.H"
#include "matcache.H"
#include "clone.H"

/******************************* parameter-containing class **************************/
/// A class to contain all the MCMC state except the alignment
class Parameters: public MatCache {
  /*-------- We own these ----------*/
  /// The IndelModel
  OwnedPointer<IndelModel> IModel_;

  /// The substitution::Model
  OwnedPointer<substitution::MultiRateModel> SModel_;

public:

  /// Temperature, for heated chains: when heated we aren't normalized...
  double Temp;

  /// Is each IndelModel parameter fixed or not?
  std::valarray<bool> i_fixed;

  /// Is each substitution::Model parameter fixed or not?
  std::valarray<bool> s_fixed;

  /// Is pinning enabled or not?
  vector<double> constants;
  /// Is pinning enabled or not?
  int features;

  /// The likelihood function
  double basic_likelihood(const alignment& A,const Parameters& P) const; 
  double likelihood(const alignment& A,const Parameters& P) const {return basic_likelihood(A,P)/Temp;}

  /// The prior
  double basic_prior(const alignment& A,const Parameters& P) const;
  double prior(const alignment& A,const Parameters& P) const {return basic_prior(A,P)/Temp;}

  /// Now we can link moves into probability model?
  double probability(const alignment& A,const Parameters& P) const {
    return prior(A,P) + likelihood(A,P);
  }

  double weight(const alignment& A, const Parameters& P) const;

  bool accept_MH(const alignment& A1,const Parameters& P1,
		 const alignment& A2,const Parameters& P2) const;

  /// The Tree
  SequenceTree T;

  /// The IndelModel
  const IndelModel& IModel() const {return *IModel_;}
  /// The IndelModel
  IndelModel& IModel() {return *IModel_;}

  /// Get the substitution::Model
  const substitution::MultiRateModel& SModel() const {return *SModel_;}
  /// Get the substitution::Model
  substitution::MultiRateModel& SModel() {return *SModel_;}

  /// Get the alphabet
  const alphabet& get_alphabet() const {return SModel_->Alphabet();}

  /// Symmetrically propose another state
  void fiddle();

  /// Set branch 'b' to have length 'l', and compute the transition matrices
  void setlength(int b,double l) {MatCache::setlength(b,l,T,*SModel_); }
  
  /// Recalculate all the cached transition matrices
  void recalc();

  /// parameter for exponential prior on branch length
  double branch_mean; 

  Parameters(const substitution::MultiRateModel&,const IndelModel&,const SequenceTree&);
};

#endif