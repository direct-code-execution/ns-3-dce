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

#ifdef __cplusplus
extern "C" {
#endif

DIR * dce_opendir (const char *name);
DIR * dce_fdopendir (int fd);
struct dirent * dce_readdir (DIR *dirp);
int dce_readdir_r (DIR *dirp, struct dirent *entry, struct dirent **result);
int dce_closedir (DIR *dirp);
int dce_dirfd (DIR *dirp);
void dce_rewinddir (DIR *dirp);
int dce_scandir (const char *dirp, struct dirent ***namelist,
                 int (*filter)(const struct dirent *),
                 int (*compar)(const struct dirent **, const struct dirent **));

#ifdef __cplusplus
}
#endif

namespace ns3 {
struct Thread;

int dce_internalClosedir (DIR *dirp, struct Thread *cur);

}

#endif /* SIMU_DIRENT_H */
