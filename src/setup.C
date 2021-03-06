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

#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/shared_ptr.hpp>

#include "setup.H"
#include "util.H"
#include "rates.H"
#include "alphabet.H"
#include "alignment-util.H"
#include "tree-util.H"
#include "substitution-index.H"

using std::vector;
using std::valarray;
using std::cout;
using std::cerr;
using std::endl;

namespace fs = boost::filesystem;

using namespace boost::program_options;
using boost::shared_ptr;

/// Reorder internal sequences of A to correspond to standardized node names for T
alignment standardize(const alignment& A, const SequenceTree& T) 
{
  SequenceTree T2 = T;

  // if we don't have any internal node sequences, then we are already standardized
  if (A.n_sequences() == T.n_leaves())
    return A;

  // standardize NON-LEAF node and branch names in T
  vector<int> mapping = T2.standardize();
  vector<int> new_order = invert(mapping);

  return reorder_sequences(A,new_order);
}

int letter_count(const alignment& A,int l) 
{
  // Count the occurrence of the different letters
  int count=0;
  for(int i=0;i<A.length();i++)
    for(int j=0;j<A.n_sequences();j++)
      if (A(i,j) == l)
	count++;

  return count;
}


valarray<double> letter_counts(const alignment& A) 
{
  const alphabet& a = A.get_alphabet();

  // Count the occurrence of the different letters
  valarray<double> counts(0.0, a.size());
  for(int i=0;i<A.length();i++) {
    for(int j=0;j<A.n_sequences();j++) {
      if (a.is_letter(A(i,j)))
	counts[A(i,j)]++;
    }
  }

  return counts;
}


/// Estimate the empirical frequencies of different letters from the alignment, with pseudocounts
valarray<double> empirical_frequencies(const variables_map& args,const alignment& A) 
{
  const alphabet& a = A.get_alphabet();

  // Count the occurrence of the different letters
  valarray<double> counts = letter_counts(A);

  valarray<double> frequencies(a.size());

  // empirical frequencies
  if (not args.count("frequencies"))
    frequencies = A.get_alphabet().get_frequencies_from_counts(counts,A.n_sequences()/2);

  // uniform frequencies
  else if (args["frequencies"].as<string>() == "uniform")
    frequencies = 1.0/a.size();

  // triplet frequencies <- nucleotide frequencies
  else if (args["frequencies"].as<string>() == "nucleotides") {
    const Triplets* T = dynamic_cast<const Triplets*>(&a);

    if (not T) throw myexception()<<"You can only specify nucleotide frequencies on Triplet or Codon alphabets.";
    valarray<double> N_counts = get_nucleotide_counts_from_codon_counts(*T,counts);
    valarray<double> fN = T->getNucleotides().get_frequencies_from_counts(N_counts,A.n_sequences()/2);

    frequencies = get_codon_frequencies_from_independant_nucleotide_frequencies(*T,fN);
  }

  // specified frequencies
  else {
    vector<double> f = split<double>(args["frequencies"].as<string>(),',');

    if (f.size() != a.size())
      throw myexception()<<"You specified "<<f.size()<<" frequencies, but there are "
			 <<a.size()<<" letters of the alphabet!";

    for(int i=0;i<f.size();i++)
      frequencies[i] = f[i];
  }

  return frequencies;
}

/// Estimate the empirical frequencies of different letters from the alignment, with pseudocounts
valarray<double> empirical_frequencies(const variables_map& args,const vector<alignment>& alignments) 
{
  int total=0;
  for(int i=0;i<alignments.size();i++)
    total += alignments[i].length();

  alignment A(alignments[0]);
  A.changelength(total);

  int L=0;
  for(int i=0;i<alignments.size();i++) 
  {
    for(int c=0;c<alignments[i].length();c++)
      for(int s=0;s<alignments[i].n_sequences();s++)
	A(c+L,s) = alignments[i](c,s);
    L += alignments[i].length();
  }

  return empirical_frequencies(args,A);
}


void remap_T_indices(SequenceTree& T,const vector<string>& names)
{
  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    vector<int> mapping = compute_mapping(T.get_sequences(),names);

    T.standardize(mapping);
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in names.";
    else
      b2<<"Sequence '"<<b2.missing<<"' not found in the tree.";
    throw b2;
  }
}

