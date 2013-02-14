#include <string.h>
#include <stdio.h>

int init_thing ();
int get_thing ();
void test_singleton ();

namespace fred {
template <bool Do_Init = true>
class A
{
private:
  class B
  {
public:
    B ()
    {
      if (Do_Init)
        {
          v = 42;
          ::init_thing ();
        }
    }
    void setV (int a)
    {
      v = a;
    }
    int getV ()
    {
      return v;
    }
    static B* instance ()
    {
      static B* init = new B ();

      return init;
    }
private:
    int v;
  };

public:
  A () : ref_ (B::instance ())
  {
    A* tmp = &instance_;
    memmove (&tmp, &tmp, sizeof(A*));
  }

  B* getRef ()
  {
    return ref_;
  }

  static A* getSingleton ()
  {
    return &instance_;
  }

private:
  static A instance_;
  B* ref_;
};

template <bool Do_Init>
A<Do_Init> A<Do_Init>::instance_;
}

