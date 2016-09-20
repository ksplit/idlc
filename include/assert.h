#ifndef ASSERT_H
#define ASSERT_H

#include <errno.h>
#include <cstdlib>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <iostream>

/* taken from Scott Bauer 
 */
static inline void AssertionFailure(std::string exp, std::string file, int line, const char* format, ...)
//static void AssertionFailure(char *exp, char *file, int line, const char* format, ... )
{
  std::cout << "Assertion '" << exp.c_str() << "' failed at line " << line << " of file " << file.c_str() << std::endl;
  std::cout << "Error is " <<  strerror(errno) << std::endl;
  va_list args;
  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

/* taken from Scott Bauer 
 * Will be moved to more appropriate place
 */
#define Assert(exp, format, ...) if (exp) ; else AssertionFailure( #exp, __FILE__,  __LINE__, format, ##__VA_ARGS__ ) 

#endif
