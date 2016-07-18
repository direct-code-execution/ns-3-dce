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
#ifndef SIMU_DIRENT_H
#define SIMU_DIRENT_H

#include <sys/types.h>
#include <dirent.h>


#include "dce-guard.h"

#ifdef __cplusplus
extern "C" {
#endif

DCE(DIR * , opendir, (const char *name));
DCE(DIR * , fdopendir, (int fd));
DCE(struct dirent * , readdir, (DIR *dirp));
DCE(int , readdir_r, (DIR *dirp, struct dirent *entry, struct dirent **result));
DCE(int , closedir, (DIR *dirp));
DCE(int , dirfd, (DIR *dirp));
DCE(void , rewinddir, (DIR *dirp));
DCE(int , scandir, (const char *dirp, struct dirent ***namelist, int (*filter)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **)));


NATIVE (alphasort, const struct dirent **, const struct dirent **)
NATIVE (alphasort64)
NATIVE (versionsort)

#ifdef __cplusplus
}
#endif

namespace ns3 {
struct Thread;

// used internally only
int dce_internalClosedir (DIR *dirp, struct Thread *cur);

}

#endif /* SIMU_DIRENT_H */
