#include "alignment.H"
#include "rng.H"

alignment randomize(const alignment& A,int n) {
  if (n == -1)
    n = A.size2();

  int maxlength = -1;
  for(int s=0;s<n;s++) {
    if (A.seqlength(s) > maxlength)
      maxlength = A.seqlength(s);
  }

  alignment A2 = A;
  int newlength = int( maxlength + 2 + 0.1*maxlength);
  A2.changelength(newlength);

  const int temp = alphabet::gap;
  for(int i=0;i<n;i++) {
    vector<int> s = A.seq(i);
    while(s.size() < newlength) {
      int pos = myrandom(s.size()+1);
      s.insert(s.begin()+pos,temp);
    }
    for(int c=0;c<A2.length();c++)
      A2(c,i) = s[c];
  }

  remove_empty_columns(A2);
  std::cerr<<A2<<endl;
  return A2;
}