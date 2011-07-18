#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main (int c, char **v)
{
  printf("go go go go\n");
  sleep (1);
  char * const args[] = { "test-exec-target-1", "2", "3", 0 };
  char * const env[] = { "FRED=NEWONE", 0 };

  setenv ("FRED", "ERIC", 1);


  //  int ret =  execv("build/bin/test-exec-target-1", args);
  //  int ret =  execvp("test-exec-target-1", args);

//  int ret =  execl("build/bin/test-exec-target-1", "test-exec-target-1", "2", "3", (char*)0);
//  int ret =  execlp("test-exec-target-1", "test-exec-target-1", "2", "3", (char*)0);


  // int ret =  execve("test-exec-target-1", args, env);

  int ret =  execle("build/bin/test-exec-target-1", "test-exec-target-1", "2", "3", (char*)0 , "FRED=NEWONE" , (char *) 0 );

  printf(" ret = %d , errno = %d \n", ret, errno );

  if (ret) return errno;
  return ret;
}
