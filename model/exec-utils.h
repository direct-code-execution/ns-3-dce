/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Frederic Urbani <frederic.urbani@inria.fr>
 *
 */
#ifndef EXEC_UTILS_H
#define EXEC_UTILS_H

#include "utils.h"

namespace ns3 {

/**
 * \brief Utilities functions to help to find DCE binaries location to be executed using Virtuals and real PATHs.
 *
 *  DCE search policy:
 *
 *  There is 3 kind of searched file:
 *       - simple file name like 'foo'
 *       - relative path like './dir/foo'
 *       - or absolute path like '/tmp/foo'
 *
 *  There is 2 kind of 'exec' like methods the one with a postfix 'p' and the others,
 *  the first one group (ie: execlp and execvp) may search the file using the environment variable PATH value
 *  (only if the file to search is in the simple form),
 *  the other group of methods do not use PATH in any case.
 *
 *  DCE execlp and execvp search algorithm:
 *
 *    for simple file like 'foo'
 *
 *      1 'foo' is searched along the path of the env var PATH of the virtual process doing exec,
 *         this path is relative to the Virtual Root of the corresponding node,
 *      2 'foo' is searched along the path defined by the DceManagerHelper (SetVirtualPath) of the corresponding node,
 *         this path is relative to the Virtual Root of the corresponding node,
 *      2 'foo' is searched along the path of the real env var DCE_PATH.
 *
 *    for relative path like './dir/foo' the file is searched at CWD/dir/foo where CWD is current
 *    working directory of the virtual process doing exec,
 *
 *    and for absolute path like '/tmp/foo'
 *      1 the file is first searched in   VROOT/tmp/foo where VROOT is the location of the root of the
 *      node where the virtual process is doing exec
 *      2 else the file is searched using the roots defined by DCE_ROOT real env var. ie if DCE_ROOT=/tmp:/var
 *        the file '/tmp/foo' will be searched in the locations '/tmp/tmp/foo' and '/var/tmp/foo'.
  *
 *  DCE execl execle execv execve search algorithm:
 *
 *    for simple file like 'foo'
 *
 *      foo' is searched in the CWD of the virtual process doing exec,
 *
 *    for relative path like './dir/foo' the file is searched at CWD/dir/foo where CWD is current
 *    working directory of the virtual process doing exec,
 *
 *    and for absolute path like '/tmp/foo'
 *      1 the file is first searched in   VROOT/tmp/foo where VROOT is the location of the root of the
 *      node where the virtual process is doing exec
 *      2 else the file is searched using the roots defined by DCE_ROOT real env var. ie if DCE_ROOT=/tmp:/var
 *        the file '/tmp/foo' will be searched in the locations '/tmp/tmp/foo' and '/var/tmp/foo'.
 *
 *
 */

/**
 * Canonize A Path, checking if a ChRoot occurs
 * \param path to canonize,
 * \param ChRootProtected boolean if true check if a change of root occurs,
 *        ie if true path is not allow to go to upper directories for example .. is not allowed or ./foo/../..
 * \param result result string if return true
 */
bool CanonizePath (std::string path, bool ChRootProtected, std::ostringstream &result);

/**
 * \brief Class representing a search PATH
 */
class SearchPath
{
public:
  /**
   * \param basepath indicate the Virtual root if needed.
   * \param path a search path like the well know PATH environment variable.
   * \param virt if true the basepath indicate a virtual root and then it is forbidden to
   *        find file not under this basepath.
   */
  SearchPath (std::string basepath, std::string path, bool virt);

  /**
   * \param file file seeked, can be a simple name (myfile) or a relative path (./myfile) or an absolute path (/tmp/myfile)
   * \param cwd current working directory of the virtual process seeking a file, it is used only
   *            if the upper parameter file is a relative path.
   * \param userData an opaque data passed to the callback just below
   * \param checker a required callback called to check and validate or not the potential files found by this method.
   */
  std::string SeekFile (std::string file, std::string cwd,
                        void *userData, bool (*checker)(std::string, void*));

private:
  bool m_virtual;
  std::string m_basePath;
  std::vector <std::string> m_paths;
};

std::string SearchFile (std::string file,
                        std::string vroot,
                        std::string vpath,
                        std::string dcepath,
                        std::string cwd,
                        std::string altRoots,
                        void *userData,
                        bool (*checker)(std::string, void*));

std::string
SearchFile (std::string file,
            std::string vroot,
            std::string cwd,
            std::string altRoots,
            void *userData,
            bool (*checker)(std::string, void*));

bool CheckFileExe (std::string file,void *userData);

std::string SearchExecFile (std::string file, std::string vpath, uid_t uid, gid_t gid, int *errNo);

// Without PATH only Real or Virtual root
std::string SearchExecFile (std::string file, uid_t uid, gid_t gid, int *errNo);

// Search using only a path within an environment variable
std::string SearchExecFile (std::string env, std::string file, int *errNo);


}

#endif