void remap_T_indices(SequenceTree& T,const alignment& A)
{
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves, but alignment has only "<<A.n_sequences()<<" sequences.";

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    vector<string> names = sequence_names(A,T.n_leaves());  

    remap_T_indices(T,names);
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in alignment.";
    else
      b2<<"Alignment sequence '"<<b2.missing<<"' not found in the tree.";
    throw b2;
  }
}

void remap_T_indices(SequenceTree& T1,const SequenceTree& T2)
{
  if (T1.n_leaves() != T2.n_leaves())
    throw myexception()<<"Trees do not correspond: different numbers of leaves.";

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  try {
    remap_T_indices(T1,T2.get_sequences());
  }
  catch(const bad_mapping<string>& b)
  {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in second tree.";
    else
      b2<<"Couldn't find leaf sequence \""<<b2.missing<<"\" in first tree.";
    throw b2;
  }
}

/// Remap T leaf indices to match A: check the result
void link(alignment& A,SequenceTree& T,bool internal_sequences) 
{
  check_names_unique(A);

  // Later, might we WANT sub-branches???
  if (has_sub_branches(T))
    remove_sub_branches(T);

  if (internal_sequences and not is_Cayley(T)) {
    assert(has_polytomy(T));
    throw myexception()<<"Cannot link a multifurcating tree to an alignment with internal sequences.";
  }

  //------ IF sequences < leaf nodes THEN complain ---------//
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves but Alignment only has "
		       <<A.n_sequences()<<" sequences.";

  //----- IF sequences = leaf nodes THEN maybe add internal sequences.
  else if (A.n_sequences() == T.n_leaves()) {
    if (internal_sequences)
      A = add_internal(A,T);
  }
  //----- IF sequences > leaf nodes THEN maybe complain -------//
  else {
    if (not internal_sequences) {
      alignment A2 = chop_internal(A);
      if (A2.n_sequences() == T.n_leaves()) {
	A = A2;
      }
      else
	throw myexception()<<"More alignment sequences than leaf nodes!";
    } 
    else if (A.n_sequences() > T.n_nodes())
      throw myexception()<<"More alignment sequences than tree nodes!";
    else if (A.n_sequences() < T.n_nodes())
      throw myexception()<<"Fewer alignment sequences than tree nodes!";
  }
  
  //---------- double-check that we have the right number of sequences ---------//
  if (internal_sequences)
    assert(A.n_sequences() == T.n_nodes());
  else
    assert(A.n_sequences() == T.n_leaves());

  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  remap_T_indices(T,A);

  if (internal_sequences)
    connect_leaf_characters(A,T);

  //---- Check to see that internal nodes satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

/// Remap T leaf indices to match A: check the result
void link(alignment& A,RootedSequenceTree& T,bool internal_sequences) 
{
  check_names_unique(A);

  // Later, might we WANT sub-branches???
  if (has_sub_branches(T))
    remove_sub_branches(T);

  if (internal_sequences and not is_Cayley(T)) {
    assert(has_polytomy(T));
    throw myexception()<<"Cannot link a multifurcating tree to an alignment with internal sequences.";
  }

  //------ IF sequences < leaf nodes THEN complain ---------//
  if (A.n_sequences() < T.n_leaves())
    throw myexception()<<"Tree has "<<T.n_leaves()<<" leaves but Alignment only has "
		       <<A.n_sequences()<<" sequences.";

  //----- IF sequences = leaf nodes THEN maybe add internal sequences.
  else if (A.n_sequences() == T.n_leaves()) {
    if (internal_sequences)
      A = add_internal(A,T);
  }
  //----- IF sequences > leaf nodes THEN maybe complain -------//
  else {
    if (not internal_sequences)
      throw myexception()<<"More alignment sequences than leaf nodes!";

    if (A.n_sequences() > T.n_nodes())
      throw myexception()<<"More alignment sequences than tree nodes!";
    else if (A.n_sequences() < T.n_nodes())
      throw myexception()<<"Fewer alignment sequences than tree nodes!";
  }
  
  //---------- double-check that we have the right number of sequences ---------//
  if (internal_sequences)
    assert(A.n_sequences() == T.n_nodes());
  else
    assert(A.n_sequences() == T.n_leaves());


  //----- Remap leaf indices for T onto A's leaf sequence indices -----//
  remap_T_indices(T,A);

  if (internal_sequences)
    connect_leaf_characters(A,T);

  //---- Check to see that internal nodes satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

void link(vector<alignment>& alignments, SequenceTree& T, const vector<bool>& internal_sequences)
{
  for(int i=1;i<alignments.size();i++)
  {
    if (alignments[i].n_sequences() != alignments[0].n_sequences())
      throw myexception()<<"Alignment #"<<i+1<<" has "<<alignments[i].n_sequences()<<" sequences, but the previous alignments have "<<alignments[0].n_sequences()<<" sequences!";

    vector<int> mapping = compute_mapping(sequence_names(alignments[i]),sequence_names(alignments[0]));
    vector<int> new_order = invert(mapping);

    alignments[i] = reorder_sequences(alignments[i],new_order);
  }

  for(int i=0;i<alignments.size();i++) 
    link(alignments[i],T,internal_sequences[i]);
}

void link(vector<alignment>& alignments, RootedSequenceTree& T, const vector<bool>& internal_sequences)
{
  for(int i=1;i<alignments.size();i++)
  {
    if (alignments[i].n_sequences() != alignments[0].n_sequences())
      throw myexception()<<"Alignment #"<<i+1<<" has "<<alignments[i].n_sequences()<<" sequences, but the previous alignments have "<<alignments[0].n_sequences()<<" sequences!";

    vector<int> mapping = compute_mapping(sequence_names(alignments[i]),sequence_names(alignments[0]));
    vector<int> new_order = invert(mapping);

    alignments[i] = reorder_sequences(alignments[i],new_order);
  }

  for(int i=0;i<alignments.size();i++) 
    link(alignments[i],T,internal_sequences[i]);
}

void load_As_and_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_T(args,alignments,T,i);
}

void load_As_and_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,const vector<bool>& internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  // load the alignments
  alignments = load_alignments(filenames,load_alphabets(args));

  T = load_T(args);

  link(alignments,T,internal_sequences);

  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}

