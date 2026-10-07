/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "dce-x11-helper.h"
#include "ns3/global-value.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

NS_LOG_COMPONENT_DEFINE ("DceX11Helper");

namespace ns3 {

void
DceX11Helper::Enable (void)
{
  StringValue value;
  GlobalValue::GetValueByName ("DceHostUnixSocketPaths", value);
  std::string paths = value.Get ();
  if (paths.find ("/tmp/.X11-unix/") != std::string::npos)
    {
      return;
    }
  GlobalValue::Bind ("DceHostUnixSocketPaths",
                     StringValue (paths == "" ? "/tmp/.X11-unix/" : paths + ":/tmp/.X11-unix/"));
}

std::string
DceX11Helper::GetDisplay (void)
{
  const char *display = getenv ("DISPLAY");
  return display ? display : "";
}

// Xauthority entries: family, address, display number, name and data, each
// of the four strings preceded by its 16 bit big endian length.
static bool
ReadField (std::istream &in, std::string &field)
{
  unsigned char len[2];
  if (!in.read ((char *)len, 2))
    {
      return false;
    }
  size_t n = (len[0] << 8) | len[1];
  std::vector<char> buf (n);
  if (n > 0 && !in.read (&buf[0], n))
    {
      return false;
    }
  field.assign (buf.begin (), buf.end ());
  return true;
}

static void
WriteField (std::ostream &out, const std::string &field)
{
  unsigned char len[2] = { (unsigned char)(field.size () >> 8), (unsigned char)(field.size () & 0xff) };
  out.write ((const char *)len, 2);
  out.write (field.data (), field.size ());
}

bool
DceX11Helper::InstallAuthority (Ptr<Node> node, std::string authority)
{
  if (authority == "")
    {
      if (getenv ("XAUTHORITY") != 0)
        {
          authority = getenv ("XAUTHORITY");
        }
      else if (getenv ("HOME") != 0)
        {
          authority = std::string (getenv ("HOME")) + "/.Xauthority";
        }
    }
  std::ifstream in (authority.c_str (), std::ios::binary);
  if (!in)
    {
      NS_LOG_WARN ("no X authority file at '" << authority << "'");
      return false;
    }
  std::ostringstream dir;
  dir << "files-" << node->GetId ();
  mkdir (dir.str ().c_str (), 0755);
  std::ofstream out ((dir.str () + "/.Xauthority").c_str (), std::ios::binary | std::ios::trunc);
  int entries = 0;
  while (true)
    {
      unsigned char family[2];
      if (!in.read ((char *)family, 2))
        {
          break;
        }
      std::string address, number, name, data;
      if (!ReadField (in, address) || !ReadField (in, number)
          || !ReadField (in, name) || !ReadField (in, data))
        {
          break;
        }
      // FamilyWild (0xffff) with an empty address matches any host.
      unsigned char wild[2] = { 0xff, 0xff };
      out.write ((const char *)wild, 2);
      WriteField (out, "");
      WriteField (out, number);
      WriteField (out, name);
      WriteField (out, data);
      entries++;
    }
  NS_LOG_INFO (entries << " X authority entries written to " << dir.str () << "/.Xauthority");
  return entries > 0;
}

void
DceX11Helper::SetEnvironment (DceApplicationHelper &dce, std::string display)
{
  dce.AddEnvironment ("DISPLAY", display == "" ? GetDisplay () : display);
  dce.AddEnvironment ("XAUTHORITY", "/.Xauthority");
  dce.AddEnvironment ("HOME", "/");
}

} // namespace ns3
