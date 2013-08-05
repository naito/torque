#include "license_pbs.h" /* See here for the software license */
/*
 *
 *  qgpureset - Reset GPU EEC error counts on a MOM
 *
 * Authors:
 *      Al Taufer
 *      Adaptive Computing
 */

#include "cmds.h"
#include "net_cache.h"
#include <pbs_config.h>   /* the master config generated by configure */

int pbs_gpureset_err(int c, char *node, char *gpuid, int permanent, int vol, int *);
int exitstatus = 0; /* Exit Status */

static void execute(char *, char *, int, int, const char *);

/*  qgpureset */

int main(

  int    argc,
  char **argv)

  {
  int   c;
  int   errflg = 0;
  char *gpuid = NULL;
  int   ecc_perm = 0;
  int   ecc_vol = 0;

  char *mom_node = NULL;

#define GETOPT_ARGS "H:g:pv"

  while ((c = getopt(argc, argv, GETOPT_ARGS)) != EOF)
    {
    switch (c)
      {

      case 'H':

        if (strlen(optarg) == 0)
          {
          fprintf(stderr, "qgpureset: illegal -H value\n");
          errflg++;
          break;
          }

        mom_node = optarg;

        break;

      case 'g':

        if (strlen(optarg) == 0)
          {
          fprintf(stderr, " qgpureset: illegal -g value\n");
          errflg++;
          break;
          }

        gpuid = optarg;

        break;

      case 'p':

        ecc_perm = 1;

        break;

      case 'v':

        ecc_vol = 1;

        break;

      default:

        errflg++;

        break;
      }
    }    /* END while (c) */

  if (errflg || (mom_node == NULL) || (gpuid == NULL) ||
      ((ecc_perm == 1) && (ecc_vol == 1)))
    {
    static char usage[] = "usage:  qgpureset -H host -g gpuid -p -v\n";

    fprintf(stderr, "%s", usage);

    fprintf(stderr, "       -p and -v are mutually exclusive\n");

    exit(2);
    }
  else if ((ecc_perm == 0) && (ecc_vol == 0))
    {
    fprintf(stderr, "%s", "-p or -v is required\n");

    exit(2);
    }

  execute(mom_node, gpuid, ecc_perm, ecc_vol, "");

  exit(exitstatus);

  /*NOTREACHED*/

  return(0);
  }  /* END main() */

/*
 * void execute( char *node, char *gpuid, int ecc_perm, int ecc_vol, char *server )
 *
 * node     The name of the MOM node.
 * gpuid    The id of the GPU.
 * ecc_perm The value for resetting the permanent ECC count.
 * ecc_vol  The value for resetting the volatile ECC count.
 * server   The name of the server to send to.
 *
 * Returns:
 *  None
 *
 * File Variables:
 *  exitstatus  Set to two if an error occurs.
 */
static void execute(
    
  char *node,
  char *gpuid,
  int   ecc_perm,
  int   ecc_vol,
  const char *server)

  {
  int local_errno = 0;
  int ct;         /* Connection to the server */
  int merr;       /* Error return from pbs_manager */
  char *errmsg;   /* Error message from pbs_manager */

  /* The request to change mode */

  if ((ct = cnt2server(server)) > 0)
    {
    merr = pbs_gpureset_err(ct, node, gpuid, ecc_perm, ecc_vol, &local_errno);

    if (merr != 0)
      {
      errmsg = pbs_geterrmsg(ct);

      if (errmsg != NULL)
        {
        fprintf(stderr, " qgpureset: %s ", errmsg);

        free(errmsg);
        }
      else
        {
        fprintf(stderr, " qgpureset: Error (%d - %s) resetting GPU ECC counts",
                local_errno,
                pbs_strerror(local_errno));
        }

      if (notNULL(server))
        fprintf(stderr, "@%s", server);

      fprintf(stderr, "\n");

      exitstatus = 2;
      }

    pbs_disconnect(ct);
    }
  else
    {
    local_errno = -1 * ct;

    fprintf(stderr, " qgpureset: could not connect to server %s (%d) %s\n",
            server,
            local_errno,
            pbs_strerror(local_errno));

    exitstatus = 2;
    }
  }
