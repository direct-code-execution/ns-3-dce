/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c)
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
 */

#ifndef DCE_GLOBAL_VARIABLES_H
#define DCE_GLOBAL_VARIABLES_H

#ifdef __cplusplus
extern "C" {
#endif


struct DceGlobalVariables
{
  FILE **pstdin;
  FILE **pstdout;
  FILE **pstderr;
  char ***penvp;
  char **poptarg;
  int *poptind;
  int *popterr;
  int *poptopt;
  char **pprogname;
  char **pprogram_invocation_name;
  char **pprogram_invocation_short_name;
};

void dce_global_variables_setup (struct DceGlobalVariables *variables);


#ifdef __cplusplus
}
#endif

#endif /* DCE_GLOBAL_VARIABLES_H */
