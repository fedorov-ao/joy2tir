#ifndef SIG_DATA_HPP
#define SIG_DATA_HPP

struct sig_data {
  char dllsig[200];
  char appsig[200];
};

extern sig_data const sigdata;

#endif
