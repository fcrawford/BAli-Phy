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

/**
 * @file alignment.H
 */

#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include "alphabet.H"
#include "sequence.H"
#include "sequence-format.H"
#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/numeric/ublas/matrix.hpp>

namespace ublas = boost::numeric::ublas;

/// A multiple sequence alignment
class alignment 
{
  /// The unaligned sequences on which the alignment is based.
  std::vector<sequence> sequences;

  /// The homology array - accessed through operator()
  ublas::matrix<int> array;
  
  /// Reset the alignment: no sequences, an empty array.
  void clear();

  /// Add a new row to the homology array.
  void add_row(const std::vector<int>&);

  /// The alphabet that translates integers back to letters.
  boost::shared_ptr<const alphabet> a;

  /// An expandable list of arrays for caching information.
  mutable std::vector<ublas::matrix<int> > notes;
  
public:

  /// Unsafe - Make a copy of the alignment without preserving the homology array
  friend alignment blank_copy(const alignment&,int length);

  /// Returns the number of notes
  int n_notes() const {return notes.size();}

  /// Adds a note of size (length()+1 X l)
  int add_note(int l) const;

  /// Access the n-th note.
  ublas::matrix<int>& note(int n) const {return notes[n];}

  /// Access the n-th note.
  int& note(int n,int i,int j) const {return notes[n](i,j);}

  /// The order of the sequence named s
  int index(const std::string& s) const;

  /// The i-th sequence
  const sequence& seq(int i) const { return sequences[i];}
  /// The i-th sequence
  sequence& seq(int i) { return sequences[i];}

  /// Remove sequence 'i' from the alignment
  void del_sequence(int);
  /// Add sequence 's' to the alignment
  void add_sequence(const sequence& s);

  /// Add sequences sequences to the alignment.
  void load(const std::vector<sequence>& sequences);
  /// Add sequences sequences to the alignment.
  void load(const std::vector<boost::shared_ptr<const alphabet> >& alphabets,const std::vector<sequence>& sequences);

  /// Parse the file file using loader, and add the resulting sequences.
  void load(sequence_format::loader_t loader,std::istream& file);
  /// Parse the file file using loader, and add the resulting sequences.
  void load(const std::vector<boost::shared_ptr<const alphabet> >& alphabets,sequence_format::loader_t,std::istream&);

  /// Load PHYLIP or FASTA file 'filename', depending on extention
  void load(const std::string& filename);
  /// Load PHYLIP or FASTA file 'filename', depending on extention
  void load(const std::vector<boost::shared_ptr<const alphabet> >& alphabets,const std::string& filename);

  /// Get a list of sequences (with gaps) that represent the current alignment.
  std::vector<sequence> convert_to_sequences() const;

  /// Get a list of sequences (with gaps) that represent the current alignment.
  void write_sequences(sequence_format::dumper_t,std::ostream&) const;

  /// Print alignment in default format (FASTA)
  void print(std::ostream&) const;
  /// Print alignment in FASTA format
  void print_fasta(std::ostream&) const;
  /// Print alignment in PHYLIP format
  void print_phylip(std::ostream&) const;

  /// Resize the alignment array and the notes
  void changelength(int l);
  /// Remove a column from the alignment, preserving the information in other columns.
  void delete_column(int i);

  /// The feature (letter,gap,non-gap) of sequence s in column l
  int& operator()(int l,int s) {return array(l,s); }
  /// The feature (letter,gap,non-gap) of sequence s in column l
  const int& operator()(int l,int s) const {return array(l,s); }

  /// Does sequence i have a gap at position j ?
  bool gap(int i,int j) const {return array(i,j)==alphabet::gap;}
  /// Does sequence i have an unknown at position j ?
  bool unknown(int i,int j) const {return array(i,j)==alphabet::unknown;}
  /// Does sequence i have an character at position j ?
  bool character(int i,int j) const {return not gap(i,j) and not unknown(i,j);}

  /// The assignment operator
  alignment& operator=(const alignment&);

  /// Number of columns
  int length() const {return array.size1();}

  /// Total number of sequences
  int n_sequences() const {assert(array.size2() == sequences.size()); return array.size2();}

  //FIXME!  I should cache the results of this
  /// Compute the length of a particular sequence from the array
  int seqlength(int) const;

  /// Access the alignment's alphabet.
  const alphabet& get_alphabet() const {return *a;}

  /// Construct an empty alignment
  alignment() {}

  /// Construct an empty alignment, with alphabet a
  explicit alignment(const alphabet& a);

  /// Unsafe - Construct an empty alignment, with n sequences of length 0
  explicit alignment(const alphabet& a,int n);

  /// Unsafe - Construct an alignment from sequences, but with any empty array
  explicit alignment(const alphabet&,const std::vector<sequence>& sequence);

  /// Unsafe - Construct an empty n x L alignment.
  explicit alignment(const alphabet&,int n,int L);

  /// Construct an alignment from file filename using alphabet a
  explicit alignment(const alphabet& a,const std::string& filename);
};

/// Write alignment A to file file
std::ostream& operator<<(std::ostream& file,const alignment& A);

/// Read alignment A from file file
std::istream& operator>>(std::istream& file,alignment& A);

/// Does the alignment contain any empty columns?
bool valid(const alignment&);

/// Does the column contain only gaps for taxa in mask?
bool all_gaps(const alignment& A,int column,const boost::dynamic_bitset<>& mask);

/// Does the column contain only gaps?
bool all_gaps(const alignment& A,int column);

/// How many non-gap characters are in this column of the alignment?
int n_characters(const alignment& A, int column);

/// remove columns that contain only gaps
int remove_empty_columns(alignment&);

/// Construct a path through the A2 HMM based on the alignment of nodes n1 and n2
std::vector<int> get_path(const alignment& A,int n1,int n2);
/// Construct a path through the A3 HMM based on the alignment of nodes n1 and n2
std::vector<int> get_path_3way(const alignment& A,int n0,int n1,int n2,int n3);

/// Return the names of the sequences for alignment A
std::vector<std::string> sequence_names(const alignment& A);
/// Return the names of the sequences of the first n sequences in alignment A
std::vector<std::string> sequence_names(const alignment& A,int n);

/// Generate a randomized alignment
alignment randomize(const alignment&,int n=-1);
#endif


