#include "dce-stdlib.h"
#include "utils.h"
#include "process.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <string.h>

NS_LOG_COMPONENT_DEFINE ("DceEnv");

using namespace ns3;

char * dce_getenv (const char *name)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << name);
  NS_ASSERT (Current () != 0);
  struct Thread *current = Current ();
  char ***penvp = current->process->penvp;
  if (!penvp)
    {
      return NULL;
    }
  return seek_env (name, *penvp);
}
char * dce__secure_getenv (const char *name)
{
  // XXX
  return dce_getenv (name);
}

int dce_putenv (char *string)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << string);
  NS_ASSERT (Current () != 0);
  struct Thread *current = Current ();
  char ***penvp = current->process->penvp;
  // first, locate '=' sign.
  char *name = strchr (string, '=');
  if (name == 0)
    {
      // no '=' so, this is a request to clear the corresponding
      // entry in the env array
      return dce_unsetenv (string);
    }
  size_t namelen = ((unsigned long)name) - ((unsigned long)string) + 1;
  // try to find requested name in env array
  char **cur;
  int len = 0;
  bool found = false;
  for (cur = *penvp; *cur != 0; cur++)
    {
      if (strncmp (string, *cur, namelen) == 0)
        {
          // found it, and replace.
          *cur = string;
          found = true;
        }
      len++;
    }
  if (found)
    {
      return 0;
    }
  // not found: we are going to add it now.
  char **newEnvp = (char**)malloc (sizeof(char*) * (len + 1 + 1));
  current->process->allocated.push_back (newEnvp);
  memcpy (newEnvp, *penvp, sizeof(char*) * (len));
  newEnvp[len] = string;
  newEnvp[len + 1] = 0;
  *penvp = newEnvp;
  return 0;
}
int dce_setenv (const char *name, const char *value, int overwrite)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << name << value << overwrite);
  NS_ASSERT (Current () != 0);
  struct Thread *current = Current ();
  char ***penvp = current->process->penvp;
  if (overwrite != 0)
    {
      dce_unsetenv (name);
    }
  int namelen = strlen (name);
  char **cur;
  int len = 0;
  for (cur = *penvp; *cur != 0; cur++)
    {
      if (overwrite == 0 && strncmp (*cur, name, namelen) == 0)
        {
          // there is already an entry which matches this name and
          // we cannot remove it so, we are done.
          return 0;
        }
      len++;
    }
  char **newEnvp = (char**)malloc (sizeof(char*) * (len + 1 + 1));
  current->process->allocated.push_back (newEnvp);
  memcpy (newEnvp, *penvp, sizeof(char*) * (len));
  int valuelen = strlen (value);
  char *str = (char*)malloc (namelen + 1 + valuelen + 1);
  current->process->allocated.push_back (str);
  memcpy (str, name, namelen);
  str[namelen] = '=';
  memcpy (str + namelen + 1,value, valuelen);
  str[namelen + 1 + valuelen] = 0;
  newEnvp[len] = str;
  newEnvp[len + 1] = 0;
  *penvp = newEnvp;
  return 0;
}
int dce_unsetenv (const char *name)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId () << name);
  NS_ASSERT (Current () != 0);
  struct Thread *current = Current ();
  char ***penvp = current->process->penvp;
  int namelen = strlen (name);
  char **cur;
  for (cur = *penvp; *cur != 0;)
    {
      char *equal = strchr (*cur, '=');
      if (equal == 0)
        {
          continue;
        }
      if (strncmp (*cur, name, namelen) != 0)
        {
          cur++;
          continue;
        }
      char **i, **prev;
      *cur = *(cur + 1);
      for (i = cur + 1, prev = cur; *i != 0; i++, prev++)
        {
          *prev = *i;
        }
    }
  return 0;
}
int dce_clearenv (void)
{
  NS_LOG_FUNCTION (Current () << UtilsGetNodeId ());
  NS_ASSERT (Current () != 0);
  struct Thread *current = Current ();
  char ***penvp = current->process->penvp;
  *penvp = (char**)malloc (sizeof (char *));
  current->process->allocated.push_back (*penvp);
  **penvp = 0;
  return 0;
}