void load_As_and_T(const variables_map& args,vector<alignment>& alignments,RootedSequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_T(args,alignments,T,i);
}

void load_As_and_T(const variables_map& args,vector<alignment>& alignments,RootedSequenceTree& T,const vector<bool>& internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  // load the alignments
  alignments = load_alignments(filenames,load_alphabets(args));

  T = load_T(args);

  link(alignments,T,internal_sequences);

  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}


void load_As_and_random_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,bool internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  vector<bool> i(filenames.size(),internal_sequences);

  load_As_and_random_T(args,alignments,T,i);
}


void load_As_and_random_T(const variables_map& args,vector<alignment>& alignments,SequenceTree& T,const vector<bool>& internal_sequences)
{
  //align - filenames
  vector<string> filenames = args["align"].as<vector<string> >();

  // load the alignments
  alignments = load_alignments(filenames,load_alphabets(args));

  //------------- Load random tree ------------------------//
  SequenceTree TC = star_tree(sequence_names(alignments[0]));
  if (args.count("t-constraint"))
    TC = load_constraint_tree(args["t-constraint"].as<string>(),sequence_names(alignments[0]));

  T = TC;
  RandomTree(T,1.0);

  //-------------- Link --------------------------------//
  link(alignments,T,internal_sequences);

  //---------------process----------------//
  for(int i=0;i<alignments.size();i++) 
  {
    
    //---------------- Randomize alignment? -----------------//
    if (args.count("randomize-alignment"))
      alignments[i] = randomize(alignments[i],T.n_leaves());
  
    //------------------ Analyze 'internal'------------------//
    if ((args.count("internal") and args["internal"].as<string>() == "+")
	or args.count("randomize-alignment"))
      for(int column=0;column< alignments[i].length();column++) {
	for(int j=T.n_leaves();j<alignments[i].n_sequences();j++) 
	  alignments[i](column,j) = alphabet::not_gap;
      }

    //---- Check that internal sequence satisfy constraints ----//
    check_alignment(alignments[i],T,internal_sequences[i]);
  }
}

// FIXME - we might still want to link things if
// (a) they have nodes of degree 2
// (b) they have nodes of degree >3
// (c) however, this linking would be limitted to leaf nodes...
// So, we should do some kind of check that when we have internal sequences, we have
// a Cayley tree.

