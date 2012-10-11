#ifndef __HELPER__
#define __HELPER__

#include <string>
std::string int2string(int i);
std::string flt2string(float i);
std::string dbl2string(double i);
int string2int(std::string str);

int min(int x,int y);
int max(int x,int y);
bool isPow2(int n);
bool isInteger(const char *str);
void setEnPrintf(bool enable);
int dbgPrintf(const char *format, ...);
#endif
