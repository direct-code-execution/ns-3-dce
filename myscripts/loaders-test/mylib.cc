#include <time.h>
#include "mylib.h"

static int good = 0;
int init_thing ()
{
  if (good == 0)
    {
      good = (int) time (NULL);
    }
  else
    {
      good = -45;
    }
    return 0;
}

int get_thing ()
{
  return good;
}

void test_singleton ()
{
  printf ("pointer %p  %d \n", fred::A<>::getSingleton ()->getRef (), fred::A<>::getSingleton ()->getRef ()->getV ());
  fred::A<>::getSingleton ()->getRef ()->setV (55);
}