// FIXME - should I make this more generic, so that it doesn't rely on a file?
void load_A_and_T(const variables_map& args,alignment& A,RootedSequenceTree& T,bool internal_sequences)
{
  A = load_A(args,internal_sequences);

  T = load_T(args);

  //------------- Link Alignment and Tree -----------------//
  link(A,T,internal_sequences);

  //---------------- Randomize alignment? -----------------//
  if (args.count("randomize-alignment"))
    A = randomize(A,T.n_leaves());
  
  //------------------ Analyze 'internal'------------------//
  if ((args.count("internal") and args["internal"].as<string>() == "+")
      or args.count("randomize-alignment"))
    for(int column=0;column< A.length();column++) {
      for(int i=T.n_leaves();i<A.n_sequences();i++) 
	A(column,i) = alphabet::not_gap;
    }

  //---- Check that internal sequence satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

void load_A_and_T(const variables_map& args,alignment& A,SequenceTree& T,bool internal_sequences)
{
  RootedSequenceTree RT;
  load_A_and_T(args,A,RT,internal_sequences);
  T = RT;
}

void load_A_and_random_T(const variables_map& args,alignment& A,SequenceTree& T,bool internal_sequences)
{
  // NO internal sequences, yet!
  A = load_A(args,internal_sequences);

  //------------- Load random tree ------------------------//
  SequenceTree TC = star_tree(sequence_names(A));
  if (args.count("t-constraint"))
    TC = load_constraint_tree(args["t-constraint"].as<string>(),sequence_names(A));

  T = TC;
  RandomTree(T,1.0);

  //------------- Link Alignment and Tree -----------------//
  link(A,T,internal_sequences);

  //---------------- Randomize alignment? -----------------//
  if (args.count("randomize-alignment"))
    A = randomize(A,T.n_leaves());
  
  //------------------ Analyze 'internal'------------------//
  if ((args.count("internal") and args["internal"].as<string>() == "+")
      or args.count("randomize-alignment"))
    for(int column=0;column< A.length();column++) {
      for(int i=T.n_leaves();i<A.n_sequences();i++) 
	A(column,i) = alphabet::not_gap;
    }

  //---- Check that internal sequence satisfy constraints ----//
  check_alignment(A,T,internal_sequences);
}

SequenceTree load_constraint_tree(const string& filename,const vector<string>& names)
{
  RootedSequenceTree RT;
  RT.read(filename);

  SequenceTree constraint = RT;
      
  remove_sub_branches(constraint);
  
  try{
    remap_T_indices(constraint,names);
  }
  catch(const bad_mapping<string>& b) {
    bad_mapping<string> b2(b.missing,b.from);
    if (b.from == 0)
      b2<<"Constraint tree leaf sequence '"<<b2.missing<<"' not found in the alignment.";
    else
      b2<<"Alignment sequence '"<<b2.missing<<"' not found in the constraint tree.";
    throw b2;
  }
  return constraint;
}

OwnedPointer<IndelModel> get_imodel(string name) 
{
  //-------------Choose an indel model--------------//
  OwnedPointer<IndelModel> imodel;

  // Default
  if (name == "") 
    name = "RS07";

  if (name == "none")
    { }
  else if (name == "RS05")
    imodel = SimpleIndelModel();
  else if (name == "RS07-no-T")
    imodel = NewIndelModel(false);
  else if (name == "RS07")
    imodel = NewIndelModel(true);
  else
    throw myexception()<<"Unrecognized indel model '"<<name<<"'";
  
  return imodel;
}

void load_bali_phy_rc(variables_map& args,const options_description& options)
{
  if (fs::path::default_name_check_writable())
    fs::path::default_name_check(fs::portable_posix_name);

  if (getenv("HOME")) {
    string home_dir = getenv("HOME");
    if (not fs::exists(home_dir))
      cerr<<"Home directory '"<<home_dir<<"' does not exist!"<<endl;
    else if (not fs::is_directory(home_dir))
      cerr<<"Home directory '"<<home_dir<<"' is not a directory!"<<endl;
    else {
      string filename = home_dir + "/.bali-phy";

      if (fs::exists(filename)) {
	if (log_verbose)
	  cerr<<"Reading ~/.bali-phy ...";
	ifstream file(filename.c_str());
	if (not file)
	  throw myexception()<<"Can't load config file '"<<filename<<"'";
      
	store(parse_config_file(file, options), args);
	file.close();
	notify(args);
	if (log_verbose)
	  cerr<<" done."<<endl;
      }
    }
  }
  else
    cerr<<"Environment variable HOME not set!"<<endl;
}
