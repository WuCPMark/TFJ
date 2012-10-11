#include "helper.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <sstream>

/* super inefficient */
std::string int2string(int i)
{
  std::stringstream ss;
  ss << i;
  return ss.str();
}

std::string flt2string(float i)
{
  std::stringstream ss;
  ss << i;
  return "((float)"+ss.str() +")";
}
std::string dbl2string(double i)
{
  std::stringstream ss;
  ss << i;
  return ss.str();
}

int string2int(std::string str)
{
  const char *s = str.c_str();
  return atoi(s);
}

int min(int x, int y)
{
  return (x<y)?x:y;
}

int max(int x, int y)
{
  return (x>y)?x:y;
}

bool isPow2(int n)
{
  int r = (n-1)&n;
  return (r==0);
}

bool isInteger(const char *str)
{
  int len = strlen(str);
  for(int i = 0; i < len; i++)
    {
      if(isdigit(str[i])==0)
	return false;
    }
  return true;

}

static bool enPrintf = true;

void setEnPrintf(bool enable)
{
  enPrintf = enable;
}

int dbgPrintf(const char *format, ...)
{
  if(enPrintf)
    {
      va_list ap;
      va_start(ap, format);
      for(char *p=(char*)format; *p!='\0';p++)
	{
	  int i;
	  char *pp;
	  unsigned u;
	  char *s;
	  double f;
	  void *ptr;
	  unsigned long lu;
	  long li;
	  
	  if(*p == '%')
	    {
	      p++;
	      switch(*p)
		{
		case 'd':
		  i = va_arg(ap,int);
		  printf("%d",i);
		  break;
		case 's':
		  s = va_arg(ap,char*);
		  printf("%s",s);
		  break;
		case 'g':
		case 'f':
		  f = va_arg(ap, double);
		  printf("%f", f);
		  break;
		case 'p':
		  ptr = va_arg(ap, void*);
		  printf("%p", ptr);
		  break;
		case 'l':
		  p++;
		  switch(*p)
		    {
		    case 'u':
		      lu = va_arg(ap, unsigned long);
		      printf("%lu", lu);
		      break;
		    case 'i':
		      li = va_arg(ap, long);
		      printf("li", li);
		      break;
		    default:
		      printf("l%c unknown\n", *p);
		      exit(-1);
		      break;
		    }
		  break;
		  
		default:
		  printf("no code for %c\n", *p);
		  exit(-1);
		}
	    }
	  else
	    {
	      putchar(*p);
	    }
	}
      va_end(ap);
    }
}


