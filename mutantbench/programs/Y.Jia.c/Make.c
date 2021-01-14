/* modification by J. Ruthruff, 8/25 */
#define ALIASPATH "/usr/local/share/locale:."
#define LOCALEDIR "/usr/local/share/locale"
#define LIBDIR "/usr/local/lib"
#define INCLUDEDIR "/usr/local/include"
#define HAVE_CONFIG_H

#define inline

#undef stderr
#define stderr stdout

/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         ar.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Interface to `ar' archives for GNU Make.
Copyright (C) 1988,89,90,91,92,93,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "make.h"
#undef stderr
#define stderr stdout

#ifndef	NO_ARCHIVES

#include "filedef.h"
#include "dep.h"
#include <fnmatch.h>

#include <stat.h>

#undef stderr
#define stderr stdout

#define ino_t_int longlong_t
#define dev_t_int long
#define off_t_int longlong_t

ino_t_int ZERO_ino_t = 0;
dev_t_int ZERO_dev_t = 0;
off_t_int ZERO_off_t = 0;
off_t_int ZERO_L_off_t = 0L;

ino_t int_to_ino_t(long arg)
{
  ino_t_int x;
  ino_t y;
  memset(&x, '\0', sizeof(x));
  memcpy(&x, &arg, sizeof(arg));
  memcpy(&y, &x, sizeof(ino_t));
  return y;
}

dev_t int_to_dev_t(long x)
{
  dev_t y;
  memcpy(&y, &x, sizeof(dev_t));
  return y;
}

off_t int_to_off_t(long arg)
{
  off_t_int x;
  off_t y;
  memset(&x, '\0', sizeof(x));
  memcpy(&x, &arg, sizeof(arg));
  memcpy(&y, &x, sizeof(off_t));
  return y;
}

ino_t_int ino_t_to_int(ino_t x)
{
  ino_t_int y;
  memcpy(&y, &x, sizeof(ino_t));
  return y;
}

/* note: this is *not* a safe conversion, because long int's are
   4-bytes and ino_t's are 8-bytes, but I'm only mimicking what
   the developers do */
long int ino_t_to_long_int(ino_t x)
{
  long int y;
  memcpy(&y, &x, sizeof(ino_t));
  return y;
}

/* note: this is *not* a safe conversion, but I'm only mimicking 
   what the developers do */
long ino_t_to_long(ino_t x)
{
  long y;
  memcpy(&y, &x, sizeof(ino_t));
  return y;
}

/* note: this is *not* a safe conversion, but I'm only mimicking
   what the developers do */
unsigned int ino_t_to_unsigned_int(ino_t x)
{
  unsigned int y;
  memcpy(&y, &x, sizeof(ino_t));
  return y;
}

off_t_int off_t_to_int(off_t x)
{
  off_t_int y;
  memcpy(&y, &x, sizeof(off_t));
  return y;
}

dev_t_int dev_t_to_int(dev_t x)
{
  dev_t_int y;
  memcpy(&y, &x, sizeof(dev_t));
  return y;
}

size_t int_to_size_t(off_t_int x)
{
  size_t y;
  memcpy(&y, &x, sizeof(x));
  return y;
}

int ino_t_equal(ino_t_int x, ino_t_int y)
{
  return (memcmp(&x, &y, sizeof(ino_t_int)));
}

int off_t_equal(off_t_int x, off_t_int y)
{
  return (memcmp(&x, &y, sizeof(off_t_int)));
}

int dev_t_equal(dev_t_int x, dev_t_int y)
{
  return (memcmp(&x, &y, sizeof(dev_t_int)));
}

int size_t_equal(size_t x, off_t_int y)
{
  long int eight_byte_x[2];
  
  /* zero the high part, and then copy size_t into low part */
  memset(&eight_byte_x[0], '\0', sizeof(eight_byte_x[0]));
  memcpy(&eight_byte_x[1], '\0', sizeof(eight_byte_x[1]));
  
  return (memcmp(eight_byte_x, &y, sizeof(off_t_int)));
}


/* Defined in arscan.c.  */
extern long int ar_scan PARAMS ((char *archive, long int (*function) (), long int arg));
extern int ar_name_equal PARAMS ((char *name, char *mem, int truncated));
#ifndef VMS
extern int ar_member_touch PARAMS ((char *arname, char *memname));
#endif

/* Return nonzero if NAME is an archive-member reference, zero if not.
   An archive-member reference is a name like `lib(member)'.
   If a name like `lib((entry))' is used, a fatal error is signaled at
   the attempt to use this unsupported feature.  */

int ar_name (name)
     char *name;
{
  char *p = strchr (name, '('), *end = name + strlen (name) - 1;

  if (p == 0 || p == name || *end != ')')
    return 0;

  if (p[1] == '(' && end[-1] == ')')
    fatal (NILF, _("attempt to use unsupported feature: `%s'"), name);

  return 1;
}


/* Parse the archive-member reference NAME into the archive and member names.
   Put the malloc'd archive name in *ARNAME_P if ARNAME_P is non-nil;
   put the malloc'd member name in *MEMNAME_P if MEMNAME_P is non-nil.  */

void ar_parse_name (name, arname_p, memname_p)
     char *name, **arname_p, **memname_p;
{
  char *p = strchr (name, '('), *end = name + strlen (name) - 1;

  if (arname_p != 0)
    *arname_p = savestring (name, p - name);

  if (memname_p != 0)
    *memname_p = savestring (p + 1, end - (p + 1));
}

static long int ar_member_date_1 PARAMS ((int desc, char *mem, int truncated, long int hdrpos,
	long int datapos, long int size, long int date, int uid, int gid, int mode, char *name));

/* Return the modtime of NAME.  */

time_t
ar_member_date (name)
     char *name;
{
  char *arname;
  int arname_used = 0;
  char *memname;
  long int val;

  ar_parse_name (name, &arname, &memname);

  /* Make sure we know the modtime of the archive itself because we are
     likely to be called just before commands to remake a member are run,
     and they will change the archive itself.

     But we must be careful not to enter_file the archive itself if it does
     not exist, because pattern_search assumes that files found in the data
     base exist or can be made.  */
  {
    struct file *arfile;
    arfile = lookup_file (arname);
    if (arfile == 0 && file_exists_p (arname))
      {
	arfile = enter_file (arname);
	arname_used = 1;
      }

    if (arfile != 0)
      (void) f_mtime (arfile, 0);
  }

  val = ar_scan (arname, ar_member_date_1, (long int) memname);

  if (!arname_used)
    free (arname);
  free (memname);

  return (val <= 0 ? (time_t) -1 : (time_t) val);
}

/* This function is called by `ar_scan' to find which member to look at.  */

/* ARGSUSED */
static long int
ar_member_date_1 (desc, mem, truncated,
		  hdrpos, datapos, size, date, uid, gid, mode, name)
     int desc;
     char *mem;
     int truncated;
     long int hdrpos, datapos, size, date;
     int uid, gid, mode;
     char *name;
{
  return ar_name_equal (name, mem, truncated) ? date : 0;
}

/* Set the archive-member NAME's modtime to now.  */

#ifdef VMS
int
ar_touch (name)
     char *name;
{
  error (NILF, _("touch archive member is not available on VMS"));
  return -1;
}
#else
int
ar_touch (name)
     char *name;
{
  char *arname, *memname;
  int arname_used = 0;
  register int val;

  ar_parse_name (name, &arname, &memname);

  /* Make sure we know the modtime of the archive itself before we
     touch the member, since this will change the archive itself.  */
  {
    struct file *arfile;
    arfile = lookup_file (arname);
    if (arfile == 0)
      {
	arfile = enter_file (arname);
	arname_used = 1;
      }

    (void) f_mtime (arfile, 0);
  }

  val = 1;
  switch (ar_member_touch (arname, memname))
    {
    case -1:
      error (NILF, _("touch: Archive `%s' does not exist"), arname);
      break;
    case -2:
      error (NILF, _("touch: `%s' is not a valid archive"), arname);
      break;
    case -3:
      perror_with_name ("touch: ", arname);
      break;
    case 1:
      error (NILF,
             _("touch: Member `%s' does not exist in `%s'"), memname, arname);
      break;
    case 0:
      val = 0;
      break;
    default:
      error (NILF,
             _("touch: Bad return code from ar_member_touch on `%s'"), name);
    }

  if (!arname_used)
    free (arname);
  free (memname);

  return val;
}
#endif /* !VMS */

/* State of an `ar_glob' run, passed to `ar_glob_match'.  */

struct ar_glob_state
  {
    char *arname;
    char *pattern;
    unsigned int size;
    struct nameseq *chain;
    unsigned int n;
  };

/* This function is called by `ar_scan' to match one archive
   element against the pattern in STATE.  */

static long int
ar_glob_match (desc, mem, truncated,
	       hdrpos, datapos, size, date, uid, gid, mode,
	       state)
     int desc;
     char *mem;
     int truncated;
     long int hdrpos, datapos, size, date;
     int uid, gid, mode;
     struct ar_glob_state *state;
{
  if (fnmatch (state->pattern, mem, FNM_PATHNAME|FNM_PERIOD) == 0)
    {
      /* We have a match.  Add it to the chain.  */
      struct nameseq *new = (struct nameseq *) xmalloc (state->size);
      new->name = concat (state->arname, mem, ")");
      new->next = state->chain;
      state->chain = new;
      ++state->n;
    }

  return 0L;
}

/* Return nonzero if PATTERN contains any metacharacters.
   Metacharacters can be quoted with backslashes if QUOTE is nonzero.  */
static int
glob_pattern_p (pattern, quote)
     const char *pattern;
     int quote;
{
  register const char *p;
  int open = 0;

  for (p = pattern; *p != '\0'; ++p)
    switch (*p)
      {
      case '?':
      case '*':
	return 1;

      case '\\':
	if (quote)
	  ++p;
	break;

      case '[':
	open = 1;
	break;

      case ']':
	if (open)
	  return 1;
	break;
      }

  return 0;
}

/* Glob for MEMBER_PATTERN in archive ARNAME.
   Return a malloc'd chain of matching elements (or nil if none).  */

struct nameseq *
ar_glob (arname, member_pattern, size)
     char *arname, *member_pattern;
     unsigned int size;
{
  struct ar_glob_state state;
  char **names;
  struct nameseq *n;
  unsigned int i;

  if (! glob_pattern_p (member_pattern, 1))
    return 0;

  /* Scan the archive for matches.
     ar_glob_match will accumulate them in STATE.chain.  */
  i = strlen (arname);
  state.arname = (char *) alloca (i + 2);
  bcopy (arname, state.arname, i);
  state.arname[i] = '(';
  state.arname[i + 1] = '\0';
  state.pattern = member_pattern;
  state.size = size;
  state.chain = 0;
  state.n = 0;
  (void) ar_scan (arname, ar_glob_match, (long int) &state);

  if (state.chain == 0)
    return 0;

  /* Now put the names into a vector for sorting.  */
  names = (char **) alloca (state.n * sizeof (char *));
  i = 0;
  for (n = state.chain; n != 0; n = n->next)
    names[i++] = n->name;

  /* Sort them alphabetically.  */
  qsort ((char *) names, i, sizeof (*names), alpha_compare);

  /* Put them back into the chain in the sorted order.  */
  i = 0;
  for (n = state.chain; n != 0; n = n->next)
    n->name = names[i++];

  return state.chain;
}

#endif	/* Not NO_ARCHIVES.  */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         arscan.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Library function for scanning an archive file.
Copyright (C) 1987,89,91,92,93,94,95,97 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#undef stderr
#define stderr stdout
#else
#include <sys/file.h>
#undef stderr
#define stderr stdout
#endif

#ifndef	NO_ARCHIVES

#ifdef VMS
#include <lbrdef.h>
#include <mhddef.h>
#include <credef.h>
#include <descrip.h>
#include <ctype.h>
#undef stderr
#define stderr stdout
#if __DECC
#include <unixlib.h>
#include <lbr$routines.h>
#undef stderr
#define stderr stdout
#endif

static void *VMS_lib_idx;

static char *VMS_saved_memname;

static time_t VMS_member_date;

static long int (*VMS_function) ();

static int
VMS_get_member_info (module, rfa)
     struct dsc$descriptor_s *module;
     unsigned long *rfa;
{
  int status, i;
  long int fnval;

  time_t val;

  static struct dsc$descriptor_s bufdesc =
    { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

  struct mhddef *mhd;
  char filename[128];

  bufdesc.dsc$a_pointer = filename;
  bufdesc.dsc$w_length = sizeof (filename);

  status = lbr$set_module (&VMS_lib_idx, rfa, &bufdesc,
			   &bufdesc.dsc$w_length, 0);
  if (! status)
    {
      error (NILF, _("lbr$set_module failed to extract module info, status = %d"),
	     status);

      lbr$close (&VMS_lib_idx);

      return 0;
    }

  mhd = (struct mhddef *) filename;

#ifdef __DECC
  val = decc$fix_time (&mhd->mhd$l_datim);
#endif

  for (i = 0; i < module->dsc$w_length; i++)
    filename[i] = _tolower ((unsigned char)module->dsc$a_pointer[i]);

  filename[i] = '\0';

  VMS_member_date = (time_t) -1;

  fnval =
    (*VMS_function) (-1, filename, 0, 0, 0, 0, val, 0, 0, 0,
		     VMS_saved_memname);

  if (fnval)
    {
      VMS_member_date = fnval;
      return 0;
    }
  else
    return 1;
}

/* Takes three arguments ARCHIVE, FUNCTION and ARG.

   Open the archive named ARCHIVE, find its members one by one,
   and for each one call FUNCTION with the following arguments:
     archive file descriptor for reading the data,
     member name,
     member name might be truncated flag,
     member header position in file,
     member data position in file,
     member data size,
     member date,
     member uid,
     member gid,
     member protection mode,
     ARG.

   NOTE: on VMS systems, only name, date, and arg are meaningful!

   The descriptor is poised to read the data of the member
   when FUNCTION is called.  It does not matter how much
   data FUNCTION reads.

   If FUNCTION returns nonzero, we immediately return
   what FUNCTION returned.

   Returns -1 if archive does not exist,
   Returns -2 if archive has invalid format.
   Returns 0 if have scanned successfully.  */

long int
ar_scan (archive, function, arg)
     char *archive;
     long int (*function) ();
     long int arg;
{
  char *p;

  static struct dsc$descriptor_s libdesc =
    { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, NULL };

  unsigned long func = LBR$C_READ;
  unsigned long type = LBR$C_TYP_UNK;
  unsigned long index = 1;

  int status;

  status = lbr$ini_control (&VMS_lib_idx, &func, &type, 0);

  if (! status)
    {
      error (NILF, _("lbr$ini_control failed with status = %d"),status);
      return -2;
    }

  libdesc.dsc$a_pointer = archive;
  libdesc.dsc$w_length = strlen (archive);

  status = lbr$open (&VMS_lib_idx, &libdesc, 0, 0, 0, 0, 0);

  if (! status)
    {
      error (NILF, _("unable to open library `%s' to lookup member `%s'"),
	     archive, (char *)arg);
      return -1;
    }

  VMS_saved_memname = (char *)arg;

  /* For comparison, delete .obj from arg name.  */

  p = strrchr (VMS_saved_memname, '.');
  if (p)
    *p = '\0';

  VMS_function = function;

  VMS_member_date = (time_t) -1;
  lbr$get_index (&VMS_lib_idx, &index, VMS_get_member_info, 0);

  /* Undo the damage.  */
  if (p)
    *p = '.';

  lbr$close (&VMS_lib_idx);

  return VMS_member_date > 0 ? VMS_member_date : 0;
}

#else /* !VMS */

/* SCO Unix's compiler defines both of these.  */
#ifdef	M_UNIX
#undef	M_XENIX
#endif

/* On the sun386i and in System V rel 3, ar.h defines two different archive
   formats depending upon whether you have defined PORTAR (normal) or PORT5AR
   (System V Release 1).  There is no default, one or the other must be defined
   to have a nonzero value.  */

#if (!defined (PORTAR) || PORTAR == 0) && (!defined (PORT5AR) || PORT5AR == 0)
#undef	PORTAR
#ifdef M_XENIX
/* According to Jim Sievert <jas1@rsvl.unisys.com>, for SCO XENIX defining
   PORTAR to 1 gets the wrong archive format, and defining it to 0 gets the
   right one.  */
#define PORTAR 0
#else
#define PORTAR 1
#endif
#endif

/* On AIX, define these symbols to be sure to get both archive formats.
   AIX 4.3 introduced the "big" archive format to support 64-bit object
   files, so on AIX 4.3 systems we need to support both the "normal" and
   "big" archive formats.  An archive's format is indicated in the
   "fl_magic" field of the "FL_HDR" structure.  For a normal archive,
   this field will be the string defined by the AIAMAG symbol.  For a
   "big" archive, it will be the string defined by the AIAMAGBIG symbol
   (at least on AIX it works this way).

   Note: we'll define these symbols regardless of which AIX version
   we're compiling on, but this is okay since we'll use the new symbols
   only if they're present.  */
#ifdef _AIX
# define __AR_SMALL__
# define __AR_BIG__
#endif

#ifndef WINDOWS32
# include <ar.h>
#undef stderr
#define stderr stdout
#else
/* These should allow us to read Windows (VC++) libraries (according to Frank
 * Libbrecht <frankl@abzx.belgium.hp.com>)
 */
# include <windows.h>
# include <windef.h>
# include <io.h>
#undef stderr
#define stderr stdout
# define ARMAG      IMAGE_ARCHIVE_START
# define SARMAG     IMAGE_ARCHIVE_START_SIZE
# define ar_hdr     _IMAGE_ARCHIVE_MEMBER_HEADER
# define ar_name    Name
# define ar_mode    Mode
# define ar_size    Size
# define ar_date    Date
# define ar_uid     UserID
# define ar_gid     GroupID
#endif

/* Cray's <ar.h> apparently defines this.  */
#ifndef	AR_HDR_SIZE
# define   AR_HDR_SIZE	(sizeof (struct ar_hdr))
#endif

/* Takes three arguments ARCHIVE, FUNCTION and ARG.

   Open the archive named ARCHIVE, find its members one by one,
   and for each one call FUNCTION with the following arguments:
     archive file descriptor for reading the data,
     member name,
     member name might be truncated flag,
     member header position in file,
     member data position in file,
     member data size,
     member date,
     member uid,
     member gid,
     member protection mode,
     ARG.

   The descriptor is poised to read the data of the member
   when FUNCTION is called.  It does not matter how much
   data FUNCTION reads.

   If FUNCTION returns nonzero, we immediately return
   what FUNCTION returned.

   Returns -1 if archive does not exist,
   Returns -2 if archive has invalid format.
   Returns 0 if have scanned successfully.  */

long int
ar_scan (archive, function, arg)
     char *archive;
     long int (*function) ();
     long int arg;
{
#ifdef AIAMAG
  FL_HDR fl_header;
#ifdef AIAMAGBIG
  int big_archive = 0;
  FL_HDR_BIG fl_header_big;
#endif
#else
  int long_name = 0;
#endif
  char *namemap = 0;
  register int desc = open (archive, O_RDONLY, 0);
  if (desc < 0)
    return -1;
#ifdef SARMAG
  {
    char buf[SARMAG];
    register int nread = read (desc, buf, SARMAG);
    if (nread != SARMAG || bcmp (buf, ARMAG, SARMAG))
      {
	(void) close (desc);
	return -2;
      }
  }
#else
#ifdef AIAMAG
  {
    register int nread = read (desc, (char *) &fl_header, FL_HSZ);

    if (nread != FL_HSZ)
      {
	(void) close (desc);
	return -2;
      }
#ifdef AIAMAGBIG
    /* If this is a "big" archive, then set the flag and
       re-read the header into the "big" structure. */
    if (!bcmp (fl_header.fl_magic, AIAMAGBIG, SAIAMAG))
      {
	big_archive = 1;

	/* seek back to beginning of archive */
	if (off_t_equal(off_t_to_int(lseek (desc, int_to_off_t(0), 0)), ZERO_off_t) < 0)
	  /*	if (lseek (desc, 0, 0) < 0) */
	  {
	    (void) close (desc);
	    return -2;
	  }

	/* re-read the header into the "big" structure */
	nread = read (desc, (char *) &fl_header_big, FL_HSZ_BIG);
	if (nread != FL_HSZ_BIG)
	  {
	    (void) close (desc);
	    return -2;
	  }
      }
    else
#endif
       /* Check to make sure this is a "normal" archive. */
      if (bcmp (fl_header.fl_magic, AIAMAG, SAIAMAG))
	{
          (void) close (desc);
          return -2;
	}
  }
#else
  {
#ifndef M_XENIX
    int buf;
#else
    unsigned short int buf;
#endif
    register int nread = read(desc, &buf, sizeof (buf));
    if (nread != sizeof (buf) || buf != ARMAG)
      {
	(void) close (desc);
	return -2;
      }
  }
#endif
#endif

  /* Now find the members one by one.  */
  {
#ifdef SARMAG
    register long int member_offset = SARMAG;
#else
#ifdef AIAMAG
    long int member_offset;
    long int last_member_offset;

#ifdef AIAMAGBIG
    if ( big_archive )
      {
	sscanf (fl_header_big.fl_fstmoff, "%20ld", &member_offset);
	sscanf (fl_header_big.fl_lstmoff, "%20ld", &last_member_offset);
      }
    else
#endif
      {
	sscanf (fl_header.fl_fstmoff, "%12ld", &member_offset);
	sscanf (fl_header.fl_lstmoff, "%12ld", &last_member_offset);
      }

    if (member_offset == 0)
      {
	/* Empty archive.  */
	close (desc);
	return 0;
      }
#else
#ifndef	M_XENIX
    register long int member_offset = sizeof (int);
#else	/* Xenix.  */
    register long int member_offset = sizeof (unsigned short int);
#endif	/* Not Xenix.  */
#endif
#endif

    while (1)
      {
	register int nread;
	struct ar_hdr member_header;
#ifdef AIAMAGBIG
	struct ar_hdr_big member_header_big;
#endif
#ifdef AIAMAG
	char name[256];
	int name_len;
	long int dateval;
	int uidval, gidval;
	long int data_offset;
#else
	char namebuf[sizeof member_header.ar_name + 1];
	char *name;
	int is_namemap;		/* Nonzero if this entry maps long names.  */
#endif
	long int eltsize;
	int eltmode;
	long int fnval;

	if (off_t_equal(off_t_to_int(lseek (desc, int_to_off_t(member_offset), 0)), ZERO_off_t) < 0)
	  /*	if (lseek (desc, member_offset, 0) < 0) */
	  {
	    (void) close (desc);
	    return -2;
	  }

#ifdef AIAMAG
#define       AR_MEMHDR_SZ(x) (sizeof(x) - sizeof (x._ar_name))

#ifdef AIAMAGBIG
	if (big_archive)
	  {
	    nread = read (desc, (char *) &member_header_big,
			  AR_MEMHDR_SZ(member_header_big) );

	    if (nread != AR_MEMHDR_SZ(member_header_big))
	      {
		(void) close (desc);
		return -2;
	      }

	    sscanf (member_header_big.ar_namlen, "%4d", &name_len);
	    nread = read (desc, name, name_len);

	    if (nread != name_len)
	      {
		(void) close (desc);
		return -2;
	      }

	    name[name_len] = 0;

	    sscanf (member_header_big.ar_date, "%12ld", &dateval);
	    sscanf (member_header_big.ar_uid, "%12d", &uidval);
	    sscanf (member_header_big.ar_gid, "%12d", &gidval);
	    sscanf (member_header_big.ar_mode, "%12o", &eltmode);
	    sscanf (member_header_big.ar_size, "%20ld", &eltsize);

	    data_offset = (member_offset + AR_MEMHDR_SZ(member_header_big)
			   + name_len + 2);
	  }
	else
#endif
	  {
	    nread = read (desc, (char *) &member_header,
			  AR_MEMHDR_SZ(member_header) );

	    if (nread != AR_MEMHDR_SZ(member_header))
	      {
		(void) close (desc);
		return -2;
	      }

	    sscanf (member_header.ar_namlen, "%4d", &name_len);
	    nread = read (desc, name, name_len);

	    if (nread != name_len)
	      {
		(void) close (desc);
		return -2;
	      }

	    name[name_len] = 0;

	    sscanf (member_header.ar_date, "%12ld", &dateval);
	    sscanf (member_header.ar_uid, "%12d", &uidval);
	    sscanf (member_header.ar_gid, "%12d", &gidval);
	    sscanf (member_header.ar_mode, "%12o", &eltmode);
	    sscanf (member_header.ar_size, "%12ld", &eltsize);

	    data_offset = (member_offset + AR_MEMHDR_SZ(member_header)
			   + name_len + 2);
	  }
	data_offset += data_offset % 2;

	fnval =
	  (*function) (desc, name, 0,
		       member_offset, data_offset, eltsize,
		       dateval, uidval, gidval,
		       eltmode, arg);

#else	/* Not AIAMAG.  */
	nread = read (desc, (char *) &member_header, AR_HDR_SIZE);
	if (nread == 0)
	  /* No data left means end of file; that is OK.  */
	  break;

	if (nread != AR_HDR_SIZE
#if defined(ARFMAG) || defined(ARFZMAG)
	    || (
# ifdef ARFMAG
                bcmp (member_header.ar_fmag, ARFMAG, 2)
# else
                1
# endif
                &&
# ifdef ARFZMAG
                bcmp (member_header.ar_fmag, ARFZMAG, 2)
# else
                1
# endif
               )
#endif
	    )
	  {
	    (void) close (desc);
	    return -2;
	  }

	name = namebuf;
	bcopy (member_header.ar_name, name, sizeof member_header.ar_name);
	{
	  register char *p = name + sizeof member_header.ar_name;
	  do
	    *p = '\0';
	  while (p > name && *--p == ' ');

#ifndef AIAMAG
	  /* If the member name is "//" or "ARFILENAMES/" this may be
	     a list of file name mappings.  The maximum file name
 	     length supported by the standard archive format is 14
 	     characters.  This member will actually always be the
 	     first or second entry in the archive, but we don't check
 	     that.  */
 	  is_namemap = (!strcmp (name, "//")
			|| !strcmp (name, "ARFILENAMES/"));
#endif	/* Not AIAMAG. */
	  /* On some systems, there is a slash after each member name.  */
	  if (*p == '/')
	    *p = '\0';

#ifndef AIAMAG
 	  /* If the member name starts with a space or a slash, this
 	     is an index into the file name mappings (used by GNU ar).
 	     Otherwise if the member name looks like #1/NUMBER the
 	     real member name appears in the element data (used by
 	     4.4BSD).  */
 	  if (! is_namemap
 	      && (name[0] == ' ' || name[0] == '/')
 	      && namemap != 0)
	    {
	      name = namemap + atoi (name + 1);
	      long_name = 1;
	    }
 	  else if (name[0] == '#'
 		   && name[1] == '1'
 		   && name[2] == '/')
 	    {
 	      int namesize = atoi (name + 3);

 	      name = (char *) alloca (namesize + 1);
 	      nread = read (desc, name, namesize);
 	      if (nread != namesize)
 		{
 		  close (desc);
 		  return -2;
 		}
 	      name[namesize] = '\0';

	      long_name = 1;
 	    }
#endif /* Not AIAMAG. */
	}

#ifndef	M_XENIX
	sscanf (member_header.ar_mode, "%o", &eltmode);
	eltsize = atol (member_header.ar_size);
#else	/* Xenix.  */
	eltmode = (unsigned short int) member_header.ar_mode;
	eltsize = member_header.ar_size;
#endif	/* Not Xenix.  */

	fnval =
	  (*function) (desc, name, ! long_name, member_offset,
		       member_offset + AR_HDR_SIZE, eltsize,
#ifndef	M_XENIX
		       atol (member_header.ar_date),
		       atoi (member_header.ar_uid),
		       atoi (member_header.ar_gid),
#else	/* Xenix.  */
		       member_header.ar_date,
		       member_header.ar_uid,
		       member_header.ar_gid,
#endif	/* Not Xenix.  */
		       eltmode, arg);

#endif  /* AIAMAG.  */

	if (fnval)
	  {
	    (void) close (desc);
	    return fnval;
	  }

#ifdef AIAMAG
	if (member_offset == last_member_offset)
	  /* End of the chain.  */
	  break;

#ifdef AIAMAGBIG
	if (big_archive)
          sscanf (member_header_big.ar_nxtmem, "%20ld", &member_offset);
	else
#endif
	  sscanf (member_header.ar_nxtmem, "%12ld", &member_offset);

	if (off_t_to_int(lseek (desc, int_to_off_t(member_offset), 0)) != member_offset)
	  /*	if (lseek (desc, member_offset, 0) != member_offset) */
	  {
	    (void) close (desc);
	    return -2;
	  }
#else

 	/* If this member maps archive names, we must read it in.  The
 	   name map will always precede any members whose names must
 	   be mapped.  */
	if (is_namemap)
 	  {
 	    char *clear;
 	    char *limit;

 	    namemap = (char *) alloca (eltsize);
 	    nread = read (desc, namemap, eltsize);
 	    if (nread != eltsize)
 	      {
 		(void) close (desc);
 		return -2;
 	      }

 	    /* The names are separated by newlines.  Some formats have
 	       a trailing slash.  Null terminate the strings for
 	       convenience.  */
 	    limit = namemap + eltsize;
 	    for (clear = namemap; clear < limit; clear++)
 	      {
 		if (*clear == '\n')
 		  {
 		    *clear = '\0';
 		    if (clear[-1] == '/')
 		      clear[-1] = '\0';
 		  }
 	      }

	    is_namemap = 0;
 	  }

	member_offset += AR_HDR_SIZE + eltsize;
	if (member_offset % 2 != 0)
	  member_offset++;
#endif
      }
  }

  close (desc);
  return 0;
}
#endif /* !VMS */

/* Return nonzero iff NAME matches MEM.
   If TRUNCATED is nonzero, MEM may be truncated to
   sizeof (struct ar_hdr.ar_name) - 1.  */

int
ar_name_equal (name, mem, truncated)
     char *name, *mem;
     int truncated;
{
  char *p;

  p = strrchr (name, '/');
  if (p != 0)
    name = p + 1;

#ifndef VMS
  if (truncated)
    {
#ifdef AIAMAG
      /* TRUNCATED should never be set on this system.  */
      abort ();
#else
      struct ar_hdr hdr;
#if !defined (__hpux) && !defined (cray)
      return strneq (name, mem, sizeof(hdr.ar_name) - 1);
#else
      return strneq (name, mem, sizeof(hdr.ar_name) - 2);
#endif /* !__hpux && !cray */
#endif /* !AIAMAG */
    }
#endif /* !VMS */

  return !strcmp (name, mem);
}

#ifndef VMS
/* ARGSUSED */
static long int
ar_member_pos (desc, mem, truncated,
	       hdrpos, datapos, size, date, uid, gid, mode, name)
     int desc;
     char *mem;
     int truncated;
     long int hdrpos, datapos, size, date;
     int uid, gid, mode;
     char *name;
{
  if (!ar_name_equal (name, mem, truncated))
    return 0;
  return hdrpos;
}

/* Set date of member MEMNAME in archive ARNAME to current time.
   Returns 0 if successful,
   -1 if file ARNAME does not exist,
   -2 if not a valid archive,
   -3 if other random system call error (including file read-only),
   1 if valid but member MEMNAME does not exist.  */

int
ar_member_touch (arname, memname)
     char *arname, *memname;
{
  register long int pos = ar_scan (arname, ar_member_pos, (long int) memname);
  register int fd;
  struct ar_hdr ar_hdr;
  register int i;
  struct stat statbuf;

  if (pos < 0)
    return (int) pos;
  if (!pos)
    return 1;

  fd = open (arname, O_RDWR, 0666);
  if (fd < 0)
    return -3;
  /* Read in this member's header */
  if (off_t_equal(off_t_to_int(lseek (fd, int_to_off_t(pos), 0)), ZERO_off_t) < 0)
    /*  if (lseek (fd, pos, 0) < 0) */
    goto lose;
  if (AR_HDR_SIZE != read (fd, (char *) &ar_hdr, AR_HDR_SIZE))
    goto lose;
  /* Write back the header, thus touching the archive file.  */
  if (off_t_equal(off_t_to_int(lseek (fd, int_to_off_t(pos), 0)), ZERO_off_t) < 0)
    /*  if (lseek (fd, pos, 0) < 0) */
  if (off_t_equal(off_t_to_int(lseek (fd, int_to_off_t(pos), 0)), ZERO_off_t) < 0)
      /*  if (lseek (fd, pos, 0) < 0) */
    goto lose;
  if (AR_HDR_SIZE != write (fd, (char *) &ar_hdr, AR_HDR_SIZE))
    goto lose;
  /* The file's mtime is the time we we want.  */
  while (fstat (fd, &statbuf) < 0 && EINTR_SET)
    ;
#if defined(ARFMAG) || defined(ARFZMAG) || defined(AIAMAG) || defined(WINDOWS32)
  /* Advance member's time to that time */
  for (i = 0; i < sizeof ar_hdr.ar_date; i++)
    ar_hdr.ar_date[i] = ' ';
  sprintf (ar_hdr.ar_date, "%ld", (long int) statbuf.st_mtime);
#ifdef AIAMAG
  ar_hdr.ar_date[strlen(ar_hdr.ar_date)] = ' ';
#endif
#else
  ar_hdr.ar_date = statbuf.st_mtime;
#endif
  /* Write back this member's header */
  if (off_t_equal(off_t_to_int(lseek (fd, int_to_off_t(pos), 0)), ZERO_off_t) < 0)
      /*  if (lseek (fd, pos, 0) < 0) */
    goto lose;
  if (AR_HDR_SIZE != write (fd, (char *) &ar_hdr, AR_HDR_SIZE))
    goto lose;
  close (fd);
  return 0;

 lose:
  i = errno;
  close (fd);
  errno = i;
  return -3;
}
#endif

#ifdef TEST

long int
describe_member (desc, name, truncated,
		 hdrpos, datapos, size, date, uid, gid, mode)
     int desc;
     char *name;
     int truncated;
     long int hdrpos, datapos, size, date;
     int uid, gid, mode;
{
  extern char *ctime ();

  printf (_("Member `%s'%s: %ld bytes at %ld (%ld).\n"),
	  name, truncated ? _(" (name might be truncated)") : "",
	  size, hdrpos, datapos);
  printf (_("  Date %s"), ctime (&date));
  printf (_("  uid = %d, gid = %d, mode = 0%o.\n"), uid, gid, mode);

  return 0;
}

main (argc, argv)
     int argc;
     char **argv;
{
  ar_scan (argv[1], describe_member);
  return 0;
}

#endif	/* TEST.  */

#endif	/* NO_ARCHIVES.  */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         commands.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Command processing for GNU Make.
Copyright (C) 1988,89,91,92,93,94,95,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "variable.h"
#include "job.h"
#include "commands.h"
#undef stderr
#define stderr stdout

extern int remote_kill PARAMS ((int id, int sig));

#ifndef	HAVE_UNISTD_H
extern int getpid ();
#endif

/* Set FILE's automatic variables up.  */

static void
set_file_variables (file)
     register struct file *file;
{
  register char *p;
  char *at, *percent, *star, *less;

#ifndef	NO_ARCHIVES
  /* If the target is an archive member `lib(member)',
     then $@ is `lib' and $% is `member'.  */

  if (ar_name (file->name))
    {
      unsigned int len;
      p = strchr (file->name, '(');
      at = (char *) alloca (p - file->name + 1);
      bcopy (file->name, at, p - file->name);
      at[p - file->name] = '\0';
      len = strlen (p + 1);
      percent = (char *) alloca (len);
      bcopy (p + 1, percent, len - 1);
      percent[len - 1] = '\0';
    }
  else
#endif	/* NO_ARCHIVES.  */
    {
      at = file->name;
      percent = "";
    }

  /* $* is the stem from an implicit or static pattern rule.  */
  if (file->stem == 0)
    {
      /* In Unix make, $* is set to the target name with
	 any suffix in the .SUFFIXES list stripped off for
	 explicit rules.  We store this in the `stem' member.  */
      register struct dep *d;
      char *name;
      unsigned int len;

#ifndef	NO_ARCHIVES
      if (ar_name (file->name))
	{
	  name = strchr (file->name, '(') + 1;
	  len = strlen (name) - 1;
	}
      else
#endif
	{
	  name = file->name;
	  len = strlen (name);
	}

      for (d = enter_file (".SUFFIXES")->deps; d != 0; d = d->next)
	{
	  unsigned int slen = strlen (dep_name (d));
	  if (len > slen && strneq (dep_name (d), name + (len - slen), slen))
	    {
	      file->stem = savestring (name, len - slen);
	      break;
	    }
	}
      if (d == 0)
	file->stem = "";
    }
  star = file->stem;

  /* $< is the first dependency.  */
  less = file->deps != 0 ? dep_name (file->deps) : "";

  if (file->cmds == default_file->cmds)
    /* This file got its commands from .DEFAULT.
       In this case $< is the same as $@.  */
    less = at;

#define	DEFINE_VARIABLE(name, len, value) \
  (void) define_variable_for_file (name,len,value,o_automatic,0,file)

  /* Define the variables.  */

  DEFINE_VARIABLE ("<", 1, less);
  DEFINE_VARIABLE ("*", 1, star);
  DEFINE_VARIABLE ("@", 1, at);
  DEFINE_VARIABLE ("%", 1, percent);

  /* Compute the values for $^, $+, and $?.  */

  {
    register unsigned int qmark_len, plus_len;
    char *caret_value, *plus_value;
    register char *cp;
    char *qmark_value;
    register char *qp;
    register struct dep *d;
    unsigned int len;

    /* Compute first the value for $+, which is supposed to contain
       duplicate dependencies as they were listed in the makefile.  */

    plus_len = 0;
    for (d = file->deps; d != 0; d = d->next)
      plus_len += strlen (dep_name (d)) + 1;

    len = plus_len == 0 ? 1 : plus_len;
    cp = plus_value = (char *) alloca (len);

    qmark_len = plus_len;	/* Will be this or less.  */
    for (d = file->deps; d != 0; d = d->next)
      {
	char *c = dep_name (d);

#ifndef	NO_ARCHIVES
	if (ar_name (c))
	  {
	    c = strchr (c, '(') + 1;
	    len = strlen (c) - 1;
	  }
	else
#endif
	  len = strlen (c);

	bcopy (c, cp, len);
	cp += len;
#if VMS
        *cp++ = ',';
#else
	*cp++ = ' ';
#endif
	if (! d->changed)
	  qmark_len -= len + 1;	/* Don't space in $? for this one.  */
      }

    /* Kill the last space and define the variable.  */

    cp[cp > plus_value ? -1 : 0] = '\0';
    DEFINE_VARIABLE ("+", 1, plus_value);

    /* Make sure that no dependencies are repeated.  This does not
       really matter for the purpose of updating targets, but it
       might make some names be listed twice for $^ and $?.  */

    uniquize_deps (file->deps);

    /* Compute the values for $^ and $?.  */

    cp = caret_value = plus_value; /* Reuse the buffer; it's big enough.  */
    len = qmark_len == 0 ? 1 : qmark_len;
    qp = qmark_value = (char *) alloca (len);

    for (d = file->deps; d != 0; d = d->next)
      {
	char *c = dep_name (d);

#ifndef	NO_ARCHIVES
	if (ar_name (c))
	  {
	    c = strchr (c, '(') + 1;
	    len = strlen (c) - 1;
	  }
	else
#endif
	  len = strlen (c);

	bcopy (c, cp, len);
	cp += len;
#if VMS
	*cp++ = ',';
#else
	*cp++ = ' ';
#endif
	if (d->changed)
	  {
	    bcopy (c, qp, len);
	    qp += len;
#if VMS
	    *qp++ = ',';
#else
	    *qp++ = ' ';
#endif
	  }
      }

    /* Kill the last spaces and define the variables.  */

    cp[cp > caret_value ? -1 : 0] = '\0';
    DEFINE_VARIABLE ("^", 1, caret_value);

    qp[qp > qmark_value ? -1 : 0] = '\0';
    DEFINE_VARIABLE ("?", 1, qmark_value);
  }

#undef	DEFINE_VARIABLE
}

/* Chop CMDS up into individual command lines if necessary.
   Also set the `lines_flag' and `any_recurse' members.  */

void
chop_commands (cmds)
     register struct commands *cmds;
{
  register char *p;
  unsigned int nlines, idx;
  char **lines;

  /* If we don't have any commands,
     or we already parsed them, never mind.  */

  if (!cmds || cmds->command_lines != 0)
    return;

  /* Chop CMDS->commands up into lines in CMDS->command_lines.
	 Also set the corresponding CMDS->lines_flags elements,
	 and the CMDS->any_recurse flag.  */

  nlines = 5;
  lines = (char **) xmalloc (5 * sizeof (char *));
  idx = 0;
  p = cmds->commands;
  while (*p != '\0')
    {
      char *end = p;
    find_end:;
      end = strchr (end, '\n');
      if (end == 0)
        end = p + strlen (p);
      else if (end > p && end[-1] == '\\')
        {
          int backslash = 1;
          register char *b;
          for (b = end - 2; b >= p && *b == '\\'; --b)
            backslash = !backslash;
          if (backslash)
            {
              ++end;
              goto find_end;
            }
        }

      if (idx == nlines)
        {
          nlines += 2;
          lines = (char **) xrealloc ((char *) lines,
                                      nlines * sizeof (char *));
        }
      lines[idx++] = savestring (p, end - p);
      p = end;
      if (*p != '\0')
        ++p;
    }

  if (idx != nlines)
    {
      nlines = idx;
      lines = (char **) xrealloc ((char *) lines,
                                  nlines * sizeof (char *));
    }

  cmds->ncommand_lines = nlines;
  cmds->command_lines = lines;

  cmds->any_recurse = 0;
  cmds->lines_flags = (char *) xmalloc (nlines);
  for (idx = 0; idx < nlines; ++idx)
    {
      int flags = 0;

      for (p = lines[idx];
           isblank (*p) || *p == '-' || *p == '@' || *p == '+';
           ++p)
        switch (*p)
          {
          case '+':
            flags |= COMMANDS_RECURSE;
            break;
          case '@':
            flags |= COMMANDS_SILENT;
            break;
          case '-':
            flags |= COMMANDS_NOERROR;
            break;
          }
      if (!(flags & COMMANDS_RECURSE))
        {
          unsigned int len = strlen (p);
          if (sindex (p, len, "$(MAKE)", 7) != 0
              || sindex (p, len, "${MAKE}", 7) != 0)
            flags |= COMMANDS_RECURSE;
        }

      cmds->lines_flags[idx] = flags;
      cmds->any_recurse |= flags & COMMANDS_RECURSE;
    }
}

/* Execute the commands to remake FILE.  If they are currently executing,
   return or have already finished executing, just return.  Otherwise,
   fork off a child process to run the first command line in the sequence.  */

void
execute_file_commands (file)
     struct file *file;
{
  register char *p;

  /* Don't go through all the preparations if
     the commands are nothing but whitespace.  */

  for (p = file->cmds->commands; *p != '\0'; ++p)
    if (!isspace ((unsigned char)*p) && *p != '-' && *p != '@')
      break;
  if (*p == '\0')
    {
      /* If there are no commands, assume everything worked.  */
      set_command_state (file, cs_running);
      file->update_status = 0;
      notice_finished_file (file);
      return;
    }

  /* First set the automatic variables according to this file.  */

  initialize_file_variables (file, 0);

  set_file_variables (file);

  /* Start the commands running.  */
  new_job (file);
}

/* This is set while we are inside fatal_error_signal,
   so things can avoid nonreentrant operations.  */

int handling_fatal_signal = 0;

/* Handle fatal signals.  */

RETSIGTYPE
fatal_error_signal (sig)
     int sig;
{
#ifdef __MSDOS__
  extern int dos_status, dos_command_running;

  if (dos_command_running)
    {
      /* That was the child who got the signal, not us.  */
      dos_status |= (sig << 8);
      return;
    }
  remove_intermediates (1);
  exit (EXIT_FAILURE);
#else /* not __MSDOS__ */
#ifdef _AMIGA
  remove_intermediates (1);
  if (sig == SIGINT)
     fputs (_("*** Break.\n"), stderr);

  exit (10);
#else /* not Amiga */
  handling_fatal_signal = 1;

  /* Set the handling for this signal to the default.
     It is blocked now while we run this handler.  */
  signal (sig, SIG_DFL);

  /* A termination signal won't be sent to the entire
     process group, but it means we want to kill the children.  */

  if (sig == SIGTERM)
    {
      register struct child *c;
      for (c = children; c != 0; c = c->next)
	if (!c->remote)
	  (void) kill (c->pid, SIGTERM);
    }

  /* If we got a signal that means the user
     wanted to kill make, remove pending targets.  */

  if (sig == SIGTERM || sig == SIGINT
#ifdef SIGHUP
    || sig == SIGHUP
#endif
#ifdef SIGQUIT
    || sig == SIGQUIT
#endif
    )
    {
      register struct child *c;

      /* Remote children won't automatically get signals sent
	 to the process group, so we must send them.  */
      for (c = children; c != 0; c = c->next)
	if (c->remote)
	  (void) remote_kill (c->pid, sig);

      for (c = children; c != 0; c = c->next)
	delete_child_targets (c);

      /* Clean up the children.  We don't just use the call below because
	 we don't want to print the "Waiting for children" message.  */
      while (job_slots_used > 0)
	reap_children (1, 0);
    }
  else
    /* Wait for our children to die.  */
    while (job_slots_used > 0)
      reap_children (1, 1);

  /* Delete any non-precious intermediate files that were made.  */

  remove_intermediates (1);

#ifdef SIGQUIT
  if (sig == SIGQUIT)
    /* We don't want to send ourselves SIGQUIT, because it will
       cause a core dump.  Just exit instead.  */
    exit (EXIT_FAILURE);
#endif

  /* Signal the same code; this time it will really be fatal.  The signal
     will be unblocked when we return and arrive then to kill us.  */
  if (kill (getpid (), sig) < 0)
    pfatal_with_name ("kill");
#endif /* not Amiga */
#endif /* not __MSDOS__  */
}

/* Delete FILE unless it's precious or not actually a file (phony),
   and it has changed on disk since we last stat'd it.  */

static void
delete_target (file, on_behalf_of)
     struct file *file;
     char *on_behalf_of;
{
  struct stat st;

  if (file->precious || file->phony)
    return;

#ifndef NO_ARCHIVES
  if (ar_name (file->name))
    {
      if (ar_member_date (file->name) != FILE_TIMESTAMP_S (file->last_mtime))
	{
	  if (on_behalf_of)
	    error (NILF, _("*** [%s] Archive member `%s' may be bogus; not deleted"),
		   on_behalf_of, file->name);
	  else
	    error (NILF, _("*** Archive member `%s' may be bogus; not deleted"),
		   file->name);
	}
      return;
    }
#endif

  if (stat (file->name, &st) == 0
      && S_ISREG (st.st_mode)
      && FILE_TIMESTAMP_STAT_MODTIME (st) != file->last_mtime)
    {
      if (on_behalf_of)
	error (NILF, _("*** [%s] Deleting file `%s'"), on_behalf_of, file->name);
      else
	error (NILF, _("*** Deleting file `%s'"), file->name);
      if (unlink (file->name) < 0
	  && errno != ENOENT)	/* It disappeared; so what.  */
	perror_with_name ("unlink: ", file->name);
    }
}


/* Delete all non-precious targets of CHILD unless they were already deleted.
   Set the flag in CHILD to say they've been deleted.  */

void
delete_child_targets (child)
     struct child *child;
{
  struct dep *d;

  if (child->deleted)
    return;

  /* Delete the target file if it changed.  */
  delete_target (child->file, (char *) 0);

  /* Also remove any non-precious targets listed in the `also_make' member.  */
  for (d = child->file->also_make; d != 0; d = d->next)
    delete_target (d->file, child->file->name);

  child->deleted = 1;
}

/* Print out the commands in CMDS.  */

void
print_commands (cmds)
     register struct commands *cmds;
{
  register char *s;

  fputs (_("#  commands to execute"), stdout);

  if (cmds->fileinfo.filenm == 0)
    puts (_(" (built-in):"));
  else
    printf (_(" (from `%s', line %lu):\n"),
            cmds->fileinfo.filenm, cmds->fileinfo.lineno);

  s = cmds->commands;
  while (*s != '\0')
    {
      char *end;

      while (isspace ((unsigned char)*s))
	++s;

      end = strchr (s, '\n');
      if (end == 0)
	end = s + strlen (s);

      printf ("\t%.*s\n", (int) (end - s), s);

      s = end;
    }
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         dir.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Directory hashing for GNU Make.
Copyright (C) 1988,89,91,92,93,94,95,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout

#ifdef	HAVE_DIRENT_H
# include <dirent.h>
#undef stderr
#define stderr stdout
# define NAMLEN(dirent) strlen((dirent)->d_name)
# ifdef VMS
extern char *vmsify PARAMS ((char *name, int type));
# endif
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
#undef stderr
#define stderr stdout
# endif
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
#undef stderr
#define stderr stdout
# endif
# ifdef HAVE_NDIR_H
#  include <ndir.h>
#undef stderr
#define stderr stdout
# endif
# ifdef HAVE_VMSDIR_H
#  include "vmsdir.h"
#undef stderr
#define stderr stdout
# endif /* HAVE_VMSDIR_H */
#endif

/* In GNU systems, <dirent.h> defines this macro for us.  */
#ifdef _D_NAMLEN
# undef NAMLEN
# define NAMLEN(d) _D_NAMLEN(d)
#endif

#if (defined (POSIX) || defined (VMS) || defined (WINDOWS32)) && !defined (__GNU_LIBRARY__)
/* Posix does not require that the d_ino field be present, and some
   systems do not provide it. */
# define REAL_DIR_ENTRY(dp) 1
# define FAKE_DIR_ENTRY(dp)
#else
# define REAL_DIR_ENTRY(dp) (dp->d_ino != 0)
# define FAKE_DIR_ENTRY(dp) (dp->d_ino = 1)
#endif /* POSIX */

#ifdef __MSDOS__
#include <ctype.h>
#include <fcntl.h>
#undef stderr
#define stderr stdout

/* If it's MSDOS that doesn't have _USE_LFN, disable LFN support.  */
#ifndef _USE_LFN
#define _USE_LFN 0
#endif

static char *
dosify (filename)
     char *filename;
{
  static char dos_filename[14];
  char *df;
  int i;

  if (filename == 0 || _USE_LFN)
    return filename;

  /* FIXME: what about filenames which violate
     8+3 constraints, like "config.h.in", or ".emacs"?  */
  if (strpbrk (filename, "\"*+,;<=>?[\\]|") != 0)
    return filename;

  df = dos_filename;

  /* First, transform the name part.  */
  for (i = 0; *filename != '\0' && i < 8 && *filename != '.'; ++i)
    *df++ = tolower ((unsigned char)*filename++);

  /* Now skip to the next dot.  */
  while (*filename != '\0' && *filename != '.')
    ++filename;
  if (*filename != '\0')
    {
      *df++ = *filename++;
      for (i = 0; *filename != '\0' && i < 3 && *filename != '.'; ++i)
	*df++ = tolower ((unsigned char)*filename++);
    }

  /* Look for more dots.  */
  while (*filename != '\0' && *filename != '.')
    ++filename;
  if (*filename == '.')
    return filename;
  *df = 0;
  return dos_filename;
}
#endif /* __MSDOS__ */

#ifdef WINDOWS32
#include "pathstuff.h"
#undef stderr
#define stderr stdout
#endif

#ifdef _AMIGA
#include <ctype.h>
#undef stderr
#define stderr stdout
#endif

#ifdef HAVE_CASE_INSENSITIVE_FS
static char *
downcase (filename)
     char *filename;
{
#ifdef _AMIGA
  static char new_filename[136];
#else
  static char new_filename[PATH_MAX];
#endif
  char *df;
  int i;

  if (filename == 0)
    return 0;

  df = new_filename;

  /* First, transform the name part.  */
  for (i = 0; *filename != '\0'; ++i)
  {
    *df++ = tolower ((unsigned char)*filename);
    ++filename;
  }

  *df = 0;

  return new_filename;
}
#endif /* HAVE_CASE_INSENSITIVE_FS */

#ifdef VMS

static int
vms_hash (name)
    char *name;
{
  int h = 0;
  int g;

  while (*name)
    {
      h = (h << 4) + (isupper (*name) ? tolower (*name) : *name);
      name++;
      g = h & 0xf0000000;
      if (g)
	{
	  h = h ^ (g >> 24);
	  h = h ^ g;
	}
    }
  return h;
}

/* fake stat entry for a directory */
static int
vmsstat_dir (name, st)
    char *name;
    struct stat *st;
{
  char *s;
  int h;
  DIR *dir;

  dir = opendir (name);
  if (dir == 0)
    return -1;
  closedir (dir);
  s = strchr (name, ':');	/* find device */
  if (s)
    {
      *s++ = 0;
      st->st_dev = (char *)vms_hash (name);
      h = vms_hash (s);
      *(s-1) = ':';
    }
  else
    {
      st->st_dev = 0;
      s = name;
      h = vms_hash (s);
    }

  st->st_ino[0] = h & 0xff;
  st->st_ino[1] = h & 0xff00;
  st->st_ino[2] = h >> 16;

  return 0;
}
#endif /* VMS */

/* Hash table of directories.  */

#ifndef	DIRECTORY_BUCKETS
#define DIRECTORY_BUCKETS 199
#endif

struct directory_contents
  {
    struct directory_contents *next;

    dev_t dev;			/* Device and inode numbers of this dir.  */
#ifdef WINDOWS32
    /*
     * Inode means nothing on WINDOWS32. Even file key information is
     * unreliable because it is random per file open and undefined
     * for remote filesystems. The most unique attribute I can
     * come up with is the fully qualified name of the directory. Beware
     * though, this is also unreliable. I'm open to suggestion on a better
     * way to emulate inode.
     */
    char *path_key;
    int   mtime;        /* controls check for stale directory cache */
    int   fs_flags;     /* FS_FAT, FS_NTFS, ... */
#define FS_FAT      0x1
#define FS_NTFS     0x2
#define FS_UNKNOWN  0x4
#else
#ifdef VMS
    ino_t ino[3];
#else
    ino_t ino;
#endif
#endif /* WINDOWS32 */
    struct dirfile **files;	/* Files in this directory.  */
    DIR *dirstream;		/* Stream reading this directory.  */
  };

/* Table of directory contents hashed by device and inode number.  */
static struct directory_contents *directories_contents[DIRECTORY_BUCKETS];

struct directory
  {
    struct directory *next;

    char *name;			/* Name of the directory.  */

    /* The directory's contents.  This data may be shared by several
       entries in the hash table, which refer to the same directory
       (identified uniquely by `dev' and `ino') under different names.  */
    struct directory_contents *contents;
  };

/* Table of directories hashed by name.  */
static struct directory *table_of_directories[DIRECTORY_BUCKETS];


/* Never have more than this many directories open at once.  */

#define MAX_OPEN_DIRECTORIES 10

static unsigned int open_directories = 0;


/* Hash table of files in each directory.  */

struct dirfile
  {
    struct dirfile *next;
    char *name;			/* Name of the file.  */
    char impossible;		/* This file is impossible.  */
  };

#ifndef	DIRFILE_BUCKETS
#define DIRFILE_BUCKETS 107
#endif

static int dir_contents_file_exists_p PARAMS ((struct directory_contents *dir, char *filename));
static struct directory *find_directory PARAMS ((char *name));

/* Find the directory named NAME and return its `struct directory'.  */

static struct directory *
find_directory (name)
     register char *name;
{
  register unsigned int hash = 0;
  register char *p;
  register struct directory *dir;
#ifdef WINDOWS32
  char* w32_path;
  char  fs_label[BUFSIZ];
  char  fs_type[BUFSIZ];
  long  fs_serno;
  long  fs_flags;
  long  fs_len;
#endif
#ifdef VMS
  if ((*name == '.') && (*(name+1) == 0))
    name = "[]";
  else
    name = vmsify (name,1);
#endif

  for (p = name; *p != '\0'; ++p)
    HASHI (hash, *p);
  hash %= DIRECTORY_BUCKETS;

  for (dir = table_of_directories[hash]; dir != 0; dir = dir->next)
    if (strieq (dir->name, name))
      break;

  if (dir == 0)
    {
      struct stat st;

      /* The directory was not found.  Create a new entry for it.  */

      dir = (struct directory *) xmalloc (sizeof (struct directory));
      dir->next = table_of_directories[hash];
      table_of_directories[hash] = dir;
      dir->name = savestring (name, p - name);

      /* The directory is not in the name hash table.
	 Find its device and inode numbers, and look it up by them.  */

#ifdef VMS
      if (vmsstat_dir (name, &st) < 0)
#else

# ifdef WINDOWS32
      /* Remove any trailing '\'.  Windows32 stat fails even on valid
         directories if they end in '\'. */
      if (p[-1] == '\\')
        p[-1] = '\0';
# endif
      if (stat (name, &st) < 0)
#endif
	{
	/* Couldn't stat the directory.  Mark this by
	   setting the `contents' member to a nil pointer.  */
	  dir->contents = 0;
	}
      else
	{
	  /* Search the contents hash table; device and inode are the key.  */

	  struct directory_contents *dc;

#ifdef WINDOWS32
          w32_path = w32ify(name, 1);
          hash = ((unsigned int) st.st_dev << 16) | (unsigned int) st.st_ctime;
#else
# ifdef VMS
          hash = (((unsigned int) st.st_dev << 16)
                  | ((unsigned int) st.st_ino[0]
                     + (unsigned int) st.st_ino[1]
                     + (unsigned int) st.st_ino[2]));
# else
	  hash = ((unsigned int) (dev_t_to_int(st.st_dev)) << 16) | (ino_t_to_unsigned_int(st.st_ino));
	  /*	  hash = ((unsigned int) st.st_dev << 16) | (unsigned int) st.st_ino; */
# endif
#endif
	  hash %= DIRECTORY_BUCKETS;

	  for (dc = directories_contents[hash]; dc != 0; dc = dc->next)
#ifdef WINDOWS32
            if (strieq(dc->path_key, w32_path))
#else
	    if (dc->dev == st.st_dev
# ifdef VMS
		&& dc->ino[0] == st.st_ino[0]
		&& dc->ino[1] == st.st_ino[1]
		&& dc->ino[2] == st.st_ino[2]
# else
                && ino_t_equal(ino_t_to_int(dc->ino), ino_t_to_int(st.st_ino)) == 0
		/*                && dc->ino == st.st_ino */
# endif
                )
#endif /* WINDOWS32 */
	      break;

	  if (dc == 0)
	    {
	      /* Nope; this really is a directory we haven't seen before.  */

	      dc = (struct directory_contents *)
		xmalloc (sizeof (struct directory_contents));

	      /* Enter it in the contents hash table.  */
	      dc->dev = st.st_dev;
#ifdef WINDOWS32
              dc->path_key = xstrdup(w32_path);
              dc->mtime = st.st_mtime;

              /*
               * NTFS is the only WINDOWS32 filesystem that bumps mtime
               * on a directory when files are added/deleted from
               * a directory.
               */
              w32_path[3] = '\0';
              if (GetVolumeInformation(w32_path,
                     fs_label, sizeof (fs_label),
                     &fs_serno, &fs_len,
                     &fs_flags, fs_type, sizeof (fs_type)) == FALSE)
                dc->fs_flags = FS_UNKNOWN;
              else if (!strcmp(fs_type, "FAT"))
                dc->fs_flags = FS_FAT;
              else if (!strcmp(fs_type, "NTFS"))
                dc->fs_flags = FS_NTFS;
              else
                dc->fs_flags = FS_UNKNOWN;
#else
# ifdef VMS
	      dc->ino[0] = st.st_ino[0];
	      dc->ino[1] = st.st_ino[1];
	      dc->ino[2] = st.st_ino[2];
# else
	      dc->ino = st.st_ino;
# endif
#endif /* WINDOWS32 */
	      dc->next = directories_contents[hash];
	      directories_contents[hash] = dc;

	      dc->dirstream = opendir (name);
	      if (dc->dirstream == 0)
                /* Couldn't open the directory.  Mark this by
                   setting the `files' member to a nil pointer.  */
                dc->files = 0;
	      else
		{
		  /* Allocate an array of buckets for files and zero it.  */
		  dc->files = (struct dirfile **)
		    xmalloc (sizeof (struct dirfile *) * DIRFILE_BUCKETS);
		  bzero ((char *) dc->files,
			 sizeof (struct dirfile *) * DIRFILE_BUCKETS);

		  /* Keep track of how many directories are open.  */
		  ++open_directories;
		  if (open_directories == MAX_OPEN_DIRECTORIES)
		    /* We have too many directories open already.
		       Read the entire directory and then close it.  */
		    (void) dir_contents_file_exists_p (dc, (char *) 0);
		}
	    }

	  /* Point the name-hashed entry for DIR at its contents data.  */
	  dir->contents = dc;
	}
    }

  return dir;
}

/* Return 1 if the name FILENAME is entered in DIR's hash table.
   FILENAME must contain no slashes.  */

static int
dir_contents_file_exists_p (dir, filename)
     register struct directory_contents *dir;
     register char *filename;
{
  register unsigned int hash;
  register char *p;
  register struct dirfile *df;
  register struct dirent *d;
#ifdef WINDOWS32
  struct stat st;
  int rehash = 0;
#endif

  if (dir == 0 || dir->files == 0)
    {
    /* The directory could not be stat'd or opened.  */
      return 0;
    }
#ifdef __MSDOS__
  filename = dosify (filename);
#endif

#ifdef HAVE_CASE_INSENSITIVE_FS
  filename = downcase (filename);
#endif

#ifdef VMS
  filename = vmsify (filename,0);
#endif

  hash = 0;
  if (filename != 0)
    {
      if (*filename == '\0')
	{
	/* Checking if the directory exists.  */
	  return 1;
	}

      for (p = filename; *p != '\0'; ++p)
	HASH (hash, *p);
      hash %= DIRFILE_BUCKETS;

      /* Search the list of hashed files.  */

      for (df = dir->files[hash]; df != 0; df = df->next)
	{
	  if (strieq (df->name, filename))
	    {
	      return !df->impossible;
	    }
	}
    }

  /* The file was not found in the hashed list.
     Try to read the directory further.  */

  if (dir->dirstream == 0)
    {
#ifdef WINDOWS32
      /*
       * Check to see if directory has changed since last read. FAT
       * filesystems force a rehash always as mtime does not change
       * on directories (ugh!).
       */
      if (dir->path_key &&
          (dir->fs_flags & FS_FAT ||
           (stat(dir->path_key, &st) == 0 &&
            st.st_mtime > dir->mtime))) {

        /* reset date stamp to show most recent re-process */
        dir->mtime = st.st_mtime;

        /* make sure directory can still be opened */
        dir->dirstream = opendir(dir->path_key);

        if (dir->dirstream)
          rehash = 1;
        else
          return 0; /* couldn't re-read - fail */
      } else
#endif
    /* The directory has been all read in.  */
      return 0;
    }

  while ((d = readdir (dir->dirstream)) != 0)
    {
      /* Enter the file in the hash table.  */
      register unsigned int newhash = 0;
      unsigned int len;
      register unsigned int i;

#if defined(VMS) && defined(HAVE_DIRENT_H)
      /* In VMS we get file versions too, which have to be stripped off */
      {
        char *p = strrchr (d->d_name, ';');
        if (p)
          *p = '\0';
      }
#endif
      if (!REAL_DIR_ENTRY (d))
	continue;

      len = NAMLEN (d);
      for (i = 0; i < len; ++i)
	HASHI (newhash, d->d_name[i]);
      newhash %= DIRFILE_BUCKETS;
#ifdef WINDOWS32
      /*
       * If re-reading a directory, check that this file isn't already
       * in the cache.
       */
      if (rehash) {
        for (df = dir->files[newhash]; df != 0; df = df->next)
          if (streq(df->name, d->d_name))
            break;
      } else
        df = 0;

      /*
       * If re-reading a directory, don't cache files that have
       * already been discovered.
       */
      if (!df) {
#endif

      df = (struct dirfile *) xmalloc (sizeof (struct dirfile));
      df->next = dir->files[newhash];
      dir->files[newhash] = df;
      df->name = savestring (d->d_name, len);
      df->impossible = 0;
#ifdef WINDOWS32
      }
#endif
      /* Check if the name matches the one we're searching for.  */
      if (filename != 0
	  && newhash == hash && strieq (d->d_name, filename))
	{
	  return 1;
	}
    }

  /* If the directory has been completely read in,
     close the stream and reset the pointer to nil.  */
  if (d == 0)
    {
      --open_directories;
      closedir (dir->dirstream);
      dir->dirstream = 0;
    }
  return 0;
}

/* Return 1 if the name FILENAME in directory DIRNAME
   is entered in the dir hash table.
   FILENAME must contain no slashes.  */

int
dir_file_exists_p (dirname, filename)
     register char *dirname;
     register char *filename;
{
  return dir_contents_file_exists_p (find_directory (dirname)->contents,
				     filename);
}

/* Return 1 if the file named NAME exists.  */

int
file_exists_p (name)
     register char *name;
{
  char *dirend;
  char *dirname;
  char *slash;

#ifndef	NO_ARCHIVES
  if (ar_name (name))
    return ar_member_date (name) != (time_t) -1;
#endif

#ifdef VMS
  dirend = strrchr (name, ']');
  if (dirend == 0)
    dirend = strrchr (name, ':');
  dirend++;
  if (dirend == (char *)1)
    return dir_file_exists_p ("[]", name);
#else /* !VMS */
  dirend = strrchr (name, '/');
#if defined (WINDOWS32) || defined (__MSDOS__)
  /* Forward and backslashes might be mixed.  We need the rightmost one.  */
  {
    char *bslash = strrchr(name, '\\');
    if (!dirend || bslash > dirend)
      dirend = bslash;
    /* The case of "d:file".  */
    if (!dirend && name[0] && name[1] == ':')
      dirend = name + 1;
  }
#endif /* WINDOWS32 || __MSDOS__ */
  if (dirend == 0)
#ifndef _AMIGA
    return dir_file_exists_p (".", name);
#else /* !VMS && !AMIGA */
    return dir_file_exists_p ("", name);
#endif /* AMIGA */
#endif /* VMS */

  slash = dirend;
  if (dirend == name)
    dirname = "/";
  else
    {
#if defined (WINDOWS32) || defined (__MSDOS__)
  /* d:/ and d: are *very* different...  */
      if (dirend < name + 3 && name[1] == ':' &&
	  (*dirend == '/' || *dirend == '\\' || *dirend == ':'))
	dirend++;
#endif
      dirname = (char *) alloca (dirend - name + 1);
      bcopy (name, dirname, dirend - name);
      dirname[dirend - name] = '\0';
    }
  return dir_file_exists_p (dirname, slash + 1);
}

/* Mark FILENAME as `impossible' for `file_impossible_p'.
   This means an attempt has been made to search for FILENAME
   as an intermediate file, and it has failed.  */

void
file_impossible (filename)
     register char *filename;
{
  char *dirend;
  register char *p = filename;
  register unsigned int hash;
  register struct directory *dir;
  register struct dirfile *new;

#ifdef VMS
  dirend = strrchr (p, ']');
  if (dirend == 0)
    dirend = strrchr (p, ':');
  dirend++;
  if (dirend == (char *)1)
    dir = find_directory ("[]");
#else
  dirend = strrchr (p, '/');
#if defined (WINDOWS32) || defined (__MSDOS__)
  /* Forward and backslashes might be mixed.  We need the rightmost one.  */
  {
    char *bslash = strrchr(p, '\\');
    if (!dirend || bslash > dirend)
      dirend = bslash;
    /* The case of "d:file".  */
    if (!dirend && p[0] && p[1] == ':')
      dirend = p + 1;
  }
#endif /* WINDOWS32 or __MSDOS__ */
  if (dirend == 0)
#ifdef _AMIGA
    dir = find_directory ("");
#else /* !VMS && !AMIGA */
    dir = find_directory (".");
#endif /* AMIGA */
#endif /* VMS */
  else
    {
      char *dirname;
      char *slash = dirend;
      if (dirend == p)
	dirname = "/";
      else
	{
#if defined (WINDOWS32) || defined (__MSDOS__)
	  /* d:/ and d: are *very* different...  */
	  if (dirend < p + 3 && p[1] == ':' &&
	      (*dirend == '/' || *dirend == '\\' || *dirend == ':'))
	    dirend++;
#endif
	  dirname = (char *) alloca (dirend - p + 1);
	  bcopy (p, dirname, dirend - p);
	  dirname[dirend - p] = '\0';
	}
      dir = find_directory (dirname);
      filename = p = slash + 1;
    }

  for (hash = 0; *p != '\0'; ++p)
    HASHI (hash, *p);
  hash %= DIRFILE_BUCKETS;

  if (dir->contents == 0)
    {
      /* The directory could not be stat'd.  We allocate a contents
	 structure for it, but leave it out of the contents hash table.  */
      dir->contents = (struct directory_contents *)
	xmalloc (sizeof (struct directory_contents));
#ifdef WINDOWS32
      dir->contents->path_key = NULL;
      dir->contents->mtime = 0;
#else  /* WINDOWS32 */
#ifdef VMS
      dir->contents->dev = 0;
      dir->contents->ino[0] = dir->contents->ino[1] =
	dir->contents->ino[2] = 0;
#else
      dir->contents->dev = int_to_dev_t(0);
      dir->contents->ino = int_to_ino_t(0);
      /*      dir->contents->dev = dir->contents->ino = 0; */
#endif
#endif /* WINDOWS32 */
      dir->contents->files = 0;
      dir->contents->dirstream = 0;
    }

  if (dir->contents->files == 0)
    {
      /* The directory was not opened; we must allocate the hash buckets.  */
      dir->contents->files = (struct dirfile **)
	xmalloc (sizeof (struct dirfile) * DIRFILE_BUCKETS);
      bzero ((char *) dir->contents->files,
	     sizeof (struct dirfile) * DIRFILE_BUCKETS);
    }

  /* Make a new entry and put it in the table.  */

  new = (struct dirfile *) xmalloc (sizeof (struct dirfile));
  new->next = dir->contents->files[hash];
  dir->contents->files[hash] = new;
  new->name = xstrdup (filename);
  new->impossible = 1;
}

/* Return nonzero if FILENAME has been marked impossible.  */

int
file_impossible_p (filename)
     char *filename;
{
  char *dirend;
  register char *p = filename;
  register unsigned int hash;
  register struct directory_contents *dir;
  register struct dirfile *next;

#ifdef VMS
  dirend = strrchr (filename, ']');
  if (dirend == 0)
    dir = find_directory ("[]")->contents;
#else
  dirend = strrchr (filename, '/');
#if defined (WINDOWS32) || defined (__MSDOS__)
  /* Forward and backslashes might be mixed.  We need the rightmost one.  */
  {
    char *bslash = strrchr(filename, '\\');
    if (!dirend || bslash > dirend)
      dirend = bslash;
    /* The case of "d:file".  */
    if (!dirend && filename[0] && filename[1] == ':')
      dirend = filename + 1;
  }
#endif /* WINDOWS32 || __MSDOS__ */
  if (dirend == 0)
#ifdef _AMIGA
    dir = find_directory ("")->contents;
#else /* !VMS && !AMIGA */
    dir = find_directory (".")->contents;
#endif /* AMIGA */
#endif /* VMS */
  else
    {
      char *dirname;
      char *slash = dirend;
      if (dirend == filename)
	dirname = "/";
      else
	{
#if defined (WINDOWS32) || defined (__MSDOS__)
	  /* d:/ and d: are *very* different...  */
	  if (dirend < filename + 3 && filename[1] == ':' &&
	      (*dirend == '/' || *dirend == '\\' || *dirend == ':'))
	    dirend++;
#endif
	  dirname = (char *) alloca (dirend - filename + 1);
	  bcopy (p, dirname, dirend - p);
	  dirname[dirend - p] = '\0';
	}
      dir = find_directory (dirname)->contents;
      p = filename = slash + 1;
    }

  if (dir == 0 || dir->files == 0)
    /* There are no files entered for this directory.  */
    return 0;

#ifdef __MSDOS__
  p = filename = dosify (p);
#endif
#ifdef HAVE_CASE_INSENSITIVE_FS
  p = filename = downcase (p);
#endif
#ifdef VMS
  p = filename = vmsify (p, 1);
#endif

  for (hash = 0; *p != '\0'; ++p)
    HASH (hash, *p);
  hash %= DIRFILE_BUCKETS;

  for (next = dir->files[hash]; next != 0; next = next->next)
    if (strieq (filename, next->name))
      return next->impossible;

  return 0;
}

/* Return the already allocated name in the
   directory hash table that matches DIR.  */

char *
dir_name (dir)
     char *dir;
{
  return find_directory (dir)->name;
}

/* Print the data base of directories.  */

void
print_dir_data_base ()
{
  register unsigned int i, dirs, files, impossible;
  register struct directory *dir;

  puts (_("\n# Directories\n"));

  dirs = files = impossible = 0;
  for (i = 0; i < DIRECTORY_BUCKETS; ++i)
    for (dir = table_of_directories[i]; dir != 0; dir = dir->next)
      {
	++dirs;
	if (dir->contents == 0)
	  printf (_("# %s: could not be stat'd.\n"), dir->name);
	else if (dir->contents->files == 0)
#ifdef WINDOWS32
          printf (_("# %s (key %s, mtime %d): could not be opened.\n"),
                  dir->name, dir->contents->path_key,dir->contents->mtime);
#else  /* WINDOWS32 */
#ifdef VMS
	  printf (_("# %s (device %d, inode [%d,%d,%d]): could not be opened.\n"),
		  dir->name, dir->contents->dev,
		  dir->contents->ino[0], dir->contents->ino[1],
		  dir->contents->ino[2]);
#else
	  printf (_("# %s (device %ld, inode %ld): could not be opened.\n"),
		  dir->name, (long int) dir->contents->dev,
		  (ino_t_to_long_int(dir->contents->ino)));
	  /*		  (long int) dir->contents->ino); */
#endif
#endif /* WINDOWS32 */
	else
	  {
	    register unsigned int f = 0, im = 0;
	    register unsigned int j;
	    register struct dirfile *df;
	    for (j = 0; j < DIRFILE_BUCKETS; ++j)
	      for (df = dir->contents->files[j]; df != 0; df = df->next)
		if (df->impossible)
		  ++im;
		else
		  ++f;
#ifdef WINDOWS32
            printf (_("# %s (key %s, mtime %d): "),
                    dir->name, dir->contents->path_key, dir->contents->mtime);
#else  /* WINDOWS32 */
#ifdef VMS
	    printf (_("# %s (device %d, inode [%d,%d,%d]): "),
		    dir->name, dir->contents->dev,
			dir->contents->ino[0], dir->contents->ino[1],
			dir->contents->ino[2]);
#else
	    printf (_("# %s (device %ld, inode %ld): "),
		    dir->name,
                    (long)(dev_t_to_int(dir->contents->dev)), (ino_t_to_long(dir->contents->ino)));
	    /*                    (long)dir->contents->dev, (long)dir->contents->ino); */
#endif
#endif /* WINDOWS32 */
	    if (f == 0)
	      fputs (_("No"), stdout);
	    else
	      printf ("%u", f);
	    fputs (_(" files, "), stdout);
	    if (im == 0)
	      fputs (_("no"), stdout);
	    else
	      printf ("%u", im);
	    fputs (_(" impossibilities"), stdout);
	    if (dir->contents->dirstream == 0)
	      puts (".");
	    else
	      puts (_(" so far."));
	    files += f;
	    impossible += im;
	  }
      }

  fputs ("\n# ", stdout);
  if (files == 0)
    fputs (_("No"), stdout);
  else
    printf ("%u", files);
  fputs (_(" files, "), stdout);
  if (impossible == 0)
    fputs (_("no"), stdout);
  else
    printf ("%u", impossible);
  printf (_(" impossibilities in %u directories.\n"), dirs);
}

/* Hooks for globbing.  */

#include <glob.h>
#undef stderr
#define stderr stdout

/* Structure describing state of iterating through a directory hash table.  */

struct dirstream
  {
    struct directory_contents *contents; /* The directory being read.  */

    unsigned int bucket;	/* Current hash bucket.  */
    struct dirfile *elt;	/* Current elt in bucket.  */
  };

/* Forward declarations.  */
static __ptr_t open_dirstream PARAMS ((const char *));
static struct dirent *read_dirstream PARAMS ((__ptr_t));

static __ptr_t
open_dirstream (directory)
     const char *directory;
{
  struct dirstream *new;
  struct directory *dir = find_directory ((char *)directory);

  if (dir->contents == 0 || dir->contents->files == 0)
    /* DIR->contents is nil if the directory could not be stat'd.
       DIR->contents->files is nil if it could not be opened.  */
    return 0;

  /* Read all the contents of the directory now.  There is no benefit
     in being lazy, since glob will want to see every file anyway.  */

  (void) dir_contents_file_exists_p (dir->contents, (char *) 0);

  new = (struct dirstream *) xmalloc (sizeof (struct dirstream));
  new->contents = dir->contents;
  new->bucket = 0;
  new->elt = new->contents->files[0];

  return (__ptr_t) new;
}

static struct dirent *
read_dirstream (stream)
     __ptr_t stream;
{
  struct dirstream *const ds = (struct dirstream *) stream;
  register struct dirfile *df;
  static char *buf;
  static unsigned int bufsz;

  while (ds->bucket < DIRFILE_BUCKETS)
    {
      while ((df = ds->elt) != 0)
	{
	  ds->elt = df->next;
	  if (!df->impossible)
	    {
	      /* The glob interface wants a `struct dirent',
		 so mock one up.  */
	      struct dirent *d;
	      unsigned int len = strlen (df->name) + 1;
	      if (sizeof *d - sizeof d->d_name + len > bufsz)
		{
		  if (buf != 0)
		    free (buf);
		  bufsz *= 2;
		  if (sizeof *d - sizeof d->d_name + len > bufsz)
		    bufsz = sizeof *d - sizeof d->d_name + len;
		  buf = xmalloc (bufsz);
		}
	      d = (struct dirent *) buf;
	      FAKE_DIR_ENTRY (d);
#ifdef _DIRENT_HAVE_D_NAMLEN
	      d->d_namlen = len - 1;
#endif
#ifdef _DIRENT_HAVE_D_TYPE
	      d->d_type = DT_UNKNOWN;
#endif
	      memcpy (d->d_name, df->name, len);
	      return d;
	    }
	}
      if (++ds->bucket == DIRFILE_BUCKETS)
	break;
      ds->elt = ds->contents->files[ds->bucket];
    }

  return 0;
}

static void
ansi_free(p)
  void *p;
{
    if (p)
      free(p);
}

/* On 64 bit ReliantUNIX (5.44 and above) in LFS mode, stat() is actually a
 * macro for stat64().  If stat is a macro, make a local wrapper function to
 * invoke it.
 */
#ifndef stat
# ifndef VMS
extern int stat ();
# endif
# define local_stat stat
#else
static int local_stat (path, buf)
    char *path;
    struct stat *buf;
{
  return stat (path, buf);
}
#endif

void
dir_setup_glob (gl)
     glob_t *gl;
{
  /* Bogus sunos4 compiler complains (!) about & before functions.  */
  gl->gl_opendir = open_dirstream;
  gl->gl_readdir = read_dirstream;
  gl->gl_closedir = ansi_free;
  gl->gl_stat = local_stat;
  /* We don't bother setting gl_lstat, since glob never calls it.
     The slot is only there for compatibility with 4.4 BSD.  */
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         expand.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Variable expansion functions for GNU Make.
Copyright (C) 1988, 89, 91, 92, 93, 95 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <assert.h>
#undef stderr
#define stderr stdout

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#include "rule.h"
#undef stderr
#define stderr stdout

/* The next two describe the variable output buffer.
   This buffer is used to hold the variable-expansion of a line of the
   makefile.  It is made bigger with realloc whenever it is too small.
   variable_buffer_length is the size currently allocated.
   variable_buffer is the address of the buffer.

   For efficiency, it's guaranteed that the buffer will always have
   VARIABLE_BUFFER_ZONE extra bytes allocated.  This allows you to add a few
   extra chars without having to call a function.  Note you should never use
   these bytes unless you're _sure_ you have room (you know when the buffer
   length was last checked.  */

#define VARIABLE_BUFFER_ZONE    5

static unsigned int variable_buffer_length;
char *variable_buffer;

/* Subroutine of variable_expand and friends:
   The text to add is LENGTH chars starting at STRING to the variable_buffer.
   The text is added to the buffer at PTR, and the updated pointer into
   the buffer is returned as the value.  Thus, the value returned by
   each call to variable_buffer_output should be the first argument to
   the following call.  */

char *
variable_buffer_output (ptr, string, length)
     char *ptr, *string;
     unsigned int length;
{
  register unsigned int newlen = length + (ptr - variable_buffer);

  if ((newlen + VARIABLE_BUFFER_ZONE) > variable_buffer_length)
    {
      unsigned int offset = ptr - variable_buffer;
      variable_buffer_length = (newlen + 100 > 2 * variable_buffer_length
				? newlen + 100
				: 2 * variable_buffer_length);
      variable_buffer = (char *) xrealloc (variable_buffer,
					   variable_buffer_length);
      ptr = variable_buffer + offset;
    }

  bcopy (string, ptr, length);
  return ptr + length;
}

/* Return a pointer to the beginning of the variable buffer.  */

static char *
initialize_variable_output ()
{
  /* If we don't have a variable output buffer yet, get one.  */

  if (variable_buffer == 0)
    {
      variable_buffer_length = 200;
      variable_buffer = (char *) xmalloc (variable_buffer_length);
      variable_buffer[0] = '\0';
    }

  return variable_buffer;
}

/* Recursively expand V.  The returned string is malloc'd.  */

static char *allocated_variable_append PARAMS ((struct variable *v));

char *
recursively_expand (v)
     register struct variable *v;
{
  char *value;

  if (v->expanding)
    /* Expanding V causes infinite recursion.  Lose.  */
    fatal (reading_file,
           _("Recursive variable `%s' references itself (eventually)"),
           v->name);

  v->expanding = 1;
  if (v->append)
    value = allocated_variable_append (v);
  else
    value = allocated_variable_expand (v->value);
  v->expanding = 0;

  return value;
}

/* Warn that NAME is an undefined variable.  */

#ifdef __GNUC__
__inline
#endif
static void
warn_undefined (name, length)
     char *name;
     unsigned int length;
{
  if (warn_undefined_variables_flag)
    error (reading_file,
           _("warning: undefined variable `%.*s'"), (int)length, name);
}

/* Expand a simple reference to variable NAME, which is LENGTH chars long.  */

#ifdef __GNUC__
__inline
#endif
static char *
reference_variable (o, name, length)
     char *o;
     char *name;
     unsigned int length;
{
  register struct variable *v;
  char *value;

  v = lookup_variable (name, length);

  if (v == 0)
    warn_undefined (name, length);

  if (v == 0 || *v->value == '\0')
    return o;

  value = (v->recursive ? recursively_expand (v) : v->value);

  o = variable_buffer_output (o, value, strlen (value));

  if (v->recursive)
    free (value);

  return o;
}

/* Scan STRING for variable references and expansion-function calls.  Only
   LENGTH bytes of STRING are actually scanned.  If LENGTH is -1, scan until
   a null byte is found.

   Write the results to LINE, which must point into `variable_buffer'.  If
   LINE is NULL, start at the beginning of the buffer.
   Return a pointer to LINE, or to the beginning of the buffer if LINE is
   NULL.  */

char *
variable_expand_string (line, string, length)
     register char *line;
     char *string;
     long length;
{
  register struct variable *v;
  register char *p, *o, *p1;
  char save_char = '\0';
  unsigned int line_offset;

  if (!line)
    line = initialize_variable_output();

  p = string;
  o = line;
  line_offset = line - variable_buffer;

  if (length >= 0)
    {
      save_char = string[length];
      string[length] = '\0';
    }

  while (1)
    {
      /* Copy all following uninteresting chars all at once to the
         variable output buffer, and skip them.  Uninteresting chars end
	 at the next $ or the end of the input.  */

      p1 = strchr (p, '$');

      o = variable_buffer_output (o, p, p1 != 0 ? p1 - p : strlen (p) + 1);

      if (p1 == 0)
	break;
      p = p1 + 1;

      /* Dispatch on the char that follows the $.  */

      switch (*p)
	{
	case '$':
	  /* $$ seen means output one $ to the variable output buffer.  */
	  o = variable_buffer_output (o, p, 1);
	  break;

	case '(':
	case '{':
	  /* $(...) or ${...} is the general case of substitution.  */
	  {
	    char openparen = *p;
	    char closeparen = (openparen == '(') ? ')' : '}';
	    register char *beg = p + 1;
	    int free_beg = 0;
	    char *op, *begp;
	    char *end, *colon;

	    op = o;
	    begp = p;
	    if (handle_function (&op, &begp))
	      {
		o = op;
		p = begp;
		break;
	      }

	    /* Is there a variable reference inside the parens or braces?
	       If so, expand it before expanding the entire reference.  */

	    end = strchr (beg, closeparen);
	    if (end == 0)
              /* Unterminated variable reference.  */
              fatal (reading_file, _("unterminated variable reference"));
	    p1 = lindex (beg, end, '$');
	    if (p1 != 0)
	      {
		/* BEG now points past the opening paren or brace.
		   Count parens or braces until it is matched.  */
		int count = 0;
		for (p = beg; *p != '\0'; ++p)
		  {
		    if (*p == openparen)
		      ++count;
		    else if (*p == closeparen && --count < 0)
		      break;
		  }
		/* If COUNT is >= 0, there were unmatched opening parens
		   or braces, so we go to the simple case of a variable name
		   such as `$($(a)'.  */
		if (count < 0)
		  {
		    beg = expand_argument (beg, p); /* Expand the name.  */
		    free_beg = 1; /* Remember to free BEG when finished.  */
		    end = strchr (beg, '\0');
		  }
	      }
	    else
	      /* Advance P to the end of this reference.  After we are
                 finished expanding this one, P will be incremented to
                 continue the scan.  */
	      p = end;

	    /* This is not a reference to a built-in function and
	       any variable references inside are now expanded.
	       Is the resultant text a substitution reference?  */

	    colon = lindex (beg, end, ':');
	    if (colon != 0)
	      {
		/* This looks like a substitution reference: $(FOO:A=B).  */
		char *subst_beg, *subst_end, *replace_beg, *replace_end;

		subst_beg = colon + 1;
		subst_end = strchr (subst_beg, '=');
		if (subst_end == 0)
		  /* There is no = in sight.  Punt on the substitution
		     reference and treat this as a variable name containing
		     a colon, in the code below.  */
		  colon = 0;
		else
		  {
		    replace_beg = subst_end + 1;
		    replace_end = end;

		    /* Extract the variable name before the colon
		       and look up that variable.  */
		    v = lookup_variable (beg, colon - beg);
		    if (v == 0)
		      warn_undefined (beg, colon - beg);

		    if (v != 0 && *v->value != '\0')
		      {
			char *value = (v->recursive ? recursively_expand (v)
				       : v->value);
			char *pattern, *percent;
			if (free_beg)
			  {
			    *subst_end = '\0';
			    pattern = subst_beg;
			  }
			else
			  {
			    pattern = (char *) alloca (subst_end - subst_beg
						       + 1);
			    bcopy (subst_beg, pattern, subst_end - subst_beg);
			    pattern[subst_end - subst_beg] = '\0';
			  }
			percent = find_percent (pattern);
			if (percent != 0)
			  {
			    char *replace;
			    if (free_beg)
			      {
				*replace_end = '\0';
				replace = replace_beg;
			      }
			    else
			      {
				replace = (char *) alloca (replace_end
							   - replace_beg
							   + 1);
				bcopy (replace_beg, replace,
				       replace_end - replace_beg);
				replace[replace_end - replace_beg] = '\0';
			      }

			    o = patsubst_expand (o, value, pattern, replace,
						 percent, (char *) 0);
			  }
			else
			  o = subst_expand (o, value,
					    pattern, replace_beg,
					    strlen (pattern),
					    end - replace_beg,
					    0, 1);
			if (v->recursive)
			  free (value);
		      }
		  }
	      }

	    if (colon == 0)
	      /* This is an ordinary variable reference.
		 Look up the value of the variable.  */
		o = reference_variable (o, beg, end - beg);

	  if (free_beg)
	    free (beg);
	  }
	  break;

	case '\0':
	  break;

	default:
	  if (isblank (p[-1]))
	    break;

	  /* A $ followed by a random char is a variable reference:
	     $a is equivalent to $(a).  */
	  {
	    /* We could do the expanding here, but this way
	       avoids code repetition at a small performance cost.  */
	    char name[5];
	    name[0] = '$';
	    name[1] = '(';
	    name[2] = *p;
	    name[3] = ')';
	    name[4] = '\0';
	    p1 = allocated_variable_expand (name);
	    o = variable_buffer_output (o, p1, strlen (p1));
	    free (p1);
	  }

	  break;
	}

      if (*p == '\0')
	break;
      else
	++p;
    }

  if (save_char)
    string[length] = save_char;

  (void)variable_buffer_output (o, "", 1);
  return (variable_buffer + line_offset);
}

/* Scan LINE for variable references and expansion-function calls.
   Build in `variable_buffer' the result of expanding the references and calls.
   Return the address of the resulting string, which is null-terminated
   and is valid only until the next time this function is called.  */

char *
variable_expand (line)
     char *line;
{
  return variable_expand_string(NULL, line, (long)-1);
}

/* Expand an argument for an expansion function.
   The text starting at STR and ending at END is variable-expanded
   into a null-terminated string that is returned as the value.
   This is done without clobbering `variable_buffer' or the current
   variable-expansion that is in progress.  */

char *
expand_argument (str, end)
     char *str, *end;
{
  char *tmp;

  if (!end || *end == '\0')
    tmp = str;
  else
    {
      tmp = (char *) alloca (end - str + 1);
      bcopy (str, tmp, end - str);
      tmp[end - str] = '\0';
    }

  return allocated_variable_expand (tmp);
}

/* Expand LINE for FILE.  Error messages refer to the file and line where
   FILE's commands were found.  Expansion uses FILE's variable set list.  */

static char *
variable_expand_for_file (line, file)
     char *line;
     register struct file *file;
{
  char *result;
  struct variable_set_list *save;

  if (file == 0)
    return variable_expand (line);

  save = current_variable_set_list;
  current_variable_set_list = file->variables;
  if (file->cmds && file->cmds->fileinfo.filenm)
    reading_file = &file->cmds->fileinfo;
  else
    reading_file = 0;
  result = variable_expand (line);
  current_variable_set_list = save;
  reading_file = 0;

  return result;
}

/* Like allocated_variable_expand, but we first expand this variable in the
    context of the next variable set, then we append the expanded value.  */

static char *
allocated_variable_append (v)
     struct variable *v;
{
  struct variable_set_list *save;
  int len = strlen (v->name);
  char *var = alloca (len + 4);
  char *value;

  char *obuf = variable_buffer;
  unsigned int olen = variable_buffer_length;

  variable_buffer = 0;

  assert(current_variable_set_list->next != 0);
  save = current_variable_set_list;
  current_variable_set_list = current_variable_set_list->next;

  var[0] = '$';
  var[1] = '(';
  strcpy (&var[2], v->name);
  var[len+2] = ')';
  var[len+3] = '\0';

  value = variable_expand_for_file (var, 0);

  current_variable_set_list = save;

  value += strlen (value);
  value = variable_buffer_output (value, " ", 1);
  value = variable_expand_string (value, v->value, (long)-1);

  value = variable_buffer;

#if 0
  /* Waste a little memory and save time.  */
  value = xrealloc (value, strlen (value))
#endif

  variable_buffer = obuf;
  variable_buffer_length = olen;

  return value;
}

/* Like variable_expand_for_file, but the returned string is malloc'd.
   This function is called a lot.  It wants to be efficient.  */

char *
allocated_variable_expand_for_file (line, file)
     char *line;
     struct file *file;
{
  char *value;

  char *obuf = variable_buffer;
  unsigned int olen = variable_buffer_length;

  variable_buffer = 0;

  value = variable_expand_for_file (line, file);

#if 0
  /* Waste a little memory and save time.  */
  value = xrealloc (value, strlen (value))
#endif

  variable_buffer = obuf;
  variable_buffer_length = olen;

  return value;
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         file.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Target file hash table management for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <assert.h>

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout


/* Hash table of files the makefile knows how to make.  */

#ifndef	FILE_BUCKETS
#define FILE_BUCKETS	1007
#endif
static struct file *files[FILE_BUCKETS];

/* Number of files with the `intermediate' flag set.  */

unsigned int num_intermediates = 0;

/* Current value for pruning the scan of the goal chain (toggle 0/1).  */

unsigned int considered = 0;

/* Access the hash table of all file records.
   lookup_file  given a name, return the struct file * for that name,
           or nil if there is none.
   enter_file   similar, but create one if there is none.  */

struct file *
lookup_file (name)
     char *name;
{
  register struct file *f;
  register char *n;
  register unsigned int hashval;
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  register char *lname, *ln;
#endif

  assert (*name != '\0');

  /* This is also done in parse_file_seq, so this is redundant
     for names read from makefiles.  It is here for names passed
     on the command line.  */
#ifdef VMS
# ifndef WANT_CASE_SENSITIVE_TARGETS
  lname = (char *)malloc(strlen(name) + 1);
  for (n=name, ln=lname; *n != '\0'; ++n, ++ln)
    *ln = isupper((unsigned char)*n) ? tolower((unsigned char)*n) : *n;
  *ln = '\0';
  name = lname;
# endif

  while (name[0] == '[' && name[1] == ']' && name[2] != '\0')
      name += 2;
#endif
  while (name[0] == '.' && name[1] == '/' && name[2] != '\0')
    {
      name += 2;
      while (*name == '/')
	/* Skip following slashes: ".//foo" is "foo", not "/foo".  */
	++name;
    }

  if (*name == '\0')
    /* It was all slashes after a dot.  */
#ifdef VMS
    name = "[]";
#else
#ifdef _AMIGA
    name = "";
#else
    name = "./";
#endif /* AMIGA */
#endif /* VMS */

  hashval = 0;
  for (n = name; *n != '\0'; ++n)
    HASHI (hashval, *n);
  hashval %= FILE_BUCKETS;

  for (f = files[hashval]; f != 0; f = f->next)
    {
      if (strieq (f->hname, name))
	{
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
	  free (lname);
#endif
	  return f;
	}
    }
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  free (lname);
#endif
  return 0;
}

struct file *
enter_file (name)
     char *name;
{
  register struct file *f, *new;
  register char *n;
  register unsigned int hashval;
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  char *lname, *ln;
#endif

  assert (*name != '\0');

#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
  lname = (char *)malloc (strlen (name) + 1);
  for (n = name, ln = lname; *n != '\0'; ++n, ++ln)
    {
      if (isupper((unsigned char)*n))
	*ln = tolower((unsigned char)*n);
      else
	*ln = *n;
    }
  *ln = 0;
  /* Creates a possible leak, old value of name is unreachable, but I
     currently don't know how to fix it. */
  name = lname;
#endif

  hashval = 0;
  for (n = name; *n != '\0'; ++n)
    HASHI (hashval, *n);
  hashval %= FILE_BUCKETS;

  for (f = files[hashval]; f != 0; f = f->next)
    if (strieq (f->hname, name))
      break;

  if (f != 0 && !f->double_colon)
    {
#if defined(VMS) && !defined(WANT_CASE_SENSITIVE_TARGETS)
      free(lname);
#endif
      return f;
    }

  new = (struct file *) xmalloc (sizeof (struct file));
  bzero ((char *) new, sizeof (struct file));
  new->name = new->hname = name;
  new->update_status = -1;

  if (f == 0)
    {
      /* This is a completely new file.  */
      new->next = files[hashval];
      files[hashval] = new;
    }
  else
    {
      /* There is already a double-colon entry for this file.  */
      new->double_colon = f;
      while (f->prev != 0)
	f = f->prev;
      f->prev = new;
    }

  return new;
}

/* Rehash FILE to NAME.  This is not as simple as resetting
   the `hname' member, since it must be put in a new hash bucket,
   and possibly merged with an existing file called NAME.  */

void
rehash_file (file, name)
     register struct file *file;
     char *name;
{
  char *oldname = file->hname;
  register unsigned int oldhash;
  register char *n;

  while (file->renamed != 0)
    file = file->renamed;

  /* Find the hash values of the old and new names.  */

  oldhash = 0;
  for (n = oldname; *n != '\0'; ++n)
    HASHI (oldhash, *n);

  file_hash_enter (file, name, oldhash, file->name);
}

/* Rename FILE to NAME.  This is not as simple as resetting
   the `name' member, since it must be put in a new hash bucket,
   and possibly merged with an existing file called NAME.  */

void
rename_file (file, name)
     register struct file *file;
     char *name;
{
  rehash_file(file, name);
  while (file)
    {
      file->name = file->hname;
      file = file->prev;
    }
}

void
file_hash_enter (file, name, oldhash, oldname)
     register struct file *file;
     char *name;
     unsigned int oldhash;
     char *oldname;
{
  unsigned int oldbucket = oldhash % FILE_BUCKETS;
  register unsigned int newhash, newbucket;
  struct file *oldfile;
  register char *n;
  register struct file *f;

  newhash = 0;
  for (n = name; *n != '\0'; ++n)
    HASHI (newhash, *n);
  newbucket = newhash % FILE_BUCKETS;

  /* Look for an existing file under the new name.  */

  for (oldfile = files[newbucket]; oldfile != 0; oldfile = oldfile->next)
    if (strieq (oldfile->hname, name))
      break;

  /* If the old file is the same as the new file, never mind.  */
  if (oldfile == file)
    return;

  if (oldhash != 0 && (newbucket != oldbucket || oldfile != 0))
    {
      /* Remove FILE from its hash bucket.  */

      struct file *lastf = 0;

      for (f = files[oldbucket]; f != file; f = f->next)
	lastf = f;

      if (lastf == 0)
	files[oldbucket] = f->next;
      else
	lastf->next = f->next;
    }

  /* Give FILE its new name.  */

  file->hname = name;
  for (f = file->double_colon; f != 0; f = f->prev)
    f->hname = name;

  if (oldfile == 0)
    {
      /* There is no existing file with the new name.  */

      if (newbucket != oldbucket)
	{
	  /* Put FILE in its new hash bucket.  */
	  file->next = files[newbucket];
	  files[newbucket] = file;
	}
    }
  else
    {
      /* There is an existing file with the new name.
	 We must merge FILE into the existing file.  */

      register struct dep *d;

      if (file->cmds != 0)
	{
	  if (oldfile->cmds == 0)
	    oldfile->cmds = file->cmds;
	  else if (file->cmds != oldfile->cmds)
	    {
	      /* We have two sets of commands.  We will go with the
		 one given in the rule explicitly mentioning this name,
		 but give a message to let the user know what's going on.  */
	      if (oldfile->cmds->fileinfo.filenm != 0)
                error (&file->cmds->fileinfo,
                                _("Commands were specified for \
file `%s' at %s:%lu,"),
                                oldname, oldfile->cmds->fileinfo.filenm,
                                oldfile->cmds->fileinfo.lineno);
	      else
		error (&file->cmds->fileinfo,
				_("Commands for file `%s' were found by \
implicit rule search,"),
				oldname);
	      error (&file->cmds->fileinfo,
			      _("but `%s' is now considered the same file \
as `%s'."),
			      oldname, name);
	      error (&file->cmds->fileinfo,
			      _("Commands for `%s' will be ignored \
in favor of those for `%s'."),
			      name, oldname);
	    }
	}

      /* Merge the dependencies of the two files.  */

      d = oldfile->deps;
      if (d == 0)
	oldfile->deps = file->deps;
      else
	{
	  while (d->next != 0)
	    d = d->next;
	  d->next = file->deps;
	}

      merge_variable_set_lists (&oldfile->variables, file->variables);

      if (oldfile->double_colon && file->is_target && !file->double_colon)
	fatal (NILF, _("can't rename single-colon `%s' to double-colon `%s'"),
	       oldname, name);
      if (!oldfile->double_colon  && file->double_colon)
	{
	  if (oldfile->is_target)
	    fatal (NILF, _("can't rename double-colon `%s' to single-colon `%s'"),
		   oldname, name);
	  else
	    oldfile->double_colon = file->double_colon;
	}

      if (file->last_mtime > oldfile->last_mtime)
	/* %%% Kludge so -W wins on a file that gets vpathized.  */
	oldfile->last_mtime = file->last_mtime;

      oldfile->mtime_before_update = file->mtime_before_update;

#define MERGE(field) oldfile->field |= file->field
      MERGE (precious);
      MERGE (tried_implicit);
      MERGE (updating);
      MERGE (updated);
      MERGE (is_target);
      MERGE (cmd_target);
      MERGE (phony);
      MERGE (ignore_vpath);
#undef MERGE

      file->renamed = oldfile;
    }
}

/* Remove all nonprecious intermediate files.
   If SIG is nonzero, this was caused by a fatal signal,
   meaning that a different message will be printed, and
   the message will go to stderr rather than stdout.  */

void
remove_intermediates (sig)
     int sig;
{
  register int i;
  register struct file *f;
  char doneany;

  if (question_flag || touch_flag)
    return;
  if (sig && just_print_flag)
    return;

  doneany = 0;
  for (i = 0; i < FILE_BUCKETS; ++i)
    for (f = files[i]; f != 0; f = f->next)
      if (f->intermediate && (f->dontcare || !f->precious)
	  && !f->secondary && !f->cmd_target)
	{
	  int status;
	  if (f->update_status == -1)
	    /* If nothing would have created this file yet,
	       don't print an "rm" command for it.  */
            continue;
 	  else if (just_print_flag)
  	    status = 0;
	  else
	    {
	      status = unlink (f->name);
	      if (status < 0 && errno == ENOENT)
		continue;
	    }
	  if (!f->dontcare)
	    {
	      if (sig)
		error (NILF, _("*** Deleting intermediate file `%s'"), f->name);
	      else if (!silent_flag)
		{
		  if (! doneany)
		    {
		      fputs ("rm ", stdout);
		      doneany = 1;
		    }
		  else
		    putchar (' ');
		  fputs (f->name, stdout);
		  fflush (stdout);
		}
	      if (status < 0)
		perror_with_name ("unlink: ", f->name);
	    }
	}

  if (doneany && !sig)
    {
      putchar ('\n');
      fflush (stdout);
    }
}

/* For each dependency of each file, make the `struct dep' point
   at the appropriate `struct file' (which may have to be created).

   Also mark the files depended on by .PRECIOUS, .PHONY, .SILENT,
   and various other special targets.  */

void
snap_deps ()
{
  register struct file *f, *f2;
  register struct dep *d;
  register int i;

  /* Enter each dependency name as a file.  */
  for (i = 0; i < FILE_BUCKETS; ++i)
    for (f = files[i]; f != 0; f = f->next)
      for (f2 = f; f2 != 0; f2 = f2->prev)
	for (d = f2->deps; d != 0; d = d->next)
	  if (d->name != 0)
	    {
	      d->file = lookup_file (d->name);
	      if (d->file == 0)
		d->file = enter_file (d->name);
	      else
		free (d->name);
	      d->name = 0;
	    }

  for (f = lookup_file (".PRECIOUS"); f != 0; f = f->prev)
    for (d = f->deps; d != 0; d = d->next)
      for (f2 = d->file; f2 != 0; f2 = f2->prev)
	f2->precious = 1;

  for (f = lookup_file (".PHONY"); f != 0; f = f->prev)
    for (d = f->deps; d != 0; d = d->next)
      for (f2 = d->file; f2 != 0; f2 = f2->prev)
	{
	  /* Mark this file as phony and nonexistent.  */
	  f2->phony = 1;
	  f2->last_mtime = (FILE_TIMESTAMP) -1;
	  f2->mtime_before_update = (FILE_TIMESTAMP) -1;
	}

  for (f = lookup_file (".INTERMEDIATE"); f != 0; f = f->prev)
    {
      /* .INTERMEDIATE with deps listed
	 marks those deps as intermediate files.  */
      for (d = f->deps; d != 0; d = d->next)
	for (f2 = d->file; f2 != 0; f2 = f2->prev)
	  f2->intermediate = 1;
      /* .INTERMEDIATE with no deps does nothing.
	 Marking all files as intermediates is useless
	 since the goal targets would be deleted after they are built.  */
    }

  for (f = lookup_file (".SECONDARY"); f != 0; f = f->prev)
    {
      /* .SECONDARY with deps listed
	 marks those deps as intermediate files
	 in that they don't get rebuilt if not actually needed;
	 but unlike real intermediate files,
	 these are not deleted after make finishes.  */
      if (f->deps)
	{
	  for (d = f->deps; d != 0; d = d->next)
	    for (f2 = d->file; f2 != 0; f2 = f2->prev)
	      f2->intermediate = f2->secondary = 1;
	}
      /* .SECONDARY with no deps listed marks *all* files that way.  */
      else
	{
	  int i;
	  for (i = 0; i < FILE_BUCKETS; i++)
	    for (f2 = files[i]; f2; f2= f2->next)
	      f2->intermediate = f2->secondary = 1;
	}
    }

  f = lookup_file (".EXPORT_ALL_VARIABLES");
  if (f != 0 && f->is_target)
    export_all_variables = 1;

  f = lookup_file (".IGNORE");
  if (f != 0 && f->is_target)
    {
      if (f->deps == 0)
	ignore_errors_flag = 1;
      else
	for (d = f->deps; d != 0; d = d->next)
	  for (f2 = d->file; f2 != 0; f2 = f2->prev)
	    f2->command_flags |= COMMANDS_NOERROR;
    }

  f = lookup_file (".SILENT");
  if (f != 0 && f->is_target)
    {
      if (f->deps == 0)
	silent_flag = 1;
      else
	for (d = f->deps; d != 0; d = d->next)
	  for (f2 = d->file; f2 != 0; f2 = f2->prev)
	    f2->command_flags |= COMMANDS_SILENT;
    }

  f = lookup_file (".POSIX");
  if (f != 0 && f->is_target)
    posix_pedantic = 1;

  f = lookup_file (".NOTPARALLEL");
  if (f != 0 && f->is_target)
    not_parallel = 1;
}

/* Set the `command_state' member of FILE and all its `also_make's.  */

void
set_command_state (file, state)
     struct file *file;
     int state;
{
  struct dep *d;

  file->command_state = state;

  for (d = file->also_make; d != 0; d = d->next)
    d->file->command_state = state;
}

/* Get and print file timestamps.  */

FILE_TIMESTAMP
file_timestamp_now ()
{
#if HAVE_CLOCK_GETTIME && defined CLOCK_REALTIME
  struct timespec timespec;
  if (clock_gettime (CLOCK_REALTIME, &timespec) == 0)
    return FILE_TIMESTAMP_FROM_S_AND_NS (timespec.tv_sec, timespec.tv_nsec);
#endif
  return FILE_TIMESTAMP_FROM_S_AND_NS (time ((time_t *) 0), 0);
}

void
file_timestamp_sprintf (p, ts)
     char *p;
     FILE_TIMESTAMP ts;
{
  time_t t = FILE_TIMESTAMP_S (ts);
  struct tm *tm = localtime (&t);

  if (tm)
    sprintf (p, "%04d-%02d-%02d %02d:%02d:%02d",
	     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	     tm->tm_hour, tm->tm_min, tm->tm_sec);
  else if (t < 0)
    sprintf (p, "%ld", (long) t);
  else
    sprintf (p, "%lu", (unsigned long) t);
  p += strlen (p);

  /* Append nanoseconds as a fraction, but remove trailing zeros.
     We don't know the actual timestamp resolution, since clock_getres
     applies only to local times, whereas this timestamp might come
     from a remote filesystem.  So removing trailing zeros is the
     best guess that we can do.  */
  sprintf (p, ".%09ld", (long) FILE_TIMESTAMP_NS (ts));
  p += strlen (p) - 1;
  while (*p == '0')
    p--;
  p += *p != '.';

  *p = '\0';
}

/* Print the data base of files.  */

static void
print_file (f)
     struct file *f;
{
  register struct dep *d;

  putchar ('\n');
  if (!f->is_target)
    puts (_("# Not a target:"));
  printf ("%s:%s", f->name, f->double_colon ? ":" : "");

  for (d = f->deps; d != 0; d = d->next)
    printf (" %s", dep_name (d));
  putchar ('\n');

  if (f->precious)
    puts (_("#  Precious file (prerequisite of .PRECIOUS)."));
  if (f->phony)
    puts (_("#  Phony target (prerequisite of .PHONY)."));
  if (f->cmd_target)
    puts (_("#  Command-line target."));
  if (f->dontcare)
    puts (_("#  A default or MAKEFILES makefile."));
  puts (f->tried_implicit
        ? _("#  Implicit rule search has been done.")
        : _("#  Implicit rule search has not been done."));
  if (f->stem != 0)
    printf (_("#  Implicit/static pattern stem: `%s'\n"), f->stem);
  if (f->intermediate)
    puts (_("#  File is an intermediate prerequisite."));
  if (f->also_make != 0)
    {
      fputs (_("#  Also makes:"), stdout);
      for (d = f->also_make; d != 0; d = d->next)
	printf (" %s", dep_name (d));
      putchar ('\n');
    }
  if (f->last_mtime == 0)
    puts (_("#  Modification time never checked."));
  else if (f->last_mtime == (FILE_TIMESTAMP) -1)
    puts (_("#  File does not exist."));
  else
    {
      char buf[FILE_TIMESTAMP_PRINT_LEN_BOUND + 1];
      file_timestamp_sprintf (buf, f->last_mtime);
      printf("#  Last modified 00:00 Jan 01 2000\n");
      /* printf (_("#  Last modified %s\n"), buf); */
    }
  puts (f->updated
        ? _("#  File has been updated.") : _("#  File has not been updated."));
  switch (f->command_state)
    {
    case cs_running:
      puts (_("#  Commands currently running (THIS IS A BUG)."));
      break;
    case cs_deps_running:
      puts (_("#  Dependencies commands running (THIS IS A BUG)."));
      break;
    case cs_not_started:
    case cs_finished:
      switch (f->update_status)
	{
	case -1:
	  break;
	case 0:
	  puts (_("#  Successfully updated."));
	  break;
	case 1:
	  assert (question_flag);
	  puts (_("#  Needs to be updated (-q is set)."));
	  break;
	case 2:
	  puts (_("#  Failed to be updated."));
	  break;
	default:
	  puts (_("#  Invalid value in `update_status' member!"));
	  fflush (stdout);
	  fflush (stderr);
	  abort ();
	}
      break;
    default:
      puts (_("#  Invalid value in `command_state' member!"));
      fflush (stdout);
      fflush (stderr);
      abort ();
    }

  if (f->variables != 0)
    print_file_variables (f);

  if (f->cmds != 0)
    print_commands (f->cmds);
}

void
print_file_data_base ()
{
  register unsigned int i, nfiles, per_bucket;
  register struct file *file;

  puts (_("\n# Files"));

  per_bucket = nfiles = 0;
  for (i = 0; i < FILE_BUCKETS; ++i)
    {
      register unsigned int this_bucket = 0;

      for (file = files[i]; file != 0; file = file->next)
	{
	  register struct file *f;

	  ++this_bucket;

	  for (f = file; f != 0; f = f->prev)
	    print_file (f);
	}

      nfiles += this_bucket;
      if (this_bucket > per_bucket)
	per_bucket = this_bucket;
    }

  if (nfiles == 0)
    puts (_("\n# No files."));
  else
    {
      printf (_("\n# %u files in %u hash buckets.\n"), nfiles, FILE_BUCKETS);
#ifndef	NO_FLOAT
      printf (_("# average %.3f files per bucket, max %u files in one bucket.\n"),
	      ((double) nfiles) / ((double) FILE_BUCKETS), per_bucket);
#endif
    }
}

/* EOF */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         function.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Builtin function expansion for GNU Make.
Copyright (C) 1988,1989,1991-1997,1999 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"
#undef stderr
#define stderr stdout

#ifdef _AMIGA
#include "amiga.h"
#undef stderr
#define stderr stdout
#endif


struct function_table_entry
  {
    const char *name;
    unsigned char len;
    unsigned char minimum_args;
    unsigned char maximum_args;
    char expand_args;
    char *(*func_ptr) PARAMS ((char *output, char **argv, const char *fname));
  };


/* Store into VARIABLE_BUFFER at O the result of scanning TEXT and replacing
   each occurrence of SUBST with REPLACE. TEXT is null-terminated.  SLEN is
   the length of SUBST and RLEN is the length of REPLACE.  If BY_WORD is
   nonzero, substitutions are done only on matches which are complete
   whitespace-delimited words.  If SUFFIX_ONLY is nonzero, substitutions are
   done only at the ends of whitespace-delimited words.  */

char *
subst_expand (o, text, subst, replace, slen, rlen, by_word, suffix_only)
     char *o;
     char *text;
     char *subst, *replace;
     unsigned int slen, rlen;
     int by_word, suffix_only;
{
  register char *t = text;
  register char *p;

  if (slen == 0 && !by_word && !suffix_only)
    {
      /* The first occurrence of "" in any string is its end.  */
      o = variable_buffer_output (o, t, strlen (t));
      if (rlen > 0)
	o = variable_buffer_output (o, replace, rlen);
      return o;
    }

  do
    {
      if ((by_word | suffix_only) && slen == 0)
	/* When matching by words, the empty string should match
	   the end of each word, rather than the end of the whole text.  */
	p = end_of_token (next_token (t));
      else
	{
	  p = sindex (t, 0, subst, slen);
	  if (p == 0)
	    {
	      /* No more matches.  Output everything left on the end.  */
	      o = variable_buffer_output (o, t, strlen (t));
	      return o;
	    }
	}

      /* Output everything before this occurrence of the string to replace.  */
      if (p > t)
	o = variable_buffer_output (o, t, p - t);

      /* If we're substituting only by fully matched words,
	 or only at the ends of words, check that this case qualifies.  */
      if ((by_word
	   && ((p > t && !isblank (p[-1]))
	       || (p[slen] != '\0' && !isblank (p[slen]))))
	  || (suffix_only
	      && (p[slen] != '\0' && !isblank (p[slen]))))
	/* Struck out.  Output the rest of the string that is
	   no longer to be replaced.  */
	o = variable_buffer_output (o, subst, slen);
      else if (rlen > 0)
	/* Output the replacement string.  */
	o = variable_buffer_output (o, replace, rlen);

      /* Advance T past the string to be replaced.  */
      t = p + slen;
    } while (*t != '\0');

  return o;
}


/* Store into VARIABLE_BUFFER at O the result of scanning TEXT
   and replacing strings matching PATTERN with REPLACE.
   If PATTERN_PERCENT is not nil, PATTERN has already been
   run through find_percent, and PATTERN_PERCENT is the result.
   If REPLACE_PERCENT is not nil, REPLACE has already been
   run through find_percent, and REPLACE_PERCENT is the result.  */

char *
patsubst_expand (o, text, pattern, replace, pattern_percent, replace_percent)
     char *o;
     char *text;
     register char *pattern, *replace;
     register char *pattern_percent, *replace_percent;
{
  unsigned int pattern_prepercent_len, pattern_postpercent_len;
  unsigned int replace_prepercent_len, replace_postpercent_len = 0;
  char *t;
  int len;
  int doneany = 0;

  /* We call find_percent on REPLACE before checking PATTERN so that REPLACE
     will be collapsed before we call subst_expand if PATTERN has no %.  */
  if (replace_percent == 0)
    replace_percent = find_percent (replace);
  if (replace_percent != 0)
    {
      /* Record the length of REPLACE before and after the % so
	 we don't have to compute these lengths more than once.  */
      replace_prepercent_len = replace_percent - replace;
      replace_postpercent_len = strlen (replace_percent + 1);
    }
  else
    /* We store the length of the replacement
       so we only need to compute it once.  */
    replace_prepercent_len = strlen (replace);

  if (pattern_percent == 0)
    pattern_percent = find_percent (pattern);
  if (pattern_percent == 0)
    /* With no % in the pattern, this is just a simple substitution.  */
    return subst_expand (o, text, pattern, replace,
			 strlen (pattern), strlen (replace), 1, 0);

  /* Record the length of PATTERN before and after the %
     so we don't have to compute it more than once.  */
  pattern_prepercent_len = pattern_percent - pattern;
  pattern_postpercent_len = strlen (pattern_percent + 1);

  while ((t = find_next_token (&text, &len)) != 0)
    {
      int fail = 0;

      /* Is it big enough to match?  */
      if (len < pattern_prepercent_len + pattern_postpercent_len)
	fail = 1;

      /* Does the prefix match? */
      if (!fail && pattern_prepercent_len > 0
	  && (*t != *pattern
	      || t[pattern_prepercent_len - 1] != pattern_percent[-1]
	      || !strneq (t + 1, pattern + 1, pattern_prepercent_len - 1)))
	fail = 1;

      /* Does the suffix match? */
      if (!fail && pattern_postpercent_len > 0
	  && (t[len - 1] != pattern_percent[pattern_postpercent_len]
	      || t[len - pattern_postpercent_len] != pattern_percent[1]
	      || !strneq (&t[len - pattern_postpercent_len],
			  &pattern_percent[1], pattern_postpercent_len - 1)))
	fail = 1;

      if (fail)
	/* It didn't match.  Output the string.  */
	o = variable_buffer_output (o, t, len);
      else
	{
	  /* It matched.  Output the replacement.  */

	  /* Output the part of the replacement before the %.  */
	  o = variable_buffer_output (o, replace, replace_prepercent_len);

	  if (replace_percent != 0)
	    {
	      /* Output the part of the matched string that
		 matched the % in the pattern.  */
	      o = variable_buffer_output (o, t + pattern_prepercent_len,
					  len - (pattern_prepercent_len
						 + pattern_postpercent_len));
	      /* Output the part of the replacement after the %.  */
	      o = variable_buffer_output (o, replace_percent + 1,
					  replace_postpercent_len);
	    }
	}

      /* Output a space, but not if the replacement is "".  */
      if (fail || replace_prepercent_len > 0
	  || (replace_percent != 0 && len + replace_postpercent_len > 0))
	{
	  o = variable_buffer_output (o, " ", 1);
	  doneany = 1;
	}
    }
  if (doneany)
    /* Kill the last space.  */
    --o;

  return o;
}


/* Look up a function by name.
   The table is currently small enough that it's not really worthwhile to use
   a fancier lookup algorithm.  If it gets larger, maybe...
*/

static const struct function_table_entry *
lookup_function (table, s)
     const struct function_table_entry *table;
     const char *s;
{
  int len = strlen (s);

  for (; table->name != NULL; ++table)
    if (table->len <= len
        && (isblank (s[table->len]) || s[table->len] == '\0')
        && strneq (s, table->name, table->len))
      return table;

  return NULL;
}


/* Return 1 if PATTERN matches STR, 0 if not.  */

int
pattern_matches (pattern, percent, str)
     register char *pattern, *percent, *str;
{
  unsigned int sfxlen, strlength;

  if (percent == 0)
    {
      unsigned int len = strlen (pattern) + 1;
      char *new_chars = (char *) alloca (len);
      bcopy (pattern, new_chars, len);
      pattern = new_chars;
      percent = find_percent (pattern);
      if (percent == 0)
	return streq (pattern, str);
    }

  sfxlen = strlen (percent + 1);
  strlength = strlen (str);

  if (strlength < (percent - pattern) + sfxlen
      || !strneq (pattern, str, percent - pattern))
    return 0;

  return !strcmp (percent + 1, str + (strlength - sfxlen));
}


/* Find the next comma or ENDPAREN (counting nested STARTPAREN and
   ENDPARENtheses), starting at PTR before END.  Return a pointer to
   next character.

   If no next argument is found, return NULL.
*/

static char *
find_next_argument (startparen, endparen, ptr, end)
     char startparen;
     char endparen;
     const char *ptr;
     const char *end;
{
  int count = 0;

  for (; ptr < end; ++ptr)
    if (*ptr == startparen)
      ++count;

    else if (*ptr == endparen)
      {
	--count;
	if (count < 0)
	  return NULL;
      }

    else if (*ptr == ',' && !count)
      return (char *)ptr;

  /* We didn't find anything.  */
  return NULL;
}


/* Glob-expand LINE.  The returned pointer is
   only good until the next call to string_glob.  */

static char *
string_glob (line)
     char *line;
{
  static char *result = 0;
  static unsigned int length;
  register struct nameseq *chain;
  register unsigned int idx;

  chain = multi_glob (parse_file_seq
		      (&line, '\0', sizeof (struct nameseq),
		       /* We do not want parse_file_seq to strip `./'s.
			  That would break examples like:
			  $(patsubst ./%.c,obj/%.o,$(wildcard ./?*.c)).  */
		       0),
		      sizeof (struct nameseq));

  if (result == 0)
    {
      length = 100;
      result = (char *) xmalloc (100);
    }

  idx = 0;
  while (chain != 0)
    {
      register char *name = chain->name;
      unsigned int len = strlen (name);

      struct nameseq *next = chain->next;
      free ((char *) chain);
      chain = next;

      /* multi_glob will pass names without globbing metacharacters
	 through as is, but we want only files that actually exist.  */
      if (file_exists_p (name))
	{
	  if (idx + len + 1 > length)
	    {
	      length += (len + 1) * 2;
	      result = (char *) xrealloc (result, length);
	    }
	  bcopy (name, &result[idx], len);
	  idx += len;
	  result[idx++] = ' ';
	}

      free (name);
    }

  /* Kill the last space and terminate the string.  */
  if (idx == 0)
    result[0] = '\0';
  else
    result[idx - 1] = '\0';

  return result;
}

/*
  Builtin functions
 */

static char *
func_patsubst (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  o = patsubst_expand (o, argv[2], argv[0], argv[1], (char *) 0, (char *) 0);
  return o;
}


static char *
func_join (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  int doneany = 0;

  /* Write each word of the first argument directly followed
     by the corresponding word of the second argument.
     If the two arguments have a different number of words,
     the excess words are just output separated by blanks.  */
  register char *tp;
  register char *pp;
  char *list1_iterator = argv[0];
  char *list2_iterator = argv[1];
  do
    {
      unsigned int len1, len2;

      tp = find_next_token (&list1_iterator, &len1);
      if (tp != 0)
	o = variable_buffer_output (o, tp, len1);

      pp = find_next_token (&list2_iterator, &len2);
      if (pp != 0)
	o = variable_buffer_output (o, pp, len2);

      if (tp != 0 || pp != 0)
	{
	  o = variable_buffer_output (o, " ", 1);
	  doneany = 1;
	}
    }
  while (tp != 0 || pp != 0);
  if (doneany)
    /* Kill the last blank.  */
    --o;

  return o;
}


static char *
func_origin (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  /* Expand the argument.  */
  register struct variable *v = lookup_variable (argv[0], strlen (argv[0]));
  if (v == 0)
    o = variable_buffer_output (o, "undefined", 9);
  else
    switch (v->origin)
      {
      default:
      case o_invalid:
	abort ();
	break;
      case o_default:
	o = variable_buffer_output (o, "default", 7);
	break;
      case o_env:
	o = variable_buffer_output (o, "environment", 11);
	break;
      case o_file:
	o = variable_buffer_output (o, "file", 4);
	break;
      case o_env_override:
	o = variable_buffer_output (o, "environment override", 20);
	break;
      case o_command:
	o = variable_buffer_output (o, "command line", 12);
	break;
      case o_override:
	o = variable_buffer_output (o, "override", 8);
	break;
      case o_automatic:
	o = variable_buffer_output (o, "automatic", 9);
	break;
      }

  return o;
}

#ifdef VMS
#define IS_PATHSEP(c) ((c) == ']')
#else
#if defined(__MSDOS__) || defined(WINDOWS32)
#define IS_PATHSEP(c) ((c) == '/' || (c) == '\\')
#else
#define IS_PATHSEP(c) ((c) == '/')
#endif
#endif


static char *
func_notdir_suffix (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  /* Expand the argument.  */
  char *list_iterator = argv[0];
  char *p2 =0;
  int doneany =0;
  unsigned int len=0;

  int is_suffix = streq (funcname, "suffix");
  int is_notdir = !is_suffix;
  while ((p2 = find_next_token (&list_iterator, &len)) != 0)
    {
      char *p = p2 + len;


      while (p >= p2 && (!is_suffix || *p != '.'))
	{
	  if (IS_PATHSEP (*p))
	    break;
	  --p;
	}

      if (p >= p2)
	{
	  if (is_notdir)
	    ++p;
	  else if (*p != '.')
	    continue;
	  o = variable_buffer_output (o, p, len - (p - p2));
	}
#if defined(WINDOWS32) || defined(__MSDOS__)
      /* Handle the case of "d:foo/bar".  */
      else if (streq (funcname, "notdir") && p2[0] && p2[1] == ':')
	{
	  p = p2 + 2;
	  o = variable_buffer_output (o, p, len - (p - p2));
	}
#endif
      else if (is_notdir)
	o = variable_buffer_output (o, p2, len);

      if (is_notdir || p >= p2)
	{
	  o = variable_buffer_output (o, " ", 1);
	  doneany = 1;
	}
    }
  if (doneany)
    /* Kill last space.  */
    --o;


  return o;

}


static char *
func_basename_dir (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  /* Expand the argument.  */
  char *p3 = argv[0];
  char *p2=0;
  int doneany=0;
  unsigned int len=0;
  char *p=0;
  int is_basename= streq (funcname, "basename");
  int is_dir= !is_basename;

  while ((p2 = find_next_token (&p3, &len)) != 0)
	{
	  p = p2 + len;
	  while (p >= p2 && (!is_basename  || *p != '.'))
	    {
	      if (IS_PATHSEP (*p))
		break;
	      	    --p;
	    }

	  if (p >= p2 && (is_dir))
	    o = variable_buffer_output (o, p2, ++p - p2);
	  else if (p >= p2 && (*p == '.'))
	    o = variable_buffer_output (o, p2, p - p2);
#if defined(WINDOWS32) || defined(__MSDOS__)
	/* Handle the "d:foobar" case */
	  else if (p2[0] && p2[1] == ':' && is_dir)
	    o = variable_buffer_output (o, p2, 2);
#endif
	  else if (is_dir)
#ifdef VMS
	    o = variable_buffer_output (o, "[]", 2);
#else
#ifndef _AMIGA
	    o = variable_buffer_output (o, "./", 2);
#else
	    ; /* Just a nop...  */
#endif /* AMIGA */
#endif /* !VMS */
	  else
	    /* The entire name is the basename.  */
	    o = variable_buffer_output (o, p2, len);

	  o = variable_buffer_output (o, " ", 1);
	  doneany = 1;
	}
      if (doneany)
	/* Kill last space.  */
	--o;


 return o;
}

static char *
func_addsuffix_addprefix (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  int fixlen = strlen (argv[0]);
  char *list_iterator = argv[1];
  int is_addprefix = streq (funcname, "addprefix");
  int is_addsuffix = !is_addprefix;

  int doneany = 0;
  char *p;
  unsigned int len;

  while ((p = find_next_token (&list_iterator, &len)) != 0)
    {
      if (is_addprefix)
	o = variable_buffer_output (o, argv[0], fixlen);
      o = variable_buffer_output (o, p, len);
      if (is_addsuffix)
	o = variable_buffer_output (o, argv[0], fixlen);
      o = variable_buffer_output (o, " ", 1);
      doneany = 1;
    }

  if (doneany)
    /* Kill last space.  */
    --o;

  return o;
}

static char *
func_subst (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  o = subst_expand (o, argv[2], argv[0], argv[1], strlen (argv[0]),
		    strlen (argv[1]), 0, 0);

  return o;
}


static char *
func_firstword (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  unsigned int i;
  char *words = argv[0];    /* Use a temp variable for find_next_token */
  char *p = find_next_token (&words, &i);

  if (p != 0)
    o = variable_buffer_output (o, p, i);

  return o;
}


static char *
func_words (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  int i = 0;
  char *word_iterator = argv[0];
  char buf[20];

  while (find_next_token (&word_iterator, (unsigned int *) 0) != 0)
    ++i;

  sprintf (buf, "%d", i);
  o = variable_buffer_output (o, buf, strlen (buf));


  return o;
}

char *
strip_whitespace (begpp, endpp)
     char **begpp;
     char **endpp;
{
  while (isspace ((unsigned char)**begpp) && *begpp <= *endpp)
    (*begpp) ++;
  while (isspace ((unsigned char)**endpp) && *endpp >= *begpp)
    (*endpp) --;
  return *begpp;
}

int
is_numeric (p)
     char *p;
{
  char *end = p + strlen (p) - 1;
  char *beg = p;
  strip_whitespace (&p, &end);

  while (p <= end)
    if (!ISDIGIT (*(p++)))  /* ISDIGIT only evals its arg once: see make.h.  */
      return 0;

  return (end - beg >= 0);
}

void
check_numeric (s, message)
     char *s;
     char *message;
{
  if (!is_numeric (s))
    fatal (reading_file, message);
}



static char *
func_word (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char *end_p=0;
  int i=0;
  char *p=0;

  /* Check the first argument.  */
  check_numeric (argv[0], _("non-numeric first argument to `word' function"));
  i =  atoi (argv[0]);

  if (i == 0)
    fatal (reading_file, _("the `word' function takes a positive index argument"));


  end_p = argv[1];
  while ((p = find_next_token (&end_p, 0)) != 0)
    if (--i == 0)
      break;

  if (i == 0)
    o = variable_buffer_output (o, p, end_p - p);

  return o;
}

static char *
func_wordlist (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  int start, count;

  /* Check the arguments.  */
  check_numeric (argv[0],
		 _("non-numeric first argument to `wordlist' function"));
  check_numeric (argv[1],
		 _("non-numeric second argument to `wordlist' function"));

  start = atoi (argv[0]);
  count = atoi (argv[1]) - start + 1;

  if (count > 0)
    {
      char *p;
      char *end_p = argv[2];

      /* Find the beginning of the "start"th word.  */
      while (((p = find_next_token (&end_p, 0)) != 0) && --start)
        ;

      if (p)
        {
          /* Find the end of the "count"th word from start.  */
          while (--count && (find_next_token (&end_p, 0) != 0))
            ;

          /* Return the stuff in the middle.  */
          o = variable_buffer_output (o, p, end_p - p);
        }
    }

  return o;
}

static char*
func_findstring (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  /* Find the first occurrence of the first string in the second.  */
  int i = strlen (argv[0]);
  if (sindex (argv[1], 0, argv[0], i) != 0)
    o = variable_buffer_output (o, argv[0], i);

  return o;
}

static char *
func_foreach (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  /* expand only the first two.  */
  char *varname = expand_argument (argv[0], NULL);
  char *list = expand_argument (argv[1], NULL);
  char *body = argv[2];

  int doneany = 0;
  char *list_iterator = list;
  char *p;
  unsigned int len;
  register struct variable *var;

  push_new_variable_scope ();
  var = define_variable (varname, strlen (varname), "", o_automatic, 0);

  /* loop through LIST,  put the value in VAR and expand BODY */
  while ((p = find_next_token (&list_iterator, &len)) != 0)
    {
      char *result = 0;

      {
	char save = p[len];

	p[len] = '\0';
	free (var->value);
	var->value = (char *) xstrdup ((char*) p);
	p[len] = save;
      }

      result = allocated_variable_expand (body);

      o = variable_buffer_output (o, result, strlen (result));
      o = variable_buffer_output (o, " ", 1);
      doneany = 1;
      free (result);
    }

  if (doneany)
    /* Kill the last space.  */
    --o;

  pop_variable_scope ();
  free (varname);
  free (list);

  return o;
}

struct a_word
{
  struct a_word *next;
  char *str;
  int matched;
};

static char *
func_filter_filterout (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  struct a_word *wordhead = 0;
  struct a_word *wordtail = 0;

  int is_filter = streq (funcname, "filter");
  char *patterns = argv[0];
  char *word_iterator = argv[1];

  char *p;
  unsigned int len;

  /* Chop ARGV[1] up into words and then run each pattern through.  */
  while ((p = find_next_token (&word_iterator, &len)) != 0)
    {
      struct a_word *w = (struct a_word *)alloca (sizeof (struct a_word));
      if (wordhead == 0)
	wordhead = w;
      else
	wordtail->next = w;
      wordtail = w;

      if (*word_iterator != '\0')
	++word_iterator;
      p[len] = '\0';
      w->str = p;
      w->matched = 0;
    }

  if (wordhead != 0)
    {
      char *pat_iterator = patterns;
      int doneany = 0;
      struct a_word *wp;

      wordtail->next = 0;

      /* Run each pattern through the words, killing words.  */
      while ((p = find_next_token (&pat_iterator, &len)) != 0)
	{
	  char *percent;
	  char save = p[len];
	  p[len] = '\0';

	  percent = find_percent (p);
	  for (wp = wordhead; wp != 0; wp = wp->next)
	    wp->matched |= (percent == 0 ? streq (p, wp->str)
			    : pattern_matches (p, percent, wp->str));

	  p[len] = save;
	}

      /* Output the words that matched (or didn't, for filter-out).  */
      for (wp = wordhead; wp != 0; wp = wp->next)
	if (is_filter ? wp->matched : !wp->matched)
	  {
	    o = variable_buffer_output (o, wp->str, strlen (wp->str));
	    o = variable_buffer_output (o, " ", 1);
	    doneany = 1;
	  }

      if (doneany)
	/* Kill the last space.  */
	--o;
    }

  return o;
}


static char *
func_strip (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char *p = argv[0];
  int doneany =0;

  while (*p != '\0')
    {
      int i=0;
      char *word_start=0;

      while (isspace ((unsigned char)*p))
	++p;
      word_start = p;
      for (i=0; *p != '\0' && !isspace ((unsigned char)*p); ++p, ++i)
	{}
      if (!i)
	break;
      o = variable_buffer_output (o, word_start, i);
      o = variable_buffer_output (o, " ", 1);
      doneany = 1;
    }

  if (doneany)
    /* Kill the last space.  */
    --o;
  return o;
}

/*
  Print a warning or fatal message.
*/
static char *
func_error (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char **argvp;
  char *msg, *p;
  int len;

  /* The arguments will be broken on commas.  Rather than create yet
     another special case where function arguments aren't broken up,
     just create a format string that puts them back together.  */
  for (len=0, argvp=argv; *argvp != 0; ++argvp)
    len += strlen (*argvp) + 2;

  p = msg = alloca (len + 1);

  for (argvp=argv; argvp[1] != 0; ++argvp)
    {
      strcpy (p, *argvp);
      p += strlen (*argvp);
      *(p++) = ',';
      *(p++) = ' ';
    }
  strcpy (p, *argvp);

  if (*funcname == 'e')
    fatal (reading_file, "%s", msg);

  /* The warning function expands to the empty string.  */
  error (reading_file, "%s", msg);

  return o;
}


/*
  chop argv[0] into words, and sort them.
 */
static char *
func_sort (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char **words = 0;
  int nwords = 0;
  register int wordi = 0;

  /* Chop ARGV[0] into words and put them in WORDS.  */
  char *t = argv[0];
  char *p;
  unsigned int len;
  int i;

  while ((p = find_next_token (&t, &len)) != 0)
    {
      if (wordi >= nwords - 1)
	{
	  nwords = (2 * nwords) + 5;
	  words = (char **) xrealloc ((char *) words,
				      nwords * sizeof (char *));
	}
      words[wordi++] = savestring (p, len);
    }

  if (!wordi)
    return o;

  /* Now sort the list of words.  */
  qsort ((char *) words, wordi, sizeof (char *), alpha_compare);

  /* Now write the sorted list.  */
  for (i = 0; i < wordi; ++i)
    {
      len = strlen (words[i]);
      if (i == wordi - 1 || strlen (words[i + 1]) != len
          || strcmp (words[i], words[i + 1]))
        {
          o = variable_buffer_output (o, words[i], len);
          o = variable_buffer_output (o, " ", 1);
        }
      free (words[i]);
    }
  /* Kill the last space.  */
  --o;

  free (words);

  return o;
}

/*
  $(if condition,true-part[,false-part])

  CONDITION is false iff it evaluates to an empty string.  White
  space before and after condition are stripped before evaluation.

  If CONDITION is true, then TRUE-PART is evaluated, otherwise FALSE-PART is
  evaluated (if it exists).  Because only one of the two PARTs is evaluated,
  you can use $(if ...) to create side-effects (with $(shell ...), for
  example).
*/

static char *
func_if (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char *begp = argv[0];
  char *endp = begp + strlen (argv[0]);
  int result = 0;

  /* Find the result of the condition: if we have a value, and it's not
     empty, the condition is true.  If we don't have a value, or it's the
     empty string, then it's false.  */

  strip_whitespace (&begp, &endp);

  if (begp < endp)
    {
      char *expansion = expand_argument (begp, NULL);

      result = strlen (expansion);
      free (expansion);
    }

  /* If the result is true (1) we want to eval the first argument, and if
     it's false (0) we want to eval the second.  If the argument doesn't
     exist we do nothing, otherwise expand it and add to the buffer.  */

  argv += 1 + !result;

  if (argv[0])
    {
      char *expansion;

      expansion = expand_argument (argv[0], NULL);

      o = variable_buffer_output (o, expansion, strlen (expansion));

      free (expansion);
    }

  return o;
}

static char *
func_wildcard (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{

#ifdef _AMIGA
   o = wildcard_expansion (argv[0], o);
#else
   char *p = string_glob (argv[0]);
   o = variable_buffer_output (o, p, strlen (p));
#endif
   return o;
}

/*
  \r  is replaced on UNIX as well. Is this desirable?
 */
void
fold_newlines (buffer, length)
     char *buffer;
     int *length;
{
  char *dst = buffer;
  char *src = buffer;
  char *last_nonnl = buffer -1;
  src[*length] = 0;
  for (; *src != '\0'; ++src)
    {
      if (src[0] == '\r' && src[1] == '\n')
	continue;
      if (*src == '\n')
	{
	  *dst++ = ' ';
	}
      else
	{
	  last_nonnl = dst;
	  *dst++ = *src;
	}
    }
  *(++last_nonnl) = '\0';
  *length = last_nonnl - buffer;
}



int shell_function_pid = 0, shell_function_completed;


#ifdef WINDOWS32
/*untested*/

#include <windows.h>
#include <io.h>
#include "sub_proc.h"
#undef stderr
#define stderr stdout


void
windows32_openpipe (int *pipedes, int *pid_p, char **command_argv, char **envp)
{
  SECURITY_ATTRIBUTES saAttr;
  HANDLE hIn;
  HANDLE hErr;
  HANDLE hChildOutRd;
  HANDLE hChildOutWr;
  HANDLE hProcess;


  saAttr.nLength = sizeof (SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (DuplicateHandle (GetCurrentProcess(),
		      GetStdHandle(STD_INPUT_HANDLE),
		      GetCurrentProcess(),
		      &hIn,
		      0,
		      TRUE,
		      DUPLICATE_SAME_ACCESS) == FALSE) {
    fatal (NILF, _("create_child_process: DuplicateHandle(In) failed (e=%d)\n"),
	   GetLastError());

  }
  if (DuplicateHandle(GetCurrentProcess(),
		      GetStdHandle(STD_ERROR_HANDLE),
		      GetCurrentProcess(),
		      &hErr,
		      0,
		      TRUE,
		      DUPLICATE_SAME_ACCESS) == FALSE) {
    fatal (NILF, _("create_child_process: DuplicateHandle(Err) failed (e=%d)\n"),
	   GetLastError());
  }

  if (!CreatePipe(&hChildOutRd, &hChildOutWr, &saAttr, 0))
    fatal (NILF, _("CreatePipe() failed (e=%d)\n"), GetLastError());

  hProcess = process_init_fd(hIn, hChildOutWr, hErr);

  if (!hProcess)
    fatal (NILF, _("windows32_openpipe (): process_init_fd() failed\n"));

  /* make sure that CreateProcess() has Path it needs */
  sync_Path_environment();

  if (!process_begin(hProcess, command_argv, envp, command_argv[0], NULL)) {
    /* register process for wait */
    process_register(hProcess);

    /* set the pid for returning to caller */
    *pid_p = (int) hProcess;

  /* set up to read data from child */
  pipedes[0] = _open_osfhandle((long) hChildOutRd, O_RDONLY);

  /* this will be closed almost right away */
  pipedes[1] = _open_osfhandle((long) hChildOutWr, O_APPEND);
  } else {
    /* reap/cleanup the failed process */
	process_cleanup(hProcess);

    /* close handles which were duplicated, they weren't used */
	CloseHandle(hIn);
	CloseHandle(hErr);

	/* close pipe handles, they won't be used */
	CloseHandle(hChildOutRd);
	CloseHandle(hChildOutWr);

    /* set status for return */
    pipedes[0] = pipedes[1] = -1;
    *pid_p = -1;
  }
}
#endif


#ifdef __MSDOS__
FILE *
msdos_openpipe (int* pipedes, int *pidp, char *text)
{
  FILE *fpipe=0;
  /* MSDOS can't fork, but it has `popen'.  */
  struct variable *sh = lookup_variable ("SHELL", 5);
  int e;
  extern int dos_command_running, dos_status;

  /* Make sure not to bother processing an empty line.  */
  while (isblank (*text))
    ++text;
  if (*text == '\0')
    return 0;

  if (sh)
    {
      char buf[PATH_MAX + 7];
      /* This makes sure $SHELL value is used by $(shell), even
	 though the target environment is not passed to it.  */
      sprintf (buf, "SHELL=%s", sh->value);
      putenv (buf);
    }

  e = errno;
  errno = 0;
  dos_command_running = 1;
  dos_status = 0;
  /* If dos_status becomes non-zero, it means the child process
     was interrupted by a signal, like SIGINT or SIGQUIT.  See
     fatal_error_signal in commands.c.  */
  fpipe = popen (text, "rt");
  dos_command_running = 0;
  if (!fpipe || dos_status)
    {
      pipedes[0] = -1;
      *pidp = -1;
      if (dos_status)
	errno = EINTR;
      else if (errno == 0)
	errno = ENOMEM;
      shell_function_completed = -1;
    }
  else
    {
      pipedes[0] = fileno (fpipe);
      *pidp = 42; /* Yes, the Meaning of Life, the Universe, and Everything! */
      errno = e;
      shell_function_completed = 1;
    }
  return fpipe;
}
#endif

/*
  Do shell spawning, with the naughty bits for different OSes.
 */

#ifdef VMS

/* VMS can't do $(shell ...)  */
#define func_shell 0

#else
#ifndef _AMIGA
static char *
func_shell (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char* batch_filename = NULL;
  int i;

#ifdef __MSDOS__
  FILE *fpipe;
#endif
  char **command_argv;
  char *error_prefix;
  char **envp;
  int pipedes[2];
  int pid;

#ifndef __MSDOS__
  /* Construct the argument list.  */
  command_argv = construct_command_argv (argv[0],
					 (char **) NULL, (struct file *) 0,
                                         &batch_filename);
  if (command_argv == 0)
    return o;
#endif

  /* Using a target environment for `shell' loses in cases like:
     export var = $(shell echo foobie)
     because target_environment hits a loop trying to expand $(var)
     to put it in the environment.  This is even more confusing when
     var was not explicitly exported, but just appeared in the
     calling environment.  */

  envp = environ;

  /* For error messages.  */
  if (reading_file != 0)
    {
      error_prefix = (char *) alloca (strlen (reading_file->filenm)+11+4);
      sprintf (error_prefix,
	       "%s:%lu: ", reading_file->filenm, reading_file->lineno);
    }
  else
    error_prefix = "";

#ifdef WINDOWS32
  windows32_openpipe (pipedes, &pid, command_argv, envp);

  if (pipedes[0] < 0) {
	/* open of the pipe failed, mark as failed execution */
    shell_function_completed = -1;

	return o;
  } else
#else /* WINDOWS32 */

# ifdef __MSDOS__
  fpipe = msdos_openpipe (pipedes, &pid, argv[0]);
  if (pipedes[0] < 0)
    {
      perror_with_name (error_prefix, "pipe");
      return o;
    }
# else
  if (pipe (pipedes) < 0)
    {
      perror_with_name (error_prefix, "pipe");
      return o;
    }

  pid = vfork ();
  if (pid < 0)
    perror_with_name (error_prefix, "fork");
  else if (pid == 0)
    child_execute_job (0, pipedes[1], command_argv, envp);
  else
# endif /* ! __MSDOS__ */

#endif /* WINDOWS32 */
    {
      /* We are the parent.  */

      char *buffer;
      unsigned int maxlen;
      int cc;

      /* Record the PID for reap_children.  */
      shell_function_pid = pid;
#ifndef  __MSDOS__
      shell_function_completed = 0;

      /* Free the storage only the child needed.  */
      free (command_argv[0]);
      free ((char *) command_argv);

      /* Close the write side of the pipe.  */
      (void) close (pipedes[1]);
#endif

      /* Set up and read from the pipe.  */

      maxlen = 200;
      buffer = (char *) xmalloc (maxlen + 1);

      /* Read from the pipe until it gets EOF.  */
      i = 0;
      do
	{
	  if (i == maxlen)
	    {
	      maxlen += 512;
	      buffer = (char *) xrealloc (buffer, maxlen + 1);
	    }

	  errno = 0;
	  cc = read (pipedes[0], &buffer[i], maxlen - i);
	  if (cc > 0)
	    i += cc;
	}
      while (cc > 0 || EINTR_SET);

      /* Close the read side of the pipe.  */
#ifdef  __MSDOS__
      if (fpipe)
	(void) pclose (fpipe);
#else
      (void) close (pipedes[0]);
#endif

      /* Loop until child_handler sets shell_function_completed
	 to the status of our child shell.  */
      while (shell_function_completed == 0)
	reap_children (1, 0);

      if (batch_filename) {
	DB (DB_VERBOSE, (_("Cleaning up temporary batch file %s\n"),
                       batch_filename));
	remove (batch_filename);
	free (batch_filename);
      }
      shell_function_pid = 0;

      /* The child_handler function will set shell_function_completed
	 to 1 when the child dies normally, or to -1 if it
	 dies with status 127, which is most likely an exec fail.  */

      if (shell_function_completed == -1)
	{
	  /* This most likely means that the execvp failed,
	     so we should just write out the error message
	     that came in over the pipe from the child.  */
	  fputs (buffer, stderr);
	  fflush (stderr);
	}
      else
	{
	  /* The child finished normally.  Replace all
	     newlines in its output with spaces, and put
	     that in the variable output buffer.  */
	  fold_newlines (buffer, &i);
	  o = variable_buffer_output (o, buffer, i);
	}

      free (buffer);
    }

  return o;
}

#else	/* _AMIGA */

/* Do the Amiga version of func_shell.  */

static char *
func_shell (char *o, char **argv, const char *funcname)
{
  /* Amiga can't fork nor spawn, but I can start a program with
     redirection of my choice.  However, this means that we
     don't have an opportunity to reopen stdout to trap it.  Thus,
     we save our own stdout onto a new descriptor and dup a temp
     file's descriptor onto our stdout temporarily.  After we
     spawn the shell program, we dup our own stdout back to the
     stdout descriptor.  The buffer reading is the same as above,
     except that we're now reading from a file.  */

#include <dos/dos.h>
#include <proto/dos.h>
#undef stderr
#define stderr stdout

  BPTR child_stdout;
  char tmp_output[FILENAME_MAX];
  unsigned int maxlen = 200;
  int cc, i;
  char * buffer, * ptr;
  char ** aptr;
  int len = 0;
  char* batch_filename = NULL;

  /* Construct the argument list.  */
  command_argv = construct_command_argv (argv[0], (char **) NULL,
                                         (struct file *) 0, &batch_filename);
  if (command_argv == 0)
    return o;

  /* Note the mktemp() is a security hole, but this only runs on Amiga.
     Ideally we would use main.c:open_tmpfile(), but this uses a special
     Open(), not fopen(), and I'm not familiar enough with the code to mess
     with it.  */
  strcpy (tmp_output, "t:MakeshXXXXXXXX");
  mktemp (tmp_output);
  child_stdout = Open (tmp_output, MODE_NEWFILE);

  for (aptr=command_argv; *aptr; aptr++)
    len += strlen (*aptr) + 1;

  buffer = xmalloc (len + 1);
  ptr = buffer;

  for (aptr=command_argv; *aptr; aptr++)
    {
      strcpy (ptr, *aptr);
      ptr += strlen (ptr) + 1;
      *ptr ++ = ' ';
      *ptr = 0;
    }

  ptr[-1] = '\n';

  Execute (buffer, NULL, child_stdout);
  free (buffer);

  Close (child_stdout);

  child_stdout = Open (tmp_output, MODE_OLDFILE);

  buffer = xmalloc (maxlen);
  i = 0;
  do
    {
      if (i == maxlen)
	{
	  maxlen += 512;
	  buffer = (char *) xrealloc (buffer, maxlen + 1);
	}

      cc = Read (child_stdout, &buffer[i], maxlen - i);
      if (cc > 0)
	i += cc;
    } while (cc > 0);

  Close (child_stdout);

  fold_newlines (buffer, &i);
  o = variable_buffer_output (o, buffer, i);
  free (buffer);
  return o;
}
#endif  /* _AMIGA */
#endif  /* !VMS */

#ifdef EXPERIMENTAL

/*
  equality. Return is string-boolean, ie, the empty string is false.
 */
static char *
func_eq (char* o, char **argv, char *funcname)
{
  int result = ! strcmp (argv[0], argv[1]);
  o = variable_buffer_output (o,  result ? "1" : "", result);
  return o;
}


/*
  string-boolean not operator.
 */
static char *
func_not (char* o, char **argv, char *funcname)
{
  char * s = argv[0];
  int result = 0;
  while (isspace ((unsigned char)*s))
    s++;
  result = ! (*s);
  o = variable_buffer_output (o,  result ? "1" : "", result);
  return o;
}
#endif


#define STRING_SIZE_TUPLE(_s) (_s), (sizeof (_s)-1)

/* Lookup table for builtin functions.

   This doesn't have to be sorted; we use a straight lookup.  We might gain
   some efficiency by moving most often used functions to the start of the
   table.

   If MAXIMUM_ARGS is 0, that means there is no maximum and all
   comma-separated values are treated as arguments.

   EXPAND_ARGS means that all arguments should be expanded before invocation.
   Functions that do namespace tricks (foreach) don't automatically expand.  */

static char *func_call PARAMS ((char *o, char **argv, const char *funcname));


static struct function_table_entry function_table[] =
{
 /* Name/size */                    /* MIN MAX EXP? Function */
  { STRING_SIZE_TUPLE("addprefix"),     2,  2,  1,  func_addsuffix_addprefix},
  { STRING_SIZE_TUPLE("addsuffix"),     2,  2,  1,  func_addsuffix_addprefix},
  { STRING_SIZE_TUPLE("basename"),      1,  1,  1,  func_basename_dir},
  { STRING_SIZE_TUPLE("dir"),           1,  1,  1,  func_basename_dir},
  { STRING_SIZE_TUPLE("notdir"),        1,  1,  1,  func_notdir_suffix},
  { STRING_SIZE_TUPLE("subst"),         3,  3,  1,  func_subst},
  { STRING_SIZE_TUPLE("suffix"),        1,  1,  1,  func_notdir_suffix},
  { STRING_SIZE_TUPLE("filter"),        2,  2,  1,  func_filter_filterout},
  { STRING_SIZE_TUPLE("filter-out"),    2,  2,  1,  func_filter_filterout},
  { STRING_SIZE_TUPLE("findstring"),    2,  2,  1,  func_findstring},
  { STRING_SIZE_TUPLE("firstword"),     1,  1,  1,  func_firstword},
  { STRING_SIZE_TUPLE("join"),          2,  2,  1,  func_join},
  { STRING_SIZE_TUPLE("patsubst"),      3,  3,  1,  func_patsubst},
  { STRING_SIZE_TUPLE("shell"),         1,  1,  1,  func_shell},
  { STRING_SIZE_TUPLE("sort"),          1,  1,  1,  func_sort},
  { STRING_SIZE_TUPLE("strip"),         1,  1,  1,  func_strip},
  { STRING_SIZE_TUPLE("wildcard"),      1,  1,  1,  func_wildcard},
  { STRING_SIZE_TUPLE("word"),          2,  2,  1,  func_word},
  { STRING_SIZE_TUPLE("wordlist"),      3,  3,  1,  func_wordlist},
  { STRING_SIZE_TUPLE("words"),         1,  1,  1,  func_words},
  { STRING_SIZE_TUPLE("origin"),        1,  1,  1,  func_origin},
  { STRING_SIZE_TUPLE("foreach"),       3,  3,  0,  func_foreach},
  { STRING_SIZE_TUPLE("call"),          1,  0,  1,  func_call},
  { STRING_SIZE_TUPLE("error"),         1,  1,  1,  func_error},
  { STRING_SIZE_TUPLE("warning"),       1,  1,  1,  func_error},
  { STRING_SIZE_TUPLE("if"),            2,  3,  0,  func_if},
#ifdef EXPERIMENTAL
  { STRING_SIZE_TUPLE("eq"),            2,  2,  1,  func_eq},
  { STRING_SIZE_TUPLE("not"),           1,  1,  1,  func_not},
#endif
  { 0 }
};


/* These must come after the definition of function_table[].  */

static char *
expand_builtin_function (o, argc, argv, entry_p)
     char *o;
     int argc;
     char **argv;
     struct function_table_entry *entry_p;
{
  if (argc < entry_p->minimum_args)
    fatal (reading_file,
           _("Insufficient number of arguments (%d) to function `%s'"),
           argc, entry_p->name);

  if (!entry_p->func_ptr)
    fatal (reading_file, _("Unimplemented on this platform: function `%s'"),
           entry_p->name);

  return entry_p->func_ptr (o, argv, entry_p->name);
}

/* Check for a function invocation in *STRINGP.  *STRINGP points at the
   opening ( or { and is not null-terminated.  If a function invocation
   is found, expand it into the buffer at *OP, updating *OP, incrementing
   *STRINGP past the reference and returning nonzero.  If not, return zero.  */

int
handle_function (op, stringp)
     char **op;
     char **stringp;
{
  const struct function_table_entry *entry_p;
  char openparen = (*stringp)[0];
  char closeparen = openparen == '(' ? ')' : '}';
  char *beg;
  char *end;
  int count = 0;
  register char *p;
  char **argv, **argvp;
  int nargs;

  beg = *stringp + 1;

  entry_p = lookup_function (function_table, beg);

  if (!entry_p)
    return 0;

  /* We found a builtin function.  Find the beginning of its arguments (skip
     whitespace after the name).  */

  beg = next_token (beg + entry_p->len);

  /* Find the end of the function invocation, counting nested use of
     whichever kind of parens we use.  Since we're looking, count commas
     to get a rough estimate of how many arguments we might have.  The
     count might be high, but it'll never be low.  */

  for (nargs=1, end=beg; *end != '\0'; ++end)
    if (*end == ',')
      ++nargs;
    else if (*end == openparen)
      ++count;
    else if (*end == closeparen && --count < 0)
      break;

  if (count >= 0)
    fatal (reading_file,
	   _("unterminated call to function `%s': missing `%c'"),
	   entry_p->name, closeparen);

  *stringp = end;

  /* Get some memory to store the arg pointers.  */
  argvp = argv = (char **) alloca (sizeof (char *) * (nargs + 2));

  /* Chop the string into arguments, then a nul.  As soon as we hit
     MAXIMUM_ARGS (if it's >0) assume the rest of the string is part of the
     last argument.

     If we're expanding, store pointers to the expansion of each one.  If
     not, make a duplicate of the string and point into that, nul-terminating
     each argument.  */

  if (!entry_p->expand_args)
    {
      int len = end - beg;

      p = xmalloc (len+1);
      memcpy (p, beg, len);
      p[len] = '\0';
      beg = p;
      end = beg + len;
    }

  p = beg;
  nargs = 0;
  for (p=beg, nargs=0; p < end; ++argvp)
    {
      char *next;

      ++nargs;

      if (nargs == entry_p->maximum_args
          || (! (next = find_next_argument (openparen, closeparen, p, end))))
        next = end;

      if (entry_p->expand_args)
        *argvp = expand_argument (p, next);
      else
        {
          *argvp = p;
          *next = '\0';
        }

      p = next + 1;
    }
  *argvp = NULL;

  /* Finally!  Run the function...  */
  *op = expand_builtin_function (*op, nargs, argv, entry_p);

  /* Free memory.  */
  if (entry_p->expand_args)
    for (argvp=argv; *argvp != 0; ++argvp)
      free (*argvp);
  else
    free (beg);

  return 1;
}


/* User-defined functions.  Expand the first argument as either a builtin
   function or a make variable, in the context of the rest of the arguments
   assigned to $1, $2, ... $N.  $0 is the name of the function.  */

static char *
func_call (o, argv, funcname)
     char *o;
     char **argv;
     const char *funcname;
{
  char *fname;
  char *cp;
  int flen;
  char *body;
  int i;
  const struct function_table_entry *entry_p;

  /* There is no way to define a variable with a space in the name, so strip
     leading and trailing whitespace as a favor to the user.  */
  fname = argv[0];
  while (*fname != '\0' && isspace ((unsigned char)*fname))
    ++fname;

  cp = fname + strlen (fname) - 1;
  while (cp > fname && isspace ((unsigned char)*cp))
    --cp;
  cp[1] = '\0';

  /* Calling nothing is a no-op */
  if (*fname == '\0')
    return o;

  /* Are we invoking a builtin function?  */

  entry_p = lookup_function (function_table, fname);

  if (entry_p)
    {
      /* How many arguments do we have?  */
      for (i=0; argv[i+1]; ++i)
  	;

      return expand_builtin_function (o, i, argv+1, entry_p);
    }

  /* Not a builtin, so the first argument is the name of a variable to be
     expanded and interpreted as a function.  Create the variable
     reference.  */
  flen = strlen (fname);

  body = alloca (flen + 4);
  body[0] = '$';
  body[1] = '(';
  memcpy (body + 2, fname, flen);
  body[flen+2] = ')';
  body[flen+3] = '\0';

  /* Set up arguments $(1) .. $(N).  $(0) is the function name.  */

  push_new_variable_scope ();

  for (i=0; *argv; ++i, ++argv)
    {
      char num[11];

      sprintf (num, "%d", i);
      define_variable (num, strlen (num), *argv, o_automatic, 1);
    }

  /* Expand the body in the context of the arguments, adding the result to
     the variable buffer.  */

  o = variable_expand_string (o, body, flen+3);

  pop_variable_scope ();

  return o + strlen (o);
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         getopt.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Getopt for GNU.
   NOTE: getopt is now part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to drepper@gnu.org
   before changing it!

   Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98
   	Free Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C Library.
   Bugs can be reported to bug-glibc@gnu.org.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

/* This tells Alpha OSF/1 not to define a getopt prototype in <stdio.h>.
   Ditto for AIX 3.2 and <stdlib.h>.  */
#ifndef _NO_PROTO
# define _NO_PROTO
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#undef stderr
#define stderr stdout
#endif

#if !defined __STDC__ || !__STDC__
/* This is a separate conditional since some stdc systems
   reject `defined (const)'.  */
# ifndef const
#  define const
# endif
#endif

#include <stdio.h>
#undef stderr
#define stderr stdout

/* Comment out all this code if we are using the GNU C Library, and are not
   actually compiling the library itself.  This code is part of the GNU C
   Library, but also included in many other GNU distributions.  Compiling
   and linking in this code is a waste when using the GNU C library
   (especially if it is a shared library).  Rather than having every GNU
   program understand `configure --with-gnu-libc' and omit the object files,
   it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined _LIBC && defined __GLIBC__ && __GLIBC__ >= 2
# include <gnu-versions.h>
#undef stderr
#define stderr stdout
# if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#  define ELIDE_CODE
# endif
#endif

#ifndef ELIDE_CODE


/* This needs to come after some library #include
   to get __GNU_LIBRARY__ defined.  */
#ifdef	__GNU_LIBRARY__
/* Don't include stdlib.h for non-GNU C libraries because some of them
   contain conflicting prototypes for getopt.  */
# include <stdlib.h>
# include <unistd.h>
#undef stderr
#define stderr stdout
#endif	/* GNU C library.  */

#ifdef VMS
# include <unixlib.h>
#undef stderr
#define stderr stdout
# if HAVE_STRING_H - 0
#  include <string.h>
#undef stderr
#define stderr stdout
# endif
#endif

/* This is for other GNU distributions with internationalized messages.
   When compiling libc, the _ macro is predefined.  */
#include "gettext.h"
#undef stderr
#define stderr stdout
//#define _(msgid)    gettext (msgid)


/* This version of `getopt' appears to the caller like standard Unix `getopt'
   but it behaves differently for the user, since it allows the user
   to intersperse the options with the other arguments.

   As `getopt' works, it permutes the elements of ARGV so that,
   when it is done, all the options precede everything else.  Thus
   all application programs are extended to handle flexible argument order.

   Setting the environment variable POSIXLY_CORRECT disables permutation.
   Then the behavior is completely standard.

   GNU application programs can use a third alternative mode in which
   they can distinguish the relative order of options and other arguments.  */

#include "getopt.h"
#undef stderr
#define stderr stdout

/* For communication from `getopt' to the caller.
   When `getopt' finds an option that takes an argument,
   the argument value is returned here.
   Also, when `ordering' is RETURN_IN_ORDER,
   each non-option ARGV-element is returned here.  */

char *optarg = NULL;

/* Index in ARGV of the next element to be scanned.
   This is used for communication to and from the caller
   and for communication between successive calls to `getopt'.

   On entry to `getopt', zero means this is the first call; initialize.

   When `getopt' returns -1, this is the index of the first of the
   non-option elements that the caller should itself scan.

   Otherwise, `optind' communicates from one call to the next
   how much of ARGV has been scanned so far.  */

/* 1003.2 says this must be 1 before any call.  */
int optind = 1;

/* Formerly, initialization of getopt depended on optind==0, which
   causes problems with re-calling getopt as programs generally don't
   know that. */

int __getopt_initialized = 0;

/* The next char to be scanned in the option-element
   in which the last option character we returned was found.
   This allows us to pick up the scan where we left off.

   If this is zero, or a null string, it means resume the scan
   by advancing to the next ARGV-element.  */

static char *nextchar;

/* Callers store zero here to inhibit the error message
   for unrecognized options.  */

int opterr = 1;

/* Set to an option character which was unrecognized.
   This must be initialized on some systems to avoid linking in the
   system's own getopt implementation.  */

int optopt = '?';

/* Describe how to deal with options that follow non-option ARGV-elements.

   If the caller did not specify anything,
   the default is REQUIRE_ORDER if the environment variable
   POSIXLY_CORRECT is defined, PERMUTE otherwise.

   REQUIRE_ORDER means don't recognize them as options;
   stop option processing when the first non-option is seen.
   This is what Unix does.
   This mode of operation is selected by either setting the environment
   variable POSIXLY_CORRECT, or using `+' as the first character
   of the list of option characters.

   PERMUTE is the default.  We permute the contents of ARGV as we scan,
   so that eventually all the non-options are at the end.  This allows options
   to be given in any order, even with programs that were not written to
   expect this.

   RETURN_IN_ORDER is an option available to programs that were written
   to expect options and other ARGV-elements in any order and that care about
   the ordering of the two.  We describe each non-option ARGV-element
   as if it were the argument of an option with character code 1.
   Using `-' as the first character of the list of option characters
   selects this mode of operation.

   The special argument `--' forces an end of option-scanning regardless
   of the value of `ordering'.  In the case of RETURN_IN_ORDER, only
   `--' can cause `getopt' to return -1 with `optind' != ARGC.  */

static enum
{
  REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;

/* Value of POSIXLY_CORRECT environment variable.  */
static char *posixly_correct;

#ifdef	__GNU_LIBRARY__
/* We want to avoid inclusion of string.h with non-GNU libraries
   because there are many ways it can cause trouble.
   On some systems, it contains special magic macros that don't work
   in GCC.  */
# include <string.h>
#undef stderr
#define stderr stdout
# define my_index	strchr
#else

# if HAVE_STRING_H
#  include <string.h>
#undef stderr
#define stderr stdout
# else
#  include <strings.h>
#undef stderr
#define stderr stdout
# endif

/* Avoid depending on library functions or files
   whose names are inconsistent.  */

#ifndef getenv
extern char *getenv ();
#endif

static char *
my_index (str, chr)
     const char *str;
     int chr;
{
  while (*str)
    {
      if (*str == chr)
	return (char *) str;
      str++;
    }
  return 0;
}

/* If using GCC, we can safely declare strlen this way.
   If not using GCC, it is ok not to declare it.  */
#ifdef __GNUC__
/* Note that Motorola Delta 68k R3V7 comes with GCC but not stddef.h.
   That was relevant to code that was here before.  */
# if (!defined __STDC__ || !__STDC__) && !defined strlen
/* gcc with -traditional declares the built-in strlen to return int,
   and has done so at least since version 2.4.5. -- rms.  */
extern int strlen (const char *);
# endif /* not __STDC__ */
#endif /* __GNUC__ */

#endif /* not __GNU_LIBRARY__ */

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

#ifdef _LIBC
/* Bash 2.0 gives us an environment variable containing flags
   indicating ARGV elements that should not be considered arguments.  */

/* Defined in getopt_init.c  */
extern char *__getopt_nonoption_flags;

static int nonoption_flags_max_len;
static int nonoption_flags_len;

static int original_argc;
static char *const *original_argv;

/* Make sure the environment variable bash 2.0 puts in the environment
   is valid for the getopt call we must make sure that the ARGV passed
   to getopt is that one passed to the process.  */
static void
__attribute__ ((unused))
store_args_and_env (int argc, char *const *argv)
{
  /* XXX This is no good solution.  We should rather copy the args so
     that we can compare them later.  But we must not use malloc(3).  */
  original_argc = argc;
  original_argv = argv;
}
# ifdef text_set_element
text_set_element (__libc_subinit, store_args_and_env);
# endif /* text_set_element */

# define SWAP_FLAGS(ch1, ch2) \
  if (nonoption_flags_len > 0)						      \
    {									      \
      char __tmp = __getopt_nonoption_flags[ch1];			      \
      __getopt_nonoption_flags[ch1] = __getopt_nonoption_flags[ch2];	      \
      __getopt_nonoption_flags[ch2] = __tmp;				      \
    }
#else	/* !_LIBC */
# define SWAP_FLAGS(ch1, ch2)
#endif	/* _LIBC */

/* Exchange two adjacent subsequences of ARGV.
   One subsequence is elements [first_nonopt,last_nonopt)
   which contains all the non-options that have been skipped so far.
   The other is elements [last_nonopt,optind), which contains all
   the options processed since those non-options were skipped.

   `first_nonopt' and `last_nonopt' are relocated so that they describe
   the new indices of the non-options in ARGV after they are moved.  */

#if defined __STDC__ && __STDC__
static void exchange (char **);
#endif

static void
exchange (argv)
     char **argv;
{
  int bottom = first_nonopt;
  int middle = last_nonopt;
  int top = optind;
  char *tem;

  /* Exchange the shorter segment with the far end of the longer segment.
     That puts the shorter segment into the right place.
     It leaves the longer segment in the right place overall,
     but it consists of two parts that need to be swapped next.  */

#ifdef _LIBC
  /* First make sure the handling of the `__getopt_nonoption_flags'
     string can work normally.  Our top argument must be in the range
     of the string.  */
  if (nonoption_flags_len > 0 && top >= nonoption_flags_max_len)
    {
      /* We must extend the array.  The user plays games with us and
	 presents new arguments.  */
      char *new_str = malloc (top + 1);
      if (new_str == NULL)
	nonoption_flags_len = nonoption_flags_max_len = 0;
      else
	{
	  memset (__mempcpy (new_str, __getopt_nonoption_flags,
			     nonoption_flags_max_len),
		  '\0', top + 1 - nonoption_flags_max_len);
	  nonoption_flags_max_len = top + 1;
	  __getopt_nonoption_flags = new_str;
	}
    }
#endif

  while (top > middle && middle > bottom)
    {
      if (top - middle > middle - bottom)
	{
	  /* Bottom segment is the short one.  */
	  int len = middle - bottom;
	  register int i;

	  /* Swap it with the top part of the top segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[top - (middle - bottom) + i];
	      argv[top - (middle - bottom) + i] = tem;
	      SWAP_FLAGS (bottom + i, top - (middle - bottom) + i);
	    }
	  /* Exclude the moved bottom segment from further swapping.  */
	  top -= len;
	}
      else
	{
	  /* Top segment is the short one.  */
	  int len = top - middle;
	  register int i;

	  /* Swap it with the bottom part of the bottom segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[middle + i];
	      argv[middle + i] = tem;
	      SWAP_FLAGS (bottom + i, middle + i);
	    }
	  /* Exclude the moved top segment from further swapping.  */
	  bottom += len;
	}
    }

  /* Update records for the slots the non-options now occupy.  */

  first_nonopt += (optind - last_nonopt);
  last_nonopt = optind;
}

/* Initialize the internal data when the first call is made.  */

#if defined __STDC__ && __STDC__
static const char *_getopt_initialize (int, char *const *, const char *);
#endif
static const char *
_getopt_initialize (argc, argv, optstring)
     int argc;
     char *const *argv;
     const char *optstring;
{
  /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  first_nonopt = last_nonopt = optind;

  nextchar = NULL;

  posixly_correct = getenv ("POSIXLY_CORRECT");

  /* Determine how to handle the ordering of options and nonoptions.  */

  if (optstring[0] == '-')
    {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    }
  else if (optstring[0] == '+')
    {
      ordering = REQUIRE_ORDER;
      ++optstring;
    }
  else if (posixly_correct != NULL)
    ordering = REQUIRE_ORDER;
  else
    ordering = PERMUTE;

#ifdef _LIBC
  if (posixly_correct == NULL
      && argc == original_argc && argv == original_argv)
    {
      if (nonoption_flags_max_len == 0)
	{
	  if (__getopt_nonoption_flags == NULL
	      || __getopt_nonoption_flags[0] == '\0')
	    nonoption_flags_max_len = -1;
	  else
	    {
	      const char *orig_str = __getopt_nonoption_flags;
	      int len = nonoption_flags_max_len = strlen (orig_str);
	      if (nonoption_flags_max_len < argc)
		nonoption_flags_max_len = argc;
	      __getopt_nonoption_flags =
		(char *) malloc (nonoption_flags_max_len);
	      if (__getopt_nonoption_flags == NULL)
		nonoption_flags_max_len = -1;
	      else
		memset (__mempcpy (__getopt_nonoption_flags, orig_str, len),
			'\0', nonoption_flags_max_len - len);
	    }
	}
      nonoption_flags_len = nonoption_flags_max_len;
    }
  else
    nonoption_flags_len = 0;
#endif

  return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns -1.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return '?' after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return '?'.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   The elements of ARGV aren't really const, because we permute them.
   But we pretend they're const in the prototype to be compatible
   with other systems.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.  */

int
_getopt_internal (argc, argv, optstring, longopts, longind, long_only)
     int argc;
     char *const *argv;
     const char *optstring;
     const struct option *longopts;
     int *longind;
     int long_only;
{
  optarg = NULL;

  if (optind == 0 || !__getopt_initialized)
    {
      if (optind == 0)
	optind = 1;	/* Don't scan ARGV[0], the program name.  */
      optstring = _getopt_initialize (argc, argv, optstring);
      __getopt_initialized = 1;
    }

  /* Test whether ARGV[optind] points to a non-option argument.
     Either it does not have option syntax, or there is an environment flag
     from the shell indicating it is not an option.  The later information
     is only used when the used in the GNU libc.  */
#ifdef _LIBC
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0'	      \
		      || (optind < nonoption_flags_len			      \
			  && __getopt_nonoption_flags[optind] == '1'))
#else
# define NONOPTION_P (argv[optind][0] != '-' || argv[optind][1] == '\0')
#endif

  if (nextchar == NULL || *nextchar == '\0')
    {
      /* Advance to the next ARGV-element.  */

      /* Give FIRST_NONOPT & LAST_NONOPT rational values if OPTIND has been
	 moved back by the user (who may also have changed the arguments).  */
      if (last_nonopt > optind)
	last_nonopt = optind;
      if (first_nonopt > optind)
	first_nonopt = optind;

      if (ordering == PERMUTE)
	{
	  /* If we have just processed some options following some non-options,
	     exchange them so that the options come first.  */

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (last_nonopt != optind)
	    first_nonopt = optind;

	  /* Skip any additional non-options
	     and extend the range of non-options previously skipped.  */

	  while (optind < argc && NONOPTION_P)
	    optind++;
	  last_nonopt = optind;
	}

      /* The special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (optind != argc && !strcmp (argv[optind], "--"))
	{
	  optind++;

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (first_nonopt == last_nonopt)
	    first_nonopt = optind;
	  last_nonopt = argc;

	  optind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (optind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (first_nonopt != last_nonopt)
	    optind = first_nonopt;
	  return -1;
	}

      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if (NONOPTION_P)
	{
	  if (ordering == REQUIRE_ORDER)
	    return -1;
	  optarg = argv[optind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Skip the initial punctuation.  */

      nextchar = (argv[optind] + 1
		  + (longopts != NULL && argv[optind][1] == '-'));
    }

  /* Decode the current option-ARGV-element.  */

  /* Check whether the ARGV-element is a long option.

     If long_only and the ARGV-element has the form "-f", where f is
     a valid short option, don't consider it an abbreviated form of
     a long option that starts with f.  Otherwise there would be no
     way to give the -f short option.

     On the other hand, if there's a long option "fubar" and
     the ARGV-element is "-fu", do consider that an abbreviation of
     the long option, just like "--fu", and not "-f" with arg "u".

     This distinction seems to be the most useful approach.  */

  if (longopts != NULL
      && (argv[optind][1] == '-'
	  || (long_only && (argv[optind][2] || !my_index (optstring, argv[optind][1])))))
    {
      char *nameend;
      const struct option *p;
      const struct option *pfound = NULL;
      int exact = 0;
      int ambig = 0;
      int indfound = -1;
      int option_index;

      for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
	/* Do nothing.  */ ;

      /* Test all long options for either exact match
	 or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name; p++, option_index++)
	if (!strncmp (p->name, nextchar, nameend - nextchar))
	  {
	    if ((unsigned int) (nameend - nextchar)
		== (unsigned int) strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == NULL)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = option_index;
	      }
	    else
	      /* Second or later nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  if (opterr)
	    fprintf (stderr, _("%s: option `%s' is ambiguous\n"),
		     argv[0], argv[optind]);
	  nextchar += strlen (nextchar);
	  optind++;
	  optopt = 0;
	  return '?';
	}

      if (pfound != NULL)
	{
	  option_index = indfound;
	  optind++;
	  if (*nameend)
	    {
	      /* Don't test has_arg with >, because some C compilers don't
		 allow it to be used on enums.  */
	      if (pfound->has_arg)
		optarg = nameend + 1;
	      else
		{
		  if (opterr)
		   if (argv[optind - 1][1] == '-')
		    /* --option */
		    fprintf (stderr,
		     _("%s: option `--%s' doesn't allow an argument\n"),
		     argv[0], pfound->name);
		   else
		    /* +option or -option */
		    fprintf (stderr,
		     _("%s: option `%c%s' doesn't allow an argument\n"),
		     argv[0], argv[optind - 1][0], pfound->name);

		  nextchar += strlen (nextchar);

		  optopt = pfound->val;
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (optind < argc)
		optarg = argv[optind++];
	      else
		{
		  if (opterr)
		    fprintf (stderr,
			   _("%s: option `%s' requires an argument\n"),
			   argv[0], argv[optind - 1]);
		  nextchar += strlen (nextchar);
		  optopt = pfound->val;
		  return optstring[0] == ':' ? ':' : '?';
		}
	    }
	  nextchar += strlen (nextchar);
	  if (longind != NULL)
	    *longind = option_index;
	  if (pfound->flag)
	    {
	      *(pfound->flag) = pfound->val;
	      return 0;
	    }
	  return pfound->val;
	}

      /* Can't find it as a long option.  If this is not getopt_long_only,
	 or the option starts with '--' or is not a valid short
	 option, then it's an error.
	 Otherwise interpret it as a short option.  */
      if (!long_only || argv[optind][1] == '-'
	  || my_index (optstring, *nextchar) == NULL)
	{
	  if (opterr)
	    {
	      if (argv[optind][1] == '-')
		/* --option */
		fprintf (stderr, _("%s: unrecognized option `--%s'\n"),
			 argv[0], nextchar);
	      else
		/* +option or -option */
		fprintf (stderr, _("%s: unrecognized option `%c%s'\n"),
			 argv[0], argv[optind][0], nextchar);
	    }
	  nextchar = (char *) "";
	  optind++;
	  optopt = 0;
	  return '?';
	}
    }

  /* Look at and handle the next short option-character.  */

  {
    char c = *nextchar++;
    char *temp = my_index (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*nextchar == '\0')
      ++optind;

    if (temp == NULL || c == ':')
      {
	if (opterr)
	  {
	    if (posixly_correct)
	      /* 1003.2 specifies the format of this message.  */
	      fprintf (stderr, _("%s: illegal option -- %c\n"),
		       argv[0], c);
	    else
	      fprintf (stderr, _("%s: invalid option -- %c\n"),
		       argv[0], c);
	  }
	optopt = c;
	return '?';
      }
    /* Convenience. Treat POSIX -W foo same as long option --foo */
    if (temp[0] == 'W' && temp[1] == ';')
      {
	char *nameend;
	const struct option *p;
	const struct option *pfound = NULL;
	int exact = 0;
	int ambig = 0;
	int indfound = 0;
	int option_index;

	/* This is an option that requires an argument.  */
	if (*nextchar != '\0')
	  {
	    optarg = nextchar;
	    /* If we end this ARGV-element by taking the rest as an arg,
	       we must advance to the next element now.  */
	    optind++;
	  }
	else if (optind == argc)
	  {
	    if (opterr)
	      {
		/* 1003.2 specifies the format of this message.  */
		fprintf (stderr, _("%s: option requires an argument -- %c\n"),
			 argv[0], c);
	      }
	    optopt = c;
	    if (optstring[0] == ':')
	      c = ':';
	    else
	      c = '?';
	    return c;
	  }
	else
	  /* We already incremented `optind' once;
	     increment it again when taking next ARGV-elt as argument.  */
	  optarg = argv[optind++];

	/* optarg is now the argument, see if it's in the
	   table of longopts.  */

	for (nextchar = nameend = optarg; *nameend && *nameend != '='; nameend++)
	  /* Do nothing.  */ ;

	/* Test all long options for either exact match
	   or abbreviated matches.  */
	for (p = longopts, option_index = 0; p->name; p++, option_index++)
	  if (!strncmp (p->name, nextchar, nameend - nextchar))
	    {
	      if ((unsigned int) (nameend - nextchar) == strlen (p->name))
		{
		  /* Exact match found.  */
		  pfound = p;
		  indfound = option_index;
		  exact = 1;
		  break;
		}
	      else if (pfound == NULL)
		{
		  /* First nonexact match found.  */
		  pfound = p;
		  indfound = option_index;
		}
	      else
		/* Second or later nonexact match found.  */
		ambig = 1;
	    }
	if (ambig && !exact)
	  {
	    if (opterr)
	      fprintf (stderr, _("%s: option `-W %s' is ambiguous\n"),
		       argv[0], argv[optind]);
	    nextchar += strlen (nextchar);
	    optind++;
	    return '?';
	  }
	if (pfound != NULL)
	  {
	    option_index = indfound;
	    if (*nameend)
	      {
		/* Don't test has_arg with >, because some C compilers don't
		   allow it to be used on enums.  */
		if (pfound->has_arg)
		  optarg = nameend + 1;
		else
		  {
		    if (opterr)
		      fprintf (stderr, _("\
%s: option `-W %s' doesn't allow an argument\n"),
			       argv[0], pfound->name);

		    nextchar += strlen (nextchar);
		    return '?';
		  }
	      }
	    else if (pfound->has_arg == 1)
	      {
		if (optind < argc)
		  optarg = argv[optind++];
		else
		  {
		    if (opterr)
		      fprintf (stderr,
			       _("%s: option `%s' requires an argument\n"),
			       argv[0], argv[optind - 1]);
		    nextchar += strlen (nextchar);
		    return optstring[0] == ':' ? ':' : '?';
		  }
	      }
	    nextchar += strlen (nextchar);
	    if (longind != NULL)
	      *longind = option_index;
	    if (pfound->flag)
	      {
		*(pfound->flag) = pfound->val;
		return 0;
	      }
	    return pfound->val;
	  }
	  nextchar = NULL;
	  return 'W';	/* Let the application handle it.   */
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		optind++;
	      }
	    else
	      optarg = NULL;
	    nextchar = NULL;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		optind++;
	      }
	    else if (optind == argc)
	      {
		if (opterr)
		  {
		    /* 1003.2 specifies the format of this message.  */
		    fprintf (stderr,
			   _("%s: option requires an argument -- %c\n"),
			   argv[0], c);
		  }
		optopt = c;
		if (optstring[0] == ':')
		  c = ':';
		else
		  c = '?';
	      }
	    else
	      /* We already incremented `optind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      optarg = argv[optind++];
	    nextchar = NULL;
	  }
      }
    return c;
  }
}

int
getopt (argc, argv, optstring)
     int argc;
     char *const *argv;
     const char *optstring;
{
  return _getopt_internal (argc, argv, optstring,
			   (const struct option *) 0,
			   (int *) 0,
			   0);
}

#endif	/* Not ELIDE_CODE.  */

#ifdef TEST

/* Compile with -DTEST to make an executable for use in testing
   the above definition of `getopt'.  */

int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;

      c = getopt (argc, argv, "abc:d:0123456789");
      if (c == -1)
	break;

      switch (c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value `%s'\n", optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         implicit.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Implicit rule searching for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"
#undef stderr
#define stderr stdout

static int pattern_search PARAMS ((struct file *file, int archive, unsigned int depth,
		unsigned int recursions));

/* For a FILE which has no commands specified, try to figure out some
   from the implicit pattern rules.
   Returns 1 if a suitable implicit rule was found,
   after modifying FILE to contain the appropriate commands and deps,
   or returns 0 if no implicit rule was found.  */

int
try_implicit_rule (file, depth)
     struct file *file;
     unsigned int depth;
{
  DBF (DB_IMPLICIT, _("Looking for an implicit rule for `%s'.\n"));

  /* The order of these searches was previously reversed.  My logic now is
     that since the non-archive search uses more information in the target
     (the archive search omits the archive name), it is more specific and
     should come first.  */

  if (pattern_search (file, 0, depth, 0))
    return 1;

#ifndef	NO_ARCHIVES
  /* If this is an archive member reference, use just the
     archive member name to search for implicit rules.  */
  if (ar_name (file->name))
    {
      DBF (DB_IMPLICIT,
           _("Looking for archive-member implicit rule for `%s'.\n"));
      if (pattern_search (file, 1, depth, 0))
	return 1;
    }
#endif

  return 0;
}


/* Search the pattern rules for a rule with an existing dependency to make
   FILE.  If a rule is found, the appropriate commands and deps are put in FILE
   and 1 is returned.  If not, 0 is returned.

   If ARCHIVE is nonzero, FILE->name is of the form "LIB(MEMBER)".  A rule for
   "(MEMBER)" will be searched for, and "(MEMBER)" will not be chopped up into
   directory and filename parts.

   If an intermediate file is found by pattern search, the intermediate file
   is set up as a target by the recursive call and is also made a dependency
   of FILE.

   DEPTH is used for debugging messages.  */

static int
pattern_search (file, archive, depth, recursions)
     struct file *file;
     int archive;
     unsigned int depth;
     unsigned int recursions;
{
  /* Filename we are searching for a rule for.  */
  char *filename = archive ? strchr (file->name, '(') : file->name;

  /* Length of FILENAME.  */
  unsigned int namelen = strlen (filename);

  /* The last slash in FILENAME (or nil if there is none).  */
  char *lastslash;

  /* This is a file-object used as an argument in
     recursive calls.  It never contains any data
     except during a recursive call.  */
  struct file *intermediate_file = 0;

  /* List of dependencies found recursively.  */
  struct file **intermediate_files
    = (struct file **) alloca (max_pattern_deps * sizeof (struct file *));

  /* List of the patterns used to find intermediate files.  */
  char **intermediate_patterns
    = (char **) alloca (max_pattern_deps * sizeof (char *));

  /* This buffer records all the dependencies actually found for a rule.  */
  char **found_files = (char **) alloca (max_pattern_deps * sizeof (char *));
  /* Number of dep names now in FOUND_FILES.  */
  unsigned int deps_found = 0;

  /* Names of possible dependencies are constructed in this buffer.  */
  register char *depname = (char *) alloca (namelen + max_pattern_dep_length);

  /* The start and length of the stem of FILENAME for the current rule.  */
  register char *stem = 0;
  register unsigned int stemlen = 0;

  /* Buffer in which we store all the rules that are possibly applicable.  */
  struct rule **tryrules
    = (struct rule **) alloca (num_pattern_rules * max_pattern_targets
			       * sizeof (struct rule *));

  /* Number of valid elements in TRYRULES.  */
  unsigned int nrules;

  /* The numbers of the rule targets of each rule
     in TRYRULES that matched the target file.  */
  unsigned int *matches
    = (unsigned int *) alloca (num_pattern_rules * sizeof (unsigned int));

  /* Each element is nonzero if LASTSLASH was used in
     matching the corresponding element of TRYRULES.  */
  char *checked_lastslash
    = (char *) alloca (num_pattern_rules * sizeof (char));

  /* The index in TRYRULES of the rule we found.  */
  unsigned int foundrule;

  /* Nonzero if should consider intermediate files as dependencies.  */
  int intermed_ok;

  /* Nonzero if we have matched a pattern-rule target
     that is not just `%'.  */
  int specific_rule_matched = 0;

  register unsigned int i = 0;  /* uninit checks OK */
  register struct rule *rule;
  register struct dep *dep;

  char *p, *vp;

#ifndef	NO_ARCHIVES
  if (archive || ar_name (filename))
    lastslash = 0;
  else
#endif
    {
      /* Set LASTSLASH to point at the last slash in FILENAME
	 but not counting any slash at the end.  (foo/bar/ counts as
	 bar/ in directory foo/, not empty in directory foo/bar/.)  */
#ifdef VMS
      lastslash = strrchr (filename, ']');
      if (lastslash == 0)
	lastslash = strrchr (filename, ':');
#else
      lastslash = strrchr (filename, '/');
#if defined(__MSDOS__) || defined(WINDOWS32)
      /* Handle backslashes (possibly mixed with forward slashes)
	 and the case of "d:file".  */
      {
	char *bslash = strrchr (filename, '\\');
	if (lastslash == 0 || bslash > lastslash)
	  lastslash = bslash;
	if (lastslash == 0 && filename[0] && filename[1] == ':')
	  lastslash = filename + 1;
      }
#endif
#endif
      if (lastslash != 0 && lastslash[1] == '\0')
	lastslash = 0;
    }

  /* First see which pattern rules match this target
     and may be considered.  Put them in TRYRULES.  */

  nrules = 0;
  for (rule = pattern_rules; rule != 0; rule = rule->next)
    {
      /* If the pattern rule has deps but no commands, ignore it.
	 Users cancel built-in rules by redefining them without commands.  */
      if (rule->deps != 0 && rule->cmds == 0)
	continue;

      /* If this rule is in use by a parent pattern_search,
	 don't use it here.  */
      if (rule->in_use)
	{
	  DBS (DB_IMPLICIT, (_("Avoiding implicit rule recursion.\n")));
	  continue;
	}

      for (i = 0; rule->targets[i] != 0; ++i)
	{
	  char *target = rule->targets[i];
	  char *suffix = rule->suffixes[i];
	  int check_lastslash;

	  /* Rules that can match any filename and are not terminal
	     are ignored if we're recursing, so that they cannot be
	     intermediate files.  */
	  if (recursions > 0 && target[1] == '\0' && !rule->terminal)
	    continue;

	  if (rule->lens[i] > namelen)
	    /* It can't possibly match.  */
	    continue;

	  /* From the lengths of the filename and the pattern parts,
	     find the stem: the part of the filename that matches the %.  */
	  stem = filename + (suffix - target - 1);
	  stemlen = namelen - rule->lens[i] + 1;

	  /* Set CHECK_LASTSLASH if FILENAME contains a directory
	     prefix and the target pattern does not contain a slash.  */

#ifdef VMS
	  check_lastslash = lastslash != 0
			    && ((strchr (target, ']') == 0)
			        && (strchr (target, ':') == 0));
#else
	  check_lastslash = lastslash != 0 && strchr (target, '/') == 0;
#endif
	  if (check_lastslash)
	    {
	      /* In that case, don't include the
		 directory prefix in STEM here.  */
	      unsigned int difference = lastslash - filename + 1;
	      if (difference > stemlen)
		continue;
	      stemlen -= difference;
	      stem += difference;
	    }

	  /* Check that the rule pattern matches the text before the stem.  */
	  if (check_lastslash)
	    {
	      if (stem > (lastslash + 1)
		  && !strneq (target, lastslash + 1, stem - lastslash - 1))
		continue;
	    }
	  else if (stem > filename
		   && !strneq (target, filename, stem - filename))
	    continue;

	  /* Check that the rule pattern matches the text after the stem.
	     We could test simply use streq, but this way we compare the
	     first two characters immediately.  This saves time in the very
	     common case where the first character matches because it is a
	     period.  */
	  if (*suffix != stem[stemlen]
	      || (*suffix != '\0' && !streq (&suffix[1], &stem[stemlen + 1])))
	    continue;

	  /* Record if we match a rule that not all filenames will match.  */
	  if (target[1] != '\0')
	    specific_rule_matched = 1;

	  /* A rule with no dependencies and no commands exists solely to set
	     specific_rule_matched when it matches.  Don't try to use it.  */
	  if (rule->deps == 0 && rule->cmds == 0)
	    continue;

	  /* Record this rule in TRYRULES and the index of the matching
	     target in MATCHES.  If several targets of the same rule match,
	     that rule will be in TRYRULES more than once.  */
	  tryrules[nrules] = rule;
	  matches[nrules] = i;
	  checked_lastslash[nrules] = check_lastslash;
	  ++nrules;
	}
    }

  /* If we have found a matching rule that won't match all filenames,
     retroactively reject any non-"terminal" rules that do always match.  */
  if (specific_rule_matched)
    for (i = 0; i < nrules; ++i)
      if (!tryrules[i]->terminal)
	{
	  register unsigned int j;
	  for (j = 0; tryrules[i]->targets[j] != 0; ++j)
	    if (tryrules[i]->targets[j][1] == '\0')
	      break;
	  if (tryrules[i]->targets[j] != 0)
	    tryrules[i] = 0;
	}

  /* Try each rule once without intermediate files, then once with them.  */
  for (intermed_ok = 0; intermed_ok == !!intermed_ok; ++intermed_ok)
    {
      /* Try each pattern rule till we find one that applies.
	 If it does, copy the names of its dependencies (as substituted)
	 and store them in FOUND_FILES.  DEPS_FOUND is the number of them.  */

      for (i = 0; i < nrules; i++)
	{
	  int check_lastslash;

	  rule = tryrules[i];

	  /* RULE is nil when we discover that a rule,
	     already placed in TRYRULES, should not be applied.  */
	  if (rule == 0)
	    continue;

	  /* Reject any terminal rules if we're
	     looking to make intermediate files.  */
	  if (intermed_ok && rule->terminal)
	    continue;

	  /* Mark this rule as in use so a recursive
	     pattern_search won't try to use it.  */
	  rule->in_use = 1;

	  /* From the lengths of the filename and the matching pattern parts,
	     find the stem: the part of the filename that matches the %.  */
	  stem = filename
	    + (rule->suffixes[matches[i]] - rule->targets[matches[i]]) - 1;
	  stemlen = namelen - rule->lens[matches[i]] + 1;
	  check_lastslash = checked_lastslash[i];
	  if (check_lastslash)
	    {
	      stem += lastslash - filename + 1;
	      stemlen -= (lastslash - filename) + 1;
	    }

	  DBS (DB_IMPLICIT, (_("Trying pattern rule with stem `%.*s'.\n"),
                             (int) stemlen, stem));

	  /* Try each dependency; see if it "exists".  */

	  deps_found = 0;
	  for (dep = rule->deps; dep != 0; dep = dep->next)
	    {
	      /* If the dependency name has a %, substitute the stem.  */
	      p = strchr (dep_name (dep), '%');
	      if (p != 0)
		{
		  register unsigned int i;
		  if (check_lastslash)
		    {
		      /* Copy directory name from the original FILENAME.  */
		      i = lastslash - filename + 1;
		      bcopy (filename, depname, i);
		    }
		  else
		    i = 0;
		  bcopy (dep_name (dep), depname + i, p - dep_name (dep));
		  i += p - dep_name (dep);
		  bcopy (stem, depname + i, stemlen);
		  i += stemlen;
		  strcpy (depname + i, p + 1);
		  p = depname;
		}
	      else
		p = dep_name (dep);

	      /* P is now the actual dependency name as substituted.  */

	      if (file_impossible_p (p))
		{
		  /* If this dependency has already been ruled
		     "impossible", then the rule fails and don't
		     bother trying it on the second pass either
		     since we know that will fail too.  */
		  DBS (DB_IMPLICIT,
                       (p == depname
                        ? _("Rejecting impossible implicit prerequisite `%s'.\n")
                        : _("Rejecting impossible rule prerequisite `%s'.\n"),
                        p));
		  tryrules[i] = 0;
		  break;
		}

	      intermediate_files[deps_found] = 0;

	      DBS (DB_IMPLICIT,
                   (p == depname
                    ? _("Trying implicit prerequisite `%s'.\n")
                    : _("Trying rule prerequisite `%s'.\n"), p));

	      /* The DEP->changed flag says that this dependency resides in a
		 nonexistent directory.  So we normally can skip looking for
		 the file.  However, if CHECK_LASTSLASH is set, then the
		 dependency file we are actually looking for is in a different
		 directory (the one gotten by prepending FILENAME's directory),
		 so it might actually exist.  */

	      if ((!dep->changed || check_lastslash)
		  && (lookup_file (p) != 0 || file_exists_p (p)))
		{
		  found_files[deps_found++] = xstrdup (p);
		  continue;
		}
	      /* This code, given FILENAME = "lib/foo.o", dependency name
		 "lib/foo.c", and VPATH=src, searches for "src/lib/foo.c".  */
	      vp = p;
	      if (vpath_search (&vp, (FILE_TIMESTAMP *) 0))
		{
		  DBS (DB_IMPLICIT,
                       (_("Found prerequisite `%s' as VPATH `%s'\n"), p, vp));
		  strcpy (vp, p);
		  found_files[deps_found++] = vp;
		  continue;
		}

	      /* We could not find the file in any place we should look.
		 Try to make this dependency as an intermediate file,
		 but only on the second pass.  */

	      if (intermed_ok)
		{
		  if (intermediate_file == 0)
		    intermediate_file
		      = (struct file *) alloca (sizeof (struct file));

		  DBS (DB_IMPLICIT,
                       (_("Looking for a rule with intermediate file `%s'.\n"),
                        p));

		  bzero ((char *) intermediate_file, sizeof (struct file));
		  intermediate_file->name = p;
		  if (pattern_search (intermediate_file, 0, depth + 1,
				      recursions + 1))
		    {
		      p = xstrdup (p);
		      intermediate_patterns[deps_found]
			= intermediate_file->name;
		      intermediate_file->name = p;
		      intermediate_files[deps_found] = intermediate_file;
		      intermediate_file = 0;
		      /* Allocate an extra copy to go in FOUND_FILES,
			 because every elt of FOUND_FILES is consumed
			 or freed later.  */
		      found_files[deps_found] = xstrdup (p);
		      ++deps_found;
		      continue;
		    }

		  /* If we have tried to find P as an intermediate
		     file and failed, mark that name as impossible
		     so we won't go through the search again later.  */
		  file_impossible (p);
		}

	      /* A dependency of this rule does not exist.
		 Therefore, this rule fails.  */
	      break;
	    }

	  /* This rule is no longer `in use' for recursive searches.  */
	  rule->in_use = 0;

	  if (dep != 0)
	    {
	      /* This pattern rule does not apply.
		 If some of its dependencies succeeded,
		 free the data structure describing them.  */
	      while (deps_found-- > 0)
		{
		  register struct file *f = intermediate_files[deps_found];
		  free (found_files[deps_found]);
		  if (f != 0
		      && (f->stem < f->name
			  || f->stem > f->name + strlen (f->name)))
		    free (f->stem);
		}
	    }
	  else
	    /* This pattern rule does apply.  Stop looking for one.  */
	    break;
	}

      /* If we found an applicable rule without
	 intermediate files, don't try with them.  */
      if (i < nrules)
	break;

      rule = 0;
    }

  /* RULE is nil if the loop went all the way
     through the list and everything failed.  */
  if (rule == 0)
    return 0;

  foundrule = i;

  /* If we are recursing, store the pattern that matched
     FILENAME in FILE->name for use in upper levels.  */

  if (recursions > 0)
    /* Kludge-o-matic */
    file->name = rule->targets[matches[foundrule]];

  /* FOUND_FILES lists the dependencies for the rule we found.
     This includes the intermediate files, if any.
     Convert them into entries on the deps-chain of FILE.  */

  while (deps_found-- > 0)
    {
      register char *s;

      if (intermediate_files[deps_found] != 0)
	{
	  /* If we need to use an intermediate file,
	     make sure it is entered as a target, with the info that was
	     found for it in the recursive pattern_search call.
	     We know that the intermediate file did not already exist as
	     a target; therefore we can assume that the deps and cmds
	     of F below are null before we change them.  */

	  struct file *imf = intermediate_files[deps_found];
	  register struct file *f = enter_file (imf->name);
	  f->deps = imf->deps;
	  f->cmds = imf->cmds;
	  f->stem = imf->stem;
          f->also_make = imf->also_make;
	  imf = lookup_file (intermediate_patterns[deps_found]);
	  if (imf != 0 && imf->precious)
	    f->precious = 1;
	  f->intermediate = 1;
	  f->tried_implicit = 1;
	  for (dep = f->deps; dep != 0; dep = dep->next)
	    {
	      dep->file = enter_file (dep->name);
              /* enter_file uses dep->name _if_ we created a new file.  */
              if (dep->name != dep->file->name)
                free (dep->name);
	      dep->name = 0;
	      dep->file->tried_implicit |= dep->changed;
	    }
	  num_intermediates++;
	}

      dep = (struct dep *) xmalloc (sizeof (struct dep));
      s = found_files[deps_found];
      if (recursions == 0)
	{
	  dep->name = 0;
	  dep->file = lookup_file (s);
	  if (dep->file == 0)
	    /* enter_file consumes S's storage.  */
	    dep->file = enter_file (s);
	  else
	    /* A copy of S is already allocated in DEP->file->name.
	       So we can free S.  */
	    free (s);
	}
      else
	{
	  dep->name = s;
	  dep->file = 0;
	  dep->changed = 0;
	}
      if (intermediate_files[deps_found] == 0 && tryrules[foundrule]->terminal)
	{
	  /* If the file actually existed (was not an intermediate file),
	     and the rule that found it was a terminal one, then we want
	     to mark the found file so that it will not have implicit rule
	     search done for it.  If we are not entering a `struct file' for
	     it now, we indicate this with the `changed' flag.  */
	  if (dep->file == 0)
	    dep->changed = 1;
	  else
	    dep->file->tried_implicit = 1;
	}
      dep->next = file->deps;
      file->deps = dep;
    }

  if (!checked_lastslash[foundrule])
    /* Always allocate new storage, since STEM might be
       on the stack for an intermediate file.  */
    file->stem = savestring (stem, stemlen);
  else
    {
      /* We want to prepend the directory from
	 the original FILENAME onto the stem.  */
      file->stem = (char *) xmalloc (((lastslash + 1) - filename)
				     + stemlen + 1);
      bcopy (filename, file->stem, (lastslash + 1) - filename);
      bcopy (stem, file->stem + ((lastslash + 1) - filename), stemlen);
      file->stem[((lastslash + 1) - filename) + stemlen] = '\0';
    }

  file->cmds = rule->cmds;

  /* If this rule builds other targets, too, put the others into FILE's
     `also_make' member.  */

  if (rule->targets[1] != 0)
    for (i = 0; rule->targets[i] != 0; ++i)
      if (i != matches[foundrule])
	{
	  struct dep *new = (struct dep *) xmalloc (sizeof (struct dep));
	  new->name = p = (char *) xmalloc (rule->lens[i] + stemlen + 1);
	  bcopy (rule->targets[i], p,
		 rule->suffixes[i] - rule->targets[i] - 1);
	  p += rule->suffixes[i] - rule->targets[i] - 1;
	  bcopy (stem, p, stemlen);
	  p += stemlen;
	  bcopy (rule->suffixes[i], p,
		 rule->lens[i] - (rule->suffixes[i] - rule->targets[i]) + 1);
	  new->file = enter_file (new->name);
	  new->next = file->also_make;
	  file->also_make = new;
	}


  return 1;
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         job.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Job execution and handling for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,96,97,99 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <assert.h>

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
#include "debug.h"
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"

#include <string.h>
#undef stderr
#define stderr stdout

/* Default shell to use.  */
#ifdef WINDOWS32
char *default_shell = "sh.exe";
int no_default_sh_exe = 1;
int batch_mode_shell = 1;
#else  /* WINDOWS32 */
# ifdef _AMIGA
char default_shell[] = "";
extern int MyExecute (char **);
# else /* _AMIGA */
#  ifdef __MSDOS__
/* The default shell is a pointer so we can change it if Makefile
   says so.  It is without an explicit path so we get a chance
   to search the $PATH for it (since MSDOS doesn't have standard
   directories we could trust).  */
char *default_shell = "command.com";
#  else  /* __MSDOS__ */
#   ifdef VMS
#    include <descrip.h>
#undef stderr
#define stderr stdout
char default_shell[] = "";
#   else
char default_shell[] = "/bin/sh";
#   endif /* VMS */
#  endif /* __MSDOS__ */
int batch_mode_shell = 0;
# endif /* _AMIGA */
#endif /* WINDOWS32 */

#ifdef __MSDOS__
# include <process.h>
#undef stderr
#define stderr stdout
static int execute_by_shell;
static int dos_pid = 123;
int dos_status;
int dos_command_running;
#endif /* __MSDOS__ */

#ifdef _AMIGA
# include <proto/dos.h>
#undef stderr
#define stderr stdout
static int amiga_pid = 123;
static int amiga_status;
static char amiga_bname[32];
static int amiga_batch_file;
#endif /* Amiga.  */

#ifdef VMS
# include <time.h>
#undef stderr
#define stderr stdout
# ifndef __GNUC__
#   include <processes.h>
#undef stderr
#define stderr stdout
# endif
# include <starlet.h>
#undef stderr
#define stderr stdout
# include <lib$routines.h>
#undef stderr
#define stderr stdout
#endif

#ifdef WINDOWS32
# include <windows.h>
# include <io.h>
# include <process.h>
# include "sub_proc.h"
# include "w32err.h"
# include "pathstuff.h"
#undef stderr
#define stderr stdout
#endif /* WINDOWS32 */

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#undef stderr
#define stderr stdout
#else
# include <sys/file.h>
#undef stderr
#define stderr stdout
#endif

#if defined (HAVE_SYS_WAIT_H) || defined (HAVE_UNION_WAIT)
# include <syswait.h>
#undef stderr
#define stderr stdout
#endif

#ifdef HAVE_WAITPID
# define WAIT_NOHANG(status)	waitpid (-1, (status), WNOHANG)
#else	/* Don't have waitpid.  */
# ifdef HAVE_WAIT3
#  ifndef wait3
extern int wait3 ();
#  endif
#  define WAIT_NOHANG(status)	wait3 ((status), WNOHANG, (struct rusage *) 0)
# endif /* Have wait3.  */
#endif /* Have waitpid.  */

#if !defined (wait) && !defined (POSIX)
extern int wait ();
#endif

#ifndef	HAVE_UNION_WAIT

# define WAIT_T int

# ifndef WTERMSIG
#  define WTERMSIG(x) ((x) & 0x7f)
# endif
# ifndef WCOREDUMP
#  define WCOREDUMP(x) ((x) & 0x80)
# endif
# ifndef WEXITSTATUS
#  define WEXITSTATUS(x) (((x) >> 8) & 0xff)
# endif
# ifndef WIFSIGNALED
#  define WIFSIGNALED(x) (WTERMSIG (x) != 0)
# endif
# ifndef WIFEXITED
#  define WIFEXITED(x) (WTERMSIG (x) == 0)
# endif

#else	/* Have `union wait'.  */

# define WAIT_T union wait
# ifndef WTERMSIG
#  define WTERMSIG(x) ((x).w_termsig)
# endif
# ifndef WCOREDUMP
#  define WCOREDUMP(x) ((x).w_coredump)
# endif
# ifndef WEXITSTATUS
#  define WEXITSTATUS(x) ((x).w_retcode)
# endif
# ifndef WIFSIGNALED
#  define WIFSIGNALED(x) (WTERMSIG(x) != 0)
# endif
# ifndef WIFEXITED
#  define WIFEXITED(x) (WTERMSIG(x) == 0)
# endif

#endif	/* Don't have `union wait'.  */

/* How to set close-on-exec for a file descriptor.  */

#if !defined F_SETFD
# define CLOSE_ON_EXEC(_d)
#else
# ifndef FD_CLOEXEC
#  define FD_CLOEXEC 1
# endif
# define CLOSE_ON_EXEC(_d) (void) fcntl ((_d), F_SETFD, FD_CLOEXEC)
#endif

#ifdef VMS
static int vms_jobsefnmask = 0;
#endif /* !VMS */

#ifndef	HAVE_UNISTD_H
extern int dup2 ();
extern int execve ();
extern void _exit ();
# ifndef VMS
extern int geteuid ();
extern int getegid ();
extern int setgid ();
extern int getgid ();
# endif
#endif

extern char *allocated_variable_expand_for_file PARAMS ((char *line, struct file *file));

extern int getloadavg PARAMS ((double loadavg[], int nelem));
extern int start_remote_job PARAMS ((char **argv, char **envp, int stdin_fd,
		int *is_remote, int *id_ptr, int *used_stdin));
extern int start_remote_job_p PARAMS ((int));
extern int remote_status PARAMS ((int *exit_code_ptr, int *signal_ptr,
		int *coredump_ptr, int block));

RETSIGTYPE child_handler PARAMS ((int));
static void free_child PARAMS ((struct child *));
static void start_job_command PARAMS ((struct child *child));
static int load_too_high PARAMS ((void));
static int job_next_command PARAMS ((struct child *));
static int start_waiting_job PARAMS ((struct child *));
#ifdef VMS
static void vmsWaitForChildren PARAMS ((int *));
#endif

/* Chain of all live (or recently deceased) children.  */

struct child *children = 0;

/* Number of children currently running.  */

unsigned int job_slots_used = 0;

/* Nonzero if the `good' standard input is in use.  */

static int good_stdin_used = 0;

/* Chain of children waiting to run until the load average goes down.  */

static struct child *waiting_jobs = 0;

/* Non-zero if we use a *real* shell (always so on Unix).  */

int unixy_shell = 1;


#ifdef WINDOWS32
/*
 * The macro which references this function is defined in make.h.
 */
int w32_kill(int pid, int sig)
{
  return ((process_kill(pid, sig) == TRUE) ? 0 : -1);
}
#endif /* WINDOWS32 */

/* Write an error message describing the exit status given in
   EXIT_CODE, EXIT_SIG, and COREDUMP, for the target TARGET_NAME.
   Append "(ignored)" if IGNORED is nonzero.  */

static void
child_error (target_name, exit_code, exit_sig, coredump, ignored)
     char *target_name;
     int exit_code, exit_sig, coredump;
     int ignored;
{
  if (ignored && silent_flag)
    return;

#ifdef VMS
  if (!(exit_code & 1))
      error (NILF,
             (ignored ? _("*** [%s] Error 0x%x (ignored)")
              : _("*** [%s] Error 0x%x")),
             target_name, exit_code);
#else
  if (exit_sig == 0)
    error (NILF, ignored ? _("[%s] Error %d (ignored)") :
	   _("*** [%s] Error %d"),
	   target_name, exit_code);
  else
    error (NILF, "*** [%s] %s%s",
	   target_name, strsignal (exit_sig),
	   coredump ? _(" (core dumped)") : "");
#endif /* VMS */
}

#ifdef VMS
/* Wait for nchildren children to terminate */
static void
vmsWaitForChildren(int *status)
{
  while (1)
    {
      if (!vms_jobsefnmask)
	{
	  *status = 0;
	  return;
	}

      *status = sys$wflor (32, vms_jobsefnmask);
    }
  return;
}

/* Set up IO redirection.  */

char *
vms_redirect (desc, fname, ibuf)
    struct dsc$descriptor_s *desc;
    char *fname;
    char *ibuf;
{
  char *fptr;
  extern char *vmsify ();

  ibuf++;
  while (isspace (*ibuf))
    ibuf++;
  fptr = ibuf;
  while (*ibuf && !isspace (*ibuf))
    ibuf++;
  *ibuf = 0;
  if (strcmp (fptr, "/dev/null") != 0)
    {
      strcpy (fname, vmsify (fptr, 0));
      if (strchr (fname, '.') == 0)
	strcat (fname, ".");
    }
  desc->dsc$w_length = strlen(fname);
  desc->dsc$a_pointer = fname;
  desc->dsc$b_dtype = DSC$K_DTYPE_T;
  desc->dsc$b_class = DSC$K_CLASS_S;

  if (*fname == 0)
    printf (_("Warning: Empty redirection\n"));
  return ibuf;
}


/*
   found apostrophe at (p-1)

   inc p until after closing apostrophe.  */

static char *
handle_apos (char *p)
{
  int alast;
  int inside;

#define SEPCHARS ",/()= "

  inside = 0;

  while (*p != 0)
    {
      if (*p == '"')
	{
	  if (inside)
	    {
	      while ((alast > 0)
		    && (*p == '"'))
		{
		  p++;
		  alast--;
		}
	      if (alast == 0)
		inside = 0;
	      else
		{
		  fprintf (stderr, _("Syntax error, still inside '\"'\n"));
		  exit (3);
		}
	    }
	  else
	    {
	      p++;
	      if (strchr (SEPCHARS, *p))
		break;
	      inside = 1;
	      alast = 1;
	      while (*p == '"')
		{
		  alast++;
		  p++;
		}
	    }
	}
      else
	p++;
    }

  return p;
}

#endif


/* Handle a dead child.  This handler may or may not ever be installed.

   If we're using the jobserver feature, we need it.  First, installing it
   ensures the read will interrupt on SIGCHLD.  Second, we close the dup'd
   read FD to ensure we don't enter another blocking read without reaping all
   the dead children.  In this case we don't need the dead_children count.

   If we don't have either waitpid or wait3, then make is unreliable, but we
   use the dead_children count to reap children as best we can.  */

static unsigned int dead_children = 0;

RETSIGTYPE
child_handler (sig)
     int sig;
{
  ++dead_children;

  if (job_rfd >= 0)
    {
      close (job_rfd);
      job_rfd = -1;
    }

  DB (DB_JOBS, (_("Got a SIGCHLD; %u unreaped children.\n"), dead_children));
}


extern int shell_function_pid, shell_function_completed;

/* Reap all dead children, storing the returned status and the new command
   state (`cs_finished') in the `file' member of the `struct child' for the
   dead child, and removing the child from the chain.  In addition, if BLOCK
   nonzero, we block in this function until we've reaped at least one
   complete child, waiting for it to die if necessary.  If ERR is nonzero,
   print an error message first.  */

void
reap_children (block, err)
     int block, err;
{
  WAIT_T status;
  /* Initially, assume we have some.  */
  int reap_more = 1;

#ifdef WAIT_NOHANG
# define REAP_MORE reap_more
#else
# define REAP_MORE dead_children
#endif

  /* As long as:

       We have at least one child outstanding OR a shell function in progress,
         AND
       We're blocking for a complete child OR there are more children to reap

     we'll keep reaping children.  */

  while ((children != 0 || shell_function_pid != 0) &&
	 (block || REAP_MORE))
    {
      int remote = 0;
      register int pid;
      int exit_code, exit_sig, coredump;
      register struct child *lastc, *c;
      int child_failed;
      int any_remote, any_local;

      if (err && block)
	{
	  /* We might block for a while, so let the user know why.  */
	  fflush (stdout);
	  error (NILF, _("*** Waiting for unfinished jobs...."));
	}

      /* We have one less dead child to reap.  As noted in
	 child_handler() above, this count is completely unimportant for
	 all modern, POSIX-y systems that support wait3() or waitpid().
	 The rest of this comment below applies only to early, broken
	 pre-POSIX systems.  We keep the count only because... it's there...

	 The test and decrement are not atomic; if it is compiled into:
	 	register = dead_children - 1;
		dead_children = register;
	 a SIGCHLD could come between the two instructions.
	 child_handler increments dead_children.
	 The second instruction here would lose that increment.  But the
	 only effect of dead_children being wrong is that we might wait
	 longer than necessary to reap a child, and lose some parallelism;
	 and we might print the "Waiting for unfinished jobs" message above
	 when not necessary.  */

      if (dead_children > 0)
	--dead_children;

      any_remote = 0;
      any_local = shell_function_pid != 0;
      for (c = children; c != 0; c = c->next)
	{
	  any_remote |= c->remote;
	  any_local |= ! c->remote;
	  DB (DB_JOBS, (_("Live child 0x%08lx (%s) PID %ld %s\n"),
                        (unsigned long int) c, c->file->name,
                        (long) c->pid, c->remote ? _(" (remote)") : ""));
#ifdef VMS
	  break;
#endif
	}

      /* First, check for remote children.  */
      if (any_remote)
	pid = remote_status (&exit_code, &exit_sig, &coredump, 0);
      else
	pid = 0;

      if (pid > 0)
	/* We got a remote child.  */
	remote = 1;
      else if (pid < 0)
	{
          /* A remote status command failed miserably.  Punt.  */
	remote_status_lose:
	  if (EINTR_SET)
	    continue;

	  pfatal_with_name ("remote_status");
	}
      else
	{
	  /* No remote children.  Check for local children.  */
#if !defined(__MSDOS__) && !defined(_AMIGA) && !defined(WINDOWS32)
	  if (any_local)
	    {
            local_wait:
#ifdef VMS
	      vmsWaitForChildren (&status);
	      pid = c->pid;
#else
#ifdef WAIT_NOHANG
	      if (!block)
		pid = WAIT_NOHANG (&status);
	      else
#endif
		pid = wait (&status);
#endif /* !VMS */
	    }
	  else
	    pid = 0;

	  if (pid < 0)
	    {
              /* EINTR?  Try again. */
	      if (EINTR_SET)
		goto local_wait;

              /* The wait*() failed miserably.  Punt.  */
	      pfatal_with_name ("wait");
	    }
	  else if (pid > 0)
	    {
	      /* We got a child exit; chop the status word up.  */
	      exit_code = WEXITSTATUS (status);
	      exit_sig = WIFSIGNALED (status) ? WTERMSIG (status) : 0;
	      coredump = WCOREDUMP (status);
	    }
	  else
	    {
	      /* No local children are dead.  */
              reap_more = 0;

	      if (!block || !any_remote)
                break;

              /* Now try a blocking wait for a remote child.  */
              pid = remote_status (&exit_code, &exit_sig, &coredump, 1);
              if (pid < 0)
                goto remote_status_lose;
              else if (pid == 0)
                /* No remote children either.  Finally give up.  */
                break;

              /* We got a remote child.  */
              remote = 1;
	    }
#endif /* !__MSDOS__, !Amiga, !WINDOWS32.  */

#ifdef __MSDOS__
	  /* Life is very different on MSDOS.  */
	  pid = dos_pid - 1;
	  status = dos_status;
	  exit_code = WEXITSTATUS (status);
	  if (exit_code == 0xff)
	    exit_code = -1;
	  exit_sig = WIFSIGNALED (status) ? WTERMSIG (status) : 0;
	  coredump = 0;
#endif /* __MSDOS__ */
#ifdef _AMIGA
	  /* Same on Amiga */
	  pid = amiga_pid - 1;
	  status = amiga_status;
	  exit_code = amiga_status;
	  exit_sig = 0;
	  coredump = 0;
#endif /* _AMIGA */
#ifdef WINDOWS32
          {
            HANDLE hPID;
            int err;

            /* wait for anything to finish */
            if (hPID = process_wait_for_any()) {

              /* was an error found on this process? */
              err = process_last_err(hPID);

              /* get exit data */
              exit_code = process_exit_code(hPID);

              if (err)
                fprintf(stderr, "make (e=%d): %s",
                  exit_code, map_windows32_error_to_string(exit_code));

              /* signal */
              exit_sig = process_signal(hPID);

              /* cleanup process */
              process_cleanup(hPID);

              coredump = 0;
            }
            pid = (int) hPID;
          }
#endif /* WINDOWS32 */
	}

      /* Check if this is the child of the `shell' function.  */
      if (!remote && pid == shell_function_pid)
	{
	  /* It is.  Leave an indicator for the `shell' function.  */
	  if (exit_sig == 0 && exit_code == 127)
	    shell_function_completed = -1;
	  else
	    shell_function_completed = 1;
	  break;
	}

      child_failed = exit_sig != 0 || exit_code != 0;

      /* Search for a child matching the deceased one.  */
      lastc = 0;
      for (c = children; c != 0; lastc = c, c = c->next)
	if (c->remote == remote && c->pid == pid)
	  break;

      if (c == 0)
        /* An unknown child died.
           Ignore it; it was inherited from our invoker.  */
        continue;

      DB (DB_JOBS, (child_failed
                    ? _("Reaping losing child 0x%08lx PID %ld %s\n")
                    : _("Reaping winning child 0x%08lx PID %ld %s\n"),
                    (unsigned long int) c, (long) c->pid,
                    c->remote ? _(" (remote)") : ""));

      if (c->sh_batch_file) {
        DB (DB_JOBS, (_("Cleaning up temp batch file %s\n"),
                      c->sh_batch_file));

        /* just try and remove, don't care if this fails */
        remove (c->sh_batch_file);

        /* all done with memory */
        free (c->sh_batch_file);
        c->sh_batch_file = NULL;
      }

      /* If this child had the good stdin, say it is now free.  */
      if (c->good_stdin)
        good_stdin_used = 0;

      if (child_failed && !c->noerror && !ignore_errors_flag)
        {
          /* The commands failed.  Write an error message,
             delete non-precious targets, and abort.  */
          static int delete_on_error = -1;
          child_error (c->file->name, exit_code, exit_sig, coredump, 0);
          c->file->update_status = 2;
          if (delete_on_error == -1)
            {
              struct file *f = lookup_file (".DELETE_ON_ERROR");
              delete_on_error = f != 0 && f->is_target;
            }
          if (exit_sig != 0 || delete_on_error)
            delete_child_targets (c);
        }
      else
        {
          if (child_failed)
            {
              /* The commands failed, but we don't care.  */
              child_error (c->file->name,
                           exit_code, exit_sig, coredump, 1);
              child_failed = 0;
            }

          /* If there are more commands to run, try to start them.  */
          if (job_next_command (c))
            {
              if (handling_fatal_signal)
                {
                  /* Never start new commands while we are dying.
                     Since there are more commands that wanted to be run,
                     the target was not completely remade.  So we treat
                     this as if a command had failed.  */
                  c->file->update_status = 2;
                }
              else
                {
                  /* Check again whether to start remotely.
                     Whether or not we want to changes over time.
                     Also, start_remote_job may need state set up
                     by start_remote_job_p.  */
                  c->remote = start_remote_job_p (0);
                  start_job_command (c);
                  /* Fatal signals are left blocked in case we were
                     about to put that child on the chain.  But it is
                     already there, so it is safe for a fatal signal to
                     arrive now; it will clean up this child's targets.  */
                  unblock_sigs ();
                  if (c->file->command_state == cs_running)
                    /* We successfully started the new command.
                       Loop to reap more children.  */
                    continue;
                }

              if (c->file->update_status != 0)
                /* We failed to start the commands.  */
                delete_child_targets (c);
            }
          else
            /* There are no more commands.  We got through them all
               without an unignored error.  Now the target has been
               successfully updated.  */
            c->file->update_status = 0;
        }

      /* When we get here, all the commands for C->file are finished
         (or aborted) and C->file->update_status contains 0 or 2.  But
         C->file->command_state is still cs_running if all the commands
         ran; notice_finish_file looks for cs_running to tell it that
         it's interesting to check the file's modtime again now.  */

      if (! handling_fatal_signal)
        /* Notice if the target of the commands has been changed.
           This also propagates its values for command_state and
           update_status to its also_make files.  */
        notice_finished_file (c->file);

      DB (DB_JOBS, (_("Removing child 0x%08lx PID %ld %s from chain.\n"),
                    (unsigned long int) c, (long) c->pid,
                    c->remote ? _(" (remote)") : ""));

      /* Block fatal signals while frobnicating the list, so that
         children and job_slots_used are always consistent.  Otherwise
         a fatal signal arriving after the child is off the chain and
         before job_slots_used is decremented would believe a child was
         live and call reap_children again.  */
      block_sigs ();

      /* There is now another slot open.  */
      if (job_slots_used > 0)
        --job_slots_used;

      /* Remove the child from the chain and free it.  */
      if (lastc == 0)
        children = c->next;
      else
        lastc->next = c->next;

      free_child (c);

      unblock_sigs ();

      /* If the job failed, and the -k flag was not given, die,
         unless we are already in the process of dying.  */
      if (!err && child_failed && !keep_going_flag &&
          /* fatal_error_signal will die with the right signal.  */
          !handling_fatal_signal)
        die (2);

      /* Only block for one child.  */
      block = 0;
    }

  return;
}

/* Free the storage allocated for CHILD.  */

static void
free_child (child)
     register struct child *child;
{
  /* If this child is the only one it was our "free" job, so don't put a
     token back for it.  This child has already been removed from the list,
     so if there any left this wasn't the last one.  */

  if (job_fds[1] >= 0 && children)
    {
      char token = '+';

      /* Write a job token back to the pipe.  */

      while (write (job_fds[1], &token, 1) != 1)
        if (!EINTR_SET)
          pfatal_with_name (_("write jobserver"));

      DB (DB_JOBS, (_("Released token for child 0x%08lx (%s).\n"),
                    (unsigned long int) child, child->file->name));
    }

  if (handling_fatal_signal) /* Don't bother free'ing if about to die.  */
    return;

  if (child->command_lines != 0)
    {
      register unsigned int i;
      for (i = 0; i < child->file->cmds->ncommand_lines; ++i)
	free (child->command_lines[i]);
      free ((char *) child->command_lines);
    }

  if (child->environment != 0)
    {
      register char **ep = child->environment;
      while (*ep != 0)
	free (*ep++);
      free ((char *) child->environment);
    }

  free ((char *) child);
}

#ifdef POSIX
extern sigset_t fatal_signal_set;
#endif

void
block_sigs ()
{
#ifdef POSIX
  (void) sigprocmask (SIG_BLOCK, &fatal_signal_set, (sigset_t *) 0);
#else
# ifdef HAVE_SIGSETMASK
  (void) sigblock (fatal_signal_mask);
# endif
#endif
}

#ifdef	POSIX
void
unblock_sigs ()
{
  sigset_t empty;
  sigemptyset (&empty);
  sigprocmask (SIG_SETMASK, &empty, (sigset_t *) 0);
}
#endif

/* Start a job to run the commands specified in CHILD.
   CHILD is updated to reflect the commands and ID of the child process.

   NOTE: On return fatal signals are blocked!  The caller is responsible
   for calling `unblock_sigs', once the new child is safely on the chain so
   it can be cleaned up in the event of a fatal signal.  */

static void
start_job_command (child)
     register struct child *child;
{
#ifndef _AMIGA
  static int bad_stdin = -1;
#endif
  register char *p;
  int flags;
#ifdef VMS
  char *argv;
#else
  char **argv;
#endif

  /* If we have a completely empty commandset, stop now.  */
  if (!child->command_ptr)
    goto next_command;

  /* Combine the flags parsed for the line itself with
     the flags specified globally for this target.  */
  flags = (child->file->command_flags
	   | child->file->cmds->lines_flags[child->command_line - 1]);

  p = child->command_ptr;
  child->noerror = flags & COMMANDS_NOERROR;

  while (*p != '\0')
    {
      if (*p == '@')
	flags |= COMMANDS_SILENT;
      else if (*p == '+')
	flags |= COMMANDS_RECURSE;
      else if (*p == '-')
	child->noerror = 1;
      else if (!isblank (*p))
	break;
      ++p;
    }

  /* Update the file's command flags with any new ones we found.  */
  child->file->cmds->lines_flags[child->command_line - 1] |= flags;

  /* If -q was given, just say that updating `failed'.  The exit status of
     1 tells the user that -q is saying `something to do'; the exit status
     for a random error is 2.  */
  if (question_flag && !(flags & COMMANDS_RECURSE))
    {
      child->file->update_status = 1;
      notice_finished_file (child->file);
      return;
    }

  /* There may be some preceding whitespace left if there
     was nothing but a backslash on the first line.  */
  p = next_token (p);

  /* Figure out an argument list from this command line.  */

  {
    char *end = 0;
#ifdef VMS
    argv = p;
#else
    argv = construct_command_argv (p, &end, child->file, &child->sh_batch_file);
#endif
    if (end == NULL)
      child->command_ptr = NULL;
    else
      {
	*end++ = '\0';
	child->command_ptr = end;
      }
  }

  if (touch_flag && !(flags & COMMANDS_RECURSE))
    {
      /* Go on to the next command.  It might be the recursive one.
	 We construct ARGV only to find the end of the command line.  */
#ifndef VMS
      free (argv[0]);
      free ((char *) argv);
#endif
      argv = 0;
    }

  if (argv == 0)
    {
    next_command:
#ifdef __MSDOS__
      execute_by_shell = 0;   /* in case construct_command_argv sets it */
#endif
      /* This line has no commands.  Go to the next.  */
      if (job_next_command (child))
	start_job_command (child);
      else
	{
	  /* No more commands.  Make sure we're "running"; we might not be if
             (e.g.) all commands were skipped due to -n.  */
          set_command_state (child->file, cs_running);
	  child->file->update_status = 0;
	  notice_finished_file (child->file);
	}
      return;
    }

  /* Print out the command.  If silent, we call `message' with null so it
     can log the working directory before the command's own error messages
     appear.  */

  message (0, (just_print_flag || (!(flags & COMMANDS_SILENT) && !silent_flag))
	   ? "%s" : (char *) 0, p);

  /* Optimize an empty command.  People use this for timestamp rules,
     so avoid forking a useless shell.  */

#if !defined(VMS) && !defined(_AMIGA)
  if (
#ifdef __MSDOS__
      unixy_shell	/* the test is complicated and we already did it */
#else
      (argv[0] && !strcmp (argv[0], "/bin/sh"))
#endif
      && (argv[1]
          && argv[1][0] == '-' && argv[1][1] == 'c' && argv[1][2] == '\0')
      && (argv[2] && argv[2][0] == ':' && argv[2][1] == '\0')
      && argv[3] == NULL)
    {
      free (argv[0]);
      free ((char *) argv);
      goto next_command;
    }
#endif  /* !VMS && !_AMIGA */

  /* Tell update_goal_chain that a command has been started on behalf of
     this target.  It is important that this happens here and not in
     reap_children (where we used to do it), because reap_children might be
     reaping children from a different target.  We want this increment to
     guaranteedly indicate that a command was started for the dependency
     chain (i.e., update_file recursion chain) we are processing.  */

  ++commands_started;

  /* If -n was given, recurse to get the next line in the sequence.  */

  if (just_print_flag && !(flags & COMMANDS_RECURSE))
    {
#ifndef VMS
      free (argv[0]);
      free ((char *) argv);
#endif
      goto next_command;
    }

  /* Flush the output streams so they won't have things written twice.  */

  fflush (stdout);
  fflush (stderr);

#ifndef VMS
#if !defined(WINDOWS32) && !defined(_AMIGA) && !defined(__MSDOS__)

  /* Set up a bad standard input that reads from a broken pipe.  */

  if (bad_stdin == -1)
    {
      /* Make a file descriptor that is the read end of a broken pipe.
	 This will be used for some children's standard inputs.  */
      int pd[2];
      if (pipe (pd) == 0)
	{
	  /* Close the write side.  */
	  (void) close (pd[1]);
	  /* Save the read side.  */
	  bad_stdin = pd[0];

	  /* Set the descriptor to close on exec, so it does not litter any
	     child's descriptor table.  When it is dup2'd onto descriptor 0,
	     that descriptor will not close on exec.  */
	  CLOSE_ON_EXEC (bad_stdin);
	}
    }

#endif /* !WINDOWS32 && !_AMIGA && !__MSDOS__ */

  /* Decide whether to give this child the `good' standard input
     (one that points to the terminal or whatever), or the `bad' one
     that points to the read side of a broken pipe.  */

  child->good_stdin = !good_stdin_used;
  if (child->good_stdin)
    good_stdin_used = 1;

#endif /* !VMS */

  child->deleted = 0;

#ifndef _AMIGA
  /* Set up the environment for the child.  */
  if (child->environment == 0)
    child->environment = target_environment (child->file);
#endif

#if !defined(__MSDOS__) && !defined(_AMIGA) && !defined(WINDOWS32)

#ifndef VMS
  /* start_waiting_job has set CHILD->remote if we can start a remote job.  */
  if (child->remote)
    {
      int is_remote, id, used_stdin;
      if (start_remote_job (argv, child->environment,
			    child->good_stdin ? 0 : bad_stdin,
			    &is_remote, &id, &used_stdin))
        /* Don't give up; remote execution may fail for various reasons.  If
           so, simply run the job locally.  */
	goto run_local;
      else
	{
	  if (child->good_stdin && !used_stdin)
	    {
	      child->good_stdin = 0;
	      good_stdin_used = 0;
	    }
	  child->remote = is_remote;
	  child->pid = id;
	}
    }
  else
#endif /* !VMS */
    {
      /* Fork the child process.  */

      char **parent_environ;

    run_local:
      block_sigs ();

      child->remote = 0;

#ifdef VMS

      if (!child_execute_job (argv, child)) {
        /* Fork failed!  */
        perror_with_name ("vfork", "");
        goto error;
      }

#else

      parent_environ = environ;
      child->pid = vfork ();
      environ = parent_environ;	/* Restore value child may have clobbered.  */
      if (child->pid == 0)
	{
	  /* We are the child side.  */
	  unblock_sigs ();

          /* If we aren't running a recursive command and we have a jobserver
             pipe, close it before exec'ing.  */
          if (!(flags & COMMANDS_RECURSE) && job_fds[0] >= 0)
            {
              close (job_fds[0]);
              close (job_fds[1]);
            }
          if (job_rfd >= 0)
            close (job_rfd);

	  child_execute_job (child->good_stdin ? 0 : bad_stdin, 1,
                             argv, child->environment);
	}
      else if (child->pid < 0)
	{
	  /* Fork failed!  */
	  unblock_sigs ();
	  perror_with_name ("vfork", "");
	  goto error;
	}
#endif /* !VMS */
    }

#else	/* __MSDOS__ or Amiga or WINDOWS32 */
#ifdef __MSDOS__
  {
    int proc_return;

    block_sigs ();
    dos_status = 0;

    /* We call `system' to do the job of the SHELL, since stock DOS
       shell is too dumb.  Our `system' knows how to handle long
       command lines even if pipes/redirection is needed; it will only
       call COMMAND.COM when its internal commands are used.  */
    if (execute_by_shell)
      {
	char *cmdline = argv[0];
	/* We don't have a way to pass environment to `system',
	   so we need to save and restore ours, sigh...  */
	char **parent_environ = environ;

	environ = child->environment;

	/* If we have a *real* shell, tell `system' to call
	   it to do everything for us.  */
	if (unixy_shell)
	  {
	    /* A *real* shell on MSDOS may not support long
	       command lines the DJGPP way, so we must use `system'.  */
	    cmdline = argv[2];	/* get past "shell -c" */
	  }

	dos_command_running = 1;
	proc_return = system (cmdline);
	environ = parent_environ;
	execute_by_shell = 0;	/* for the next time */
      }
    else
      {
	dos_command_running = 1;
	proc_return = spawnvpe (P_WAIT, argv[0], argv, child->environment);
      }

    /* Need to unblock signals before turning off
       dos_command_running, so that child's signals
       will be treated as such (see fatal_error_signal).  */
    unblock_sigs ();
    dos_command_running = 0;

    /* If the child got a signal, dos_status has its
       high 8 bits set, so be careful not to alter them.  */
    if (proc_return == -1)
      dos_status |= 0xff;
    else
      dos_status |= (proc_return & 0xff);
    ++dead_children;
    child->pid = dos_pid++;
  }
#endif /* __MSDOS__ */
#ifdef _AMIGA
  amiga_status = MyExecute (argv);

  ++dead_children;
  child->pid = amiga_pid++;
  if (amiga_batch_file)
  {
     amiga_batch_file = 0;
     DeleteFile (amiga_bname);        /* Ignore errors.  */
  }
#endif	/* Amiga */
#ifdef WINDOWS32
  {
      HANDLE hPID;
      char* arg0;

      /* make UNC paths safe for CreateProcess -- backslash format */
      arg0 = argv[0];
      if (arg0 && arg0[0] == '/' && arg0[1] == '/')
        for ( ; arg0 && *arg0; arg0++)
          if (*arg0 == '/')
            *arg0 = '\\';

      /* make sure CreateProcess() has Path it needs */
      sync_Path_environment();

      hPID = process_easy(argv, child->environment);

      if (hPID != INVALID_HANDLE_VALUE)
        child->pid = (int) hPID;
      else {
        int i;
        unblock_sigs();
        fprintf(stderr,
          _("process_easy() failed failed to launch process (e=%d)\n"),
          process_last_err(hPID));
               for (i = 0; argv[i]; i++)
                 fprintf(stderr, "%s ", argv[i]);
               fprintf(stderr, _("\nCounted %d args in failed launch\n"), i);
      }
  }
#endif /* WINDOWS32 */
#endif	/* __MSDOS__ or Amiga or WINDOWS32 */

  /* We are the parent side.  Set the state to
     say the commands are running and return.  */

  set_command_state (child->file, cs_running);

  /* Free the storage used by the child's argument list.  */
#ifndef VMS
  free (argv[0]);
  free ((char *) argv);
#endif

  return;

 error:
  child->file->update_status = 2;
  notice_finished_file (child->file);
  return;
}

/* Try to start a child running.
   Returns nonzero if the child was started (and maybe finished), or zero if
   the load was too high and the child was put on the `waiting_jobs' chain.  */

static int
start_waiting_job (c)
     struct child *c;
{
  struct file *f = c->file;

  /* If we can start a job remotely, we always want to, and don't care about
     the local load average.  We record that the job should be started
     remotely in C->remote for start_job_command to test.  */

  c->remote = start_remote_job_p (1);

  /* If we are running at least one job already and the load average
     is too high, make this one wait.  */
  if (!c->remote && job_slots_used > 0 && load_too_high ())
    {
      /* Put this child on the chain of children waiting for the load average
         to go down.  */
      set_command_state (f, cs_running);
      c->next = waiting_jobs;
      waiting_jobs = c;
      return 0;
    }

  /* Start the first command; reap_children will run later command lines.  */
  start_job_command (c);

  switch (f->command_state)
    {
    case cs_running:
      c->next = children;
      DB (DB_JOBS, (_("Putting child 0x%08lx (%s) PID %ld%s on the chain.\n"),
                    (unsigned long int) c, c->file->name,
                    (long) c->pid, c->remote ? _(" (remote)") : ""));
      children = c;
      /* One more job slot is in use.  */
      ++job_slots_used;
      unblock_sigs ();
      break;

    case cs_not_started:
      /* All the command lines turned out to be empty.  */
      f->update_status = 0;
      /* FALLTHROUGH */

    case cs_finished:
      notice_finished_file (f);
      free_child (c);
      break;

    default:
      assert (f->command_state == cs_finished);
      break;
    }

  return 1;
}

/* Create a `struct child' for FILE and start its commands running.  */

void
new_job (file)
     register struct file *file;
{
  register struct commands *cmds = file->cmds;
  register struct child *c;
  char **lines;
  register unsigned int i;

  /* Let any previously decided-upon jobs that are waiting
     for the load to go down start before this new one.  */
  start_waiting_jobs ();

  /* Reap any children that might have finished recently.  */
  reap_children (0, 0);

  /* Chop the commands up into lines if they aren't already.  */
  chop_commands (cmds);

  /* Expand the command lines and store the results in LINES.  */
  lines = (char **) xmalloc (cmds->ncommand_lines * sizeof (char *));
  for (i = 0; i < cmds->ncommand_lines; ++i)
    {
      /* Collapse backslash-newline combinations that are inside variable
	 or function references.  These are left alone by the parser so
	 that they will appear in the echoing of commands (where they look
	 nice); and collapsed by construct_command_argv when it tokenizes.
	 But letting them survive inside function invocations loses because
	 we don't want the functions to see them as part of the text.  */

      char *in, *out, *ref;

      /* IN points to where in the line we are scanning.
	 OUT points to where in the line we are writing.
	 When we collapse a backslash-newline combination,
	 IN gets ahead of OUT.  */

      in = out = cmds->command_lines[i];
      while ((ref = strchr (in, '$')) != 0)
	{
	  ++ref;		/* Move past the $.  */

	  if (out != in)
	    /* Copy the text between the end of the last chunk
	       we processed (where IN points) and the new chunk
	       we are about to process (where REF points).  */
	    bcopy (in, out, ref - in);

	  /* Move both pointers past the boring stuff.  */
	  out += ref - in;
	  in = ref;

	  if (*ref == '(' || *ref == '{')
	    {
	      char openparen = *ref;
	      char closeparen = openparen == '(' ? ')' : '}';
	      int count;
	      char *p;

	      *out++ = *in++;	/* Copy OPENPAREN.  */
	      /* IN now points past the opening paren or brace.
		 Count parens or braces until it is matched.  */
	      count = 0;
	      while (*in != '\0')
		{
		  if (*in == closeparen && --count < 0)
		    break;
		  else if (*in == '\\' && in[1] == '\n')
		    {
		      /* We have found a backslash-newline inside a
			 variable or function reference.  Eat it and
			 any following whitespace.  */

		      int quoted = 0;
		      for (p = in - 1; p > ref && *p == '\\'; --p)
			quoted = !quoted;

		      if (quoted)
			/* There were two or more backslashes, so this is
			   not really a continuation line.  We don't collapse
			   the quoting backslashes here as is done in
			   collapse_continuations, because the line will
			   be collapsed again after expansion.  */
			*out++ = *in++;
		      else
			{
			  /* Skip the backslash, newline and
			     any following whitespace.  */
			  in = next_token (in + 2);

			  /* Discard any preceding whitespace that has
			     already been written to the output.  */
			  while (out > ref && isblank (out[-1]))
			    --out;

			  /* Replace it all with a single space.  */
			  *out++ = ' ';
			}
		    }
		  else
		    {
		      if (*in == openparen)
			++count;

		      *out++ = *in++;
		    }
		}
	    }
	}

      /* There are no more references in this line to worry about.
	 Copy the remaining uninteresting text to the output.  */
      if (out != in)
	strcpy (out, in);

      /* Finally, expand the line.  */
      lines[i] = allocated_variable_expand_for_file (cmds->command_lines[i],
						     file);
    }

  /* Start the command sequence, record it in a new
     `struct child', and add that to the chain.  */

  c = (struct child *) xmalloc (sizeof (struct child));
  bzero ((char *)c, sizeof (struct child));
  c->file = file;
  c->command_lines = lines;
  c->sh_batch_file = NULL;

  /* Fetch the first command line to be run.  */
  job_next_command (c);

  /* Wait for a job slot to be freed up.  If we allow an infinite number
     don't bother; also job_slots will == 0 if we're using the jobserver.  */

  if (job_slots != 0)
    while (job_slots_used == job_slots)
      reap_children (1, 0);

#ifdef MAKE_JOBSERVER
  /* If we are controlling multiple jobs make sure we have a token before
     starting the child. */

  /* This can be inefficient.  There's a decent chance that this job won't
     actually have to run any subprocesses: the command script may be empty
     or otherwise optimized away.  It would be nice if we could defer
     obtaining a token until just before we need it, in start_job_command.
     To do that we'd need to keep track of whether we'd already obtained a
     token (since start_job_command is called for each line of the job, not
     just once).  Also more thought needs to go into the entire algorithm;
     this is where the old parallel job code waits, so...  */

  else if (job_fds[0] >= 0)
    while (1)
      {
        char token;

        /* If we don't already have a job started, use our "free" token.  */
        if (!children)
          break;

        /* Read a token.  As long as there's no token available we'll block.
           If we get a SIGCHLD we'll return with EINTR.  If one happened
           before we got here we'll return immediately with EBADF because
           the signal handler closes the dup'd file descriptor.  */

        if (read (job_rfd, &token, 1) == 1)
          {
            DB (DB_JOBS, (_("Obtained token for child 0x%08lx (%s).\n"),
                          (unsigned long int) c, c->file->name));
            break;
          }

        if (errno != EINTR && errno != EBADF)
          pfatal_with_name (_("read jobs pipe"));

        /* Re-dup the read side of the pipe, so the signal handler can
           notify us if we miss a child.  */
        if (job_rfd < 0)
          job_rfd = dup (job_fds[0]);

        /* Something's done.  We don't want to block for a whole child,
           just reap whatever's there.  */
        reap_children (0, 0);
      }
#endif

  /* The job is now primed.  Start it running.
     (This will notice if there are in fact no commands.)  */
  (void) start_waiting_job (c);

  if (job_slots == 1 || not_parallel)
    /* Since there is only one job slot, make things run linearly.
       Wait for the child to die, setting the state to `cs_finished'.  */
    while (file->command_state == cs_running)
      reap_children (1, 0);

  return;
}

/* Move CHILD's pointers to the next command for it to execute.
   Returns nonzero if there is another command.  */

static int
job_next_command (child)
     struct child *child;
{
  while (child->command_ptr == 0 || *child->command_ptr == '\0')
    {
      /* There are no more lines in the expansion of this line.  */
      if (child->command_line == child->file->cmds->ncommand_lines)
	{
	  /* There are no more lines to be expanded.  */
	  child->command_ptr = 0;
	  return 0;
	}
      else
	/* Get the next line to run.  */
	child->command_ptr = child->command_lines[child->command_line++];
    }
  return 1;
}

static int
load_too_high ()
{
#if defined(__MSDOS__) || defined(VMS) || defined(_AMIGA)
  return 1;
#else
  double load;

  if (max_load_average < 0)
    return 0;

  make_access ();
  if (getloadavg (&load, 1) != 1)
    {
      static int lossage = -1;
      /* Complain only once for the same error.  */
      if (lossage == -1 || errno != lossage)
	{
	  if (errno == 0)
	    /* An errno value of zero means getloadavg is just unsupported.  */
	    error (NILF,
                   _("cannot enforce load limits on this operating system"));
	  else
	    perror_with_name (_("cannot enforce load limit: "), "getloadavg");
	}
      lossage = errno;
      load = 0;
    }
  user_access ();

  return load >= max_load_average;
#endif
}

/* Start jobs that are waiting for the load to be lower.  */

void
start_waiting_jobs ()
{
  struct child *job;

  if (waiting_jobs == 0)
    return;

  do
    {
      /* Check for recently deceased descendants.  */
      reap_children (0, 0);

      /* Take a job off the waiting list.  */
      job = waiting_jobs;
      waiting_jobs = job->next;

      /* Try to start that job.  We break out of the loop as soon
	 as start_waiting_job puts one back on the waiting list.  */
    }
  while (start_waiting_job (job) && waiting_jobs != 0);

  return;
}

#ifndef WINDOWS32
#ifdef VMS
#include <descrip.h>
#include <clidef.h>
#undef stderr
#define stderr stdout

/* This is called as an AST when a child process dies (it won't get
   interrupted by anything except a higher level AST).
*/
int vmsHandleChildTerm(struct child *child)
{
    int status;
    register struct child *lastc, *c;
    int child_failed;

    vms_jobsefnmask &= ~(1 << (child->efn - 32));

    lib$free_ef(&child->efn);

    (void) sigblock (fatal_signal_mask);

    child_failed = !(child->cstatus & 1 || ((child->cstatus & 7) == 0));

    /* Search for a child matching the deceased one.  */
    lastc = 0;
#if defined(RECURSIVEJOBS) /* I've had problems with recursive stuff and process handling */
    for (c = children; c != 0 && c != child; lastc = c, c = c->next);
#else
    c = child;
#endif

    if (child_failed && !c->noerror && !ignore_errors_flag)
      {
	/* The commands failed.  Write an error message,
	   delete non-precious targets, and abort.  */
	child_error (c->file->name, c->cstatus, 0, 0, 0);
	c->file->update_status = 1;
	delete_child_targets (c);
      }
    else
      {
	if (child_failed)
	  {
	    /* The commands failed, but we don't care.  */
	    child_error (c->file->name, c->cstatus, 0, 0, 1);
	    child_failed = 0;
	  }

#if defined(RECURSIVEJOBS) /* I've had problems with recursive stuff and process handling */
	/* If there are more commands to run, try to start them.  */
	start_job (c);

	switch (c->file->command_state)
	  {
	  case cs_running:
	    /* Successfully started.  */
	    break;

	  case cs_finished:
	    if (c->file->update_status != 0) {
		/* We failed to start the commands.  */
		delete_child_targets (c);
	    }
	    break;

	  default:
	    error (NILF, _("internal error: `%s' command_state"),
                   c->file->name);
	    abort ();
	    break;
	  }
#endif /* RECURSIVEJOBS */
      }

    /* Set the state flag to say the commands have finished.  */
    c->file->command_state = cs_finished;
    notice_finished_file (c->file);

#if defined(RECURSIVEJOBS) /* I've had problems with recursive stuff and process handling */
    /* Remove the child from the chain and free it.  */
    if (lastc == 0)
      children = c->next;
    else
      lastc->next = c->next;
    free_child (c);
#endif /* RECURSIVEJOBS */

    /* There is now another slot open.  */
    if (job_slots_used > 0)
      --job_slots_used;

    /* If the job failed, and the -k flag was not given, die.  */
    if (child_failed && !keep_going_flag)
      die (EXIT_FAILURE);

    (void) sigsetmask (sigblock (0) & ~(fatal_signal_mask));

    return 1;
}

/* VMS:
   Spawn a process executing the command in ARGV and return its pid. */

#define MAXCMDLEN 200

/* local helpers to make ctrl+c and ctrl+y working, see below */
#include <iodef.h>
#include <libclidef.h>
#include <ssdef.h>
#undef stderr
#define stderr stdout

static int ctrlMask= LIB$M_CLI_CTRLY;
static int oldCtrlMask;
static int setupYAstTried= 0;
static int pidToAbort= 0;
static int chan= 0;

static void reEnableAst(void) {
	lib$enable_ctrl (&oldCtrlMask,0);
}

static astHandler (void) {
	if (pidToAbort) {
		sys$forcex (&pidToAbort, 0, SS$_ABORT);
		pidToAbort= 0;
	}
	kill (getpid(),SIGQUIT);
}

static void tryToSetupYAst(void) {
	$DESCRIPTOR(inputDsc,"SYS$COMMAND");
	int	status;
	struct {
		short int	status, count;
		int	dvi;
	} iosb;

	setupYAstTried++;

	if (!chan) {
		status= sys$assign(&inputDsc,&chan,0,0);
		if (!(status&SS$_NORMAL)) {
			lib$signal(status);
			return;
		}
	}
	status= sys$qiow (0, chan, IO$_SETMODE|IO$M_CTRLYAST,&iosb,0,0,
		astHandler,0,0,0,0,0);
	if (status==SS$_ILLIOFUNC) {
		sys$dassgn(chan);
#ifdef	CTRLY_ENABLED_ANYWAY
		fprintf (stderr,
                         _("-warning, CTRL-Y will leave sub-process(es) around.\n"));
#else
		return;
#endif
	}
	if (status==SS$_NORMAL)
		status= iosb.status;
	if (!(status&SS$_NORMAL)) {
		lib$signal(status);
		return;
	}

	/* called from AST handler ? */
	if (setupYAstTried>1)
		return;
	if (atexit(reEnableAst))
		fprintf (stderr,
                         _("-warning, you may have to re-enable CTRL-Y handling from DCL.\n"));
	status= lib$disable_ctrl (&ctrlMask, &oldCtrlMask);
	if (!(status&SS$_NORMAL)) {
		lib$signal(status);
		return;
	}
}
int
child_execute_job (argv, child)
     char *argv;
     struct child *child;
{
  int i;
  static struct dsc$descriptor_s cmddsc;
  static struct dsc$descriptor_s pnamedsc;
  static struct dsc$descriptor_s ifiledsc;
  static struct dsc$descriptor_s ofiledsc;
  static struct dsc$descriptor_s efiledsc;
  int have_redirection = 0;
  int have_newline = 0;

  int spflags = CLI$M_NOWAIT;
  int status;
  char *cmd = alloca (strlen (argv) + 512), *p, *q;
  char ifile[256], ofile[256], efile[256];
  char *comname = 0;
  char procname[100];

  /* Parse IO redirection.  */

  ifile[0] = 0;
  ofile[0] = 0;
  efile[0] = 0;

  DB (DB_JOBS, ("child_execute_job (%s)\n", argv));

  while (isspace (*argv))
    argv++;

  if (*argv == 0)
    return 0;

  sprintf (procname, "GMAKE_%05x", getpid () & 0xfffff);
  pnamedsc.dsc$w_length = strlen(procname);
  pnamedsc.dsc$a_pointer = procname;
  pnamedsc.dsc$b_dtype = DSC$K_DTYPE_T;
  pnamedsc.dsc$b_class = DSC$K_CLASS_S;

  /* Handle comments and redirection. */
  for (p = argv, q = cmd; *p; p++, q++)
    {
      switch (*p)
	{
	  case '#':
	    *p-- = 0;
	    *q-- = 0;
	    break;
	  case '\\':
	    p++;
	    if (*p == '\n')
	      p++;
	    if (isspace (*p))
	      {
		do { p++; } while (isspace (*p));
		p--;
	      }
	    *q = *p;
	    break;
	  case '<':
	    p = vms_redirect (&ifiledsc, ifile, p);
	    *q = ' ';
	    have_redirection = 1;
	    break;
	  case '>':
	    have_redirection = 1;
	    if (*(p-1) == '2')
	      {
		q--;
		if (strncmp (p, ">&1", 3) == 0)
		  {
		    p += 3;
		    strcpy (efile, "sys$output");
		    efiledsc.dsc$w_length = strlen(efile);
		    efiledsc.dsc$a_pointer = efile;
		    efiledsc.dsc$b_dtype = DSC$K_DTYPE_T;
		    efiledsc.dsc$b_class = DSC$K_CLASS_S;
		  }
		else
		  {
		    p = vms_redirect (&efiledsc, efile, p);
		  }
	      }
	    else
	      {
		p = vms_redirect (&ofiledsc, ofile, p);
	      }
	    *q = ' ';
	    break;
	  case '\n':
	    have_newline = 1;
	  default:
	    *q = *p;
	    break;
	}
    }
  *q = *p;

  if (strncmp (cmd, "builtin_", 8) == 0)
    {
      child->pid = 270163;
      child->efn = 0;
      child->cstatus = 1;

      DB (DB_JOBS, (_("BUILTIN [%s][%s]\n"), cmd, cmd+8));

      p = cmd + 8;

      if ((*(p) == 'c')
	  && (*(p+1) == 'd')
	  && ((*(p+2) == ' ') || (*(p+2) == '\t')))
	{
	  p += 3;
	  while ((*p == ' ') || (*p == '\t'))
	    p++;
	  DB (DB_JOBS, (_("BUILTIN CD %s\n"), p));
	  if (chdir (p))
	    return 0;
	  else
	    return 1;
	}
      else if ((*(p) == 'r')
	  && (*(p+1) == 'm')
	  && ((*(p+2) == ' ') || (*(p+2) == '\t')))
	{
	  int in_arg;

	  /* rm  */
	  p += 3;
	  while ((*p == ' ') || (*p == '\t'))
	    p++;
	  in_arg = 1;

	  DB (DB_JOBS, (_("BUILTIN RM %s\n"), p));
	  while (*p)
	    {
	      switch (*p)
		{
		  case ' ':
		  case '\t':
		    if (in_arg)
		      {
			*p++ = ';';
			in_arg = 0;
		      }
		    break;
		  default:
		    break;
		}
	      p++;
	    }
	}
      else
	{
	  printf(_("Unknown builtin command '%s'\n"), cmd);
	  fflush(stdout);
	  return 0;
	}
    }

  /* Create a *.com file if either the command is too long for
     lib$spawn, or the command contains a newline, or if redirection
     is desired. Forcing commands with newlines into DCLs allows to
     store search lists on user mode logicals.  */

  if (strlen (cmd) > MAXCMDLEN
      || (have_redirection != 0)
      || (have_newline != 0))
    {
      FILE *outfile;
      char c;
      char *sep;
      int alevel = 0;	/* apostrophe level */

      if (strlen (cmd) == 0)
	{
	  printf (_("Error, empty command\n"));
	  fflush (stdout);
	  return 0;
	}

      outfile = open_tmpfile (&comname, "sys$scratch:CMDXXXXXX.COM");
      if (outfile == 0)
	pfatal_with_name (_("fopen (temporary file)"));

      if (ifile[0])
	{
	  fprintf (outfile, "$ assign/user %s sys$input\n", ifile);
          DB (DB_JOBS, (_("Redirected input from %s\n"), ifile));
	  ifiledsc.dsc$w_length = 0;
	}

      if (efile[0])
	{
	  fprintf (outfile, "$ define sys$error %s\n", efile);
          DB (DB_JOBS, (_("Redirected error to %s\n"), efile));
	  efiledsc.dsc$w_length = 0;
	}

      if (ofile[0])
	{
	  fprintf (outfile, "$ define sys$output %s\n", ofile);
	  DB (DB_JOBS, (_("Redirected output to %s\n"), ofile));
	  ofiledsc.dsc$w_length = 0;
	}

      p = sep = q = cmd;
      for (c = '\n'; c; c = *q++)
	{
	  switch (c)
	    {
            case '\n':
              /* At a newline, skip any whitespace around a leading $
                 from the command and issue exactly one $ into the DCL. */
              while (isspace (*p))
                p++;
              if (*p == '$')
                p++;
              while (isspace (*p))
                p++;
              fwrite (p, 1, q - p, outfile);
              fputc ('$', outfile);
              fputc (' ', outfile);
              /* Reset variables. */
              p = sep = q;
              break;

	      /* Nice places for line breaks are after strings, after
		 comma or space and before slash. */
            case '"':
              q = handle_apos (q + 1);
              sep = q;
              break;
            case ',':
            case ' ':
              sep = q;
              break;
            case '/':
            case '\0':
              sep = q - 1;
              break;
            default:
              break;
	    }
	  if (sep - p > 78)
	    {
	      /* Enough stuff for a line. */
	      fwrite (p, 1, sep - p, outfile);
	      p = sep;
	      if (*sep)
		{
		  /* The command continues.  */
		  fputc ('-', outfile);
		}
	      fputc ('\n', outfile);
	    }
  	}

      fwrite (p, 1, q - p, outfile);
      fputc ('\n', outfile);

      fclose (outfile);

      sprintf (cmd, "$ @%s", comname);

      DB (DB_JOBS, (_("Executing %s instead\n"), cmd));
    }

  cmddsc.dsc$w_length = strlen(cmd);
  cmddsc.dsc$a_pointer = cmd;
  cmddsc.dsc$b_dtype = DSC$K_DTYPE_T;
  cmddsc.dsc$b_class = DSC$K_CLASS_S;

  child->efn = 0;
  while (child->efn < 32 || child->efn > 63)
    {
      status = lib$get_ef ((unsigned long *)&child->efn);
      if (!(status & 1))
	return 0;
    }

  sys$clref (child->efn);

  vms_jobsefnmask |= (1 << (child->efn - 32));

/*
             LIB$SPAWN  [command-string]
			[,input-file]
			[,output-file]
			[,flags]
			[,process-name]
			[,process-id] [,completion-status-address] [,byte-integer-event-flag-num]
			[,AST-address] [,varying-AST-argument]
			[,prompt-string] [,cli] [,table]
*/

#ifndef DONTWAITFORCHILD
/*
 *	Code to make ctrl+c and ctrl+y working.
 *	The problem starts with the synchronous case where after lib$spawn is
 *	called any input will go to the child. But with input re-directed,
 *	both control characters won't make it to any of the programs, neither
 *	the spawning nor to the spawned one. Hence the caller needs to spawn
 *	with CLI$M_NOWAIT to NOT give up the input focus. A sys$waitfr
 *	has to follow to simulate the wanted synchronous behaviour.
 *	The next problem is ctrl+y which isn't caught by the crtl and
 *	therefore isn't converted to SIGQUIT (for a signal handler which is
 *	already established). The only way to catch ctrl+y, is an AST
 *	assigned to the input channel. But ctrl+y handling of DCL needs to be
 *	disabled, otherwise it will handle it. Not to mention the previous
 *	ctrl+y handling of DCL needs to be re-established before make exits.
 *	One more: At the time of LIB$SPAWN signals are blocked. SIGQUIT will
 *	make it to the signal handler after the child "normally" terminates.
 *	This isn't enough. It seems reasonable for simple command lines like
 *	a 'cc foobar.c' spawned in a subprocess but it is unacceptable for
 *	spawning make. Therefore we need to abort the process in the AST.
 *
 *	Prior to the spawn it is checked if an AST is already set up for
 *	ctrl+y, if not one is set up for a channel to SYS$COMMAND. In general
 *	this will work except if make is run in a batch environment, but there
 *	nobody can press ctrl+y. During the setup the DCL handling of ctrl+y
 *	is disabled and an exit handler is established to re-enable it.
 *	If the user interrupts with ctrl+y, the assigned AST will fire, force
 *	an abort to the subprocess and signal SIGQUIT, which will be caught by
 *	the already established handler and will bring us back to common code.
 *	After the spawn (now /nowait) a sys$waitfr simulates the /wait and
 *	enables the ctrl+y be delivered to this code. And the ctrl+c too,
 *	which the crtl converts to SIGINT and which is caught by the common
 *	signal handler. Because signals were blocked before entering this code
 *	sys$waitfr will always complete and the SIGQUIT will be processed after
 *	it (after termination of the current block, somewhere in common code).
 *	And SIGINT too will be delayed. That is ctrl+c can only abort when the
 *	current command completes. Anyway it's better than nothing :-)
 */

  if (!setupYAstTried)
    tryToSetupYAst();
  status = lib$spawn (&cmddsc,					/* cmd-string  */
		      (ifiledsc.dsc$w_length == 0)?0:&ifiledsc, /* input-file  */
		      (ofiledsc.dsc$w_length == 0)?0:&ofiledsc, /* output-file */
		      &spflags,					/* flags  */
		      &pnamedsc,				/* proc name  */
		      &child->pid, &child->cstatus, &child->efn,
		      0, 0,
		      0, 0, 0);
  pidToAbort= child->pid;
  status= sys$waitfr (child->efn);
  pidToAbort= 0;
  vmsHandleChildTerm(child);
#else
  status = lib$spawn (&cmddsc,
		      (ifiledsc.dsc$w_length == 0)?0:&ifiledsc,
		      (ofiledsc.dsc$w_length == 0)?0:&ofiledsc,
		      &spflags,
		      &pnamedsc,
		      &child->pid, &child->cstatus, &child->efn,
		      vmsHandleChildTerm, child,
		      0, 0, 0);
#endif

  if (!(status & 1))
    {
      printf (_("Error spawning, %d\n") ,status);
      fflush (stdout);
    }

  if (comname && !ISDB (DB_JOBS))
    unlink (comname);

  return (status & 1);
}

#else /* !VMS */

#if !defined (_AMIGA) && !defined (__MSDOS__)
/* UNIX:
   Replace the current process with one executing the command in ARGV.
   STDIN_FD and STDOUT_FD are used as the process's stdin and stdout; ENVP is
   the environment of the new program.  This function does not return.  */

void
child_execute_job (stdin_fd, stdout_fd, argv, envp)
     int stdin_fd, stdout_fd;
     char **argv, **envp;
{
  if (stdin_fd != 0)
    (void) dup2 (stdin_fd, 0);
  if (stdout_fd != 1)
    (void) dup2 (stdout_fd, 1);
  if (stdin_fd != 0)
    (void) close (stdin_fd);
  if (stdout_fd != 1)
    (void) close (stdout_fd);

  /* Run the command.  */
  exec_command (argv, envp);
}
#endif /* !AMIGA && !__MSDOS__ */
#endif /* !VMS */
#endif /* !WINDOWS32 */

#ifndef _AMIGA
/* Replace the current process with one running the command in ARGV,
   with environment ENVP.  This function does not return.  */

void
exec_command (argv, envp)
     char **argv, **envp;
{
#ifdef VMS
  /* to work around a problem with signals and execve: ignore them */
#ifdef SIGCHLD
  signal (SIGCHLD,SIG_IGN);
#endif
  /* Run the program.  */
  execve (argv[0], argv, envp);
  perror_with_name ("execve: ", argv[0]);
  _exit (EXIT_FAILURE);
#else
#ifdef WINDOWS32
  HANDLE hPID;
  HANDLE hWaitPID;
  int err = 0;
  int exit_code = EXIT_FAILURE;

  /* make sure CreateProcess() has Path it needs */
  sync_Path_environment();

  /* launch command */
  hPID = process_easy(argv, envp);

  /* make sure launch ok */
  if (hPID == INVALID_HANDLE_VALUE)
    {
      int i;
      fprintf(stderr,
              _("process_easy() failed failed to launch process (e=%d)\n"),
              process_last_err(hPID));
      for (i = 0; argv[i]; i++)
          fprintf(stderr, "%s ", argv[i]);
      fprintf(stderr, _("\nCounted %d args in failed launch\n"), i);
      exit(EXIT_FAILURE);
    }

  /* wait and reap last child */
  while (hWaitPID = process_wait_for_any())
    {
      /* was an error found on this process? */
      err = process_last_err(hWaitPID);

      /* get exit data */
      exit_code = process_exit_code(hWaitPID);

      if (err)
          fprintf(stderr, "make (e=%d, rc=%d): %s",
                  err, exit_code, map_windows32_error_to_string(err));

      /* cleanup process */
      process_cleanup(hWaitPID);

      /* expect to find only last pid, warn about other pids reaped */
      if (hWaitPID == hPID)
          break;
      else
          fprintf(stderr,
                  _("make reaped child pid %d, still waiting for pid %d\n"),
                  hWaitPID, hPID);
    }

  /* return child's exit code as our exit code */
  exit(exit_code);

#else  /* !WINDOWS32 */

  /* Be the user, permanently.  */
  child_access ();

  /* Run the program.  */
  environ = envp;
  execvp (argv[0], argv);

  switch (errno)
    {
    case ENOENT:
      error (NILF, _("%s: Command not found"), argv[0]);
      break;
    case ENOEXEC:
      {
	/* The file is not executable.  Try it as a shell script.  */
	extern char *getenv ();
	char *shell;
	char **new_argv;
	int argc;

	shell = getenv ("SHELL");
	if (shell == 0)
	  shell = default_shell;

	argc = 1;
	while (argv[argc] != 0)
	  ++argc;

	new_argv = (char **) alloca ((1 + argc + 1) * sizeof (char *));
	new_argv[0] = shell;
	new_argv[1] = argv[0];
	while (argc > 0)
	  {
	    new_argv[1 + argc] = argv[argc];
	    --argc;
	  }

	execvp (shell, new_argv);
	if (errno == ENOENT)
	  error (NILF, _("%s: Shell program not found"), shell);
	else
	  perror_with_name ("execvp: ", shell);
	break;
      }

    default:
      perror_with_name ("execvp: ", argv[0]);
      break;
    }

  _exit (127);
#endif /* !WINDOWS32 */
#endif /* !VMS */
}
#else /* On Amiga */
void exec_command (argv)
     char **argv;
{
  MyExecute (argv);
}

void clean_tmp (void)
{
  DeleteFile (amiga_bname);
}

#endif /* On Amiga */

#ifndef VMS
/* Figure out the argument list necessary to run LINE as a command.  Try to
   avoid using a shell.  This routine handles only ' quoting, and " quoting
   when no backslash, $ or ` characters are seen in the quotes.  Starting
   quotes may be escaped with a backslash.  If any of the characters in
   sh_chars[] is seen, or any of the builtin commands listed in sh_cmds[]
   is the first word of a line, the shell is used.

   If RESTP is not NULL, *RESTP is set to point to the first newline in LINE.
   If *RESTP is NULL, newlines will be ignored.

   SHELL is the shell to use, or nil to use the default shell.
   IFS is the value of $IFS, or nil (meaning the default).  */

static char **
construct_command_argv_internal (line, restp, shell, ifs, batch_filename_ptr)
     char *line, **restp;
     char *shell, *ifs;
     char **batch_filename_ptr;
{
#ifdef __MSDOS__
  /* MSDOS supports both the stock DOS shell and ports of Unixy shells.
     We call `system' for anything that requires ``slow'' processing,
     because DOS shells are too dumb.  When $SHELL points to a real
     (unix-style) shell, `system' just calls it to do everything.  When
     $SHELL points to a DOS shell, `system' does most of the work
     internally, calling the shell only for its internal commands.
     However, it looks on the $PATH first, so you can e.g. have an
     external command named `mkdir'.

     Since we call `system', certain characters and commands below are
     actually not specific to COMMAND.COM, but to the DJGPP implementation
     of `system'.  In particular:

       The shell wildcard characters are in DOS_CHARS because they will
       not be expanded if we call the child via `spawnXX'.

       The `;' is in DOS_CHARS, because our `system' knows how to run
       multiple commands on a single line.

       DOS_CHARS also include characters special to 4DOS/NDOS, so we
       won't have to tell one from another and have one more set of
       commands and special characters.  */
  static char sh_chars_dos[] = "*?[];|<>%^&()";
  static char *sh_cmds_dos[] = { "break", "call", "cd", "chcp", "chdir", "cls",
				 "copy", "ctty", "date", "del", "dir", "echo",
				 "erase", "exit", "for", "goto", "if", "md",
				 "mkdir", "path", "pause", "prompt", "rd",
				 "rmdir", "rem", "ren", "rename", "set",
				 "shift", "time", "type", "ver", "verify",
				 "vol", ":", 0 };

  static char sh_chars_sh[]  = "#;\"*?[]&|<>(){}$`^";
  static char *sh_cmds_sh[]  = { "cd", "echo", "eval", "exec", "exit", "login",
				 "logout", "set", "umask", "wait", "while",
				 "for", "case", "if", ":", ".", "break",
				 "continue", "export", "read", "readonly",
				 "shift", "times", "trap", "switch", "unset",
                                 0 };

  char *sh_chars;
  char **sh_cmds;
#else
#ifdef _AMIGA
  static char sh_chars[] = "#;\"|<>()?*$`";
  static char *sh_cmds[] = { "cd", "eval", "if", "delete", "echo", "copy",
			     "rename", "set", "setenv", "date", "makedir",
			     "skip", "else", "endif", "path", "prompt",
			     "unset", "unsetenv", "version",
			     0 };
#else
#ifdef WINDOWS32
  static char sh_chars_dos[] = "\"|&<>";
  static char *sh_cmds_dos[] = { "break", "call", "cd", "chcp", "chdir", "cls",
			     "copy", "ctty", "date", "del", "dir", "echo",
			     "erase", "exit", "for", "goto", "if", "if", "md",
			     "mkdir", "path", "pause", "prompt", "rem", "ren",
			     "rename", "set", "shift", "time", "type",
			     "ver", "verify", "vol", ":", 0 };
  static char sh_chars_sh[] = "#;\"*?[]&|<>(){}$`^";
  static char *sh_cmds_sh[] = { "cd", "eval", "exec", "exit", "login",
			     "logout", "set", "umask", "wait", "while", "for",
			     "case", "if", ":", ".", "break", "continue",
			     "export", "read", "readonly", "shift", "times",
			     "trap", "switch", "test",
#ifdef BATCH_MODE_ONLY_SHELL
                 "echo",
#endif
                 0 };
  char*  sh_chars;
  char** sh_cmds;
#else  /* WINDOWS32 */
  static char sh_chars[] = "#;\"*?[]&|<>(){}$`^";
  static char *sh_cmds[] = { "cd", "eval", "exec", "exit", "login",
			     "logout", "set", "umask", "wait", "while", "for",
			     "case", "if", ":", ".", "break", "continue",
			     "export", "read", "readonly", "shift", "times",
			     "trap", "switch", 0 };
#endif /* WINDOWS32 */
#endif /* Amiga */
#endif /* __MSDOS__ */
  register int i;
  register char *p;
  register char *ap;
  char *end;
  int instring, word_has_equals, seen_nonequals, last_argument_was_empty;
  char **new_argv = 0;
#ifdef WINDOWS32
  int slow_flag = 0;

  if (no_default_sh_exe) {
    sh_cmds = sh_cmds_dos;
    sh_chars = sh_chars_dos;
  } else {
    sh_cmds = sh_cmds_sh;
    sh_chars = sh_chars_sh;
  }
#endif /* WINDOWS32 */

  if (restp != NULL)
    *restp = NULL;

  /* Make sure not to bother processing an empty line.  */
  while (isblank (*line))
    ++line;
  if (*line == '\0')
    return 0;

  /* See if it is safe to parse commands internally.  */
  if (shell == 0)
    shell = default_shell;
#ifdef WINDOWS32
  else if (strcmp (shell, default_shell))
  {
    char *s1 = _fullpath(NULL, shell, 0);
    char *s2 = _fullpath(NULL, default_shell, 0);

    slow_flag = strcmp((s1 ? s1 : ""), (s2 ? s2 : ""));

    if (s1)
      free (s1);
    if (s2)
      free (s2);
  }
  if (slow_flag)
    goto slow;
#else  /* not WINDOWS32 */
#ifdef __MSDOS__
  else if (stricmp (shell, default_shell))
    {
      extern int _is_unixy_shell (const char *_path);

      message (1, _("$SHELL changed (was `%s', now `%s')"), default_shell, shell);
      unixy_shell = _is_unixy_shell (shell);
      default_shell = shell;
    }
  if (unixy_shell)
    {
      sh_chars = sh_chars_sh;
      sh_cmds  = sh_cmds_sh;
    }
  else
    {
      sh_chars = sh_chars_dos;
      sh_cmds  = sh_cmds_dos;
    }
#else  /* not __MSDOS__ */
  else if (strcmp (shell, default_shell))
    goto slow;
#endif /* not __MSDOS__ */
#endif /* not WINDOWS32 */

  if (ifs != 0)
    for (ap = ifs; *ap != '\0'; ++ap)
      if (*ap != ' ' && *ap != '\t' && *ap != '\n')
	goto slow;

  i = strlen (line) + 1;

  /* More than 1 arg per character is impossible.  */
  new_argv = (char **) xmalloc (i * sizeof (char *));

  /* All the args can fit in a buffer as big as LINE is.   */
  ap = new_argv[0] = (char *) xmalloc (i);
  end = ap + i;

  /* I is how many complete arguments have been found.  */
  i = 0;
  instring = word_has_equals = seen_nonequals = last_argument_was_empty = 0;
  for (p = line; *p != '\0'; ++p)
    {
      if (ap > end)
	abort ();

      if (instring)
	{
	string_char:
	  /* Inside a string, just copy any char except a closing quote
	     or a backslash-newline combination.  */
	  if (*p == instring)
	    {
	      instring = 0;
	      if (ap == new_argv[0] || *(ap-1) == '\0')
		last_argument_was_empty = 1;
	    }
	  else if (*p == '\\' && p[1] == '\n')
	    goto swallow_escaped_newline;
	  else if (*p == '\n' && restp != NULL)
	    {
	      /* End of the command line.  */
	      *restp = p;
	      goto end_of_line;
	    }
	  /* Backslash, $, and ` are special inside double quotes.
	     If we see any of those, punt.
	     But on MSDOS, if we use COMMAND.COM, double and single
	     quotes have the same effect.  */
	  else if (instring == '"' && strchr ("\\$`", *p) != 0 && unixy_shell)
	    goto slow;
	  else
	    *ap++ = *p;
	}
      else if (strchr (sh_chars, *p) != 0)
	/* Not inside a string, but it's a special char.  */
	goto slow;
#ifdef  __MSDOS__
      else if (*p == '.' && p[1] == '.' && p[2] == '.' && p[3] != '.')
	/* `...' is a wildcard in DJGPP.  */
	goto slow;
#endif
      else
	/* Not a special char.  */
	switch (*p)
	  {
	  case '=':
	    /* Equals is a special character in leading words before the
	       first word with no equals sign in it.  This is not the case
	       with sh -k, but we never get here when using nonstandard
	       shell flags.  */
	    if (! seen_nonequals && unixy_shell)
	      goto slow;
	    word_has_equals = 1;
	    *ap++ = '=';
	    break;

	  case '\\':
	    /* Backslash-newline combinations are eaten.  */
	    if (p[1] == '\n')
	      {
	      swallow_escaped_newline:

		/* Eat the backslash, the newline, and following whitespace,
		   replacing it all with a single space.  */
		p += 2;

		/* If there is a tab after a backslash-newline,
		   remove it from the source line which will be echoed,
		   since it was most likely used to line
		   up the continued line with the previous one.  */
		if (*p == '\t')
                  /* Note these overlap and strcpy() is undefined for
                     overlapping objects in ANSI C.  The strlen() _IS_ right,
                     since we need to copy the nul byte too.  */
		  bcopy (p + 1, p, strlen (p));

		if (instring)
		  goto string_char;
		else
		  {
		    if (ap != new_argv[i])
		      /* Treat this as a space, ending the arg.
			 But if it's at the beginning of the arg, it should
			 just get eaten, rather than becoming an empty arg. */
		      goto end_of_arg;
		    else
		      p = next_token (p) - 1;
		  }
	      }
	    else if (p[1] != '\0')
              {
#if defined(__MSDOS__) || defined(WINDOWS32)
                /* Only remove backslashes before characters special
                   to Unixy shells.  All other backslashes are copied
                   verbatim, since they are probably DOS-style
                   directory separators.  This still leaves a small
                   window for problems, but at least it should work
                   for the vast majority of naive users.  */

#ifdef __MSDOS__
                /* A dot is only special as part of the "..."
                   wildcard.  */
                if (strneq (p + 1, ".\\.\\.", 5))
                  {
                    *ap++ = '.';
                    *ap++ = '.';
                    p += 4;
                  }
                else
#endif
                  if (p[1] != '\\' && p[1] != '\''
                      && !isspace ((unsigned char)p[1])
                      && (strchr (sh_chars_sh, p[1]) == 0))
                    /* back up one notch, to copy the backslash */
                    --p;

#endif  /* __MSDOS__ || WINDOWS32 */
                /* Copy and skip the following char.  */
                *ap++ = *++p;
              }
	    break;

	  case '\'':
	  case '"':
	    instring = *p;
	    break;

	  case '\n':
	    if (restp != NULL)
	      {
		/* End of the command line.  */
		*restp = p;
		goto end_of_line;
	      }
	    else
	      /* Newlines are not special.  */
	      *ap++ = '\n';
	    break;

	  case ' ':
	  case '\t':
	  end_of_arg:
	    /* We have the end of an argument.
	       Terminate the text of the argument.  */
	    *ap++ = '\0';
	    new_argv[++i] = ap;
	    last_argument_was_empty = 0;

	    /* Update SEEN_NONEQUALS, which tells us if every word
	       heretofore has contained an `='.  */
	    seen_nonequals |= ! word_has_equals;
	    if (word_has_equals && ! seen_nonequals)
	      /* An `=' in a word before the first
		 word without one is magical.  */
	      goto slow;
	    word_has_equals = 0; /* Prepare for the next word.  */

	    /* If this argument is the command name,
	       see if it is a built-in shell command.
	       If so, have the shell handle it.  */
	    if (i == 1)
	      {
		register int j;
		for (j = 0; sh_cmds[j] != 0; ++j)
		  if (streq (sh_cmds[j], new_argv[0]))
		    goto slow;
	      }

	    /* Ignore multiple whitespace chars.  */
	    p = next_token (p);
	    /* Next iteration should examine the first nonwhite char.  */
	    --p;
	    break;

	  default:
	    *ap++ = *p;
	    break;
	  }
    }
 end_of_line:

  if (instring)
    /* Let the shell deal with an unterminated quote.  */
    goto slow;

  /* Terminate the last argument and the argument list.  */

  *ap = '\0';
  if (new_argv[i][0] != '\0' || last_argument_was_empty)
    ++i;
  new_argv[i] = 0;

  if (i == 1)
    {
      register int j;
      for (j = 0; sh_cmds[j] != 0; ++j)
	if (streq (sh_cmds[j], new_argv[0]))
	  goto slow;
    }

  if (new_argv[0] == 0)
    /* Line was empty.  */
    return 0;
  else
    return new_argv;

 slow:;
  /* We must use the shell.  */

  if (new_argv != 0)
    {
      /* Free the old argument list we were working on.  */
      free (new_argv[0]);
      free ((void *)new_argv);
    }

#ifdef __MSDOS__
  execute_by_shell = 1;	/* actually, call `system' if shell isn't unixy */
#endif

#ifdef _AMIGA
  {
    char *ptr;
    char *buffer;
    char *dptr;

    buffer = (char *)xmalloc (strlen (line)+1);

    ptr = line;
    for (dptr=buffer; *ptr; )
    {
      if (*ptr == '\\' && ptr[1] == '\n')
	ptr += 2;
      else if (*ptr == '@') /* Kludge: multiline commands */
      {
	ptr += 2;
	*dptr++ = '\n';
      }
      else
	*dptr++ = *ptr++;
    }
    *dptr = 0;

    new_argv = (char **) xmalloc (2 * sizeof (char *));
    new_argv[0] = buffer;
    new_argv[1] = 0;
  }
#else	/* Not Amiga  */
#ifdef WINDOWS32
  /*
   * Not eating this whitespace caused things like
   *
   *    sh -c "\n"
   *
   * which gave the shell fits. I think we have to eat
   * whitespace here, but this code should be considered
   * suspicious if things start failing....
   */

  /* Make sure not to bother processing an empty line.  */
  while (isspace ((unsigned char)*line))
    ++line;
  if (*line == '\0')
    return 0;
#endif /* WINDOWS32 */
  {
    /* SHELL may be a multi-word command.  Construct a command line
       "SHELL -c LINE", with all special chars in LINE escaped.
       Then recurse, expanding this command line to get the final
       argument list.  */

    unsigned int shell_len = strlen (shell);
#ifndef VMS
    static char minus_c[] = " -c ";
#else
    static char minus_c[] = "";
#endif
    unsigned int line_len = strlen (line);

    char *new_line = (char *) alloca (shell_len + (sizeof (minus_c) - 1)
				      + (line_len * 2) + 1);
    char *command_ptr = NULL; /* used for batch_mode_shell mode */

    ap = new_line;
    bcopy (shell, ap, shell_len);
    ap += shell_len;
    bcopy (minus_c, ap, sizeof (minus_c) - 1);
    ap += sizeof (minus_c) - 1;
    command_ptr = ap;
    for (p = line; *p != '\0'; ++p)
      {
	if (restp != NULL && *p == '\n')
	  {
	    *restp = p;
	    break;
	  }
	else if (*p == '\\' && p[1] == '\n')
	  {
	    /* Eat the backslash, the newline, and following whitespace,
	       replacing it all with a single space (which is escaped
	       from the shell).  */
	    p += 2;

	    /* If there is a tab after a backslash-newline,
	       remove it from the source line which will be echoed,
	       since it was most likely used to line
	       up the continued line with the previous one.  */
	    if (*p == '\t')
	      bcopy (p + 1, p, strlen (p));

	    p = next_token (p);
	    --p;
            if (unixy_shell && !batch_mode_shell)
              *ap++ = '\\';
	    *ap++ = ' ';
	    continue;
	  }

        /* DOS shells don't know about backslash-escaping.  */
	if (unixy_shell && !batch_mode_shell &&
            (*p == '\\' || *p == '\'' || *p == '"'
             || isspace ((unsigned char)*p)
             || strchr (sh_chars, *p) != 0))
	  *ap++ = '\\';
#ifdef __MSDOS__
        else if (unixy_shell && strneq (p, "...", 3))
          {
            /* The case of `...' wildcard again.  */
            strcpy (ap, "\\.\\.\\");
            ap += 5;
            p  += 2;
          }
#endif
	*ap++ = *p;
      }
    if (ap == new_line + shell_len + sizeof (minus_c) - 1)
      /* Line was empty.  */
      return 0;
    *ap = '\0';

#ifdef WINDOWS32
    /* Some shells do not work well when invoked as 'sh -c xxx' to run a
       command line (e.g. Cygnus GNUWIN32 sh.exe on WIN32 systems).  In these
       cases, run commands via a script file.  */
    if ((no_default_sh_exe || batch_mode_shell) && batch_filename_ptr) {
      FILE* batch = NULL;
      int id = GetCurrentProcessId();
      PATH_VAR(fbuf);
      char* fname = NULL;

      /* create a file name */
      sprintf(fbuf, "make%d", id);
      fname = tempnam(".", fbuf);

	  /* create batch file name */
      *batch_filename_ptr = xmalloc(strlen(fname) + 5);
      strcpy(*batch_filename_ptr, fname);

      /* make sure path name is in DOS backslash format */
      if (!unixy_shell) {
        fname = *batch_filename_ptr;
        for (i = 0; fname[i] != '\0'; ++i)
          if (fname[i] == '/')
            fname[i] = '\\';
        strcat(*batch_filename_ptr, ".bat");
      } else {
        strcat(*batch_filename_ptr, ".sh");
      }

      DB (DB_JOBS, (_("Creating temporary batch file %s\n"),
                    *batch_filename_ptr));

      /* create batch file to execute command */
      batch = fopen (*batch_filename_ptr, "w");
      if (!unixy_shell)
        fputs ("@echo off\n", batch);
      fputs (command_ptr, batch);
      fputc ('\n', batch);
      fclose (batch);

      /* create argv */
      new_argv = (char **) xmalloc(3 * sizeof (char *));
      if (unixy_shell) {
        new_argv[0] = xstrdup (shell);
        new_argv[1] = *batch_filename_ptr; /* only argv[0] gets freed later */
      } else {
        new_argv[0] = xstrdup (*batch_filename_ptr);
        new_argv[1] = NULL;
      }
      new_argv[2] = NULL;
    } else
#endif /* WINDOWS32 */
    if (unixy_shell)
      new_argv = construct_command_argv_internal (new_line, (char **) NULL,
                                                  (char *) 0, (char *) 0,
                                                  (char **) 0);
#ifdef  __MSDOS__
    else
      {
      /* With MSDOS shells, we must construct the command line here
         instead of recursively calling ourselves, because we
         cannot backslash-escape the special characters (see above).  */
      new_argv = (char **) xmalloc (sizeof (char *));
      line_len = strlen (new_line) - shell_len - sizeof (minus_c) + 1;
      new_argv[0] = xmalloc (line_len + 1);
      strncpy (new_argv[0],
               new_line + shell_len + sizeof (minus_c) - 1, line_len);
      new_argv[0][line_len] = '\0';
      }
#else
    else
      fatal (NILF, _("%s (line %d) Bad shell context (!unixy && !batch_mode_shell)\n"),
            __FILE__, __LINE__);
#endif
  }
#endif	/* ! AMIGA */

  return new_argv;
}
#endif /* !VMS */

/* Figure out the argument list necessary to run LINE as a command.  Try to
   avoid using a shell.  This routine handles only ' quoting, and " quoting
   when no backslash, $ or ` characters are seen in the quotes.  Starting
   quotes may be escaped with a backslash.  If any of the characters in
   sh_chars[] is seen, or any of the builtin commands listed in sh_cmds[]
   is the first word of a line, the shell is used.

   If RESTP is not NULL, *RESTP is set to point to the first newline in LINE.
   If *RESTP is NULL, newlines will be ignored.

   FILE is the target whose commands these are.  It is used for
   variable expansion for $(SHELL) and $(IFS).  */

char **
construct_command_argv (line, restp, file, batch_filename_ptr)
     char *line, **restp;
     struct file *file;
     char** batch_filename_ptr;
{
  char *shell, *ifs;
  char **argv;

#ifdef VMS
  char *cptr;
  int argc;

  argc = 0;
  cptr = line;
  for (;;)
    {
      while ((*cptr != 0)
	     && (isspace (*cptr)))
	cptr++;
      if (*cptr == 0)
	break;
      while ((*cptr != 0)
	     && (!isspace(*cptr)))
	cptr++;
      argc++;
    }

  argv = (char **)malloc (argc * sizeof (char *));
  if (argv == 0)
    abort ();

  cptr = line;
  argc = 0;
  for (;;)
    {
      while ((*cptr != 0)
	     && (isspace (*cptr)))
	cptr++;
      if (*cptr == 0)
	break;
      DB (DB_JOBS, ("argv[%d] = [%s]\n", argc, cptr));
      argv[argc++] = cptr;
      while ((*cptr != 0)
	     && (!isspace(*cptr)))
	cptr++;
      if (*cptr != 0)
	*cptr++ = 0;
    }
#else
  {
    /* Turn off --warn-undefined-variables while we expand SHELL and IFS.  */
    int save = warn_undefined_variables_flag;
    warn_undefined_variables_flag = 0;

    shell = allocated_variable_expand_for_file ("$(SHELL)", file);
#ifdef WINDOWS32
    /*
     * Convert to forward slashes so that construct_command_argv_internal()
     * is not confused.
     */
    if (shell) {
      char *p = w32ify(shell, 0);
      strcpy(shell, p);
    }
#endif
    ifs = allocated_variable_expand_for_file ("$(IFS)", file);

    warn_undefined_variables_flag = save;
  }

  argv = construct_command_argv_internal (line, restp, shell, ifs, batch_filename_ptr);

  free (shell);
  free (ifs);
#endif /* !VMS */
  return argv;
}

#if !defined(HAVE_DUP2) && !defined(_AMIGA)
int
dup2 (old, new)
     int old, new;
{
  int fd;

  (void) close (new);
  fd = dup (old);
  if (fd != new)
    {
      (void) close (fd);
      errno = EMFILE;
      return -1;
    }

  return fd;
}
#endif /* !HAPE_DUP2 && !_AMIGA */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         main.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Argument parsing and main program of GNU Make.
Copyright (C) 1988,89,90,91,94,95,96,97,98,99 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"
#include "getopt.h"

#include <assert.h>
#undef stderr
#define stderr stdout
#ifdef _AMIGA
# include <dos/dos.h>
# include <proto/dos.h>
#undef stderr
#define stderr stdout
#endif
#ifdef WINDOWS32
#include <windows.h>
#include "pathstuff.h"
#undef stderr
#define stderr stdout
#endif
#if defined(MAKE_JOBSERVER) && defined(HAVE_FCNTL_H)
# include <fcntl.h>
#undef stderr
#define stderr stdout
#endif

#ifdef _AMIGA
int __stack = 20000; /* Make sure we have 20K of stack space */
#endif

extern void init_dir PARAMS ((void));
extern void remote_setup PARAMS ((void));
extern void remote_cleanup PARAMS ((void));
/* extern RETSIGTYPE fatal_error_signal PARAMS ((int sig)); */

extern void print_variable_data_base PARAMS ((void));
/* extern void print_dir_data_base PARAMS ((void)); */
extern void print_rule_data_base PARAMS ((void));
/* extern void print_file_data_base PARAMS ((void)); */
extern void print_vpath_data_base PARAMS ((void));

#if defined HAVE_WAITPID || defined HAVE_WAIT3
# define HAVE_WAIT_NOHANG
#endif

#ifndef	HAVE_UNISTD_H
extern int chdir ();
#endif
#ifndef	STDC_HEADERS
# ifndef sun			/* Sun has an incorrect decl in a header.  */
extern void exit PARAMS ((int)) __attribute__ ((noreturn));
# endif
extern double atof ();
#endif

static void print_data_base PARAMS ((void));
static void print_version PARAMS ((void));
static void decode_switches PARAMS ((int argc, char **argv, int env));
static void decode_env_switches PARAMS ((char *envar, unsigned int len));
static void define_makeflags PARAMS ((int all, int makefile));
static char *quote_for_env PARAMS ((char *out, char *in));

/* The structure that describes an accepted command switch.  */

struct command_switch
  {
    int c;			/* The switch character.  */

    enum			/* Type of the value.  */
      {
	flag,			/* Turn int flag on.  */
	flag_off,		/* Turn int flag off.  */
	string,			/* One string per switch.  */
	positive_int,		/* A positive integer.  */
	floating,		/* A floating-point number (double).  */
	ignore			/* Ignored.  */
      } type;

    char *value_ptr;	/* Pointer to the value-holding variable.  */

    unsigned int env:1;		/* Can come from MAKEFLAGS.  */
    unsigned int toenv:1;	/* Should be put in MAKEFLAGS.  */
    unsigned int no_makefile:1;	/* Don't propagate when remaking makefiles.  */

    char *noarg_value;	/* Pointer to value used if no argument is given.  */
    char *default_value;/* Pointer to default value.  */

    char *long_name;		/* Long option name.  */
    char *argdesc;		/* Descriptive word for argument.  */
    char *description;		/* Description for usage message.  */
                                /* 0 means internal; don't display help.  */
  };

/* True if C is a switch value that corresponds to a short option.  */

#define short_option(c) ((c) <= CHAR_MAX)

/* The structure used to hold the list of strings given
   in command switches of a type that takes string arguments.  */

struct stringlist
  {
    char **list;	/* Nil-terminated list of strings.  */
    unsigned int idx;	/* Index into above.  */
    unsigned int max;	/* Number of pointers allocated.  */
  };


/* The recognized command switches.  */

/* Nonzero means do not print commands to be executed (-s).  */

int silent_flag;

/* Nonzero means just touch the files
   that would appear to need remaking (-t)  */

int touch_flag;

/* Nonzero means just print what commands would need to be executed,
   don't actually execute them (-n).  */

int just_print_flag;

/* Print debugging info (--debug).  */

static struct stringlist *db_flags;
static int debug_flag = 0;

int db_level = 0;

#ifdef WINDOWS32
/* Suspend make in main for a short time to allow debugger to attach */

int suspend_flag = 0;
#endif

/* Environment variables override makefile definitions.  */

int env_overrides = 0;

/* Nonzero means ignore status codes returned by commands
   executed to remake files.  Just treat them all as successful (-i).  */

int ignore_errors_flag = 0;

/* Nonzero means don't remake anything, just print the data base
   that results from reading the makefile (-p).  */

int print_data_base_flag = 0;

/* Nonzero means don't remake anything; just return a nonzero status
   if the specified targets are not up to date (-q).  */

int question_flag = 0;

/* Nonzero means do not use any of the builtin rules (-r) / variables (-R).  */

int no_builtin_rules_flag = 0;
int no_builtin_variables_flag = 0;

/* Nonzero means keep going even if remaking some file fails (-k).  */

int keep_going_flag;
int default_keep_going_flag = 0;

/* Nonzero means print directory before starting and when done (-w).  */

int print_directory_flag = 0;

/* Nonzero means ignore print_directory_flag and never print the directory.
   This is necessary because print_directory_flag is set implicitly.  */

int inhibit_print_directory_flag = 0;

/* Nonzero means print version information.  */

int print_version_flag = 0;

/* List of makefiles given with -f switches.  */

static struct stringlist *makefiles = 0;

/* Number of job slots (commands that can be run at once).  */

unsigned int job_slots = 1;
unsigned int default_job_slots = 1;

/* Value of job_slots that means no limit.  */

static unsigned int inf_jobs = 0;

/* File descriptors for the jobs pipe.  */

static struct stringlist *jobserver_fds = 0;

int job_fds[2] = { -1, -1 };
int job_rfd = -1;

/* Maximum load average at which multiple jobs will be run.
   Negative values mean unlimited, while zero means limit to
   zero load (which could be useful to start infinite jobs remotely
   but one at a time locally).  */
#ifndef NO_FLOAT
double max_load_average = -1.0;
double default_load_average = -1.0;
#else
int max_load_average = -1;
int default_load_average = -1;
#endif

/* List of directories given with -C switches.  */

static struct stringlist *directories = 0;

/* List of include directories given with -I switches.  */

static struct stringlist *include_directories = 0;

/* List of files given with -o switches.  */

static struct stringlist *old_files = 0;

/* List of files given with -W switches.  */

static struct stringlist *new_files = 0;

/* If nonzero, we should just print usage and exit.  */

static int print_usage_flag = 0;

/* If nonzero, we should print a warning message
   for each reference to an undefined variable.  */

int warn_undefined_variables_flag;

/* The table of command switches.  */

static const struct command_switch switches[] =
  {
    { 'b', ignore, 0, 0, 0, 0, 0, 0,
	0, 0,
	N_("Ignored for compatibility") },
    { 'C', string, (char *) &directories, 0, 0, 0, 0, 0,
	"directory", N_("DIRECTORY"),
	N_("Change to DIRECTORY before doing anything") },
    { 'd', flag, (char *) &debug_flag, 1, 1, 0, 0, 0,
	0, 0,
	N_("Print lots of debugging information") },
    { CHAR_MAX+1, string, (char *) &db_flags, 1, 1, 0,
        "basic", 0,
	"debug", N_("FLAGS"),
	N_("Print various types of debugging information") },
#ifdef WINDOWS32
    { 'D', flag, (char *) &suspend_flag, 1, 1, 0, 0, 0,
        "suspend-for-debug", 0,
        N_("Suspend process to allow a debugger to attach") },
#endif
    { 'e', flag, (char *) &env_overrides, 1, 1, 0, 0, 0,
	"environment-overrides", 0,
	N_("Environment variables override makefiles") },
    { 'f', string, (char *) &makefiles, 0, 0, 0, 0, 0,
	"file", N_("FILE"),
	N_("Read FILE as a makefile") },
    { 'h', flag, (char *) &print_usage_flag, 0, 0, 0, 0, 0,
	"help", 0,
	N_("Print this message and exit") },
    { 'i', flag, (char *) &ignore_errors_flag, 1, 1, 0, 0, 0,
	"ignore-errors", 0,
	N_("Ignore errors from commands") },
    { 'I', string, (char *) &include_directories, 1, 1, 0, 0, 0,
	"include-dir", N_("DIRECTORY"),
	N_("Search DIRECTORY for included makefiles") },
    { 'j',
        positive_int, (char *) &job_slots, 1, 1, 0,
	(char *) &inf_jobs, (char *) &default_job_slots,
	"jobs", "N",
	N_("Allow N jobs at once; infinite jobs with no arg") },
    { CHAR_MAX+2, string, (char *) &jobserver_fds, 1, 1, 0, 0, 0,
        "jobserver-fds", 0,
        0 },
    { 'k', flag, (char *) &keep_going_flag, 1, 1, 0,
	0, (char *) &default_keep_going_flag,
	"keep-going", 0,
	N_("Keep going when some targets can't be made") },
#ifndef NO_FLOAT
    { 'l', floating, (char *) &max_load_average, 1, 1, 0,
	(char *) &default_load_average, (char *) &default_load_average,
	"load-average", "N",
	N_("Don't start multiple jobs unless load is below N") },
#else
    { 'l', positive_int, (char *) &max_load_average, 1, 1, 0,
	(char *) &default_load_average, (char *) &default_load_average,
	"load-average", "N",
	N_("Don't start multiple jobs unless load is below N") },
#endif
    { 'm', ignore, 0, 0, 0, 0, 0, 0,
	0, 0,
	"-b" },
    { 'n', flag, (char *) &just_print_flag, 1, 1, 1, 0, 0,
	"just-print", 0,
	N_("Don't actually run any commands; just print them") },
    { 'o', string, (char *) &old_files, 0, 0, 0, 0, 0,
	"old-file", N_("FILE"),
	N_("Consider FILE to be very old and don't remake it") },
    { 'p', flag, (char *) &print_data_base_flag, 1, 1, 0, 0, 0,
	"print-data-base", 0,
	N_("Print make's internal database") },
    { 'q', flag, (char *) &question_flag, 1, 1, 1, 0, 0,
	"question", 0,
	N_("Run no commands; exit status says if up to date") },
    { 'r', flag, (char *) &no_builtin_rules_flag, 1, 1, 0, 0, 0,
	"no-builtin-rules", 0,
	N_("Disable the built-in implicit rules") },
    { 'R', flag, (char *) &no_builtin_variables_flag, 1, 1, 0, 0, 0,
	"no-builtin-variables", 0,
	N_("Disable the built-in variable settings") },
    { 's', flag, (char *) &silent_flag, 1, 1, 0, 0, 0,
	"silent", 0,
	N_("Don't echo commands") },
    { 'S', flag_off, (char *) &keep_going_flag, 1, 1, 0,
	0, (char *) &default_keep_going_flag,
	"no-keep-going", 0,
	N_("Turns off -k") },
    { 't', flag, (char *) &touch_flag, 1, 1, 1, 0, 0,
	"touch", 0,
	N_("Touch targets instead of remaking them") },
    { 'v', flag, (char *) &print_version_flag, 1, 1, 0, 0, 0,
	"version", 0,
	N_("Print the version number of make and exit") },
    { 'w', flag, (char *) &print_directory_flag, 1, 1, 0, 0, 0,
	"print-directory", 0,
	N_("Print the current directory") },
    { CHAR_MAX+3, flag, (char *) &inhibit_print_directory_flag, 1, 1, 0, 0, 0,
	"no-print-directory", 0,
	N_("Turn off -w, even if it was turned on implicitly") },
    { 'W', string, (char *) &new_files, 0, 0, 0, 0, 0,
	"what-if", N_("FILE"),
	N_("Consider FILE to be infinitely new") },
    { CHAR_MAX+4, flag, (char *) &warn_undefined_variables_flag, 1, 1, 0, 0, 0,
	"warn-undefined-variables", 0,
	N_("Warn when an undefined variable is referenced") },
    { '\0', }
  };

/* Secondary long names for options.  */

static struct option long_option_aliases[] =
  {
    { "quiet",		no_argument,		0, 's' },
    { "stop",		no_argument,		0, 'S' },
    { "new-file",	required_argument,	0, 'W' },
    { "assume-new",	required_argument,	0, 'W' },
    { "assume-old",	required_argument,	0, 'o' },
    { "max-load",	optional_argument,	0, 'l' },
    { "dry-run",	no_argument,		0, 'n' },
    { "recon",		no_argument,		0, 'n' },
    { "makefile",	required_argument,	0, 'f' },
  };

/* The usage message prints the descriptions of options starting in
   this column.  Make sure it leaves enough room for the longest
   description to fit in less than 80 characters.  */

#define	DESCRIPTION_COLUMN	30

/* List of goal targets.  */

static struct dep *goals, *lastgoal;

/* List of variables which were defined on the command line
   (or, equivalently, in MAKEFLAGS).  */

struct command_variable
  {
    struct command_variable *next;
    struct variable *variable;
  };
static struct command_variable *command_variables;

/* The name we were invoked with.  */

char *program;

/* Our current directory before processing any -C options.  */

char *directory_before_chdir;

/* Our current directory after processing all -C options.  */

char *starting_directory;

/* Value of the MAKELEVEL variable at startup (or 0).  */

unsigned int makelevel;

/* First file defined in the makefile whose name does not
   start with `.'.  This is the default to remake if the
   command line does not specify.  */

struct file *default_goal_file;

/* Pointer to structure for the file .DEFAULT
   whose commands are used for any file that has none of its own.
   This is zero if the makefiles do not define .DEFAULT.  */

struct file *default_file;

/* Nonzero if we have seen the magic `.POSIX' target.
   This turns on pedantic compliance with POSIX.2.  */

int posix_pedantic;

/* Nonzero if we have seen the `.NOTPARALLEL' target.
   This turns off parallel builds for this invocation of make.  */

int not_parallel;

/* Nonzero if some rule detected clock skew; we keep track so (a) we only
   print one warning about it during the run, and (b) we can print a final
   warning at the end of the run. */

int clock_skew_detected;

/* Mask of signals that are being caught with fatal_error_signal.  */

#ifdef	POSIX
sigset_t fatal_signal_set;
#else
#ifdef	HAVE_SIGSETMASK
int fatal_signal_mask;
#endif
#endif

static struct file *
enter_command_line_file (name)
     char *name;
{
  if (name[0] == '\0')
    fatal (NILF, _("empty string invalid as file name"));

  if (name[0] == '~')
    {
      char *expanded = tilde_expand (name);
      if (expanded != 0)
	name = expanded;	/* Memory leak; I don't care.  */
    }

  /* This is also done in parse_file_seq, so this is redundant
     for names read from makefiles.  It is here for names passed
     on the command line.  */
  while (name[0] == '.' && name[1] == '/' && name[2] != '\0')
    {
      name += 2;
      while (*name == '/')
	/* Skip following slashes: ".//foo" is "foo", not "/foo".  */
	++name;
    }

  if (*name == '\0')
    {
      /* It was all slashes!  Move back to the dot and truncate
	 it after the first slash, so it becomes just "./".  */
      do
	--name;
      while (name[0] != '.');
      name[2] = '\0';
    }

  return enter_file (xstrdup (name));
}

/* Toggle -d on receipt of SIGUSR1.  */

static RETSIGTYPE
debug_signal_handler (sig)
     int sig;
{
  db_level = db_level ? DB_NONE : DB_BASIC;
}

static void
decode_debug_flags ()
{
  char **pp;

  if (debug_flag)
    db_level = DB_ALL;

  if (!db_flags)
    return;

  for (pp=db_flags->list; *pp; ++pp)
    {
      const char *p = *pp;

      while (1)
        {
          switch (tolower (p[0]))
            {
            case 'a':
              db_level |= DB_ALL;
              break;
            case 'b':
              db_level |= DB_BASIC;
              break;
            case 'i':
              db_level |= DB_BASIC | DB_IMPLICIT;
              break;
            case 'j':
              db_level |= DB_JOBS;
              break;
            case 'm':
              db_level |= DB_BASIC | DB_MAKEFILES;
              break;
            case 'v':
              db_level |= DB_BASIC | DB_VERBOSE;
              break;
            default:
              fatal (NILF, _("unknown debug level specification `%s'"), p);
            }

          while (*(++p) != '\0')
            if (*p == ',' || *p == ' ')
              break;
          if (*p == '\0')
            break;

          ++p;
        }
    }
}

#ifdef WINDOWS32
/*
 * HANDLE runtime exceptions by avoiding a requestor on the GUI. Capture
 * exception and print it to stderr instead.
 *
 * If ! DB_VERBOSE, just print a simple message and exit.
 * If DB_VERBOSE, print a more verbose message.
 * If compiled for DEBUG, let exception pass through to GUI so that
 *   debuggers can attach.
 */
LONG WINAPI
handle_runtime_exceptions( struct _EXCEPTION_POINTERS *exinfo )
{
  PEXCEPTION_RECORD exrec = exinfo->ExceptionRecord;
  LPSTR cmdline = GetCommandLine();
  LPSTR prg = strtok(cmdline, " ");
  CHAR errmsg[1024];
#ifdef USE_EVENT_LOG
  HANDLE hEventSource;
  LPTSTR lpszStrings[1];
#endif

  if (! ISDB (DB_VERBOSE))
    {
      sprintf(errmsg,
              _("%s: Interrupt/Exception caught (code = 0x%x, addr = 0x%x)\n"),
              prg, exrec->ExceptionCode, exrec->ExceptionAddress);
      fprintf(stderr, errmsg);
      exit(255);
    }

  sprintf(errmsg,
          _("\nUnhandled exception filter called from program %s\nExceptionCode = %x\nExceptionFlags = %x\nExceptionAddress = %x\n"),
          prg, exrec->ExceptionCode, exrec->ExceptionFlags,
          exrec->ExceptionAddress);

  if (exrec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
      && exrec->NumberParameters >= 2)
    sprintf(&errmsg[strlen(errmsg)],
            (exrec->ExceptionInformation[0]
             ? _("Access violation: write operation at address %x\n")
             : _("Access violation: read operation at address %x\n")),
            exrec->ExceptionInformation[1]);

  /* turn this on if we want to put stuff in the event log too */
#ifdef USE_EVENT_LOG
  hEventSource = RegisterEventSource(NULL, "GNU Make");
  lpszStrings[0] = errmsg;

  if (hEventSource != NULL)
    {
      ReportEvent(hEventSource,         /* handle of event source */
                  EVENTLOG_ERROR_TYPE,  /* event type */
                  0,                    /* event category */
                  0,                    /* event ID */
                  NULL,                 /* current user's SID */
                  1,                    /* strings in lpszStrings */
                  0,                    /* no bytes of raw data */
                  lpszStrings,          /* array of error strings */
                  NULL);                /* no raw data */

      (VOID) DeregisterEventSource(hEventSource);
    }
#endif

  /* Write the error to stderr too */
  fprintf(stderr, errmsg);

#ifdef DEBUG
  return EXCEPTION_CONTINUE_SEARCH;
#else
  exit(255);
  return (255); /* not reached */
#endif
}

/*
 * On WIN32 systems we don't have the luxury of a /bin directory that
 * is mapped globally to every drive mounted to the system. Since make could
 * be invoked from any drive, and we don't want to propogate /bin/sh
 * to every single drive. Allow ourselves a chance to search for
 * a value for default shell here (if the default path does not exist).
 */

int
find_and_set_default_shell(char *token)
{
  int sh_found = 0;
  char* search_token;
  PATH_VAR(sh_path);
  extern char *default_shell;

  if (!token)
    search_token = default_shell;
  else
    search_token = token;

  if (!no_default_sh_exe &&
      (token == NULL || !strcmp(search_token, default_shell))) {
    /* no new information, path already set or known */
    sh_found = 1;
  } else if (file_exists_p(search_token)) {
    /* search token path was found */
    sprintf(sh_path, "%s", search_token);
    default_shell = xstrdup(w32ify(sh_path,0));
    DB (DB_VERBOSE,
        (_("find_and_set_shell setting default_shell = %s\n"), default_shell));
    sh_found = 1;
  } else {
    char *p;
    struct variable *v = lookup_variable ("Path", 4);

    /*
     * Search Path for shell
     */
    if (v && v->value) {
      char *ep;

      p  = v->value;
      ep = strchr(p, PATH_SEPARATOR_CHAR);

      while (ep && *ep) {
        *ep = '\0';

        if (dir_file_exists_p(p, search_token)) {
          sprintf(sh_path, "%s/%s", p, search_token);
          default_shell = xstrdup(w32ify(sh_path,0));
          sh_found = 1;
          *ep = PATH_SEPARATOR_CHAR;

          /* terminate loop */
          p += strlen(p);
        } else {
          *ep = PATH_SEPARATOR_CHAR;
           p = ++ep;
        }

        ep = strchr(p, PATH_SEPARATOR_CHAR);
      }

      /* be sure to check last element of Path */
      if (p && *p && dir_file_exists_p(p, search_token)) {
          sprintf(sh_path, "%s/%s", p, search_token);
          default_shell = xstrdup(w32ify(sh_path,0));
          sh_found = 1;
      }

      if (sh_found)
        DB (DB_VERBOSE,
            (_("find_and_set_shell path search set default_shell = %s\n"),
             default_shell));
    }
  }

  /* naive test */
  if (!unixy_shell && sh_found &&
      (strstr(default_shell, "sh") || strstr(default_shell, "SH"))) {
    unixy_shell = 1;
    batch_mode_shell = 0;
  }

#ifdef BATCH_MODE_ONLY_SHELL
  batch_mode_shell = 1;
#endif

  return (sh_found);
}
#endif  /* WINDOWS32 */

#ifdef  __MSDOS__

static void
msdos_return_to_initial_directory ()
{
  if (directory_before_chdir)
    chdir (directory_before_chdir);
}
#endif

extern char *mktemp ();
extern int mkstemp ();

FILE *
open_tmpfile(name, template)
     char **name;
     const char *template;
{
  int fd;

#if defined HAVE_MKSTEMP || defined HAVE_MKTEMP
# define TEMPLATE_LEN   strlen (template)
#else
# define TEMPLATE_LEN   L_tmpnam
#endif
  *name = xmalloc (TEMPLATE_LEN + 1);
  strcpy (*name, template);

#if defined HAVE_MKSTEMP && defined HAVE_FDOPEN
  /* It's safest to use mkstemp(), if we can.  */
  fd = mkstemp (*name);
  if (fd == -1)
    return 0;
  return fdopen (fd, "w");
#else
# ifdef HAVE_MKTEMP
  (void) mktemp (*name);
# else
  (void) tmpnam (*name);
# endif

# ifdef HAVE_FDOPEN
  /* Can't use mkstemp(), but guard against a race condition.  */
  fd = open (*name, O_CREAT|O_EXCL|O_WRONLY, 0600);
  if (fd == -1)
    return 0;
  return fdopen (fd, "w");
# else
  /* Not secure, but what can we do?  */
  return fopen (*name, "w");
# endif
#endif
}


#ifndef _AMIGA
int
main (argc, argv, envp)
     int argc;
     char **argv;
     char **envp;
#else
int main (int argc, char ** argv)
#endif
{
  static char *stdin_nm = 0;
  register struct file *f;
  register unsigned int i;
  char **p;
  struct dep *read_makefiles;
  PATH_VAR (current_directory);
#ifdef WINDOWS32
  char *unix_path = NULL;
  char *windows32_path = NULL;

  SetUnhandledExceptionFilter(handle_runtime_exceptions);

  /* start off assuming we have no shell */
  unixy_shell = 0;
  no_default_sh_exe = 1;
#endif

  default_goal_file = 0;
  reading_file = 0;

#if defined (__MSDOS__) && !defined (_POSIX_SOURCE)
  /* Request the most powerful version of `system', to
     make up for the dumb default shell.  */
  __system_flags = (__system_redirect
		    | __system_use_shell
		    | __system_allow_multiple_cmds
		    | __system_allow_long_cmds
		    | __system_handle_null_commands
		    | __system_emulate_chdir);

#endif

  /* Set up gettext/internationalization support.  */
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

#if !defined (HAVE_STRSIGNAL) && !defined (HAVE_SYS_SIGLIST)
  {
    extern void signame_init ();
    signame_init ();
  }
#endif

#ifdef	POSIX
  sigemptyset (&fatal_signal_set);
#define	ADD_SIG(sig)	sigaddset (&fatal_signal_set, sig)
#else
#ifdef	HAVE_SIGSETMASK
  fatal_signal_mask = 0;
#define	ADD_SIG(sig)	fatal_signal_mask |= sigmask (sig)
#else
#define	ADD_SIG(sig)
#endif
#endif

#define	FATAL_SIG(sig)							      \
  if (signal ((sig), fatal_error_signal) == SIG_IGN)			      \
    (void) signal ((sig), SIG_IGN);					      \
  else									      \
    ADD_SIG (sig);

#ifdef SIGHUP
  FATAL_SIG (SIGHUP);
#endif
#ifdef SIGQUIT
  FATAL_SIG (SIGQUIT);
#endif
  FATAL_SIG (SIGINT);
  FATAL_SIG (SIGTERM);

#ifdef	SIGDANGER
  FATAL_SIG (SIGDANGER);
#endif
#ifdef SIGXCPU
  FATAL_SIG (SIGXCPU);
#endif
#ifdef SIGXFSZ
  FATAL_SIG (SIGXFSZ);
#endif

#undef	FATAL_SIG

  /* Do not ignore the child-death signal.  This must be done before
     any children could possibly be created; otherwise, the wait
     functions won't work on systems with the SVR4 ECHILD brain
     damage, if our invoker is ignoring this signal.  */

#ifdef HAVE_WAIT_NOHANG
# if defined SIGCHLD
  (void) signal (SIGCHLD, SIG_DFL);
# endif
# if defined SIGCLD && SIGCLD != SIGCHLD
  (void) signal (SIGCLD, SIG_DFL);
# endif
#endif

  /* Make sure stdout is line-buffered.  */

#ifdef	HAVE_SETLINEBUF
  setlinebuf (stdout);
#else
#ifndef	SETVBUF_REVERSED
  setvbuf (stdout, (char *) 0, _IOLBF, BUFSIZ);
#else	/* setvbuf not reversed.  */
  /* Some buggy systems lose if we pass 0 instead of allocating ourselves.  */
  setvbuf (stdout, _IOLBF, xmalloc (BUFSIZ), BUFSIZ);
#endif	/* setvbuf reversed.  */
#endif	/* setlinebuf missing.  */

  /* Figure out where this program lives.  */

  if (argv[0] == 0)
    argv[0] = "";
  if (argv[0][0] == '\0')
    program = "make";
  else
    {
#ifdef VMS
      program = strrchr (argv[0], ']');
#else
      program = strrchr (argv[0], '/');
#endif
#ifdef __MSDOS__
      if (program == 0)
	program = strrchr (argv[0], '\\');
      else
	{
	  /* Some weird environments might pass us argv[0] with
	     both kinds of slashes; we must find the rightmost.  */
	  char *p = strrchr (argv[0], '\\');
	  if (p && p > program)
	    program = p;
	}
      if (program == 0 && argv[0][1] == ':')
	program = argv[0] + 1;
#endif
      if (program == 0)
	program = argv[0];
      else
	++program;
    }

  /* Set up to access user data (files).  */
  user_access ();

  /* Figure out where we are.  */

#ifdef WINDOWS32
  if (getcwd_fs (current_directory, GET_PATH_MAX) == 0)
#else
  if (getcwd (current_directory, GET_PATH_MAX) == 0)
#endif
    {
#ifdef	HAVE_GETCWD
      perror_with_name ("getcwd: ", "");
#else
      error (NILF, "getwd: %s", current_directory);
#endif
      current_directory[0] = '\0';
      directory_before_chdir = 0;
    }
  else
    directory_before_chdir = xstrdup (current_directory);
#ifdef  __MSDOS__
  /* Make sure we will return to the initial directory, come what may.  */
  atexit (msdos_return_to_initial_directory);
#endif

  /* Read in variables from the environment.  It is important that this be
     done before $(MAKE) is figured out so its definitions will not be
     from the environment.  */

#ifndef _AMIGA
  for (i = 0; envp[i] != 0; ++i)
    {
      int do_not_define;
      register char *ep = envp[i];

      /* by default, everything gets defined and exported */
      do_not_define = 0;

      while (*ep != '=')
        ++ep;
#ifdef WINDOWS32
      if (!unix_path && strneq(envp[i], "PATH=", 5))
        unix_path = ep+1;
      else if (!windows32_path && !strnicmp(envp[i], "Path=", 5)) {
        do_not_define = 1; /* it gets defined after loop exits */
        windows32_path = ep+1;
      }
#endif
      /* The result of pointer arithmetic is cast to unsigned int for
	 machines where ptrdiff_t is a different size that doesn't widen
	 the same.  */
      if (!do_not_define)
        define_variable (envp[i], (unsigned int) (ep - envp[i]),
                         ep + 1, o_env, 1)
	/* Force exportation of every variable culled from the environment.
	   We used to rely on target_environment's v_default code to do this.
	   But that does not work for the case where an environment variable
	   is redefined in a makefile with `override'; it should then still
	   be exported, because it was originally in the environment.  */
	->export = v_export;
    }
#ifdef WINDOWS32
    /*
     * Make sure that this particular spelling of 'Path' is available
     */
    if (windows32_path)
      define_variable("Path", 4, windows32_path, o_env, 1)->export = v_export;
    else if (unix_path)
      define_variable("Path", 4, unix_path, o_env, 1)->export = v_export;
    else
      define_variable("Path", 4, "", o_env, 1)->export = v_export;

    /*
     * PATH defaults to Path iff PATH not found and Path is found.
     */
    if (!unix_path && windows32_path)
      define_variable("PATH", 4, windows32_path, o_env, 1)->export = v_export;
#endif
#else /* For Amiga, read the ENV: device, ignoring all dirs */
    {
	BPTR env, file, old;
	char buffer[1024];
	int len;
	__aligned struct FileInfoBlock fib;

	env = Lock ("ENV:", ACCESS_READ);
	if (env)
	{
	    old = CurrentDir (DupLock(env));
	    Examine (env, &fib);

	    while (ExNext (env, &fib))
	    {
		if (fib.fib_DirEntryType < 0) /* File */
		{
		    /* Define an empty variable. It will be filled in
			variable_lookup(). Makes startup quite a bit
			faster. */
			define_variable (fib.fib_FileName,
			    strlen (fib.fib_FileName),
			"", o_env, 1)->export = v_export;
		}
	    }
	    UnLock (env);
	    UnLock(CurrentDir(old));
	}
    }
#endif

  /* Decode the switches.  */

  decode_env_switches ("MAKEFLAGS", 9);
#if 0
  /* People write things like:
     	MFLAGS="CC=gcc -pipe" "CFLAGS=-g"
     and we set the -p, -i and -e switches.  Doesn't seem quite right.  */
  decode_env_switches ("MFLAGS", 6);
#endif
  decode_switches (argc, argv, 0);
#ifdef WINDOWS32
  if (suspend_flag) {
        fprintf(stderr, "%s (pid = %d)\n", argv[0], GetCurrentProcessId());
        fprintf(stderr, _("%s is suspending for 30 seconds..."), argv[0]);
        Sleep(30 * 1000);
        fprintf(stderr, _("done sleep(30). Continuing.\n"));
  }
#endif

  decode_debug_flags ();

  /* Print version information.  */

  if (print_version_flag || print_data_base_flag || db_level)
    print_version ();

  /* `make --version' is supposed to just print the version and exit.  */
  if (print_version_flag)
    die (0);

#ifndef VMS
  /* Set the "MAKE_COMMAND" variable to the name we were invoked with.
     (If it is a relative pathname with a slash, prepend our directory name
     so the result will run the same program regardless of the current dir.
     If it is a name with no slash, we can only hope that PATH did not
     find it in the current directory.)  */
#ifdef WINDOWS32
  /*
   * Convert from backslashes to forward slashes for
   * programs like sh which don't like them. Shouldn't
   * matter if the path is one way or the other for
   * CreateProcess().
   */
  if (strpbrk(argv[0], "/:\\") ||
      strstr(argv[0], "..") ||
      strneq(argv[0], "//", 2))
    argv[0] = xstrdup(w32ify(argv[0],1));
#else /* WINDOWS32 */
#ifdef __MSDOS__
  if (strchr (argv[0], '\\'))
    {
      char *p;

      argv[0] = xstrdup (argv[0]);
      for (p = argv[0]; *p; p++)
	if (*p == '\\')
	  *p = '/';
    }
  /* If argv[0] is not in absolute form, prepend the current
     directory.  This can happen when Make is invoked by another DJGPP
     program that uses a non-absolute name.  */
  if (current_directory[0] != '\0'
      && argv[0] != 0
      && (argv[0][0] != '/' && (argv[0][0] == '\0' || argv[0][1] != ':')))
    argv[0] = concat (current_directory, "/", argv[0]);
#else  /* !__MSDOS__ */
  if (current_directory[0] != '\0'
      && argv[0] != 0 && argv[0][0] != '/' && strchr (argv[0], '/') != 0)
    argv[0] = concat (current_directory, "/", argv[0]);
#endif /* !__MSDOS__ */
#endif /* WINDOWS32 */
#endif

  /* The extra indirection through $(MAKE_COMMAND) is done
     for hysterical raisins.  */
  (void) define_variable ("MAKE_COMMAND", 12, argv[0], o_default, 0);
  (void) define_variable ("MAKE", 4, "$(MAKE_COMMAND)", o_default, 1);

  if (command_variables != 0)
    {
      struct command_variable *cv;
      struct variable *v;
      unsigned int len = 0;
      char *value, *p;

      /* Figure out how much space will be taken up by the command-line
	 variable definitions.  */
      for (cv = command_variables; cv != 0; cv = cv->next)
	{
	  v = cv->variable;
	  len += 2 * strlen (v->name);
	  if (! v->recursive)
	    ++len;
	  ++len;
	  len += 2 * strlen (v->value);
	  ++len;
	}

      /* Now allocate a buffer big enough and fill it.  */
      p = value = (char *) alloca (len);
      for (cv = command_variables; cv != 0; cv = cv->next)
	{
	  v = cv->variable;
	  p = quote_for_env (p, v->name);
	  if (! v->recursive)
	    *p++ = ':';
	  *p++ = '=';
	  p = quote_for_env (p, v->value);
	  *p++ = ' ';
	}
      p[-1] = '\0';		/* Kill the final space and terminate.  */

      /* Define an unchangeable variable with a name that no POSIX.2
	 makefile could validly use for its own variable.  */
      (void) define_variable ("-*-command-variables-*-", 23,
			      value, o_automatic, 0);

      /* Define the variable; this will not override any user definition.
         Normally a reference to this variable is written into the value of
         MAKEFLAGS, allowing the user to override this value to affect the
         exported value of MAKEFLAGS.  In POSIX-pedantic mode, we cannot
         allow the user's setting of MAKEOVERRIDES to affect MAKEFLAGS, so
         a reference to this hidden variable is written instead. */
      (void) define_variable ("MAKEOVERRIDES", 13,
			      "${-*-command-variables-*-}", o_env, 1);
    }

  /* If there were -C flags, move ourselves about.  */
  if (directories != 0)
    for (i = 0; directories->list[i] != 0; ++i)
      {
	char *dir = directories->list[i];
	if (dir[0] == '~')
	  {
	    char *expanded = tilde_expand (dir);
	    if (expanded != 0)
	      dir = expanded;
	  }
	if (chdir (dir) < 0)
	  pfatal_with_name (dir);
	if (dir != directories->list[i])
	  free (dir);
      }

#ifdef WINDOWS32
  /*
   * THIS BLOCK OF CODE MUST COME AFTER chdir() CALL ABOVE IN ORDER
   * TO NOT CONFUSE THE DEPENDENCY CHECKING CODE IN implicit.c.
   *
   * The functions in dir.c can incorrectly cache information for "."
   * before we have changed directory and this can cause file
   * lookups to fail because the current directory (.) was pointing
   * at the wrong place when it was first evaluated.
   */
   no_default_sh_exe = !find_and_set_default_shell(NULL);

#endif /* WINDOWS32 */
  /* Figure out the level of recursion.  */
  {
    struct variable *v = lookup_variable ("MAKELEVEL", 9);
    if (v != 0 && *v->value != '\0' && *v->value != '-')
      makelevel = (unsigned int) atoi (v->value);
    else
      makelevel = 0;
  }

  /* Except under -s, always do -w in sub-makes and under -C.  */
  if (!silent_flag && (directories != 0 || makelevel > 0))
    print_directory_flag = 1;

  /* Let the user disable that with --no-print-directory.  */
  if (inhibit_print_directory_flag)
    print_directory_flag = 0;

  /* If -R was given, set -r too (doesn't make sense otherwise!)  */
  if (no_builtin_variables_flag)
    no_builtin_rules_flag = 1;

  /* Construct the list of include directories to search.  */

  construct_include_path (include_directories == 0 ? (char **) 0
			  : include_directories->list);

  /* Figure out where we are now, after chdir'ing.  */
  if (directories == 0)
    /* We didn't move, so we're still in the same place.  */
    starting_directory = current_directory;
  else
    {
#ifdef WINDOWS32
      if (getcwd_fs (current_directory, GET_PATH_MAX) == 0)
#else
      if (getcwd (current_directory, GET_PATH_MAX) == 0)
#endif
	{
#ifdef	HAVE_GETCWD
	  perror_with_name ("getcwd: ", "");
#else
	  error (NILF, "getwd: %s", current_directory);
#endif
	  starting_directory = 0;
	}
      else
	starting_directory = current_directory;
    }

  (void) define_variable ("CURDIR", 6, current_directory, o_default, 0);

  /* Read any stdin makefiles into temporary files.  */

  if (makefiles != 0)
    {
      register unsigned int i;
      for (i = 0; i < makefiles->idx; ++i)
	if (makefiles->list[i][0] == '-' && makefiles->list[i][1] == '\0')
	  {
	    /* This makefile is standard input.  Since we may re-exec
	       and thus re-read the makefiles, we read standard input
	       into a temporary file and read from that.  */
	    FILE *outfile;

            if (stdin_nm)
              fatal (NILF, _("Makefile from standard input specified twice."));

#ifdef VMS
# define TMP_TEMPLATE   "sys$scratch:GmXXXXXX"
#else
# define TMP_TEMPLATE   "/tmp/GmXXXXXX"
#endif

	    outfile = open_tmpfile (&stdin_nm, TMP_TEMPLATE);
	    if (outfile == 0)
	      pfatal_with_name (_("fopen (temporary file)"));
	    while (!feof (stdin))
	      {
		char buf[2048];
		unsigned int n = fread (buf, 1, sizeof (buf), stdin);
		if (n > 0 && fwrite (buf, 1, n, outfile) != n)
		  pfatal_with_name (_("fwrite (temporary file)"));
	      }
	    (void) fclose (outfile);

	    /* Replace the name that read_all_makefiles will
	       see with the name of the temporary file.  */
            makefiles->list[i] = xstrdup (stdin_nm);

	    /* Make sure the temporary file will not be remade.  */
	    f = enter_file (stdin_nm);
	    f->updated = 1;
	    f->update_status = 0;
	    f->command_state = cs_finished;
 	    /* Can't be intermediate, or it'll be removed too early for
               make re-exec.  */
 	    f->intermediate = 0;
	    f->dontcare = 0;
	  }
    }

#if defined(MAKE_JOBSERVER) || !defined(HAVE_WAIT_NOHANG)
  /* Set up to handle children dying.  This must be done before
     reading in the makefiles so that `shell' function calls will work.

     If we don't have a hanging wait we have to fall back to old, broken
     functionality here and rely on the signal handler and counting
     children.

     If we're using the jobs pipe we need a signal handler so that
     SIGCHLD is not ignored; we need it to interrupt the read(2) of the
     jobserver pipe in job.c if we're waiting for a token.

     If none of these are true, we don't need a signal handler at all.  */
  {
    extern RETSIGTYPE child_handler PARAMS ((int sig));

# if defined HAVE_SIGACTION
    struct sigaction sa;

    bzero ((char *)&sa, sizeof (struct sigaction));
    sa.sa_handler = child_handler;
#  if defined SA_INTERRUPT
    /* This is supposed to be the default, but what the heck... */
    sa.sa_flags = SA_INTERRUPT;
#  endif
#  define HANDLESIG(s) sigaction (s, &sa, NULL)
# else
#  define HANDLESIG(s) signal (s, child_handler)
# endif

    /* OK, now actually install the handlers.  */
# if defined SIGCHLD
    (void) HANDLESIG (SIGCHLD);
# endif
# if defined SIGCLD && SIGCLD != SIGCHLD
    (void) HANDLESIG (SIGCLD);
# endif
  }
#endif

  /* Let the user send us SIGUSR1 to toggle the -d flag during the run.  */
#ifdef SIGUSR1
  (void) signal (SIGUSR1, debug_signal_handler);
#endif

  /* Define the initial list of suffixes for old-style rules.  */

  set_default_suffixes ();

  /* Define the file rules for the built-in suffix rules.  These will later
     be converted into pattern rules.  We used to do this in
     install_default_implicit_rules, but since that happens after reading
     makefiles, it results in the built-in pattern rules taking precedence
     over makefile-specified suffix rules, which is wrong.  */

  install_default_suffix_rules ();

  /* Define some internal and special variables.  */

  define_automatic_variables ();

  /* Set up the MAKEFLAGS and MFLAGS variables
     so makefiles can look at them.  */

  define_makeflags (0, 0);

  /* Define the default variables.  */
  define_default_variables ();

  /* Read all the makefiles.  */

  default_file = enter_file (".DEFAULT");

  read_makefiles
    = read_all_makefiles (makefiles == 0 ? (char **) 0 : makefiles->list);

#ifdef WINDOWS32
  /* look one last time after reading all Makefiles */
  if (no_default_sh_exe)
    no_default_sh_exe = !find_and_set_default_shell(NULL);

  if (no_default_sh_exe && job_slots != 1) {
    error (NILF, _("Do not specify -j or --jobs if sh.exe is not available."));
    error (NILF, _("Resetting make for single job mode."));
    job_slots = 1;
  }
#endif /* WINDOWS32 */

#ifdef __MSDOS__
  /* We need to know what kind of shell we will be using.  */
  {
    extern int _is_unixy_shell (const char *_path);
    struct variable *shv = lookup_variable ("SHELL", 5);
    extern int unixy_shell;
    extern char *default_shell;

    if (shv && *shv->value)
      {
	char *shell_path = recursively_expand(shv);

	if (shell_path && _is_unixy_shell (shell_path))
	  unixy_shell = 1;
	else
	  unixy_shell = 0;
	if (shell_path)
	  default_shell = shell_path;
      }
  }
#endif /* __MSDOS__ */

  /* Decode switches again, in case the variables were set by the makefile.  */
  decode_env_switches ("MAKEFLAGS", 9);
#if 0
  decode_env_switches ("MFLAGS", 6);
#endif

#ifdef __MSDOS__
  if (job_slots != 1)
    {
      error (NILF,
             _("Parallel jobs (-j) are not supported on this platform."));
      error (NILF, _("Resetting to single job (-j1) mode."));
      job_slots = 1;
    }
#endif

#ifdef MAKE_JOBSERVER
  /* If the jobserver-fds option is seen, make sure that -j is reasonable.  */

  if (jobserver_fds)
  {
    char *cp;

    for (i=1; i < jobserver_fds->idx; ++i)
      if (!streq (jobserver_fds->list[0], jobserver_fds->list[i]))
        fatal (NILF, _("internal error: multiple --jobserver-fds options"));

    /* Now parse the fds string and make sure it has the proper format.  */

    cp = jobserver_fds->list[0];

    if (sscanf (cp, "%d,%d", &job_fds[0], &job_fds[1]) != 2)
      fatal (NILF,
             _("internal error: invalid --jobserver-fds string `%s'"), cp);

    /* The combination of a pipe + !job_slots means we're using the
       jobserver.  If !job_slots and we don't have a pipe, we can start
       infinite jobs.  If we see both a pipe and job_slots >0 that means the
       user set -j explicitly.  This is broken; in this case obey the user
       (ignore the jobserver pipe for this make) but print a message.  */

    if (job_slots > 0)
      error (NILF,
             _("warning: -jN forced in submake: disabling jobserver mode."));

    /* Create a duplicate pipe, that will be closed in the SIGCHLD
       handler.  If this fails with EBADF, the parent has closed the pipe
       on us because it didn't think we were a submake.  If so, print a
       warning then default to -j1.  */

    else if ((job_rfd = dup (job_fds[0])) < 0)
      {
        if (errno != EBADF)
          pfatal_with_name (_("dup jobserver"));

        error (NILF,
               _("warning: jobserver unavailable: using -j1.  Add `+' to parent make rule."));
        job_slots = 1;
      }

    if (job_slots > 0)
      {
        close (job_fds[0]);
        close (job_fds[1]);
        job_fds[0] = job_fds[1] = -1;
        free (jobserver_fds->list);
        free (jobserver_fds);
        jobserver_fds = 0;
      }
  }

  /* If we have >1 slot but no jobserver-fds, then we're a top-level make.
     Set up the pipe and install the fds option for our children.  */

  if (job_slots > 1)
    {
      char c = '+';

      if (pipe (job_fds) < 0 || (job_rfd = dup (job_fds[0])) < 0)
	pfatal_with_name (_("creating jobs pipe"));

      /* Every make assumes that it always has one job it can run.  For the
         submakes it's the token they were given by their parent.  For the
         top make, we just subtract one from the number the user wants.  We
         want job_slots to be 0 to indicate we're using the jobserver.  */

      while (--job_slots)
        while (write (job_fds[1], &c, 1) != 1)
          if (!EINTR_SET)
            pfatal_with_name (_("init jobserver pipe"));

      /* Fill in the jobserver_fds struct for our children.  */

      jobserver_fds = (struct stringlist *)
                        xmalloc (sizeof (struct stringlist));
      jobserver_fds->list = (char **) xmalloc (sizeof (char *));
      jobserver_fds->list[0] = xmalloc ((sizeof ("1024")*2)+1);

      sprintf (jobserver_fds->list[0], "%d,%d", job_fds[0], job_fds[1]);
      jobserver_fds->idx = 1;
      jobserver_fds->max = 1;
    }
#endif

  /* Set up MAKEFLAGS and MFLAGS again, so they will be right.  */

  define_makeflags (1, 0);

  /* Make each `struct dep' point at the `struct file' for the file
     depended on.  Also do magic for special targets.  */

  snap_deps ();

  /* Convert old-style suffix rules to pattern rules.  It is important to
     do this before installing the built-in pattern rules below, so that
     makefile-specified suffix rules take precedence over built-in pattern
     rules.  */

  convert_to_pattern ();

  /* Install the default implicit pattern rules.
     This used to be done before reading the makefiles.
     But in that case, built-in pattern rules were in the chain
     before user-defined ones, so they matched first.  */

  install_default_implicit_rules ();

  /* Compute implicit rule limits.  */

  count_implicit_rule_limits ();

  /* Construct the listings of directories in VPATH lists.  */

  build_vpath_lists ();

  /* Mark files given with -o flags as very old (00:00:01.00 Jan 1, 1970)
     and as having been updated already, and files given with -W flags as
     brand new (time-stamp as far as possible into the future).  */

  if (old_files != 0)
    for (p = old_files->list; *p != 0; ++p)
      {
	f = enter_command_line_file (*p);
	f->last_mtime = f->mtime_before_update = (FILE_TIMESTAMP) 1;
	f->updated = 1;
	f->update_status = 0;
	f->command_state = cs_finished;
      }

  if (new_files != 0)
    {
      for (p = new_files->list; *p != 0; ++p)
	{
	  f = enter_command_line_file (*p);
	  f->last_mtime = f->mtime_before_update = NEW_MTIME;
	}
    }

  /* Initialize the remote job module.  */
  remote_setup ();

  if (read_makefiles != 0)
    {
      /* Update any makefiles if necessary.  */

      FILE_TIMESTAMP *makefile_mtimes = 0;
      unsigned int mm_idx = 0;
      char **nargv = argv;
      int nargc = argc;
      int orig_db_level = db_level;

      if (! ISDB (DB_MAKEFILES))
        db_level = DB_NONE;

      DB (DB_BASIC, (_("Updating makefiles....\n")));

      /* Remove any makefiles we don't want to try to update.
	 Also record the current modtimes so we can compare them later.  */
      {
	register struct dep *d, *last;
	last = 0;
	d = read_makefiles;
	while (d != 0)
	  {
	    register struct file *f = d->file;
	    if (f->double_colon)
	      for (f = f->double_colon; f != NULL; f = f->prev)
		{
		  if (f->deps == 0 && f->cmds != 0)
		    {
		      /* This makefile is a :: target with commands, but
			 no dependencies.  So, it will always be remade.
			 This might well cause an infinite loop, so don't
			 try to remake it.  (This will only happen if
			 your makefiles are written exceptionally
			 stupidly; but if you work for Athena, that's how
			 you write your makefiles.)  */

		      DB (DB_VERBOSE,
                          (_("Makefile `%s' might loop; not remaking it.\n"),
                           f->name));

		      if (last == 0)
			read_makefiles = d->next;
		      else
			last->next = d->next;

		      /* Free the storage.  */
		      free ((char *) d);

		      d = last == 0 ? read_makefiles : last->next;

		      break;
		    }
		}
	    if (f == NULL || !f->double_colon)
	      {
                makefile_mtimes = (FILE_TIMESTAMP *)
                  xrealloc ((char *) makefile_mtimes,
                            (mm_idx + 1) * sizeof (FILE_TIMESTAMP));
		makefile_mtimes[mm_idx++] = file_mtime_no_search (d->file);
		last = d;
		d = d->next;
	      }
	  }
      }

      /* Set up `MAKEFLAGS' specially while remaking makefiles.  */
      define_makeflags (1, 1);

      switch (update_goal_chain (read_makefiles, 1))
	{
	case 1:
	default:
#define BOGUS_UPDATE_STATUS 0
	  assert (BOGUS_UPDATE_STATUS);
	  break;

	case -1:
	  /* Did nothing.  */
	  break;

	case 2:
	  /* Failed to update.  Figure out if we care.  */
	  {
	    /* Nonzero if any makefile was successfully remade.  */
	    int any_remade = 0;
	    /* Nonzero if any makefile we care about failed
	       in updating or could not be found at all.  */
	    int any_failed = 0;
	    register unsigned int i;
            struct dep *d;

	    for (i = 0, d = read_makefiles; d != 0; ++i, d = d->next)
              {
                /* Reset the considered flag; we may need to look at the file
                   again to print an error.  */
                d->file->considered = 0;

                if (d->file->updated)
                  {
                    /* This makefile was updated.  */
                    if (d->file->update_status == 0)
                      {
                        /* It was successfully updated.  */
                        any_remade |= (file_mtime_no_search (d->file)
                                       != makefile_mtimes[i]);
                      }
                    else if (! (d->changed & RM_DONTCARE))
                      {
                        FILE_TIMESTAMP mtime;
                        /* The update failed and this makefile was not
                           from the MAKEFILES variable, so we care.  */
                        error (NILF, _("Failed to remake makefile `%s'."),
                               d->file->name);
                        mtime = file_mtime_no_search (d->file);
                        any_remade |= (mtime != (FILE_TIMESTAMP) -1
                                       && mtime != makefile_mtimes[i]);
                      }
                  }
                else
                  /* This makefile was not found at all.  */
                  if (! (d->changed & RM_DONTCARE))
                    {
                      /* This is a makefile we care about.  See how much.  */
                      if (d->changed & RM_INCLUDED)
                        /* An included makefile.  We don't need
                           to die, but we do want to complain.  */
                        error (NILF,
                               _("Included makefile `%s' was not found."),
                               dep_name (d));
                      else
                        {
                          /* A normal makefile.  We must die later.  */
                          error (NILF, _("Makefile `%s' was not found"),
                                 dep_name (d));
                          any_failed = 1;
                        }
                    }
              }
            /* Reset this to empty so we get the right error message below.  */
            read_makefiles = 0;

	    if (any_remade)
	      goto re_exec;
	    if (any_failed)
	      die (2);
            break;
	  }

	case 0:
	re_exec:
	  /* Updated successfully.  Re-exec ourselves.  */

	  remove_intermediates (0);

	  if (print_data_base_flag)
	    print_data_base ();

	  log_working_directory (0);

	  if (makefiles != 0)
	    {
	      /* These names might have changed.  */
	      register unsigned int i, j = 0;
	      for (i = 1; i < argc; ++i)
		if (strneq (argv[i], "-f", 2)) /* XXX */
		  {
		    char *p = &argv[i][2];
		    if (*p == '\0')
		      argv[++i] = makefiles->list[j];
		    else
		      argv[i] = concat ("-f", makefiles->list[j], "");
		    ++j;
		  }
	    }

          /* Add -o option for the stdin temporary file, if necessary.  */
          if (stdin_nm)
            {
              nargv = (char **) xmalloc ((nargc + 2) * sizeof (char *));
              bcopy ((char *) argv, (char *) nargv, argc * sizeof (char *));
              nargv[nargc++] = concat ("-o", stdin_nm, "");
              nargv[nargc] = 0;
            }

	  if (directories != 0 && directories->idx > 0)
	    {
	      char bad;
	      if (directory_before_chdir != 0)
		{
		  if (chdir (directory_before_chdir) < 0)
		    {
		      perror_with_name ("chdir", "");
		      bad = 1;
		    }
		  else
		    bad = 0;
		}
	      else
		bad = 1;
	      if (bad)
		fatal (NILF, _("Couldn't change back to original directory."));
	    }

#ifndef _AMIGA
	  for (p = environ; *p != 0; ++p)
	    if (strneq (*p, "MAKELEVEL=", 10))
	      {
		/* The SGI compiler apparently can't understand
		   the concept of storing the result of a function
		   in something other than a local variable.  */
		char *sgi_loses;
		sgi_loses = (char *) alloca (40);
		*p = sgi_loses;
		sprintf (*p, "MAKELEVEL=%u", makelevel);
		break;
	      }
#else /* AMIGA */
	  {
	    char buffer[256];
	    int len;

	    len = GetVar ("MAKELEVEL", buffer, sizeof (buffer), GVF_GLOBAL_ONLY);

	    if (len != -1)
	    {
	    sprintf (buffer, "%u", makelevel);
	      SetVar ("MAKELEVEL", buffer, -1, GVF_GLOBAL_ONLY);
	    }
	  }
#endif

	  if (ISDB (DB_BASIC))
	    {
	      char **p;
	      fputs (_("Re-executing:"), stdout);
	      for (p = nargv; *p != 0; ++p)
		printf (" %s", *p);
	      putchar ('\n');
	    }

	  fflush (stdout);
	  fflush (stderr);

          /* Close the dup'd jobserver pipe if we opened one.  */
          if (job_rfd >= 0)
            close (job_rfd);

#ifndef _AMIGA
	  exec_command (nargv, environ);
#else
	  exec_command (nargv);
	  exit (0);
#endif
	  /* NOTREACHED */
	}

      db_level = orig_db_level;
    }

  /* Set up `MAKEFLAGS' again for the normal targets.  */
  define_makeflags (1, 0);

  /* If there is a temp file from reading a makefile from stdin, get rid of
     it now.  */
  if (stdin_nm && unlink (stdin_nm) < 0 && errno != ENOENT)
    perror_with_name (_("unlink (temporary file): "), stdin_nm);

  {
    int status;

    /* If there were no command-line goals, use the default.  */
    if (goals == 0)
      {
	if (default_goal_file != 0)
	  {
	    goals = (struct dep *) xmalloc (sizeof (struct dep));
	    goals->next = 0;
	    goals->name = 0;
	    goals->file = default_goal_file;
	  }
      }
    else
      lastgoal->next = 0;

    if (!goals)
      {
        if (read_makefiles == 0)
          fatal (NILF, _("No targets specified and no makefile found"));

        fatal (NILF, _("No targets"));
      }

    /* Update the goals.  */

    DB (DB_BASIC, (_("Updating goal targets....\n")));

    switch (update_goal_chain (goals, 0))
    {
      case -1:
        /* Nothing happened.  */
      case 0:
        /* Updated successfully.  */
        status = EXIT_SUCCESS;
        break;
      case 2:
        /* Updating failed.  POSIX.2 specifies exit status >1 for this;
           but in VMS, there is only success and failure.  */
        status = EXIT_FAILURE ? 2 : EXIT_FAILURE;
        break;
      case 1:
        /* We are under -q and would run some commands.  */
        status = EXIT_FAILURE;
        break;
      default:
        abort ();
    }

    /* If we detected some clock skew, generate one last warning */
    if (clock_skew_detected)
      error (NILF,
             _("warning:  Clock skew detected.  Your build may be incomplete."));

    /* Exit.  */
    die (status);
  }

  return 0;
}

/* Parsing of arguments, decoding of switches.  */

static char options[1 + sizeof (switches) / sizeof (switches[0]) * 3];
static struct option long_options[(sizeof (switches) / sizeof (switches[0])) +
				  (sizeof (long_option_aliases) /
				   sizeof (long_option_aliases[0]))];

/* Fill in the string and vector for getopt.  */
static void
init_switches ()
{
  register char *p;
  register int c;
  register unsigned int i;

  if (options[0] != '\0')
    /* Already done.  */
    return;

  p = options;

  /* Return switch and non-switch args in order, regardless of
     POSIXLY_CORRECT.  Non-switch args are returned as option 1.  */
  *p++ = '-';

  for (i = 0; switches[i].c != '\0'; ++i)
    {
      long_options[i].name = (switches[i].long_name == 0 ? "" :
			      switches[i].long_name);
      long_options[i].flag = 0;
      long_options[i].val = switches[i].c;
      if (short_option (switches[i].c))
	*p++ = switches[i].c;
      switch (switches[i].type)
	{
	case flag:
	case flag_off:
	case ignore:
	  long_options[i].has_arg = no_argument;
	  break;

	case string:
	case positive_int:
	case floating:
	  if (short_option (switches[i].c))
	    *p++ = ':';
	  if (switches[i].noarg_value != 0)
	    {
	      if (short_option (switches[i].c))
		*p++ = ':';
	      long_options[i].has_arg = optional_argument;
	    }
	  else
	    long_options[i].has_arg = required_argument;
	  break;
	}
    }
  *p = '\0';
  for (c = 0; c < (sizeof (long_option_aliases) /
		   sizeof (long_option_aliases[0]));
       ++c)
    long_options[i++] = long_option_aliases[c];
  long_options[i].name = 0;
}

static void
handle_non_switch_argument (arg, env)
     char *arg;
     int env;
{
  /* Non-option argument.  It might be a variable definition.  */
  struct variable *v;
  if (arg[0] == '-' && arg[1] == '\0')
    /* Ignore plain `-' for compatibility.  */
    return;
  v = try_variable_definition (0, arg, o_command, 0);
  if (v != 0)
    {
      /* It is indeed a variable definition.  Record a pointer to
	 the variable for later use in define_makeflags.  */
      struct command_variable *cv
	= (struct command_variable *) xmalloc (sizeof (*cv));
      cv->variable = v;
      cv->next = command_variables;
      command_variables = cv;
    }
  else if (! env)
    {
      /* Not an option or variable definition; it must be a goal
	 target!  Enter it as a file and add it to the dep chain of
	 goals.  */
      struct file *f = enter_command_line_file (arg);
      f->cmd_target = 1;

      if (goals == 0)
	{
	  goals = (struct dep *) xmalloc (sizeof (struct dep));
	  lastgoal = goals;
	}
      else
	{
	  lastgoal->next = (struct dep *) xmalloc (sizeof (struct dep));
	  lastgoal = lastgoal->next;
	}
      lastgoal->name = 0;
      lastgoal->file = f;

      {
        /* Add this target name to the MAKECMDGOALS variable. */
        struct variable *v;
        char *value;

        v = lookup_variable ("MAKECMDGOALS", 12);
        if (v == 0)
          value = f->name;
        else
          {
            /* Paste the old and new values together */
            unsigned int oldlen, newlen;

            oldlen = strlen (v->value);
            newlen = strlen (f->name);
            value = (char *) alloca (oldlen + 1 + newlen + 1);
            bcopy (v->value, value, oldlen);
            value[oldlen] = ' ';
            bcopy (f->name, &value[oldlen + 1], newlen + 1);
          }
        define_variable ("MAKECMDGOALS", 12, value, o_default, 0);
      }
    }
}

/* Print a nice usage method.  */

static void
print_usage (bad)
     int bad;
{
  register const struct command_switch *cs;
  FILE *usageto;

  if (print_version_flag)
    print_version ();

  usageto = bad ? stderr : stdout;

  fprintf (usageto, _("Usage: %s [options] [target] ...\n"), program);

  fputs (_("Options:\n"), usageto);
  for (cs = switches; cs->c != '\0'; ++cs)
    {
      char buf[1024], shortarg[50], longarg[50], *p;

      if (!cs->description || cs->description[0] == '-')
	continue;

      switch (long_options[cs - switches].has_arg)
	{
	case no_argument:
	  shortarg[0] = longarg[0] = '\0';
	  break;
	case required_argument:
	  sprintf (longarg, "=%s", gettext (cs->argdesc));
	  sprintf (shortarg, " %s", gettext (cs->argdesc));
	  break;
	case optional_argument:
	  sprintf (longarg, "[=%s]", gettext (cs->argdesc));
	  sprintf (shortarg, " [%s]", gettext (cs->argdesc));
	  break;
	}

      p = buf;

      if (short_option (cs->c))
	{
	  sprintf (buf, "  -%c%s", cs->c, shortarg);
	  p += strlen (p);
	}
      if (cs->long_name != 0)
	{
	  unsigned int i;
	  sprintf (p, "%s--%s%s",
		   !short_option (cs->c) ? "  " : ", ",
		   cs->long_name, longarg);
	  p += strlen (p);
	  for (i = 0; i < (sizeof (long_option_aliases) /
			   sizeof (long_option_aliases[0]));
	       ++i)
	    if (long_option_aliases[i].val == cs->c)
	      {
		sprintf (p, ", --%s%s",
			 long_option_aliases[i].name, longarg);
		p += strlen (p);
	      }
	}
      {
	const struct command_switch *ncs = cs;
	while ((++ncs)->c != '\0')
	  if (ncs->description
              && ncs->description[0] == '-'
              && ncs->description[1] == cs->c)
	    {
	      /* This is another switch that does the same
		 one as the one we are processing.  We want
		 to list them all together on one line.  */
	      sprintf (p, ", -%c%s", ncs->c, shortarg);
	      p += strlen (p);
	      if (ncs->long_name != 0)
		{
		  sprintf (p, ", --%s%s", ncs->long_name, longarg);
		  p += strlen (p);
		}
	    }
      }

      if (p - buf > DESCRIPTION_COLUMN - 2)
	/* The list of option names is too long to fit on the same
	   line with the description, leaving at least two spaces.
	   Print it on its own line instead.  */
	{
	  fprintf (usageto, "%s\n", buf);
	  buf[0] = '\0';
	}

      fprintf (usageto, "%*s%s.\n",
	       - DESCRIPTION_COLUMN,
	       buf, gettext (cs->description));
    }

  fprintf (usageto, _("\nReport bugs to <bug-make@gnu.org>.\n"));
}

/* Decode switches from ARGC and ARGV.
   They came from the environment if ENV is nonzero.  */

static void
decode_switches (argc, argv, env)
     int argc;
     char **argv;
     int env;
{
  int bad = 0;
  register const struct command_switch *cs;
  register struct stringlist *sl;
  register int c;

  /* getopt does most of the parsing for us.
     First, get its vectors set up.  */

  init_switches ();

  /* Let getopt produce error messages for the command line,
     but not for options from the environment.  */
  opterr = !env;
  /* Reset getopt's state.  */
  optind = 0;

  while (optind < argc)
    {
      /* Parse the next argument.  */
      c = getopt_long (argc, argv, options, long_options, (int *) 0);
      if (c == EOF)
	/* End of arguments, or "--" marker seen.  */
	break;
      else if (c == 1)
	/* An argument not starting with a dash.  */
	handle_non_switch_argument (optarg, env);
      else if (c == '?')
	/* Bad option.  We will print a usage message and die later.
	   But continue to parse the other options so the user can
	   see all he did wrong.  */
	bad = 1;
      else
	for (cs = switches; cs->c != '\0'; ++cs)
	  if (cs->c == c)
	    {
	      /* Whether or not we will actually do anything with
		 this switch.  We test this individually inside the
		 switch below rather than just once outside it, so that
		 options which are to be ignored still consume args.  */
	      int doit = !env || cs->env;

	      switch (cs->type)
		{
		default:
		  abort ();

		case ignore:
		  break;

		case flag:
		case flag_off:
		  if (doit)
		    *(int *) cs->value_ptr = cs->type == flag;
		  break;

		case string:
		  if (!doit)
		    break;

		  if (optarg == 0)
		    optarg = cs->noarg_value;

		  sl = *(struct stringlist **) cs->value_ptr;
		  if (sl == 0)
		    {
		      sl = (struct stringlist *)
			xmalloc (sizeof (struct stringlist));
		      sl->max = 5;
		      sl->idx = 0;
		      sl->list = (char **) xmalloc (5 * sizeof (char *));
		      *(struct stringlist **) cs->value_ptr = sl;
		    }
		  else if (sl->idx == sl->max - 1)
		    {
		      sl->max += 5;
		      sl->list = (char **)
			xrealloc ((char *) sl->list,
				  sl->max * sizeof (char *));
		    }
		  sl->list[sl->idx++] = optarg;
		  sl->list[sl->idx] = 0;
		  break;

		case positive_int:
		  if (optarg == 0 && argc > optind
                      && ISDIGIT (argv[optind][0]))
		    optarg = argv[optind++];

		  if (!doit)
		    break;

		  if (optarg != 0)
		    {
		      int i = atoi (optarg);
		      if (i < 1)
			{
			  if (doit)
			    error (NILF, _("the `-%c' option requires a positive integral argument"),
				   cs->c);
			  bad = 1;
			}
		      else
			*(unsigned int *) cs->value_ptr = i;
		    }
		  else
		    *(unsigned int *) cs->value_ptr
		      = *(unsigned int *) cs->noarg_value;
		  break;

#ifndef NO_FLOAT
		case floating:
		  if (optarg == 0 && optind < argc
		      && (ISDIGIT (argv[optind][0]) || argv[optind][0] == '.'))
		    optarg = argv[optind++];

		  if (doit)
		    *(double *) cs->value_ptr
		      = (optarg != 0 ? atof (optarg)
			 : *(double *) cs->noarg_value);

		  break;
#endif
		}

	      /* We've found the switch.  Stop looking.  */
	      break;
	    }
    }

  /* There are no more options according to getting getopt, but there may
     be some arguments left.  Since we have asked for non-option arguments
     to be returned in order, this only happens when there is a "--"
     argument to prevent later arguments from being options.  */
  while (optind < argc)
    handle_non_switch_argument (argv[optind++], env);


  if (!env && (bad || print_usage_flag))
    {
      print_usage (bad);
      die (bad ? 2 : 0);
    }
}

/* Decode switches from environment variable ENVAR (which is LEN chars long).
   We do this by chopping the value into a vector of words, prepending a
   dash to the first word if it lacks one, and passing the vector to
   decode_switches.  */

static void
decode_env_switches (envar, len)
     char *envar;
     unsigned int len;
{
  char *varref = (char *) alloca (2 + len + 2);
  char *value, *p;
  int argc;
  char **argv;

  /* Get the variable's value.  */
  varref[0] = '$';
  varref[1] = '(';
  bcopy (envar, &varref[2], len);
  varref[2 + len] = ')';
  varref[2 + len + 1] = '\0';
  value = variable_expand (varref);

  /* Skip whitespace, and check for an empty value.  */
  value = next_token (value);
  len = strlen (value);
  if (len == 0)
    return;

  /* Allocate a vector that is definitely big enough.  */
  argv = (char **) alloca ((1 + len + 1) * sizeof (char *));

  /* Allocate a buffer to copy the value into while we split it into words
     and unquote it.  We must use permanent storage for this because
     decode_switches may store pointers into the passed argument words.  */
  p = (char *) xmalloc (2 * len);

  /* getopt will look at the arguments starting at ARGV[1].
     Prepend a spacer word.  */
  argv[0] = 0;
  argc = 1;
  argv[argc] = p;
  while (*value != '\0')
    {
      if (*value == '\\' && value[1] != '\0')
	++value;		/* Skip the backslash.  */
      else if (isblank (*value))
	{
	  /* End of the word.  */
	  *p++ = '\0';
	  argv[++argc] = p;
	  do
	    ++value;
	  while (isblank (*value));
	  continue;
	}
      *p++ = *value++;
    }
  *p = '\0';
  argv[++argc] = 0;

  if (argv[1][0] != '-' && strchr (argv[1], '=') == 0)
    /* The first word doesn't start with a dash and isn't a variable
       definition.  Add a dash and pass it along to decode_switches.  We
       need permanent storage for this in case decode_switches saves
       pointers into the value.  */
    argv[1] = concat ("-", argv[1], "");

  /* Parse those words.  */
  decode_switches (argc, argv, 1);
}

/* Quote the string IN so that it will be interpreted as a single word with
   no magic by decode_env_switches; also double dollar signs to avoid
   variable expansion in make itself.  Write the result into OUT, returning
   the address of the next character to be written.
   Allocating space for OUT twice the length of IN is always sufficient.  */

static char *
quote_for_env (out, in)
     char *out, *in;
{
  while (*in != '\0')
    {
      if (*in == '$')
	*out++ = '$';
      else if (isblank (*in) || *in == '\\')
        *out++ = '\\';
      *out++ = *in++;
    }

  return out;
}

/* Define the MAKEFLAGS and MFLAGS variables to reflect the settings of the
   command switches.  Include options with args if ALL is nonzero.
   Don't include options with the `no_makefile' flag set if MAKEFILE.  */

static void
define_makeflags (all, makefile)
     int all, makefile;
{
  static const char ref[] = "$(MAKEOVERRIDES)";
  static const char posixref[] = "$(-*-command-variables-*-)";
  register const struct command_switch *cs;
  char *flagstring;
  register char *p;
  unsigned int words;
  struct variable *v;

  /* We will construct a linked list of `struct flag's describing
     all the flags which need to go in MAKEFLAGS.  Then, once we
     know how many there are and their lengths, we can put them all
     together in a string.  */

  struct flag
    {
      struct flag *next;
      const struct command_switch *cs;
      char *arg;
    };
  struct flag *flags = 0;
  unsigned int flagslen = 0;
#define	ADD_FLAG(ARG, LEN) \
  do {									      \
    struct flag *new = (struct flag *) alloca (sizeof (struct flag));	      \
    new->cs = cs;							      \
    new->arg = (ARG);							      \
    new->next = flags;							      \
    flags = new;							      \
    if (new->arg == 0)							      \
      ++flagslen;		/* Just a single flag letter.  */	      \
    else								      \
      flagslen += 1 + 1 + 1 + 1 + 3 * (LEN); /* " -x foo" */		      \
    if (!short_option (cs->c))						      \
      /* This switch has no single-letter version, so we use the long.  */    \
      flagslen += 2 + strlen (cs->long_name);				      \
  } while (0)

  for (cs = switches; cs->c != '\0'; ++cs)
    if (cs->toenv && (!makefile || !cs->no_makefile))
      switch (cs->type)
	{
	default:
	  abort ();

	case ignore:
	  break;

	case flag:
	case flag_off:
	  if (!*(int *) cs->value_ptr == (cs->type == flag_off)
	      && (cs->default_value == 0
		  || *(int *) cs->value_ptr != *(int *) cs->default_value))
	    ADD_FLAG (0, 0);
	  break;

	case positive_int:
	  if (all)
	    {
	      if ((cs->default_value != 0
		   && (*(unsigned int *) cs->value_ptr
		       == *(unsigned int *) cs->default_value)))
		break;
	      else if (cs->noarg_value != 0
		       && (*(unsigned int *) cs->value_ptr ==
			   *(unsigned int *) cs->noarg_value))
		ADD_FLAG ("", 0); /* Optional value omitted; see below.  */
	      else if (cs->c == 'j')
		/* Special case for `-j'.  */
		ADD_FLAG ("1", 1);
	      else
		{
		  char *buf = (char *) alloca (30);
		  sprintf (buf, "%u", *(unsigned int *) cs->value_ptr);
		  ADD_FLAG (buf, strlen (buf));
		}
	    }
	  break;

#ifndef NO_FLOAT
	case floating:
	  if (all)
	    {
	      if (cs->default_value != 0
		  && (*(double *) cs->value_ptr
		      == *(double *) cs->default_value))
		break;
	      else if (cs->noarg_value != 0
		       && (*(double *) cs->value_ptr
			   == *(double *) cs->noarg_value))
		ADD_FLAG ("", 0); /* Optional value omitted; see below.  */
	      else
		{
		  char *buf = (char *) alloca (100);
		  sprintf (buf, "%g", *(double *) cs->value_ptr);
		  ADD_FLAG (buf, strlen (buf));
		}
	    }
	  break;
#endif

	case string:
	  if (all)
	    {
	      struct stringlist *sl = *(struct stringlist **) cs->value_ptr;
	      if (sl != 0)
		{
		  /* Add the elements in reverse order, because
		     all the flags get reversed below; and the order
		     matters for some switches (like -I).  */
		  register unsigned int i = sl->idx;
		  while (i-- > 0)
		    ADD_FLAG (sl->list[i], strlen (sl->list[i]));
		}
	    }
	  break;
	}

  flagslen += 4 + sizeof posixref; /* Four more for the possible " -- ".  */

#undef	ADD_FLAG

  /* Construct the value in FLAGSTRING.
     We allocate enough space for a preceding dash and trailing null.  */
  flagstring = (char *) alloca (1 + flagslen + 1);
  bzero (flagstring, 1 + flagslen + 1);
  p = flagstring;
  words = 1;
  *p++ = '-';
  while (flags != 0)
    {
      /* Add the flag letter or name to the string.  */
      if (short_option (flags->cs->c))
	*p++ = flags->cs->c;
      else
	{
          if (*p != '-')
            {
              *p++ = ' ';
              *p++ = '-';
            }
	  *p++ = '-';
	  strcpy (p, flags->cs->long_name);
	  p += strlen (p);
	}
      if (flags->arg != 0)
	{
	  /* A flag that takes an optional argument which in this case is
	     omitted is specified by ARG being "".  We must distinguish
	     because a following flag appended without an intervening " -"
	     is considered the arg for the first.  */
	  if (flags->arg[0] != '\0')
	    {
	      /* Add its argument too.  */
	      *p++ = !short_option (flags->cs->c) ? '=' : ' ';
	      p = quote_for_env (p, flags->arg);
	    }
	  ++words;
	  /* Write a following space and dash, for the next flag.  */
	  *p++ = ' ';
	  *p++ = '-';
	}
      else if (!short_option (flags->cs->c))
	{
	  ++words;
	  /* Long options must each go in their own word,
	     so we write the following space and dash.  */
	  *p++ = ' ';
	  *p++ = '-';
	}
      flags = flags->next;
    }

  /* Define MFLAGS before appending variable definitions.  */

  if (p == &flagstring[1])
    /* No flags.  */
    flagstring[0] = '\0';
  else if (p[-1] == '-')
    {
      /* Kill the final space and dash.  */
      p -= 2;
      *p = '\0';
    }
  else
    /* Terminate the string.  */
    *p = '\0';

  /* Since MFLAGS is not parsed for flags, there is no reason to
     override any makefile redefinition.  */
  (void) define_variable ("MFLAGS", 6, flagstring, o_env, 1);

  if (all && command_variables != 0)
    {
      /* Now write a reference to $(MAKEOVERRIDES), which contains all the
	 command-line variable definitions.  */

      if (p == &flagstring[1])
	/* No flags written, so elide the leading dash already written.  */
	p = flagstring;
      else
	{
	  /* Separate the variables from the switches with a "--" arg.  */
	  if (p[-1] != '-')
	    {
	      /* We did not already write a trailing " -".  */
	      *p++ = ' ';
	      *p++ = '-';
	    }
	  /* There is a trailing " -"; fill it out to " -- ".  */
	  *p++ = '-';
	  *p++ = ' ';
	}

      /* Copy in the string.  */
      if (posix_pedantic)
	{
	  bcopy (posixref, p, sizeof posixref - 1);
	  p += sizeof posixref - 1;
	}
      else
	{
	  bcopy (ref, p, sizeof ref - 1);
	  p += sizeof ref - 1;
	}
    }
  else if (p == &flagstring[1])
    {
      words = 0;
      --p;
    }
  else if (p[-1] == '-')
    /* Kill the final space and dash.  */
    p -= 2;
  /* Terminate the string.  */
  *p = '\0';

  v = define_variable ("MAKEFLAGS", 9,
		       /* If there are switches, omit the leading dash
			  unless it is a single long option with two
			  leading dashes.  */
		       &flagstring[(flagstring[0] == '-'
				    && flagstring[1] != '-')
				   ? 1 : 0],
		       /* This used to use o_env, but that lost when a
			  makefile defined MAKEFLAGS.  Makefiles set
			  MAKEFLAGS to add switches, but we still want
			  to redefine its value with the full set of
			  switches.  Of course, an override or command
			  definition will still take precedence.  */
		       o_file, 1);
  if (! all)
    /* The first time we are called, set MAKEFLAGS to always be exported.
       We should not do this again on the second call, because that is
       after reading makefiles which might have done `unexport MAKEFLAGS'. */
    v->export = v_export;
}

/* Print version information.  */

static void
print_version ()
{
  extern char *make_host;
  static int printed_version = 0;

  char *precede = print_data_base_flag ? "# " : "";

  if (printed_version)
    /* Do it only once.  */
    return;

  printf ("%sGNU Make version %s", precede, version_string);
  if (remote_description != 0 && *remote_description != '\0')
    printf ("-%s", remote_description);

  printf (_(", by Richard Stallman and Roland McGrath.\n\
%sBuilt for %s\n\
%sCopyright (C) 1988, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99\n\
%s\tFree Software Foundation, Inc.\n\
%sThis is free software; see the source for copying conditions.\n\
%sThere is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
%sPARTICULAR PURPOSE.\n\n\
%sReport bugs to <bug-make@gnu.org>.\n\n"),
          precede, make_host,
	  precede, precede, precede, precede, precede, precede);

  printed_version = 1;

  /* Flush stdout so the user doesn't have to wait to see the
     version information while things are thought about.  */
  fflush (stdout);
}

/* Print a bunch of information about this and that.  */

static void
print_data_base ()
{
  time_t when;

  when = time ((time_t *) 0);
  printf ("\n# Make data base, printed on 00:00 Jan 01 2000");
  /* printf (_("\n# Make data base, printed on %s"), ctime (&when)); */

  print_variable_data_base ();
  print_dir_data_base ();
  print_rule_data_base ();
  print_file_data_base ();
  print_vpath_data_base ();

  when = time ((time_t *) 0);
  printf ("\n# Make data base, printed on 00:00 Jan 01 2000\n");
  /* printf (_("\n# Finished Make data base on %s\n"), ctime (&when)); */
}

/* Exit with STATUS, cleaning up as necessary.  */

void
die (status)
     int status;
{
  static char dying = 0;

  if (!dying)
    {
      int err;

      dying = 1;

      if (print_version_flag)
	print_version ();

      /* Wait for children to die.  */
      for (err = (status != 0); job_slots_used > 0; err = 0)
	reap_children (1, err);

      /* Let the remote job module clean up its state.  */
      remote_cleanup ();

      /* Remove the intermediate files.  */
      remove_intermediates (0);

      if (print_data_base_flag)
	print_data_base ();

      /* Try to move back to the original directory.  This is essential on
	 MS-DOS (where there is really only one process), and on Unix it
	 puts core files in the original directory instead of the -C
	 directory.  Must wait until after remove_intermediates(), or unlinks
         of relative pathnames fail.  */
      if (directory_before_chdir != 0)
	chdir (directory_before_chdir);

      log_working_directory (0);
    }

  exit (status);
}

/* Write a message indicating that we've just entered or
   left (according to ENTERING) the current directory.  */

void
log_working_directory (entering)
     int entering;
{
  static int entered = 0;
  char *msg = entering ? _("Entering") : _("Leaving");

  /* Print nothing without the flag.  Don't print the entering message
     again if we already have.  Don't print the leaving message if we
     haven't printed the entering message.  */
  if (! print_directory_flag || entering == entered)
    return;

  entered = entering;

  if (print_data_base_flag)
    fputs ("# ", stdout);

  if (makelevel == 0)
    printf ("%s: %s ", program, msg);
  else
    printf ("%s[%u]: %s ", program, makelevel, msg);

  if (starting_directory == 0)
    puts (_("an unknown directory"));
  else
    printf (_("directory `%s'\n"), starting_directory);
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         misc.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Miscellaneous generic support functions for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"
#undef stderr
#define stderr stdout

/* Variadic functions.  We go through contortions to allow proper function
   prototypes for both ANSI and pre-ANSI C compilers, and also for those
   which support stdarg.h vs. varargs.h, and finally those which have
   vfprintf(), etc. and those who have _doprnt... or nothing.

   This fancy stuff all came from GNU fileutils, except for the VA_PRINTF and
   VA_END macros used here since we have multiple print functions.  */

#if HAVE_VPRINTF || HAVE_DOPRNT
# define HAVE_STDVARARGS 1
# if __STDC__
#  include <stdarg.h>
#undef stderr
#define stderr stdout
#  define VA_START(args, lastarg) va_start(args, lastarg)
# else
#  include <varargs.h>
#undef stderr
#define stderr stdout
#  define VA_START(args, lastarg) va_start(args)
# endif
# if HAVE_VPRINTF
#  define VA_PRINTF(fp, lastarg, args) vfprintf((fp), (lastarg), (args))
# else
#  define VA_PRINTF(fp, lastarg, args) _doprnt((lastarg), (args), (fp))
# endif
# define VA_END(args) va_end(args)
#else
/* # undef HAVE_STDVARARGS */
# define va_alist a1, a2, a3, a4, a5, a6, a7, a8
# define va_dcl char *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
# define VA_START(args, lastarg)
# define VA_END(args)
#endif


/* Compare strings *S1 and *S2.
   Return negative if the first is less, positive if it is greater,
   zero if they are equal.  */

int
alpha_compare (v1, v2)
     const void *v1, *v2;
{
  const char *s1 = *((char **)v1);
  const char *s2 = *((char **)v2);

  if (*s1 != *s2)
    return *s1 - *s2;
  return strcmp (s1, s2);
}

/* Discard each backslash-newline combination from LINE.
   Backslash-backslash-newline combinations become backslash-newlines.
   This is done by copying the text at LINE into itself.  */

void
collapse_continuations (line)
     char *line;
{
  register char *in, *out, *p;
  register int backslash;
  register unsigned int bs_write;

  in = strchr (line, '\n');
  if (in == 0)
    return;

  out = in;
  while (out > line && out[-1] == '\\')
    --out;

  while (*in != '\0')
    {
      /* BS_WRITE gets the number of quoted backslashes at
	 the end just before IN, and BACKSLASH gets nonzero
	 if the next character is quoted.  */
      backslash = 0;
      bs_write = 0;
      for (p = in - 1; p >= line && *p == '\\'; --p)
	{
	  if (backslash)
	    ++bs_write;
	  backslash = !backslash;

	  /* It should be impossible to go back this far without exiting,
	     but if we do, we can't get the right answer.  */
	  if (in == out - 1)
	    abort ();
	}

      /* Output the appropriate number of backslashes.  */
      while (bs_write-- > 0)
	*out++ = '\\';

      /* Skip the newline.  */
      ++in;

      /* If the newline is quoted, discard following whitespace
	 and any preceding whitespace; leave just one space.  */
      if (backslash)
	{
	  in = next_token (in);
	  while (out > line && isblank (out[-1]))
	    --out;
	  *out++ = ' ';
	}
      else
	/* If the newline isn't quoted, put it in the output.  */
	*out++ = '\n';

      /* Now copy the following line to the output.
	 Stop when we find backslashes followed by a newline.  */
      while (*in != '\0')
	if (*in == '\\')
	  {
	    p = in + 1;
	    while (*p == '\\')
	      ++p;
	    if (*p == '\n')
	      {
		in = p;
		break;
	      }
	    while (in < p)
	      *out++ = *in++;
	  }
	else
	  *out++ = *in++;
    }

  *out = '\0';
}


/* Remove comments from LINE.
   This is done by copying the text at LINE onto itself.  */

void
remove_comments (line)
     char *line;
{
  char *comment;

  comment = find_char_unquote (line, "#", 0);

  if (comment != 0)
    /* Cut off the line at the #.  */
    *comment = '\0';
}

/* Print N spaces (used in debug for target-depth).  */

void
print_spaces (n)
     register unsigned int n;
{
  while (n-- > 0)
    putchar (' ');
}


/* Return a newly-allocated string whose contents
   concatenate those of s1, s2, s3.  */

char *
concat (s1, s2, s3)
     register char *s1, *s2, *s3;
{
  register unsigned int len1, len2, len3;
  register char *result;

  len1 = *s1 != '\0' ? strlen (s1) : 0;
  len2 = *s2 != '\0' ? strlen (s2) : 0;
  len3 = *s3 != '\0' ? strlen (s3) : 0;

  result = (char *) xmalloc (len1 + len2 + len3 + 1);

  if (*s1 != '\0')
    bcopy (s1, result, len1);
  if (*s2 != '\0')
    bcopy (s2, result + len1, len2);
  if (*s3 != '\0')
    bcopy (s3, result + len1 + len2, len3);
  *(result + len1 + len2 + len3) = '\0';

  return result;
}

/* Print a message on stdout.  */

void
#if __STDC__ && HAVE_STDVARARGS
message (int prefix, const char *fmt, ...)
#else
message (prefix, fmt, va_alist)
     int prefix;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (fmt != 0)
    {
      if (prefix)
	{
	  if (makelevel == 0)
	    printf ("%s: ", program);
	  else
	    printf ("%s[%u]: ", program, makelevel);
	}
      VA_START (args, fmt);
      VA_PRINTF (stdout, fmt, args);
      VA_END (args);
      putchar ('\n');
    }

  fflush (stdout);
}

/* Print an error message.  */

void
#if __STDC__ && HAVE_STDVARARGS
error (const struct floc *flocp, const char *fmt, ...)
#else
error (flocp, fmt, va_alist)
     const struct floc *flocp;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (flocp && flocp->filenm)
    fprintf (stderr, "%s:%lu: ", flocp->filenm, flocp->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: ", program);
  else
    fprintf (stderr, "%s[%u]: ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  putc ('\n', stderr);
  fflush (stderr);
}

/* Print an error message and exit.  */

void
#if __STDC__ && HAVE_STDVARARGS
fatal (const struct floc *flocp, const char *fmt, ...)
#else
fatal (flocp, fmt, va_alist)
     const struct floc *flocp;
     const char *fmt;
     va_dcl
#endif
{
#if HAVE_STDVARARGS
  va_list args;
#endif

  log_working_directory (1);

  if (flocp && flocp->filenm)
    fprintf (stderr, "%s:%lu: *** ", flocp->filenm, flocp->lineno);
  else if (makelevel == 0)
    fprintf (stderr, "%s: *** ", program);
  else
    fprintf (stderr, "%s[%u]: *** ", program, makelevel);

  VA_START(args, fmt);
  VA_PRINTF (stderr, fmt, args);
  VA_END (args);

  fputs (_(".  Stop.\n"), stderr);

  die (2);
}

#ifndef HAVE_STRERROR

#undef	strerror

char *
strerror (errnum)
     int errnum;
{
  extern int errno, sys_nerr;
#ifndef __DECC
  extern char *sys_errlist[];
#endif
  static char buf[] = "Unknown error 12345678901234567890";

  if (errno < sys_nerr)
    return sys_errlist[errnum];

  sprintf (buf, _("Unknown error %d"), errnum);
  return buf;
}
#endif

/* Print an error message from errno.  */

void
perror_with_name (str, name)
     char *str, *name;
{
  error (NILF, "%s%s: %s", str, name, strerror (errno));
}

/* Print an error message from errno and exit.  */

void
pfatal_with_name (name)
     char *name;
{
  fatal (NILF, "%s: %s", name, strerror (errno));

  /* NOTREACHED */
}

/* Like malloc but get fatal error if memory is exhausted.  */
/* Don't bother if we're using dmalloc; it provides these for us.  */

#ifndef HAVE_DMALLOC_H

#undef xmalloc
#undef xrealloc
#undef xstrdup

char *
xmalloc (size)
     unsigned int size;
{
  char *result = (char *) malloc (size);
  if (result == 0)
    fatal (NILF, _("virtual memory exhausted"));
  return result;
}


char *
xrealloc (ptr, size)
     char *ptr;
     unsigned int size;
{
  char *result;

  /* Some older implementations of realloc() don't conform to ANSI.  */
  result = ptr ? realloc (ptr, size) : malloc (size);
  if (result == 0)
    fatal (NILF, _("virtual memory exhausted"));
  return result;
}


char *
xstrdup (ptr)
     const char *ptr;
{
  char *result;

#ifdef HAVE_STRDUP
  result = strdup (ptr);
#else
  result = (char *) malloc (strlen (ptr) + 1);
#endif

  if (result == 0)
    fatal (NILF, _("virtual memory exhausted"));

#ifdef HAVE_STRDUP
  return result;
#else
  return strcpy(result, ptr);
#endif
}

#endif  /* HAVE_DMALLOC_H */

char *
savestring (str, length)
     const char *str;
     unsigned int length;
{
  register char *out = (char *) xmalloc (length + 1);
  if (length > 0)
    bcopy (str, out, length);
  out[length] = '\0';
  return out;
}

/* Search string BIG (length BLEN) for an occurrence of
   string SMALL (length SLEN).  Return a pointer to the
   beginning of the first occurrence, or return nil if none found.  */

char *
sindex (big, blen, small, slen)
     const char *big;
     unsigned int blen;
     const char *small;
     unsigned int slen;
{
  if (!blen)
    blen = strlen (big);
  if (!slen)
    slen = strlen (small);

  if (slen && blen >= slen)
    {
      register unsigned int b;

      /* Quit when there's not enough room left for the small string.  */
      --slen;
      blen -= slen;

      for (b = 0; b < blen; ++b, ++big)
        if (*big == *small && strneq (big + 1, small + 1, slen))
          return (char *)big;
    }

  return 0;
}

/* Limited INDEX:
   Search through the string STRING, which ends at LIMIT, for the character C.
   Returns a pointer to the first occurrence, or nil if none is found.
   Like INDEX except that the string searched ends where specified
   instead of at the first null.  */

char *
lindex (s, limit, c)
     register const char *s, *limit;
     int c;
{
  while (s < limit)
    if (*s++ == c)
      return (char *)(s - 1);

  return 0;
}

/* Return the address of the first whitespace or null in the string S.  */

char *
end_of_token (s)
     char *s;
{
  while (*s != '\0' && !isblank (*s))
    ++s;
  return s;
}

#ifdef WINDOWS32
/*
 * Same as end_of_token, but take into account a stop character
 */
char *
end_of_token_w32 (s, stopchar)
     char *s;
     char stopchar;
{
  register char *p = s;
  register int backslash = 0;

  while (*p != '\0' && *p != stopchar && (backslash || !isblank (*p)))
    {
      if (*p++ == '\\')
        {
          backslash = !backslash;
          while (*p == '\\')
            {
              backslash = !backslash;
              ++p;
            }
        }
      else
        backslash = 0;
    }

  return p;
}
#endif

/* Return the address of the first nonwhitespace or null in the string S.  */

char *
next_token (s)
     char *s;
{
  register char *p = s;

  while (isblank (*p))
    ++p;
  return p;
}

/* Find the next token in PTR; return the address of it, and store the
   length of the token into *LENGTHPTR if LENGTHPTR is not nil.  */

char *
find_next_token (ptr, lengthptr)
     char **ptr;
     unsigned int *lengthptr;
{
  char *p = next_token (*ptr);
  char *end;

  if (*p == '\0')
    return 0;

  *ptr = end = end_of_token (p);
  if (lengthptr != 0)
    *lengthptr = end - p;
  return p;
}

/* Copy a chain of `struct dep', making a new chain
   with the same contents as the old one.  */

struct dep *
copy_dep_chain (d)
     register struct dep *d;
{
  register struct dep *c;
  struct dep *firstnew = 0;
  struct dep *lastnew = 0;

  while (d != 0)
    {
      c = (struct dep *) xmalloc (sizeof (struct dep));
      bcopy ((char *) d, (char *) c, sizeof (struct dep));
      if (c->name != 0)
	c->name = xstrdup (c->name);
      c->next = 0;
      if (firstnew == 0)
	firstnew = lastnew = c;
      else
    lastnew = lastnew->next = c;
      d = d->next;
    }

  return firstnew;
}

#ifdef	iAPX286
/* The losing compiler on this machine can't handle this macro.  */

char *
dep_name (dep)
     struct dep *dep;
{
  return dep->name == 0 ? dep->file->name : dep->name;
}
#endif

#ifdef	GETLOADAVG_PRIVILEGED

#ifdef POSIX

/* Hopefully if a system says it's POSIX.1 and has the setuid and setgid
   functions, they work as POSIX.1 says.  Some systems (Alpha OSF/1 1.2,
   for example) which claim to be POSIX.1 also have the BSD setreuid and
   setregid functions, but they don't work as in BSD and only the POSIX.1
   way works.  */

#undef HAVE_SETREUID
#undef HAVE_SETREGID

#else	/* Not POSIX.  */

/* Some POSIX.1 systems have the seteuid and setegid functions.  In a
   POSIX-like system, they are the best thing to use.  However, some
   non-POSIX systems have them too but they do not work in the POSIX style
   and we must use setreuid and setregid instead.  */

#undef HAVE_SETEUID
#undef HAVE_SETEGID

#endif	/* POSIX.  */

#ifndef	HAVE_UNISTD_H
extern int getuid (), getgid (), geteuid (), getegid ();
extern int setuid (), setgid ();
#ifdef HAVE_SETEUID
extern int seteuid ();
#else
#ifdef	HAVE_SETREUID
extern int setreuid ();
#endif	/* Have setreuid.  */
#endif	/* Have seteuid.  */
#ifdef HAVE_SETEGID
extern int setegid ();
#else
#ifdef	HAVE_SETREGID
extern int setregid ();
#endif	/* Have setregid.  */
#endif	/* Have setegid.  */
#endif	/* No <unistd.h>.  */

/* Keep track of the user and group IDs for user- and make- access.  */
static int user_uid = -1, user_gid = -1, make_uid = -1, make_gid = -1;
#define	access_inited	(user_uid != -1)
static enum { make, user } current_access;


/* Under -d, write a message describing the current IDs.  */

static void
log_access (flavor)
     char *flavor;
{
  if (! ISDB (DB_JOBS))
    return;

  /* All the other debugging messages go to stdout,
     but we write this one to stderr because it might be
     run in a child fork whose stdout is piped.  */

  fprintf (stderr, _("%s access: user %lu (real %lu), group %lu (real %lu)\n"),
	   flavor, (unsigned long) geteuid (), (unsigned long) getuid (),
           (unsigned long) getegid (), (unsigned long) getgid ());
  fflush (stderr);
}


static void
init_access ()
{
#ifndef VMS
  user_uid = getuid ();
  user_gid = getgid ();

  make_uid = geteuid ();
  make_gid = getegid ();

  /* Do these ever fail?  */
  if (user_uid == -1 || user_gid == -1 || make_uid == -1 || make_gid == -1)
    pfatal_with_name ("get{e}[gu]id");

  log_access (_("Initialized"));

  current_access = make;
#endif
}

#endif	/* GETLOADAVG_PRIVILEGED */

/* Give the process appropriate permissions for access to
   user data (i.e., to stat files, or to spawn a child process).  */
void
user_access ()
{
#ifdef	GETLOADAVG_PRIVILEGED

  if (!access_inited)
    init_access ();

  if (current_access == user)
    return;

  /* We are in "make access" mode.  This means that the effective user and
     group IDs are those of make (if it was installed setuid or setgid).
     We now want to set the effective user and group IDs to the real IDs,
     which are the IDs of the process that exec'd make.  */

#ifdef	HAVE_SETEUID

  /* Modern systems have the seteuid/setegid calls which set only the
     effective IDs, which is ideal.  */

  if (seteuid (user_uid) < 0)
    pfatal_with_name ("user_access: seteuid");

#else	/* Not HAVE_SETEUID.  */

#ifndef	HAVE_SETREUID

  /* System V has only the setuid/setgid calls to set user/group IDs.
     There is an effective ID, which can be set by setuid/setgid.
     It can be set (unless you are root) only to either what it already is
     (returned by geteuid/getegid, now in make_uid/make_gid),
     the real ID (return by getuid/getgid, now in user_uid/user_gid),
     or the saved set ID (what the effective ID was before this set-ID
     executable (make) was exec'd).  */

  if (setuid (user_uid) < 0)
    pfatal_with_name ("user_access: setuid");

#else	/* HAVE_SETREUID.  */

  /* In 4BSD, the setreuid/setregid calls set both the real and effective IDs.
     They may be set to themselves or each other.  So you have two alternatives
     at any one time.  If you use setuid/setgid, the effective will be set to
     the real, leaving only one alternative.  Using setreuid/setregid, however,
     you can toggle between your two alternatives by swapping the values in a
     single setreuid or setregid call.  */

  if (setreuid (make_uid, user_uid) < 0)
    pfatal_with_name ("user_access: setreuid");

#endif	/* Not HAVE_SETREUID.  */
#endif	/* HAVE_SETEUID.  */

#ifdef	HAVE_SETEGID
  if (setegid (user_gid) < 0)
    pfatal_with_name ("user_access: setegid");
#else
#ifndef	HAVE_SETREGID
  if (setgid (user_gid) < 0)
    pfatal_with_name ("user_access: setgid");
#else
  if (setregid (make_gid, user_gid) < 0)
    pfatal_with_name ("user_access: setregid");
#endif
#endif

  current_access = user;

  log_access ("User");

#endif	/* GETLOADAVG_PRIVILEGED */
}

/* Give the process appropriate permissions for access to
   make data (i.e., the load average).  */
void
make_access ()
{
#ifdef	GETLOADAVG_PRIVILEGED

  if (!access_inited)
    init_access ();

  if (current_access == make)
    return;

  /* See comments in user_access, above.  */

#ifdef	HAVE_SETEUID
  if (seteuid (make_uid) < 0)
    pfatal_with_name ("make_access: seteuid");
#else
#ifndef	HAVE_SETREUID
  if (setuid (make_uid) < 0)
    pfatal_with_name ("make_access: setuid");
#else
  if (setreuid (user_uid, make_uid) < 0)
    pfatal_with_name ("make_access: setreuid");
#endif
#endif

#ifdef	HAVE_SETEGID
  if (setegid (make_gid) < 0)
    pfatal_with_name ("make_access: setegid");
#else
#ifndef	HAVE_SETREGID
  if (setgid (make_gid) < 0)
    pfatal_with_name ("make_access: setgid");
#else
  if (setregid (user_gid, make_gid) < 0)
    pfatal_with_name ("make_access: setregid");
#endif
#endif

  current_access = make;

  log_access ("Make");

#endif	/* GETLOADAVG_PRIVILEGED */
}

/* Give the process appropriate permissions for a child process.
   This is like user_access, but you can't get back to make_access.  */
void
child_access ()
{
#ifdef	GETLOADAVG_PRIVILEGED

  if (!access_inited)
    abort ();

  /* Set both the real and effective UID and GID to the user's.
     They cannot be changed back to make's.  */

#ifndef	HAVE_SETREUID
  if (setuid (user_uid) < 0)
    pfatal_with_name ("child_access: setuid");
#else
  if (setreuid (user_uid, user_uid) < 0)
    pfatal_with_name ("child_access: setreuid");
#endif

#ifndef	HAVE_SETREGID
  if (setgid (user_gid) < 0)
    pfatal_with_name ("child_access: setgid");
#else
  if (setregid (user_gid, user_gid) < 0)
    pfatal_with_name ("child_access: setregid");
#endif

  log_access ("Child");

#endif	/* GETLOADAVG_PRIVILEGED */
}

#ifdef NEED_GET_PATH_MAX
unsigned int
get_path_max ()
{
  static unsigned int value;

  if (value == 0)
    {
      long int x = pathconf ("/", _PC_PATH_MAX);
      if (x > 0)
	value = x;
      else
	return MAXPATHLEN;
    }

  return value;
}
#endif



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         read.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Reading and parsing of makefiles for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */

#include <assert.h>

#include <glob.h>

/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"
#undef stderr
#define stderr stdout


#ifndef WINDOWS32
#ifndef _AMIGA
#ifndef VMS
#include <pwd.h>
#undef stderr
#define stderr stdout
#else
struct passwd *getpwnam PARAMS ((char *name));
#endif
#endif
#endif /* !WINDOWS32 */

/* A `struct linebuffer' is a structure which holds a line of text.
   `readline' reads a line from a stream into a linebuffer
   and works regardless of the length of the line.  */

struct linebuffer
  {
    /* Note:  This is the number of bytes malloc'ed for `buffer'
       It does not indicate `buffer's real length.
       Instead, a null char indicates end-of-string.  */
    unsigned int size;
    char *buffer;
  };

#define initbuffer(lb) (lb)->buffer = (char *) xmalloc ((lb)->size = 200)
#define freebuffer(lb) free ((lb)->buffer)


/* Types of "words" that can be read in a makefile.  */
enum make_word_type
  {
     w_bogus, w_eol, w_static, w_variable, w_colon, w_dcolon, w_semicolon,
     w_comment, w_varassign
  };


/* A `struct conditionals' contains the information describing
   all the active conditionals in a makefile.

   The global variable `conditionals' contains the conditionals
   information for the current makefile.  It is initialized from
   the static structure `toplevel_conditionals' and is later changed
   to new structures for included makefiles.  */

struct conditionals
  {
    unsigned int if_cmds;	/* Depth of conditional nesting.  */
    unsigned int allocated;	/* Elts allocated in following arrays.  */
    char *ignoring;		/* Are we ignoring or interepreting?  */
    char *seen_else;		/* Have we already seen an `else'?  */
  };

static struct conditionals toplevel_conditionals;
static struct conditionals *conditionals = &toplevel_conditionals;


/* Default directories to search for include files in  */

static char *default_include_directories[] =
  {
#if defined(WINDOWS32) && !defined(INCLUDEDIR)
/*
 * This completly up to the user when they install MSVC or other packages.
 * This is defined as a placeholder.
 */
#define INCLUDEDIR "."
#endif
    INCLUDEDIR,
#ifndef _AMIGA
    "/usr/gnu/include",
    "/usr/local/include",
    "/usr/include",
#endif
    0
  };

/* List of directories to search for include files in  */

static char **include_directories_to_search;

/* Maximum length of an element of the above.  */

static unsigned int max_incl_len;

/* The filename and pointer to line number of the
   makefile currently being read in.  */

const struct floc *reading_file;

/* The chain of makefiles read by read_makefile.  */

static struct dep *read_makefiles = 0;

static int read_makefile PARAMS ((char *filename, int flags));
static unsigned long readline PARAMS ((struct linebuffer *linebuffer,
                                       FILE *stream, const struct floc *flocp));
static void do_define PARAMS ((char *name, unsigned int namelen,
                               enum variable_origin origin, FILE *infile,
                               struct floc *flocp));
static int conditional_line PARAMS ((char *line, const struct floc *flocp));
static void record_files PARAMS ((struct nameseq *filenames, char *pattern, char *pattern_percent,
			struct dep *deps, unsigned int cmds_started, char *commands,
			unsigned int commands_idx, int two_colon,
			const struct floc *flocp, int set_default));
static void record_target_var PARAMS ((struct nameseq *filenames, char *defn,
                                       int two_colon,
                                       enum variable_origin origin,
                                       const struct floc *flocp));
static enum make_word_type get_next_mword PARAMS ((char *buffer, char *delim,
                        char **startp, unsigned int *length));

/* Read in all the makefiles and return the chain of their names.  */

struct dep *
read_all_makefiles (makefiles)
     char **makefiles;
{
  unsigned int num_makefiles = 0;

  DB (DB_BASIC, (_("Reading makefiles...\n")));

  /* If there's a non-null variable MAKEFILES, its value is a list of
     files to read first thing.  But don't let it prevent reading the
     default makefiles and don't let the default goal come from there.  */

  {
    char *value;
    char *name, *p;
    unsigned int length;

    {
      /* Turn off --warn-undefined-variables while we expand MAKEFILES.  */
      int save = warn_undefined_variables_flag;
      warn_undefined_variables_flag = 0;

      value = allocated_variable_expand ("$(MAKEFILES)");

      warn_undefined_variables_flag = save;
    }

    /* Set NAME to the start of next token and LENGTH to its length.
       MAKEFILES is updated for finding remaining tokens.  */
    p = value;

    while ((name = find_next_token (&p, &length)) != 0)
      {
	if (*p != '\0')
	  *p++ = '\0';
        name = xstrdup (name);
	if (read_makefile (name,
                           RM_NO_DEFAULT_GOAL|RM_INCLUDED|RM_DONTCARE) < 2)
          free (name);
      }

    free (value);
  }

  /* Read makefiles specified with -f switches.  */

  if (makefiles != 0)
    while (*makefiles != 0)
      {
	struct dep *tail = read_makefiles;
	register struct dep *d;

	if (! read_makefile (*makefiles, 0))
	  perror_with_name ("", *makefiles);

	/* Find the right element of read_makefiles.  */
	d = read_makefiles;
	while (d->next != tail)
	  d = d->next;

	/* Use the storage read_makefile allocates.  */
	*makefiles = dep_name (d);
	++num_makefiles;
	++makefiles;
      }

  /* If there were no -f switches, try the default names.  */

  if (num_makefiles == 0)
    {
      static char *default_makefiles[] =
#ifdef VMS
	/* all lower case since readdir() (the vms version) 'lowercasifies' */
	{ "makefile.vms", "gnumakefile.", "makefile.", 0 };
#else
#ifdef _AMIGA
	{ "GNUmakefile", "Makefile", "SMakefile", 0 };
#else /* !Amiga && !VMS */
	{ "GNUmakefile", "makefile", "Makefile", 0 };
#endif /* AMIGA */
#endif /* VMS */
      register char **p = default_makefiles;
      while (*p != 0 && !file_exists_p (*p))
	++p;

      if (*p != 0)
	{
	  if (! read_makefile (*p, 0))
	    perror_with_name ("", *p);
	}
      else
	{
	  /* No default makefile was found.  Add the default makefiles to the
	     `read_makefiles' chain so they will be updated if possible.  */
	  struct dep *tail = read_makefiles;
	  /* Add them to the tail, after any MAKEFILES variable makefiles.  */
	  while (tail != 0 && tail->next != 0)
	    tail = tail->next;
	  for (p = default_makefiles; *p != 0; ++p)
	    {
	      struct dep *d = (struct dep *) xmalloc (sizeof (struct dep));
	      d->name = 0;
	      d->file = enter_file (*p);
	      d->file->dontcare = 1;
	      /* Tell update_goal_chain to bail out as soon as this file is
		 made, and main not to die if we can't make this file.  */
	      d->changed = RM_DONTCARE;
	      if (tail == 0)
		read_makefiles = d;
	      else
		tail->next = d;
	      tail = d;
	    }
	  if (tail != 0)
	    tail->next = 0;
	}
    }

  return read_makefiles;
}

/* Read file FILENAME as a makefile and add its contents to the data base.

   FLAGS contains bits as above.

   FILENAME is added to the `read_makefiles' chain.

   Returns 0 if a file was not found or not read.
   Returns 1 if FILENAME was found and read.
   Returns 2 if FILENAME was read, and we kept a reference (don't free it).  */

static int
read_makefile (filename, flags)
     char *filename;
     int flags;
{
  static char *collapsed = 0;
  static unsigned int collapsed_length = 0;
  register FILE *infile;
  struct linebuffer lb;
  unsigned int commands_len = 200;
  char *commands;
  unsigned int commands_idx = 0;
  unsigned int cmds_started, tgts_started;
  char *p;
  char *p2;
  int len, reading_target;
  int ignoring = 0, in_ignored_define = 0;
  int no_targets = 0;		/* Set when reading a rule without targets.  */
  int using_filename = 0;
  struct floc fileinfo;
  char *passed_filename = filename;

  struct nameseq *filenames = 0;
  struct dep *deps;
  unsigned int nlines = 0;
  int two_colon = 0;
  char *pattern = 0, *pattern_percent;

  int makefile_errno;
#if defined (WINDOWS32) || defined (__MSDOS__)
  int check_again;
#endif

#define record_waiting_files()						      \
  do									      \
    { 									      \
      if (filenames != 0)						      \
        {                                                                     \
	  struct floc fi;                                                     \
	  fi.filenm = fileinfo.filenm;                                        \
	  fi.lineno = tgts_started;                                           \
	  record_files (filenames, pattern, pattern_percent, deps,            \
                        cmds_started, commands, commands_idx, two_colon,      \
                        &fi, !(flags & RM_NO_DEFAULT_GOAL));                  \
          using_filename |= commands_idx > 0;                                 \
        }                                                                     \
      filenames = 0;							      \
      commands_idx = 0;							      \
      if (pattern) { free(pattern); pattern = 0; }                            \
    } while (0)

  fileinfo.filenm = filename;
  fileinfo.lineno = 1;

  pattern_percent = 0;
  cmds_started = tgts_started = fileinfo.lineno;

  if (ISDB (DB_VERBOSE))
    {
      printf (_("Reading makefile `%s'"), fileinfo.filenm);
      if (flags & RM_NO_DEFAULT_GOAL)
	printf (_(" (no default goal)"));
      if (flags & RM_INCLUDED)
	printf (_(" (search path)"));
      if (flags & RM_DONTCARE)
	printf (_(" (don't care)"));
      if (flags & RM_NO_TILDE)
	printf (_(" (no ~ expansion)"));
      puts ("...");
    }

  /* First, get a stream to read.  */

  /* Expand ~ in FILENAME unless it came from `include',
     in which case it was already done.  */
  if (!(flags & RM_NO_TILDE) && filename[0] == '~')
    {
      char *expanded = tilde_expand (filename);
      if (expanded != 0)
	filename = expanded;
    }

  infile = fopen (filename, "r");
  /* Save the error code so we print the right message later.  */
  makefile_errno = errno;

  /* If the makefile wasn't found and it's either a makefile from
     the `MAKEFILES' variable or an included makefile,
     search the included makefile search path for this makefile.  */
  if (infile == 0 && (flags & RM_INCLUDED) && *filename != '/')
    {
      register unsigned int i;
      for (i = 0; include_directories_to_search[i] != 0; ++i)
	{
	  char *name = concat (include_directories_to_search[i], "/", filename);
	  infile = fopen (name, "r");
	  if (infile == 0)
	    free (name);
	  else
	    {
	      filename = name;
	      break;
	    }
	}
    }

  /* Add FILENAME to the chain of read makefiles.  */
  deps = (struct dep *) xmalloc (sizeof (struct dep));
  deps->next = read_makefiles;
  read_makefiles = deps;
  deps->name = 0;
  deps->file = lookup_file (filename);
  if (deps->file == 0)
    {
      deps->file = enter_file (xstrdup (filename));
      if (flags & RM_DONTCARE)
	deps->file->dontcare = 1;
    }
  if (filename != passed_filename)
    free (filename);
  filename = deps->file->name;
  deps->changed = flags;
  deps = 0;

  /* If the makefile can't be found at all, give up entirely.  */

  if (infile == 0)
    {
      /* If we did some searching, errno has the error from the last
	 attempt, rather from FILENAME itself.  Restore it in case the
	 caller wants to use it in a message.  */
      errno = makefile_errno;
      return 0;
    }

  reading_file = &fileinfo;

  /* Loop over lines in the file.
     The strategy is to accumulate target names in FILENAMES, dependencies
     in DEPS and commands in COMMANDS.  These are used to define a rule
     when the start of the next rule (or eof) is encountered.  */

  initbuffer (&lb);
  commands = xmalloc (200);

  while (!feof (infile))
    {
      fileinfo.lineno += nlines;
      nlines = readline (&lb, infile, &fileinfo);

      /* Check for a shell command line first.
	 If it is not one, we can stop treating tab specially.  */
      if (lb.buffer[0] == '\t')
	{
	  /* This line is a probably shell command.  */
	  unsigned int len;

	  if (no_targets)
	    /* Ignore the commands in a rule with no targets.  */
	    continue;

	  /* If there is no preceding rule line, don't treat this line
	     as a command, even though it begins with a tab character.
	     SunOS 4 make appears to behave this way.  */

	  if (filenames != 0)
	    {
	      if (ignoring)
		/* Yep, this is a shell command, and we don't care.  */
		continue;

	      /* Append this command line to the line being accumulated.  */
	      p = lb.buffer;
	      if (commands_idx == 0)
		cmds_started = fileinfo.lineno;
	      len = strlen (p);
	      if (len + 1 + commands_idx > commands_len)
		{
		  commands_len = (len + 1 + commands_idx) * 2;
		  commands = (char *) xrealloc (commands, commands_len);
		}
	      bcopy (p, &commands[commands_idx], len);
	      commands_idx += len;
	      commands[commands_idx++] = '\n';

	      continue;
	    }
	}

      /* This line is not a shell command line.  Don't worry about tabs.  */

      if (collapsed_length < lb.size)
	{
	  collapsed_length = lb.size;
	  if (collapsed != 0)
	    free (collapsed);
	  collapsed = (char *) xmalloc (collapsed_length);
	}
      strcpy (collapsed, lb.buffer);
      /* Collapse continuation lines.  */
      collapse_continuations (collapsed);
      remove_comments (collapsed);

      /* Compare a word, both length and contents. */
#define	word1eq(s, l) 	(len == l && strneq (s, p, l))
      p = collapsed;
      while (isspace ((unsigned char)*p))
	++p;
      if (*p == '\0')
	/* This line is completely empty.  */
	continue;

      /* Find the end of the first token.  Note we don't need to worry about
       * ":" here since we compare tokens by length (so "export" will never
       * be equal to "export:").
       */
      for (p2 = p+1; *p2 != '\0' && !isspace ((unsigned char)*p2); ++p2)
        {}
      len = p2 - p;

      /* Find the start of the second token.  If it's a `:' remember it,
         since it can't be a preprocessor token--this allows targets named
         `ifdef', `export', etc. */
      reading_target = 0;
      while (isspace ((unsigned char)*p2))
        ++p2;
      if (*p2 == '\0')
        p2 = NULL;
      else if (p2[0] == ':' && p2[1] == '\0')
        {
          reading_target = 1;
          goto skip_conditionals;
        }

      /* We must first check for conditional and `define' directives before
	 ignoring anything, since they control what we will do with
	 following lines.  */

      if (!in_ignored_define
	  && (word1eq ("ifdef", 5) || word1eq ("ifndef", 6)
	      || word1eq ("ifeq", 4) || word1eq ("ifneq", 5)
	      || word1eq ("else", 4) || word1eq ("endif", 5)))
	{
	  int i = conditional_line (p, &fileinfo);
	  if (i >= 0)
	    ignoring = i;
	  else
	    fatal (&fileinfo, _("invalid syntax in conditional"));
	  continue;
	}

      if (word1eq ("endef", 5))
	{
	  if (in_ignored_define)
	    in_ignored_define = 0;
	  else
	    fatal (&fileinfo, _("extraneous `endef'"));
	  continue;
	}

      if (word1eq ("define", 6))
	{
	  if (ignoring)
	    in_ignored_define = 1;
	  else
	    {
	      p2 = next_token (p + 6);
              if (*p2 == '\0')
                fatal (&fileinfo, _("empty variable name"));

	      /* Let the variable name be the whole rest of the line,
		 with trailing blanks stripped (comments have already been
		 removed), so it could be a complex variable/function
		 reference that might contain blanks.  */
	      p = strchr (p2, '\0');
	      while (isblank (p[-1]))
		--p;
	      do_define (p2, p - p2, o_file, infile, &fileinfo);
	    }
	  continue;
	}

      if (word1eq ("override", 8))
        {
	  p2 = next_token (p + 8);
	  if (*p2 == '\0')
	    error (&fileinfo, _("empty `override' directive"));
	  if (strneq (p2, "define", 6) && (isblank (p2[6]) || p2[6] == '\0'))
	    {
	      if (ignoring)
		in_ignored_define = 1;
	      else
		{
		  p2 = next_token (p2 + 6);
                  if (*p2 == '\0')
                    fatal (&fileinfo, _("empty variable name"));

		  /* Let the variable name be the whole rest of the line,
		     with trailing blanks stripped (comments have already been
		     removed), so it could be a complex variable/function
		     reference that might contain blanks.  */
		  p = strchr (p2, '\0');
		  while (isblank (p[-1]))
		    --p;
		  do_define (p2, p - p2, o_override, infile, &fileinfo);
		}
	    }
	  else if (!ignoring
		   && !try_variable_definition (&fileinfo, p2, o_override, 0))
	    error (&fileinfo, _("invalid `override' directive"));

	  continue;
	}
 skip_conditionals:

      if (ignoring)
	/* Ignore the line.  We continue here so conditionals
	   can appear in the middle of a rule.  */
	continue;

      if (!reading_target && word1eq ("export", 6))
	{
	  struct variable *v;
	  p2 = next_token (p + 6);
	  if (*p2 == '\0')
	    export_all_variables = 1;
	  v = try_variable_definition (&fileinfo, p2, o_file, 0);
	  if (v != 0)
	    v->export = v_export;
	  else
	    {
	      unsigned int len;
	      for (p = find_next_token (&p2, &len); p != 0;
		   p = find_next_token (&p2, &len))
		{
		  v = lookup_variable (p, len);
		  if (v == 0)
		    v = define_variable_loc (p, len, "", o_file, 0, &fileinfo);
		  v->export = v_export;
		}
	    }
	}
      else if (!reading_target && word1eq ("unexport", 8))
	{
	  unsigned int len;
	  struct variable *v;
	  p2 = next_token (p + 8);
	  if (*p2 == '\0')
	    export_all_variables = 0;
	  for (p = find_next_token (&p2, &len); p != 0;
	       p = find_next_token (&p2, &len))
	    {
	      v = lookup_variable (p, len);
	      if (v == 0)
		v = define_variable_loc (p, len, "", o_file, 0, &fileinfo);
	      v->export = v_noexport;
	    }
	}
      else if (word1eq ("vpath", 5))
	{
	  char *pattern;
	  unsigned int len;
	  p2 = variable_expand (p + 5);
	  p = find_next_token (&p2, &len);
	  if (p != 0)
	    {
	      pattern = savestring (p, len);
	      p = find_next_token (&p2, &len);
	      /* No searchpath means remove all previous
		 selective VPATH's with the same pattern.  */
	    }
	  else
	    /* No pattern means remove all previous selective VPATH's.  */
	    pattern = 0;
	  construct_vpath_list (pattern, p);
	  if (pattern != 0)
	    free (pattern);
	}
      else if (word1eq ("include", 7) || word1eq ("-include", 8)
	       || word1eq ("sinclude", 8))
	{
	  /* We have found an `include' line specifying a nested
	     makefile to be read at this point.  */
	  struct conditionals *save, new_conditionals;
	  struct nameseq *files;
	  /* "-include" (vs "include") says no error if the file does not
	     exist.  "sinclude" is an alias for this from SGI.  */
	  int noerror = p[0] != 'i';

	  p = allocated_variable_expand (next_token (p + (noerror ? 8 : 7)));
	  if (*p == '\0')
	    {
	      error (&fileinfo,
                     _("no file name for `%sinclude'"), noerror ? "-" : "");
	      continue;
	    }

	  /* Parse the list of file names.  */
	  p2 = p;
	  files = multi_glob (parse_file_seq (&p2, '\0',
					      sizeof (struct nameseq),
					      1),
			      sizeof (struct nameseq));
	  free (p);

	  /* Save the state of conditionals and start
	     the included makefile with a clean slate.  */
	  save = conditionals;
	  bzero ((char *) &new_conditionals, sizeof new_conditionals);
	  conditionals = &new_conditionals;

	  /* Record the rules that are waiting so they will determine
	     the default goal before those in the included makefile.  */
	  record_waiting_files ();

	  /* Read each included makefile.  */
	  while (files != 0)
	    {
	      struct nameseq *next = files->next;
	      char *name = files->name;
              int r;

	      free ((char *)files);
	      files = next;

              r = read_makefile (name, (RM_INCLUDED | RM_NO_TILDE
                                        | (noerror ? RM_DONTCARE : 0)));
	      if (!r && !noerror)
		error (&fileinfo, "%s: %s", name, strerror (errno));

              if (r < 2)
                free (name);
	    }

	  /* Free any space allocated by conditional_line.  */
	  if (conditionals->ignoring)
	    free (conditionals->ignoring);
	  if (conditionals->seen_else)
	    free (conditionals->seen_else);

	  /* Restore state.  */
	  conditionals = save;
	  reading_file = &fileinfo;
	}
#undef	word1eq
      else if (try_variable_definition (&fileinfo, p, o_file, 0))
	/* This line has been dealt with.  */
	;
      else if (lb.buffer[0] == '\t')
	{
	  p = collapsed;	/* Ignore comments.  */
	  while (isblank (*p))
	    ++p;
	  if (*p == '\0')
	    /* The line is completely blank; that is harmless.  */
	    continue;
	  /* This line starts with a tab but was not caught above
	     because there was no preceding target, and the line
	     might have been usable as a variable definition.
	     But now it is definitely lossage.  */
	  fatal(&fileinfo, _("commands commence before first target"));
	}
      else
	{
	  /* This line describes some target files.  This is complicated by
             the existence of target-specific variables, because we can't
             expand the entire line until we know if we have one or not.  So
             we expand the line word by word until we find the first `:',
             then check to see if it's a target-specific variable.

             In this algorithm, `lb_next' will point to the beginning of the
             unexpanded parts of the input buffer, while `p2' points to the
             parts of the expanded buffer we haven't searched yet. */

          enum make_word_type wtype;
          enum variable_origin v_origin;
          char *cmdleft, *lb_next;
          unsigned int len, plen = 0;
          char *colonp;

	  /* Record the previous rule.  */

	  record_waiting_files ();
          tgts_started = fileinfo.lineno;

	  /* Search the line for an unquoted ; that is not after an
             unquoted #.  */
	  cmdleft = find_char_unquote (lb.buffer, ";#", 0);
	  if (cmdleft != 0 && *cmdleft == '#')
	    {
	      /* We found a comment before a semicolon.  */
	      *cmdleft = '\0';
	      cmdleft = 0;
	    }
	  else if (cmdleft != 0)
	    /* Found one.  Cut the line short there before expanding it.  */
	    *(cmdleft++) = '\0';

	  collapse_continuations (lb.buffer);

	  /* We can't expand the entire line, since if it's a per-target
             variable we don't want to expand it.  So, walk from the
             beginning, expanding as we go, and looking for "interesting"
             chars.  The first word is always expandable.  */
          wtype = get_next_mword(lb.buffer, NULL, &lb_next, &len);
          switch (wtype)
            {
            case w_eol:
              if (cmdleft != 0)
                fatal(&fileinfo, _("missing rule before commands"));
              /* This line contained something but turned out to be nothing
                 but whitespace (a comment?).  */
              continue;

            case w_colon:
            case w_dcolon:
              /* We accept and ignore rules without targets for
                 compatibility with SunOS 4 make.  */
              no_targets = 1;
              continue;

            default:
              break;
            }

          p2 = variable_expand_string(NULL, lb_next, len);
          while (1)
            {
              lb_next += len;
              if (cmdleft == 0)
                {
                  /* Look for a semicolon in the expanded line.  */
                  cmdleft = find_char_unquote (p2, ";", 0);

                  if (cmdleft != 0)
                    {
                      unsigned long p2_off = p2 - variable_buffer;
                      unsigned long cmd_off = cmdleft - variable_buffer;
                      char *pend = p2 + strlen(p2);

                      /* Append any remnants of lb, then cut the line short
                         at the semicolon.  */
                      *cmdleft = '\0';

                      /* One school of thought says that you shouldn't expand
                         here, but merely copy, since now you're beyond a ";"
                         and into a command script.  However, the old parser
                         expanded the whole line, so we continue that for
                         backwards-compatiblity.  Also, it wouldn't be
                         entirely consistent, since we do an unconditional
                         expand below once we know we don't have a
                         target-specific variable. */
                      (void)variable_expand_string(pend, lb_next, (long)-1);
                      lb_next += strlen(lb_next);
                      p2 = variable_buffer + p2_off;
                      cmdleft = variable_buffer + cmd_off + 1;
                    }
                }

              colonp = find_char_unquote(p2, ":", 0);
#if defined(__MSDOS__) || defined(WINDOWS32)
	      /* The drive spec brain-damage strikes again...  */
	      /* Note that the only separators of targets in this context
		 are whitespace and a left paren.  If others are possible,
		 they should be added to the string in the call to index.  */
	      while (colonp && (colonp[1] == '/' || colonp[1] == '\\') &&
		     colonp > p2 && isalpha ((unsigned char)colonp[-1]) &&
		     (colonp == p2 + 1 || strchr (" \t(", colonp[-2]) != 0))
		colonp = find_char_unquote(colonp + 1, ":", 0);
#endif
              if (colonp != 0)
                break;

              wtype = get_next_mword(lb_next, NULL, &lb_next, &len);
              if (wtype == w_eol)
                break;

              p2 += strlen(p2);
              *(p2++) = ' ';
              p2 = variable_expand_string(p2, lb_next, len);
              /* We don't need to worry about cmdleft here, because if it was
                 found in the variable_buffer the entire buffer has already
                 been expanded... we'll never get here.  */
            }

	  p2 = next_token (variable_buffer);

          /* If the word we're looking at is EOL, see if there's _anything_
             on the line.  If not, a variable expanded to nothing, so ignore
             it.  If so, we can't parse this line so punt.  */
          if (wtype == w_eol)
            {
              if (*p2 != '\0')
                /* There's no need to be ivory-tower about this: check for
                   one of the most common bugs found in makefiles...  */
                fatal (&fileinfo, _("missing separator%s"),
                       !strneq(lb.buffer, "        ", 8) ? ""
                       : _(" (did you mean TAB instead of 8 spaces?)"));
              continue;
            }

          /* Make the colon the end-of-string so we know where to stop
             looking for targets.  */
          *colonp = '\0';
	  filenames = multi_glob (parse_file_seq (&p2, '\0',
						  sizeof (struct nameseq),
						  1),
				  sizeof (struct nameseq));
          *p2 = ':';

          if (!filenames)
            {
              /* We accept and ignore rules without targets for
                 compatibility with SunOS 4 make.  */
              no_targets = 1;
              continue;
            }
          /* This should never be possible; we handled it above.  */
	  assert (*p2 != '\0');
          ++p2;

	  /* Is this a one-colon or two-colon entry?  */
	  two_colon = *p2 == ':';
	  if (two_colon)
	    p2++;

          /* Test to see if it's a target-specific variable.  Copy the rest
             of the buffer over, possibly temporarily (we'll expand it later
             if it's not a target-specific variable).  PLEN saves the length
             of the unparsed section of p2, for later.  */
          if (*lb_next != '\0')
            {
              unsigned int l = p2 - variable_buffer;
              plen = strlen (p2);
              (void) variable_buffer_output (p2+plen,
                                             lb_next, strlen (lb_next)+1);
              p2 = variable_buffer + l;
            }

          /* See if it's an "override" keyword; if so see if what comes after
             it looks like a variable definition.  */

          wtype = get_next_mword (p2, NULL, &p, &len);

          v_origin = o_file;
          if (wtype == w_static && (len == (sizeof ("override")-1)
                                    && strneq (p, "override", len)))
            {
              v_origin = o_override;
              wtype = get_next_mword (p+len, NULL, &p, &len);
            }

          if (wtype != w_eol)
            wtype = get_next_mword (p+len, NULL, NULL, NULL);

          if (wtype == w_varassign)
            {
              record_target_var (filenames, p, two_colon, v_origin, &fileinfo);
              filenames = 0;
              continue;
            }

          /* This is a normal target, _not_ a target-specific variable.
             Unquote any = in the dependency list.  */
          find_char_unquote (lb_next, "=", 0);

	  /* We have some targets, so don't ignore the following commands.  */
	  no_targets = 0;

          /* Expand the dependencies, etc.  */
          if (*lb_next != '\0')
            {
              unsigned int l = p2 - variable_buffer;
              (void) variable_expand_string (p2 + plen, lb_next, (long)-1);
              p2 = variable_buffer + l;

              /* Look for a semicolon in the expanded line.  */
              if (cmdleft == 0)
                {
                  cmdleft = find_char_unquote (p2, ";", 0);
                  if (cmdleft != 0)
                    *(cmdleft++) = '\0';
                }
            }

	  /* Is this a static pattern rule: `target: %targ: %dep; ...'?  */
	  p = strchr (p2, ':');
	  while (p != 0 && p[-1] == '\\')
	    {
	      register char *q = &p[-1];
	      register int backslash = 0;
	      while (*q-- == '\\')
		backslash = !backslash;
	      if (backslash)
		p = strchr (p + 1, ':');
	      else
		break;
	    }
#ifdef _AMIGA
	  /* Here, the situation is quite complicated. Let's have a look
	    at a couple of targets:

		install: dev:make

		dev:make: make

		dev:make:: xyz

	    The rule is that it's only a target, if there are TWO :'s
	    OR a space around the :.
	  */
	  if (p && !(isspace ((unsigned char)p[1]) || !p[1]
                     || isspace ((unsigned char)p[-1])))
	    p = 0;
#endif
#if defined (WINDOWS32) || defined (__MSDOS__)
          do {
            check_again = 0;
            /* For MSDOS and WINDOWS32, skip a "C:\..." or a "C:/..." */
            if (p != 0 && (p[1] == '\\' || p[1] == '/') &&
		isalpha ((unsigned char)p[-1]) &&
		(p == p2 + 1 || strchr (" \t:(", p[-2]) != 0)) {
              p = strchr (p + 1, ':');
              check_again = 1;
            }
          } while (check_again);
#endif
	  if (p != 0)
	    {
	      struct nameseq *target;
	      target = parse_file_seq (&p2, ':', sizeof (struct nameseq), 1);
	      ++p2;
	      if (target == 0)
		fatal (&fileinfo, _("missing target pattern"));
	      else if (target->next != 0)
		fatal (&fileinfo, _("multiple target patterns"));
	      pattern = target->name;
	      pattern_percent = find_percent (pattern);
	      if (pattern_percent == 0)
		fatal (&fileinfo, _("target pattern contains no `%%'"));
              free((char *)target);
	    }
	  else
	    pattern = 0;

	  /* Parse the dependencies.  */
	  deps = (struct dep *)
	    multi_glob (parse_file_seq (&p2, '\0', sizeof (struct dep), 1),
			sizeof (struct dep));

	  commands_idx = 0;
	  if (cmdleft != 0)
	    {
	      /* Semicolon means rest of line is a command.  */
	      unsigned int len = strlen (cmdleft);

	      cmds_started = fileinfo.lineno;

	      /* Add this command line to the buffer.  */
	      if (len + 2 > commands_len)
		{
		  commands_len = (len + 2) * 2;
		  commands = (char *) xrealloc (commands, commands_len);
		}
	      bcopy (cmdleft, commands, len);
	      commands_idx += len;
	      commands[commands_idx++] = '\n';
	    }

	  continue;
	}

      /* We get here except in the case that we just read a rule line.
	 Record now the last rule we read, so following spurious
	 commands are properly diagnosed.  */
      record_waiting_files ();
      no_targets = 0;
    }

  if (conditionals->if_cmds)
    fatal (&fileinfo, _("missing `endif'"));

  /* At eof, record the last rule.  */
  record_waiting_files ();

  freebuffer (&lb);
  free ((char *) commands);
  fclose (infile);

  reading_file = 0;

  return 1+using_filename;
}

/* Execute a `define' directive.
   The first line has already been read, and NAME is the name of
   the variable to be defined.  The following lines remain to be read.
   LINENO, INFILE and FILENAME refer to the makefile being read.
   The value returned is LINENO, updated for lines read here.  */

static void
do_define (name, namelen, origin, infile, flocp)
     char *name;
     unsigned int namelen;
     enum variable_origin origin;
     FILE *infile;
     struct floc *flocp;
{
  struct linebuffer lb;
  unsigned int nlines = 0;
  unsigned int length = 100;
  char *definition = (char *) xmalloc (100);
  register unsigned int idx = 0;
  register char *p;

  /* Expand the variable name.  */
  char *var = (char *) alloca (namelen + 1);
  bcopy (name, var, namelen);
  var[namelen] = '\0';
  var = variable_expand (var);

  initbuffer (&lb);
  while (!feof (infile))
    {
      unsigned int len;

      flocp->lineno += nlines;
      nlines = readline (&lb, infile, flocp);

      collapse_continuations (lb.buffer);

      p = next_token (lb.buffer);
      len = strlen (p);
      if ((len == 5 || (len > 5 && isblank (p[5])))
          && strneq (p, "endef", 5))
	{
	  p += 5;
	  remove_comments (p);
	  if (*next_token (p) != '\0')
	    error (flocp, _("Extraneous text after `endef' directive"));
	  /* Define the variable.  */
	  if (idx == 0)
	    definition[0] = '\0';
	  else
	    definition[idx - 1] = '\0';
	  (void) define_variable_loc (var, strlen (var), definition, origin,
                                      1, flocp);
	  free (definition);
	  freebuffer (&lb);
	  return;
	}
      else
	{
          len = strlen (lb.buffer);
	  /* Increase the buffer size if necessary.  */
	  if (idx + len + 1 > length)
	    {
	      length = (idx + len) * 2;
	      definition = (char *) xrealloc (definition, length + 1);
	    }

	  bcopy (lb.buffer, &definition[idx], len);
	  idx += len;
	  /* Separate lines with a newline.  */
	  definition[idx++] = '\n';
	}
    }

  /* No `endef'!!  */
  fatal (flocp, _("missing `endef', unterminated `define'"));

  /* NOTREACHED */
  return;
}

/* Interpret conditional commands "ifdef", "ifndef", "ifeq",
   "ifneq", "else" and "endif".
   LINE is the input line, with the command as its first word.

   FILENAME and LINENO are the filename and line number in the
   current makefile.  They are used for error messages.

   Value is -1 if the line is invalid,
   0 if following text should be interpreted,
   1 if following text should be ignored.  */

static int
conditional_line (line, flocp)
     char *line;
     const struct floc *flocp;
{
  int notdef;
  char *cmdname;
  register unsigned int i;

  if (*line == 'i')
    {
      /* It's an "if..." command.  */
      notdef = line[2] == 'n';
      if (notdef)
	{
	  cmdname = line[3] == 'd' ? "ifndef" : "ifneq";
	  line += cmdname[3] == 'd' ? 7 : 6;
	}
      else
	{
	  cmdname = line[2] == 'd' ? "ifdef" : "ifeq";
	  line += cmdname[2] == 'd' ? 6 : 5;
	}
    }
  else
    {
      /* It's an "else" or "endif" command.  */
      notdef = line[1] == 'n';
      cmdname = notdef ? "endif" : "else";
      line += notdef ? 5 : 4;
    }

  line = next_token (line);

  if (*cmdname == 'e')
    {
      if (*line != '\0')
	error (flocp, _("Extraneous text after `%s' directive"), cmdname);
      /* "Else" or "endif".  */
      if (conditionals->if_cmds == 0)
	fatal (flocp, _("extraneous `%s'"), cmdname);
      /* NOTDEF indicates an `endif' command.  */
      if (notdef)
	--conditionals->if_cmds;
      else if (conditionals->seen_else[conditionals->if_cmds - 1])
	fatal (flocp, _("only one `else' per conditional"));
      else
	{
	  /* Toggle the state of ignorance.  */
	  conditionals->ignoring[conditionals->if_cmds - 1]
	    = !conditionals->ignoring[conditionals->if_cmds - 1];
	  /* Record that we have seen an `else' in this conditional.
	     A second `else' will be erroneous.  */
	  conditionals->seen_else[conditionals->if_cmds - 1] = 1;
	}
      for (i = 0; i < conditionals->if_cmds; ++i)
	if (conditionals->ignoring[i])
	  return 1;
      return 0;
    }

  if (conditionals->allocated == 0)
    {
      conditionals->allocated = 5;
      conditionals->ignoring = (char *) xmalloc (conditionals->allocated);
      conditionals->seen_else = (char *) xmalloc (conditionals->allocated);
    }

  ++conditionals->if_cmds;
  if (conditionals->if_cmds > conditionals->allocated)
    {
      conditionals->allocated += 5;
      conditionals->ignoring = (char *)
	xrealloc (conditionals->ignoring, conditionals->allocated);
      conditionals->seen_else = (char *)
	xrealloc (conditionals->seen_else, conditionals->allocated);
    }

  /* Record that we have seen an `if...' but no `else' so far.  */
  conditionals->seen_else[conditionals->if_cmds - 1] = 0;

  /* Search through the stack to see if we're already ignoring.  */
  for (i = 0; i < conditionals->if_cmds - 1; ++i)
    if (conditionals->ignoring[i])
      {
	/* We are already ignoring, so just push a level
	   to match the next "else" or "endif", and keep ignoring.
	   We don't want to expand variables in the condition.  */
	conditionals->ignoring[conditionals->if_cmds - 1] = 1;
	return 1;
      }

  if (cmdname[notdef ? 3 : 2] == 'd')
    {
      /* "Ifdef" or "ifndef".  */
      struct variable *v;
      register char *p = end_of_token (line);
      i = p - line;
      p = next_token (p);
      if (*p != '\0')
	return -1;
      v = lookup_variable (line, i);
      conditionals->ignoring[conditionals->if_cmds - 1]
	= (v != 0 && *v->value != '\0') == notdef;
    }
  else
    {
      /* "Ifeq" or "ifneq".  */
      char *s1, *s2;
      unsigned int len;
      char termin = *line == '(' ? ',' : *line;

      if (termin != ',' && termin != '"' && termin != '\'')
	return -1;

      s1 = ++line;
      /* Find the end of the first string.  */
      if (termin == ',')
	{
	  register int count = 0;
	  for (; *line != '\0'; ++line)
	    if (*line == '(')
	      ++count;
	    else if (*line == ')')
	      --count;
	    else if (*line == ',' && count <= 0)
	      break;
	}
      else
	while (*line != '\0' && *line != termin)
	  ++line;

      if (*line == '\0')
	return -1;

      if (termin == ',')
	{
	  /* Strip blanks after the first string.  */
	  char *p = line++;
	  while (isblank (p[-1]))
	    --p;
	  *p = '\0';
	}
      else
	*line++ = '\0';

      s2 = variable_expand (s1);
      /* We must allocate a new copy of the expanded string because
	 variable_expand re-uses the same buffer.  */
      len = strlen (s2);
      s1 = (char *) alloca (len + 1);
      bcopy (s2, s1, len + 1);

      if (termin != ',')
	/* Find the start of the second string.  */
	line = next_token (line);

      termin = termin == ',' ? ')' : *line;
      if (termin != ')' && termin != '"' && termin != '\'')
	return -1;

      /* Find the end of the second string.  */
      if (termin == ')')
	{
	  register int count = 0;
	  s2 = next_token (line);
	  for (line = s2; *line != '\0'; ++line)
	    {
	      if (*line == '(')
		++count;
	      else if (*line == ')')
		{
		  if (count <= 0)
		    break;
		  else
		    --count;
		}
	    }
	}
      else
	{
	  ++line;
	  s2 = line;
	  while (*line != '\0' && *line != termin)
	    ++line;
	}

      if (*line == '\0')
	return -1;

      *line = '\0';
      line = next_token (++line);
      if (*line != '\0')
	error (flocp, _("Extraneous text after `%s' directive"), cmdname);

      s2 = variable_expand (s2);
      conditionals->ignoring[conditionals->if_cmds - 1]
	= streq (s1, s2) == notdef;
    }

  /* Search through the stack to see if we're ignoring.  */
  for (i = 0; i < conditionals->if_cmds; ++i)
    if (conditionals->ignoring[i])
      return 1;
  return 0;
}

/* Remove duplicate dependencies in CHAIN.  */

void
uniquize_deps (chain)
     struct dep *chain;
{
  register struct dep *d;

  /* Make sure that no dependencies are repeated.  This does not
     really matter for the purpose of updating targets, but it
     might make some names be listed twice for $^ and $?.  */

  for (d = chain; d != 0; d = d->next)
    {
      struct dep *last, *next;

      last = d;
      next = d->next;
      while (next != 0)
	if (streq (dep_name (d), dep_name (next)))
	  {
	    struct dep *n = next->next;
	    last->next = n;
	    if (next->name != 0 && next->name != d->name)
	      free (next->name);
	    if (next != d)
	      free ((char *) next);
	    next = n;
	  }
	else
	  {
	    last = next;
	    next = next->next;
	  }
    }
}

/* Record target-specific variable values for files FILENAMES.
   TWO_COLON is nonzero if a double colon was used.

   The links of FILENAMES are freed, and so are any names in it
   that are not incorporated into other data structures.

   If the target is a pattern, add the variable to the pattern-specific
   variable value list.  */

static void
record_target_var (filenames, defn, two_colon, origin, flocp)
     struct nameseq *filenames;
     char *defn;
     int two_colon;
     enum variable_origin origin;
     const struct floc *flocp;
{
  struct nameseq *nextf;
  struct variable_set_list *global;

  global = current_variable_set_list;

  /* If the variable is an append version, store that but treat it as a
     normal recursive variable.  */

  for (; filenames != 0; filenames = nextf)
    {
      struct variable *v;
      register char *name = filenames->name;
      struct variable_set_list *vlist;
      char *fname;
      char *percent;

      nextf = filenames->next;
      free ((char *) filenames);

      /* If it's a pattern target, then add it to the pattern-specific
         variable list.  */
      percent = find_percent (name);
      if (percent)
        {
          struct pattern_var *p;

          /* Get a reference for this pattern-specific variable struct.  */
          p = create_pattern_var(name, percent);
          vlist = p->vars;
          fname = p->target;
        }
      else
        {
          struct file *f;

          /* Get a file reference for this file, and initialize it.  */
          f = enter_file (name);
          initialize_file_variables (f, 1);
          vlist = f->variables;
          fname = f->name;
        }

      /* Make the new variable context current and define the variable.  */
      current_variable_set_list = vlist;
      v = try_variable_definition (flocp, defn, origin, 1);
      if (!v)
        error (flocp, _("Malformed per-target variable definition"));
      v->per_target = 1;

      /* If it's not an override, check to see if there was a command-line
         setting.  If so, reset the value.  */
      if (origin != o_override)
        {
          struct variable *gv;
          int len = strlen(v->name);

          current_variable_set_list = global;
          gv = lookup_variable (v->name, len);
          if (gv && (gv->origin == o_env_override || gv->origin == o_command))
            define_variable_in_set (v->name, len, gv->value, gv->origin,
                                    gv->recursive, vlist->set, flocp);
        }

      /* Free name if not needed further.  */
      if (name != fname && (name < fname || name > fname + strlen (fname)))
        free (name);
    }

  current_variable_set_list = global;
}

/* Record a description line for files FILENAMES,
   with dependencies DEPS, commands to execute described
   by COMMANDS and COMMANDS_IDX, coming from FILENAME:COMMANDS_STARTED.
   TWO_COLON is nonzero if a double colon was used.
   If not nil, PATTERN is the `%' pattern to make this
   a static pattern rule, and PATTERN_PERCENT is a pointer
   to the `%' within it.

   The links of FILENAMES are freed, and so are any names in it
   that are not incorporated into other data structures.  */

static void
record_files (filenames, pattern, pattern_percent, deps, cmds_started,
	      commands, commands_idx, two_colon, flocp, set_default)
     struct nameseq *filenames;
     char *pattern, *pattern_percent;
     struct dep *deps;
     unsigned int cmds_started;
     char *commands;
     unsigned int commands_idx;
     int two_colon;
     const struct floc *flocp;
     int set_default;
{
  struct nameseq *nextf;
  int implicit = 0;
  unsigned int max_targets = 0, target_idx = 0;
  char **targets = 0, **target_percents = 0;
  struct commands *cmds;

  if (commands_idx > 0)
    {
      cmds = (struct commands *) xmalloc (sizeof (struct commands));
      cmds->fileinfo.filenm = flocp->filenm;
      cmds->fileinfo.lineno = cmds_started;
      cmds->commands = savestring (commands, commands_idx);
      cmds->command_lines = 0;
    }
  else
    cmds = 0;

  for (; filenames != 0; filenames = nextf)
    {

      register char *name = filenames->name;
      register struct file *f;
      register struct dep *d;
      struct dep *this;
      char *implicit_percent;

      nextf = filenames->next;
      free (filenames);

      implicit_percent = find_percent (name);
      implicit |= implicit_percent != 0;

      if (implicit && pattern != 0)
	fatal (flocp, _("mixed implicit and static pattern rules"));

      if (implicit && implicit_percent == 0)
	fatal (flocp, _("mixed implicit and normal rules"));

      if (implicit)
	{
	  if (targets == 0)
	    {
	      max_targets = 5;
	      targets = (char **) xmalloc (5 * sizeof (char *));
	      target_percents = (char **) xmalloc (5 * sizeof (char *));
	      target_idx = 0;
	    }
	  else if (target_idx == max_targets - 1)
	    {
	      max_targets += 5;
	      targets = (char **) xrealloc ((char *) targets,
					    max_targets * sizeof (char *));
	      target_percents
		= (char **) xrealloc ((char *) target_percents,
				      max_targets * sizeof (char *));
	    }
	  targets[target_idx] = name;
	  target_percents[target_idx] = implicit_percent;
	  ++target_idx;
	  continue;
	}

      /* If there are multiple filenames, copy the chain DEPS
	 for all but the last one.  It is not safe for the same deps
	 to go in more than one place in the data base.  */
      this = nextf != 0 ? copy_dep_chain (deps) : deps;

      if (pattern != 0)
	{
	  /* If this is an extended static rule:
	     `targets: target%pattern: dep%pattern; cmds',
	     translate each dependency pattern into a plain filename
	     using the target pattern and this target's name.  */
	  if (!pattern_matches (pattern, pattern_percent, name))
	    {
	      /* Give a warning if the rule is meaningless.  */
	      error (flocp,
		     _("target `%s' doesn't match the target pattern"), name);
	      this = 0;
	    }
	  else
	    {
	      /* We use patsubst_expand to do the work of translating
		 the target pattern, the target's name and the dependencies'
		 patterns into plain dependency names.  */
	      char *buffer = variable_expand ("");

	      for (d = this; d != 0; d = d->next)
		{
		  char *o;
		  char *percent = find_percent (d->name);
		  if (percent == 0)
		    continue;
		  o = patsubst_expand (buffer, name, pattern, d->name,
				       pattern_percent, percent);
                  /* If the name expanded to the empty string, that's
                     illegal.  */
                  if (o == buffer)
                    fatal (flocp,
                           _("target `%s' leaves prerequisite pattern empty"),
                           name);
		  free (d->name);
		  d->name = savestring (buffer, o - buffer);
		}
	    }
	}

      if (!two_colon)
	{
	  /* Single-colon.  Combine these dependencies
	     with others in file's existing record, if any.  */
	  f = enter_file (name);

	  if (f->double_colon)
	    fatal (flocp,
                   _("target file `%s' has both : and :: entries"), f->name);

	  /* If CMDS == F->CMDS, this target was listed in this rule
	     more than once.  Just give a warning since this is harmless.  */
	  if (cmds != 0 && cmds == f->cmds)
	    error (flocp,
                   _("target `%s' given more than once in the same rule."),
                   f->name);

	  /* Check for two single-colon entries both with commands.
	     Check is_target so that we don't lose on files such as .c.o
	     whose commands were preinitialized.  */
	  else if (cmds != 0 && f->cmds != 0 && f->is_target)
	    {
	      error (&cmds->fileinfo,
                     _("warning: overriding commands for target `%s'"),
                     f->name);
	      error (&f->cmds->fileinfo,
                     _("warning: ignoring old commands for target `%s'"),
                     f->name);
	    }

	  f->is_target = 1;

	  /* Defining .DEFAULT with no deps or cmds clears it.  */
	  if (f == default_file && this == 0 && cmds == 0)
	    f->cmds = 0;
	  if (cmds != 0)
	    f->cmds = cmds;
	  /* Defining .SUFFIXES with no dependencies
	     clears out the list of suffixes.  */
	  if (f == suffix_file && this == 0)
	    {
	      d = f->deps;
	      while (d != 0)
		{
		  struct dep *nextd = d->next;
 		  free (d->name);
 		  free ((char *)d);
		  d = nextd;
		}
	      f->deps = 0;
	    }
	  else if (f->deps != 0)
	    {
	      /* Add the file's old deps and the new ones in THIS together.  */

	      struct dep *firstdeps, *moredeps;
	      if (cmds != 0)
		{
		  /* This is the rule with commands, so put its deps first.
		     The rationale behind this is that $< expands to the
		     first dep in the chain, and commands use $< expecting
		     to get the dep that rule specifies.  */
		  firstdeps = this;
		  moredeps = f->deps;
		}
	      else
		{
		  /* Append the new deps to the old ones.  */
		  firstdeps = f->deps;
		  moredeps = this;
		}

	      if (firstdeps == 0)
		firstdeps = moredeps;
	      else
		{
		  d = firstdeps;
		  while (d->next != 0)
		    d = d->next;
		  d->next = moredeps;
		}

	      f->deps = firstdeps;
	    }
	  else
	    f->deps = this;

	  /* If this is a static pattern rule, set the file's stem to
	     the part of its name that matched the `%' in the pattern,
	     so you can use $* in the commands.  */
	  if (pattern != 0)
	    {
	      static char *percent = "%";
	      char *buffer = variable_expand ("");
	      char *o = patsubst_expand (buffer, name, pattern, percent,
					 pattern_percent, percent);
	      f->stem = savestring (buffer, o - buffer);
	    }
	}
      else
	{
	  /* Double-colon.  Make a new record
	     even if the file already has one.  */
	  f = lookup_file (name);
	  /* Check for both : and :: rules.  Check is_target so
	     we don't lose on default suffix rules or makefiles.  */
	  if (f != 0 && f->is_target && !f->double_colon)
	    fatal (flocp,
                   _("target file `%s' has both : and :: entries"), f->name);
	  f = enter_file (name);
	  /* If there was an existing entry and it was a double-colon
	     entry, enter_file will have returned a new one, making it the
	     prev pointer of the old one, and setting its double_colon
	     pointer to the first one.  */
	  if (f->double_colon == 0)
	    /* This is the first entry for this name, so we must
	       set its double_colon pointer to itself.  */
	    f->double_colon = f;
	  f->is_target = 1;
	  f->deps = this;
	  f->cmds = cmds;
	}

      /* Free name if not needed further.  */
      if (f != 0 && name != f->name
	  && (name < f->name || name > f->name + strlen (f->name)))
	{
	  free (name);
	  name = f->name;
	}

      /* See if this is first target seen whose name does
	 not start with a `.', unless it contains a slash.  */
      if (default_goal_file == 0 && set_default
	  && (*name != '.' || strchr (name, '/') != 0
#if defined(__MSDOS__) || defined(WINDOWS32)
			   || strchr (name, '\\') != 0
#endif
	      ))
	{
	  int reject = 0;

	  /* If this file is a suffix, don't
	     let it be the default goal file.  */

	  for (d = suffix_file->deps; d != 0; d = d->next)
	    {
	      register struct dep *d2;
	      if (*dep_name (d) != '.' && streq (name, dep_name (d)))
		{
		  reject = 1;
		  break;
		}
	      for (d2 = suffix_file->deps; d2 != 0; d2 = d2->next)
		{
		  register unsigned int len = strlen (dep_name (d2));
		  if (!strneq (name, dep_name (d2), len))
		    continue;
		  if (streq (name + len, dep_name (d)))
		    {
		      reject = 1;
		      break;
		    }
		}
	      if (reject)
		break;
	    }

	  if (!reject)
	    default_goal_file = f;
	}
    }

  if (implicit)
    {
      targets[target_idx] = 0;
      target_percents[target_idx] = 0;
      create_pattern_rule (targets, target_percents, two_colon, deps, cmds, 1);
      free ((char *) target_percents);
    }
}

/* Search STRING for an unquoted STOPCHAR or blank (if BLANK is nonzero).
   Backslashes quote STOPCHAR, blanks if BLANK is nonzero, and backslash.
   Quoting backslashes are removed from STRING by compacting it into
   itself.  Returns a pointer to the first unquoted STOPCHAR if there is
   one, or nil if there are none.  */

char *
find_char_unquote (string, stopchars, blank)
     char *string;
     char *stopchars;
     int blank;
{
  unsigned int string_len = 0;
  register char *p = string;

  while (1)
    {
      while (*p != '\0' && strchr (stopchars, *p) == 0
	     && (!blank || !isblank (*p)))
	++p;
      if (*p == '\0')
	break;

      if (p > string && p[-1] == '\\')
	{
	  /* Search for more backslashes.  */
	  register int i = -2;
	  while (&p[i] >= string && p[i] == '\\')
	    --i;
	  ++i;
	  /* Only compute the length if really needed.  */
	  if (string_len == 0)
	    string_len = strlen (string);
	  /* The number of backslashes is now -I.
	     Copy P over itself to swallow half of them.  */
	  bcopy (&p[i / 2], &p[i], (string_len - (p - string)) - (i / 2) + 1);
	  p += i / 2;
	  if (i % 2 == 0)
	    /* All the backslashes quoted each other; the STOPCHAR was
	       unquoted.  */
	    return p;

	  /* The STOPCHAR was quoted by a backslash.  Look for another.  */
	}
      else
	/* No backslash in sight.  */
	return p;
    }

  /* Never hit a STOPCHAR or blank (with BLANK nonzero).  */
  return 0;
}

/* Search PATTERN for an unquoted %.  */

char *
find_percent (pattern)
     char *pattern;
{
  return find_char_unquote (pattern, "%", 0);
}

/* Parse a string into a sequence of filenames represented as a
   chain of struct nameseq's in reverse order and return that chain.

   The string is passed as STRINGP, the address of a string pointer.
   The string pointer is updated to point at the first character
   not parsed, which either is a null char or equals STOPCHAR.

   SIZE is how big to construct chain elements.
   This is useful if we want them actually to be other structures
   that have room for additional info.

   If STRIP is nonzero, strip `./'s off the beginning.  */

struct nameseq *
parse_file_seq (stringp, stopchar, size, strip)
     char **stringp;
     int stopchar;
     unsigned int size;
     int strip;
{
  register struct nameseq *new = 0;
  register struct nameseq *new1, *lastnew1;
  register char *p = *stringp;
  char *q;
  char *name;
  char stopchars[3];

#ifdef VMS
  stopchars[0] = ',';
  stopchars[1] = stopchar;
  stopchars[2] = '\0';
#else
  stopchars[0] = stopchar;
  stopchars[1] = '\0';
#endif

  while (1)
    {
      /* Skip whitespace; see if any more names are left.  */
      p = next_token (p);
      if (*p == '\0')
	break;
      if (*p == stopchar)
	break;

      /* Yes, find end of next name.  */
      q = p;
      p = find_char_unquote (q, stopchars, 1);
#ifdef VMS
	/* convert comma separated list to space separated */
      if (p && *p == ',')
	*p =' ';
#endif
#ifdef _AMIGA
      if (stopchar == ':' && p && *p == ':'
          && !(isspace ((unsigned char)p[1]) || !p[1]
               || isspace ((unsigned char)p[-1])))
      {
	p = find_char_unquote (p+1, stopchars, 1);
      }
#endif
#if defined(WINDOWS32) || defined(__MSDOS__)
    /* For WINDOWS32, skip a "C:\..." or a "C:/..." until we find the
       first colon which isn't followed by a slash or a backslash.
       Note that tokens separated by spaces should be treated as separate
       tokens since make doesn't allow path names with spaces */
    if (stopchar == ':')
      while (p != 0 && !isspace ((unsigned char)*p) &&
             (p[1] == '\\' || p[1] == '/') && isalpha ((unsigned char)p[-1]))
        p = find_char_unquote (p + 1, stopchars, 1);
#endif
      if (p == 0)
	p = q + strlen (q);

      if (strip)
#ifdef VMS
	/* Skip leading `[]'s.  */
	while (p - q > 2 && q[0] == '[' && q[1] == ']')
#else
	/* Skip leading `./'s.  */
	while (p - q > 2 && q[0] == '.' && q[1] == '/')
#endif
	  {
	    q += 2;		/* Skip "./".  */
	    while (q < p && *q == '/')
	      /* Skip following slashes: ".//foo" is "foo", not "/foo".  */
	      ++q;
	  }

      /* Extract the filename just found, and skip it.  */

      if (q == p)
	/* ".///" was stripped to "". */
#ifdef VMS
	continue;
#else
#ifdef _AMIGA
	name = savestring ("", 0);
#else
	name = savestring ("./", 2);
#endif
#endif
      else
#ifdef VMS
/* VMS filenames can have a ':' in them but they have to be '\'ed but we need
 *  to remove this '\' before we can use the filename.
 * Savestring called because q may be read-only string constant.
 */
	{
	  char *qbase = xstrdup (q);
	  char *pbase = qbase + (p-q);
	  char *q1 = qbase;
	  char *q2 = q1;
	  char *p1 = pbase;

	  while (q1 != pbase)
	    {
	      if (*q1 == '\\' && *(q1+1) == ':')
		{
		  q1++;
		  p1--;
		}
	      *q2++ = *q1++;
	    }
	  name = savestring (qbase, p1 - qbase);
	  free (qbase);
	}
#else
	name = savestring (q, p - q);
#endif

      /* Add it to the front of the chain.  */
      new1 = (struct nameseq *) xmalloc (size);
      new1->name = name;
      new1->next = new;
      new = new1;
    }

#ifndef NO_ARCHIVES

  /* Look for multi-word archive references.
     They are indicated by a elt ending with an unmatched `)' and
     an elt further down the chain (i.e., previous in the file list)
     with an unmatched `(' (e.g., "lib(mem").  */

  new1 = new;
  lastnew1 = 0;
  while (new1 != 0)
    if (new1->name[0] != '('	/* Don't catch "(%)" and suchlike.  */
	&& new1->name[strlen (new1->name) - 1] == ')'
	&& strchr (new1->name, '(') == 0)
      {
	/* NEW1 ends with a `)' but does not contain a `('.
	   Look back for an elt with an opening `(' but no closing `)'.  */

	struct nameseq *n = new1->next, *lastn = new1;
	char *paren = 0;
	while (n != 0 && (paren = strchr (n->name, '(')) == 0)
	  {
	    lastn = n;
	    n = n->next;
	  }
	if (n != 0
	    /* Ignore something starting with `(', as that cannot actually
	       be an archive-member reference (and treating it as such
	       results in an empty file name, which causes much lossage).  */
	    && n->name[0] != '(')
	  {
	    /* N is the first element in the archive group.
	       Its name looks like "lib(mem" (with no closing `)').  */

	    char *libname;

	    /* Copy "lib(" into LIBNAME.  */
	    ++paren;
	    libname = (char *) alloca (paren - n->name + 1);
	    bcopy (n->name, libname, paren - n->name);
	    libname[paren - n->name] = '\0';

	    if (*paren == '\0')
	      {
		/* N was just "lib(", part of something like "lib( a b)".
		   Edit it out of the chain and free its storage.  */
		lastn->next = n->next;
		free (n->name);
		free ((char *) n);
		/* LASTN->next is the new stopping elt for the loop below.  */
		n = lastn->next;
	      }
	    else
	      {
		/* Replace N's name with the full archive reference.  */
		name = concat (libname, paren, ")");
		free (n->name);
		n->name = name;
	      }

	    if (new1->name[1] == '\0')
	      {
		/* NEW1 is just ")", part of something like "lib(a b )".
		   Omit it from the chain and free its storage.  */
		if (lastnew1 == 0)
		  new = new1->next;
		else
		  lastnew1->next = new1->next;
		lastn = new1;
		new1 = new1->next;
		free (lastn->name);
		free ((char *) lastn);
	      }
	    else
	      {
		/* Replace also NEW1->name, which already has closing `)'.  */
		name = concat (libname, new1->name, "");
		free (new1->name);
		new1->name = name;
		new1 = new1->next;
	      }

	    /* Trace back from NEW1 (the end of the list) until N
	       (the beginning of the list), rewriting each name
	       with the full archive reference.  */

	    while (new1 != n)
	      {
		name = concat (libname, new1->name, ")");
		free (new1->name);
		new1->name = name;
		lastnew1 = new1;
		new1 = new1->next;
	      }
	  }
	else
	  {
	    /* No frobnication happening.  Just step down the list.  */
	    lastnew1 = new1;
	    new1 = new1->next;
	  }
      }
    else
      {
	lastnew1 = new1;
	new1 = new1->next;
      }

#endif

  *stringp = p;
  return new;
}

/* Read a line of text from STREAM into LINEBUFFER.
   Combine continuation lines into one line.
   Return the number of actual lines read (> 1 if hacked continuation lines).
 */

static unsigned long
readline (linebuffer, stream, flocp)
     struct linebuffer *linebuffer;
     FILE *stream;
     const struct floc *flocp;
{
  char *buffer = linebuffer->buffer;
  register char *p = linebuffer->buffer;
  register char *end = p + linebuffer->size;
  register int len, lastlen = 0;
  register char *p2;
  register unsigned int nlines = 0;
  register int backslash;

  *p = '\0';

  while (fgets (p, end - p, stream) != 0)
    {
      len = strlen (p);
      if (len == 0)
	{
	  /* This only happens when the first thing on the line is a '\0'.
	     It is a pretty hopeless case, but (wonder of wonders) Athena
	     lossage strikes again!  (xmkmf puts NULs in its makefiles.)
	     There is nothing really to be done; we synthesize a newline so
	     the following line doesn't appear to be part of this line.  */
	  error (flocp, _("warning: NUL character seen; rest of line ignored"));
	  p[0] = '\n';
	  len = 1;
	}

      p += len;
      if (p[-1] != '\n')
	{
	  /* Probably ran out of buffer space.  */
	  register unsigned int p_off = p - buffer;
	  linebuffer->size *= 2;
	  buffer = (char *) xrealloc (buffer, linebuffer->size);
	  p = buffer + p_off;
	  end = buffer + linebuffer->size;
	  linebuffer->buffer = buffer;
	  *p = '\0';
	  lastlen = len;
	  continue;
	}

      ++nlines;

#if !defined(WINDOWS32) && !defined(__MSDOS__)
      /* Check to see if the line was really ended with CRLF; if so ignore
         the CR.  */
      if (len > 1 && p[-2] == '\r')
        {
          --len;
          --p;
          p[-1] = '\n';
        }
#endif

      if (len == 1 && p > buffer)
	/* P is pointing at a newline and it's the beginning of
	   the buffer returned by the last fgets call.  However,
	   it is not necessarily the beginning of a line if P is
	   pointing past the beginning of the holding buffer.
	   If the buffer was just enlarged (right before the newline),
	   we must account for that, so we pretend that the two lines
	   were one line.  */
	len += lastlen;
      lastlen = len;
      backslash = 0;
      for (p2 = p - 2; --len > 0; --p2)
	{
	  if (*p2 == '\\')
	    backslash = !backslash;
	  else
	    break;
	}

      if (!backslash)
	{
	  p[-1] = '\0';
	  break;
	}

      if (end - p <= 1)
	{
	  /* Enlarge the buffer.  */
	  register unsigned int p_off = p - buffer;
	  linebuffer->size *= 2;
	  buffer = (char *) xrealloc (buffer, linebuffer->size);
	  p = buffer + p_off;
	  end = buffer + linebuffer->size;
	  linebuffer->buffer = buffer;
	}
    }

  if (ferror (stream))
    pfatal_with_name (flocp->filenm);

  return nlines;
}

/* Parse the next "makefile word" from the input buffer, and return info
   about it.

   A "makefile word" is one of:

     w_bogus        Should never happen
     w_eol          End of input
     w_static       A static word; cannot be expanded
     w_variable     A word containing one or more variables/functions
     w_colon        A colon
     w_dcolon       A double-colon
     w_semicolon    A semicolon
     w_comment      A comment character
     w_varassign    A variable assignment operator (=, :=, +=, or ?=)

   Note that this function is only used when reading certain parts of the
   makefile.  Don't use it where special rules hold sway (RHS of a variable,
   in a command list, etc.)  */

static enum make_word_type
get_next_mword (buffer, delim, startp, length)
     char *buffer;
     char *delim;
     char **startp;
     unsigned int *length;
{
  enum make_word_type wtype = w_bogus;
  char *p = buffer, *beg;
  char c;

  /* Skip any leading whitespace.  */
  while (isblank(*p))
    ++p;

  beg = p;
  c = *(p++);
  switch (c)
    {
    case '\0':
      wtype = w_eol;
      break;

    case '#':
      wtype = w_comment;
      break;

    case ';':
      wtype = w_semicolon;
      break;

    case '=':
      wtype = w_varassign;
      break;

    case ':':
      wtype = w_colon;
      switch (*p)
        {
        case ':':
          ++p;
          wtype = w_dcolon;
          break;

        case '=':
          ++p;
          wtype = w_varassign;
          break;
        }
      break;

    case '+':
    case '?':
      if (*p == '=')
        {
          ++p;
          wtype = w_varassign;
          break;
        }

    default:
      if (delim && strchr (delim, c))
        wtype = w_static;
      break;
    }

  /* Did we find something?  If so, return now.  */
  if (wtype != w_bogus)
    goto done;

  /* This is some non-operator word.  A word consists of the longest
     string of characters that doesn't contain whitespace, one of [:=#],
     or [?+]=, or one of the chars in the DELIM string.  */

  /* We start out assuming a static word; if we see a variable we'll
     adjust our assumptions then.  */
  wtype = w_static;

  /* We already found the first value of "c", above.  */
  while (1)
    {
      char closeparen;
      int count;

      switch (c)
        {
        case '\0':
        case ' ':
        case '\t':
        case '=':
        case '#':
          goto done_word;

        case ':':
#if defined(__MSDOS__) || defined(WINDOWS32)
	  /* A word CAN include a colon in its drive spec.  The drive
	     spec is allowed either at the beginning of a word, or as part
	     of the archive member name, like in "libfoo.a(d:/foo/bar.o)".  */
	  if (!(p - beg >= 2
		&& (*p == '/' || *p == '\\') && isalpha ((unsigned char)p[-2])
		&& (p - beg == 2 || p[-3] == '(')))
#endif
	  goto done_word;

        case '$':
          c = *(p++);
          if (c == '$')
            break;

          /* This is a variable reference, so note that it's expandable.
             Then read it to the matching close paren.  */
          wtype = w_variable;

          if (c == '(')
            closeparen = ')';
          else if (c == '{')
            closeparen = '}';
          else
            /* This is a single-letter variable reference.  */
            break;

          for (count=0; *p != '\0'; ++p)
            {
              if (*p == c)
                ++count;
              else if (*p == closeparen && --count < 0)
                {
                  ++p;
                  break;
                }
            }
          break;

        case '?':
        case '+':
          if (*p == '=')
            goto done_word;
          break;

        case '\\':
          switch (*p)
            {
            case ':':
            case ';':
            case '=':
            case '\\':
              ++p;
              break;
            }
          break;

        default:
          if (delim && strchr (delim, c))
            goto done_word;
          break;
        }

      c = *(p++);
    }
 done_word:
  --p;

 done:
  if (startp)
    *startp = beg;
  if (length)
    *length = p - beg;
  return wtype;
}

/* Construct the list of include directories
   from the arguments and the default list.  */

void
construct_include_path (arg_dirs)
     char **arg_dirs;
{
  register unsigned int i;
#ifdef VAXC		/* just don't ask ... */
  stat_t stbuf;
#else
  struct stat stbuf;
#endif
  /* Table to hold the dirs.  */

  register unsigned int defsize = (sizeof (default_include_directories)
				   / sizeof (default_include_directories[0]));
  register unsigned int max = 5;
  register char **dirs = (char **) xmalloc ((5 + defsize) * sizeof (char *));
  register unsigned int idx = 0;

#ifdef  __MSDOS__
  defsize++;
#endif

  /* First consider any dirs specified with -I switches.
     Ignore dirs that don't exist.  */

  if (arg_dirs != 0)
    while (*arg_dirs != 0)
      {
	char *dir = *arg_dirs++;

	if (dir[0] == '~')
	  {
	    char *expanded = tilde_expand (dir);
	    if (expanded != 0)
	      dir = expanded;
	  }

	if (stat (dir, &stbuf) == 0 && S_ISDIR (stbuf.st_mode))
	  {
	    if (idx == max - 1)
	      {
		max += 5;
		dirs = (char **)
		  xrealloc ((char *) dirs, (max + defsize) * sizeof (char *));
	      }
	    dirs[idx++] = dir;
	  }
	else if (dir != arg_dirs[-1])
	  free (dir);
      }

  /* Now add at the end the standard default dirs.  */

#ifdef  __MSDOS__
  {
    /* The environment variable $DJDIR holds the root of the
       DJGPP directory tree; add ${DJDIR}/include.  */
    struct variable *djdir = lookup_variable ("DJDIR", 5);

    if (djdir)
      {
	char *defdir = (char *) xmalloc (strlen (djdir->value) + 8 + 1);

	strcat (strcpy (defdir, djdir->value), "/include");
	dirs[idx++] = defdir;
      }
  }
#endif
  for (i = 0; default_include_directories[i] != 0; ++i)
    if (stat (default_include_directories[i], &stbuf) == 0
	&& S_ISDIR (stbuf.st_mode))
      dirs[idx++] = default_include_directories[i];

  dirs[idx] = 0;

  /* Now compute the maximum length of any name in it.  */

  max_incl_len = 0;
  for (i = 0; i < idx; ++i)
    {
      unsigned int len = strlen (dirs[i]);
      /* If dir name is written with a trailing slash, discard it.  */
      if (dirs[i][len - 1] == '/')
	/* We can't just clobber a null in because it may have come from
	   a literal string and literal strings may not be writable.  */
	dirs[i] = savestring (dirs[i], len - 1);
      if (len > max_incl_len)
	max_incl_len = len;
    }

  include_directories_to_search = dirs;
}

/* Expand ~ or ~USER at the beginning of NAME.
   Return a newly malloc'd string or 0.  */

char *
tilde_expand (name)
     char *name;
{
#ifndef VMS
  if (name[1] == '/' || name[1] == '\0')
    {
      extern char *getenv ();
      char *home_dir;
      int is_variable;

      {
	/* Turn off --warn-undefined-variables while we expand HOME.  */
	int save = warn_undefined_variables_flag;
	warn_undefined_variables_flag = 0;

	home_dir = allocated_variable_expand ("$(HOME)");

	warn_undefined_variables_flag = save;
      }

      is_variable = home_dir[0] != '\0';
      if (!is_variable)
	{
	  free (home_dir);
	  home_dir = getenv ("HOME");
	}
#if !defined(_AMIGA) && !defined(WINDOWS32)
      if (home_dir == 0 || home_dir[0] == '\0')
	{
	  extern char *getlogin ();
	  char *logname = getlogin ();
	  home_dir = 0;
	  if (logname != 0)
	    {
	      struct passwd *p = getpwnam (logname);
	      if (p != 0)
		home_dir = p->pw_dir;
	    }
	}
#endif /* !AMIGA && !WINDOWS32 */
      if (home_dir != 0)
	{
	  char *new = concat (home_dir, "", name + 1);
	  if (is_variable)
	    free (home_dir);
	  return new;
	}
    }
#if !defined(_AMIGA) && !defined(WINDOWS32)
  else
    {
      struct passwd *pwent;
      char *userend = strchr (name + 1, '/');
      if (userend != 0)
	*userend = '\0';
      pwent = getpwnam (name + 1);
      if (pwent != 0)
	{
	  if (userend == 0)
	    return xstrdup (pwent->pw_dir);
	  else
	    return concat (pwent->pw_dir, "/", userend + 1);
	}
      else if (userend != 0)
	*userend = '/';
    }
#endif /* !AMIGA && !WINDOWS32 */
#endif /* !VMS */
  return 0;
}

/* Given a chain of struct nameseq's describing a sequence of filenames,
   in reverse of the intended order, return a new chain describing the
   result of globbing the filenames.  The new chain is in forward order.
   The links of the old chain are freed or used in the new chain.
   Likewise for the names in the old chain.

   SIZE is how big to construct chain elements.
   This is useful if we want them actually to be other structures
   that have room for additional info.  */

struct nameseq *
multi_glob (chain, size)
     struct nameseq *chain;
     unsigned int size;
{
  extern void dir_setup_glob ();
  register struct nameseq *new = 0;
  register struct nameseq *old;
  struct nameseq *nexto;
  glob_t gl;

  dir_setup_glob (&gl);

  for (old = chain; old != 0; old = nexto)
    {
#ifndef NO_ARCHIVES
      char *memname;
#endif

      nexto = old->next;

      if (old->name[0] == '~')
	{
	  char *newname = tilde_expand (old->name);
	  if (newname != 0)
	    {
	      free (old->name);
	      old->name = newname;
	    }
	}

#ifndef NO_ARCHIVES
      if (ar_name (old->name))
	{
	  /* OLD->name is an archive member reference.
	     Replace it with the archive file name,
	     and save the member name in MEMNAME.
	     We will glob on the archive name and then
	     reattach MEMNAME later.  */
	  char *arname;
	  ar_parse_name (old->name, &arname, &memname);
	  free (old->name);
	  old->name = arname;
	}
      else
	memname = 0;
#endif /* !NO_ARCHIVES */

      switch (glob (old->name, GLOB_NOCHECK|GLOB_ALTDIRFUNC, NULL, &gl))
	{
	case 0:			/* Success.  */
	  {
	    register int i = gl.gl_pathc;
	    while (i-- > 0)
	      {
#ifndef NO_ARCHIVES
		if (memname != 0)
		  {
		    /* Try to glob on MEMNAME within the archive.  */
		    struct nameseq *found
		      = ar_glob (gl.gl_pathv[i], memname, size);
		    if (found == 0)
		      {
			/* No matches.  Use MEMNAME as-is.  */
			struct nameseq *elt
			  = (struct nameseq *) xmalloc (size);
			unsigned int alen = strlen (gl.gl_pathv[i]);
			unsigned int mlen = strlen (memname);
			elt->name = (char *) xmalloc (alen + 1 + mlen + 2);
			bcopy (gl.gl_pathv[i], elt->name, alen);
			elt->name[alen] = '(';
			bcopy (memname, &elt->name[alen + 1], mlen);
			elt->name[alen + 1 + mlen] = ')';
			elt->name[alen + 1 + mlen + 1] = '\0';
			elt->next = new;
			new = elt;
		      }
		    else
		      {
			/* Find the end of the FOUND chain.  */
			struct nameseq *f = found;
			while (f->next != 0)
			  f = f->next;

			/* Attach the chain being built to the end of the FOUND
			   chain, and make FOUND the new NEW chain.  */
			f->next = new;
			new = found;
		      }

		    free (memname);
		  }
		else
#endif /* !NO_ARCHIVES */
		  {
		    struct nameseq *elt = (struct nameseq *) xmalloc (size);
		    elt->name = xstrdup (gl.gl_pathv[i]);
		    elt->next = new;
		    new = elt;
		  }
	      }
	    globfree (&gl);
	    free (old->name);
	    free ((char *)old);
	    break;
	  }

	case GLOB_NOSPACE:
	  fatal (NILF, _("virtual memory exhausted"));
	  break;

	default:
	  old->next = new;
	  new = old;
	  break;
	}
    }

  return new;
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         remake.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Basic dependency engine for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,96,97,99 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#include "debug.h"

#include <assert.h>
#undef stderr
#define stderr stdout

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#undef stderr
#define stderr stdout
#else
#include <sys/file.h>
#undef stderr
#define stderr stdout
#endif

#ifdef VMS
#include <starlet.h>
#undef stderr
#define stderr stdout
#endif
#ifdef WINDOWS32
#include <io.h>
#undef stderr
#define stderr stdout
#endif

/* extern int try_implicit_rule PARAMS ((struct file *file, unsigned int depth)); */


/* The test for circular dependencies is based on the 'updating' bit in
   `struct file'.  However, double colon targets have seperate `struct
   file's; make sure we always use the base of the double colon chain. */

#define start_updating(_f)  (((_f)->double_colon ? (_f)->double_colon : (_f))\
                             ->updating = 1)
#define finish_updating(_f) (((_f)->double_colon ? (_f)->double_colon : (_f))\
                             ->updating = 0)
#define is_updating(_f)     (((_f)->double_colon ? (_f)->double_colon : (_f))\
                             ->updating)


/* Incremented when a command is started (under -n, when one would be).  */
unsigned int commands_started = 0;

static int update_file PARAMS ((struct file *file, unsigned int depth));
static int update_file_1 PARAMS ((struct file *file, unsigned int depth));
static int check_dep PARAMS ((struct file *file, unsigned int depth, FILE_TIMESTAMP this_mtime, int *must_make_ptr));
static int touch_file PARAMS ((struct file *file));
static void remake_file PARAMS ((struct file *file));
static FILE_TIMESTAMP name_mtime PARAMS ((char *name));
static int library_search PARAMS ((char **lib, FILE_TIMESTAMP *mtime_ptr));


/* Remake all the goals in the `struct dep' chain GOALS.  Return -1 if nothing
   was done, 0 if all goals were updated successfully, or 1 if a goal failed.
   If MAKEFILES is nonzero, these goals are makefiles, so -t, -q, and -n should
   be disabled for them unless they were also command-line targets, and we
   should only make one goal at a time and return as soon as one goal whose
   `changed' member is nonzero is successfully made.  */

int
update_goal_chain (goals, makefiles)
     register struct dep *goals;
     int makefiles;
{
  int t = touch_flag, q = question_flag, n = just_print_flag;
  unsigned int j = job_slots;
  int status = -1;

#define	MTIME(file) (makefiles ? file_mtime_no_search (file) \
		     : file_mtime (file))

  /* Duplicate the chain so we can remove things from it.  */

  goals = copy_dep_chain (goals);

  {
    /* Clear the `changed' flag of each goal in the chain.
       We will use the flag below to notice when any commands
       have actually been run for a target.  When no commands
       have been run, we give an "up to date" diagnostic.  */

    struct dep *g;
    for (g = goals; g != 0; g = g->next)
      g->changed = 0;
  }

  /* All files start with the considered bit 0, so the global value is 1.  */
  considered = 1;

  /* Update all the goals until they are all finished.  */

  while (goals != 0)
    {
      register struct dep *g, *lastgoal;

      /* Start jobs that are waiting for the load to go down.  */

      start_waiting_jobs ();

      /* Wait for a child to die.  */

      reap_children (1, 0);

      lastgoal = 0;
      g = goals;
      while (g != 0)
	{
	  /* Iterate over all double-colon entries for this file.  */
	  struct file *file;
	  int stop = 0, any_not_updated = 0;

	  for (file = g->file->double_colon ? g->file->double_colon : g->file;
	       file != NULL;
	       file = file->prev)
	    {
	      unsigned int ocommands_started;
	      int x;
	      check_renamed (file);
	      if (makefiles)
		{
		  if (file->cmd_target)
		    {
		      touch_flag = t;
		      question_flag = q;
		      just_print_flag = n;
		    }
		  else
		    touch_flag = question_flag = just_print_flag = 0;
		}

	      /* Save the old value of `commands_started' so we can compare
		 later.  It will be incremented when any commands are
		 actually run.  */
	      ocommands_started = commands_started;

	      x = update_file (file, makefiles ? 1 : 0);
	      check_renamed (file);

	      /* Set the goal's `changed' flag if any commands were started
		 by calling update_file above.  We check this flag below to
		 decide when to give an "up to date" diagnostic.  */
	      g->changed += commands_started - ocommands_started;

              /* If we updated a file and STATUS was not already 1, set it to
                 1 if updating failed, or to 0 if updating succeeded.  Leave
                 STATUS as it is if no updating was done.  */

	      stop = 0;
	      if ((x != 0 || file->updated) && status < 1)
                {
                  if (file->update_status != 0)
                    {
                      /* Updating failed, or -q triggered.  The STATUS value
                         tells our caller which.  */
                      status = file->update_status;
                      /* If -q just triggered, stop immediately.  It doesn't
                         matter how much more we run, since we already know
                         the answer to return.  */
                      stop = (!keep_going_flag && !question_flag
                              && !makefiles);
                    }
                  else
                    {
                      FILE_TIMESTAMP mtime = MTIME (file);
                      check_renamed (file);

                      if (file->updated && g->changed &&
                           mtime != file->mtime_before_update)
                        {
                          /* Updating was done.  If this is a makefile and
                             just_print_flag or question_flag is set (meaning
                             -n or -q was given and this file was specified
                             as a command-line target), don't change STATUS.
                             If STATUS is changed, we will get re-exec'd, and
                             enter an infinite loop.  */
                          if (!makefiles
                              || (!just_print_flag && !question_flag))
                            status = 0;
                          if (makefiles && file->dontcare)
                            /* This is a default makefile; stop remaking.  */
                            stop = 1;
                        }
                    }
                }

	      /* Keep track if any double-colon entry is not finished.
                 When they are all finished, the goal is finished.  */
	      any_not_updated |= !file->updated;

	      if (stop)
		break;
	    }

	  /* Reset FILE since it is null at the end of the loop.  */
	  file = g->file;

	  if (stop || !any_not_updated)
	    {
	      /* If we have found nothing whatever to do for the goal,
		 print a message saying nothing needs doing.  */

	      if (!makefiles
		  /* If the update_status is zero, we updated successfully
		     or not at all.  G->changed will have been set above if
		     any commands were actually started for this goal.  */
		  && file->update_status == 0 && !g->changed
		  /* Never give a message under -s or -q.  */
		  && !silent_flag && !question_flag)
		message (1, ((file->phony || file->cmds == 0)
			     ? _("Nothing to be done for `%s'.")
			     : _("`%s' is up to date.")),
			 file->name);

	      /* This goal is finished.  Remove it from the chain.  */
	      if (lastgoal == 0)
		goals = g->next;
	      else
		lastgoal->next = g->next;

	      /* Free the storage.  */
	      free ((char *) g);

	      g = lastgoal == 0 ? goals : lastgoal->next;

	      if (stop)
		break;
	    }
	  else
	    {
	      lastgoal = g;
	      g = g->next;
	    }
	}

      /* If we reached the end of the dependency graph toggle the considered
         flag for the next pass.  */
      if (g == 0)
        considered = !considered;
    }

  if (makefiles)
    {
      touch_flag = t;
      question_flag = q;
      just_print_flag = n;
      job_slots = j;
    }
  return status;
}

/* If FILE is not up to date, execute the commands for it.
   Return 0 if successful, 1 if unsuccessful;
   but with some flag settings, just call `exit' if unsuccessful.

   DEPTH is the depth in recursions of this function.
   We increment it during the consideration of our dependencies,
   then decrement it again after finding out whether this file
   is out of date.

   If there are multiple double-colon entries for FILE,
   each is considered in turn.  */

static int
update_file (file, depth)
     struct file *file;
     unsigned int depth;
{
  register int status = 0;
  register struct file *f;

  f = file->double_colon ? file->double_colon : file;

  /* Prune the dependency graph: if we've already been here on _this_
     pass through the dependency graph, we don't have to go any further.
     We won't reap_children until we start the next pass, so no state
     change is possible below here until then.  */
  if (f->considered == considered)
    {
      DBF (DB_VERBOSE, _("Pruning file `%s'.\n"));
      return f->command_state == cs_finished ? f->update_status : 0;
    }

  /* This loop runs until we start commands for a double colon rule, or until
     the chain is exhausted. */
  for (; f != 0; f = f->prev)
    {
      f->considered = considered;

      status |= update_file_1 (f, depth);
      check_renamed (f);

      if (status != 0 && !keep_going_flag)
	break;

      if (f->command_state == cs_running
          || f->command_state == cs_deps_running)
        {
	  /* Don't run the other :: rules for this
	     file until this rule is finished.  */
          status = 0;
          break;
        }
    }

  /* Process the remaining rules in the double colon chain so they're marked
     considered.  Start their prerequisites, too.  */
  for (; f != 0 ; f = f->prev)
    {
      struct dep *d;

      f->considered = considered;

      for (d = f->deps; d != 0; d = d->next)
        status |= update_file (d->file, depth + 1);
    }

  return status;
}

/* Consider a single `struct file' and update it as appropriate.  */

static int
update_file_1 (file, depth)
     struct file *file;
     unsigned int depth;
{
  register FILE_TIMESTAMP this_mtime;
  int noexist, must_make, deps_changed;
  int dep_status = 0;
  register struct dep *d, *lastd;
  int running = 0;

  DBF (DB_VERBOSE, _("Considering target file `%s'.\n"));

  if (file->updated)
    {
      if (file->update_status > 0)
	{
	  DBF (DB_VERBOSE,
               _("Recently tried and failed to update file `%s'.\n"));
	  return file->update_status;
	}

      DBF (DB_VERBOSE, _("File `%s' was considered already.\n"));
      return 0;
    }

  switch (file->command_state)
    {
    case cs_not_started:
    case cs_deps_running:
      break;
    case cs_running:
      DBF (DB_VERBOSE, _("Still updating file `%s'.\n"));
      return 0;
    case cs_finished:
      DBF (DB_VERBOSE, _("Finished updating file `%s'.\n"));
      return file->update_status;
    default:
      abort ();
    }

  ++depth;

  /* Notice recursive update of the same file.  */
  start_updating (file);

  /* Looking at the file's modtime beforehand allows the possibility
     that its name may be changed by a VPATH search, and thus it may
     not need an implicit rule.  If this were not done, the file
     might get implicit commands that apply to its initial name, only
     to have that name replaced with another found by VPATH search.  */

  this_mtime = file_mtime (file);
  check_renamed (file);
  noexist = this_mtime == (FILE_TIMESTAMP) -1;
  if (noexist)
    DBF (DB_BASIC, _("File `%s' does not exist.\n"));

  must_make = noexist;

  /* If file was specified as a target with no commands,
     come up with some default commands.  */

  if (!file->phony && file->cmds == 0 && !file->tried_implicit)
    {
      if (try_implicit_rule (file, depth))
	DBF (DB_IMPLICIT, _("Found an implicit rule for `%s'.\n"));
      else
	DBF (DB_IMPLICIT, _("No implicit rule found for `%s'.\n"));
      file->tried_implicit = 1;
    }
  if (file->cmds == 0 && !file->is_target
      && default_file != 0 && default_file->cmds != 0)
    {
      DBF (DB_IMPLICIT, _("Using default commands for `%s'.\n"));
      file->cmds = default_file->cmds;
    }

  /* Update all non-intermediate files we depend on, if necessary,
     and see whether any of them is more recent than this file.  */

  lastd = 0;
  d = file->deps;
  while (d != 0)
    {
      FILE_TIMESTAMP mtime;

      check_renamed (d->file);

      mtime = file_mtime (d->file);
      check_renamed (d->file);

      if (is_updating (d->file))
	{
	  error (NILF, _("Circular %s <- %s dependency dropped."),
		 file->name, d->file->name);
	  /* We cannot free D here because our the caller will still have
	     a reference to it when we were called recursively via
	     check_dep below.  */
	  if (lastd == 0)
	    file->deps = d->next;
	  else
	    lastd->next = d->next;
	  d = d->next;
	  continue;
	}

      d->file->parent = file;
      dep_status |= check_dep (d->file, depth, this_mtime, &must_make);
      check_renamed (d->file);

      {
	register struct file *f = d->file;
	if (f->double_colon)
	  f = f->double_colon;
	do
	  {
	    running |= (f->command_state == cs_running
			|| f->command_state == cs_deps_running);
	    f = f->prev;
	  }
	while (f != 0);
      }

      if (dep_status != 0 && !keep_going_flag)
	break;

      if (!running)
	d->changed = file_mtime (d->file) != mtime;

      lastd = d;
      d = d->next;
    }

  /* Now we know whether this target needs updating.
     If it does, update all the intermediate files we depend on.  */

  if (must_make)
    {
      for (d = file->deps; d != 0; d = d->next)
	if (d->file->intermediate)
	  {
	    FILE_TIMESTAMP mtime = file_mtime (d->file);
	    check_renamed (d->file);
	    d->file->parent = file;
	    dep_status |= update_file (d->file, depth);
	    check_renamed (d->file);

	    {
	      register struct file *f = d->file;
	      if (f->double_colon)
		f = f->double_colon;
	      do
		{
		  running |= (f->command_state == cs_running
			      || f->command_state == cs_deps_running);
		  f = f->prev;
		}
	      while (f != 0);
	    }

	    if (dep_status != 0 && !keep_going_flag)
	      break;

	    if (!running)
	      d->changed = ((file->phony && file->cmds != 0)
			    || file_mtime (d->file) != mtime);
	  }
    }

  finish_updating (file);

  DBF (DB_VERBOSE, _("Finished prerequisites of target file `%s'.\n"));

  if (running)
    {
      set_command_state (file, cs_deps_running);
      --depth;
      DBF (DB_VERBOSE, _("The prerequisites of `%s' are being made.\n"));
      return 0;
    }

  /* If any dependency failed, give up now.  */

  if (dep_status != 0)
    {
      file->update_status = dep_status;
      notice_finished_file (file);

      depth--;

      DBF (DB_VERBOSE, _("Giving up on target file `%s'.\n"));

      if (depth == 0 && keep_going_flag
	  && !just_print_flag && !question_flag)
	error (NILF,
               _("Target `%s' not remade because of errors."), file->name);

      return dep_status;
    }

  if (file->command_state == cs_deps_running)
    /* The commands for some deps were running on the last iteration, but
       they have finished now.  Reset the command_state to not_started to
       simplify later bookkeeping.  It is important that we do this only
       when the prior state was cs_deps_running, because that prior state
       was definitely propagated to FILE's also_make's by set_command_state
       (called above), but in another state an also_make may have
       independently changed to finished state, and we would confuse that
       file's bookkeeping (updated, but not_started is bogus state).  */
    set_command_state (file, cs_not_started);

  /* Now record which dependencies are more
     recent than this file, so we can define $?.  */

  deps_changed = 0;
  for (d = file->deps; d != 0; d = d->next)
    {
      FILE_TIMESTAMP d_mtime = file_mtime (d->file);
      check_renamed (d->file);

#if 1	/* %%% In version 4, remove this code completely to
	   implement not remaking deps if their deps are newer
	   than their parents.  */
      if (d_mtime == (FILE_TIMESTAMP) -1 && !d->file->intermediate)
	/* We must remake if this dep does not
	   exist and is not intermediate.  */
	must_make = 1;
#endif

      /* Set DEPS_CHANGED if this dep actually changed.  */
      deps_changed |= d->changed;

      /* Set D->changed if either this dep actually changed,
	 or its dependent, FILE, is older or does not exist.  */
      d->changed |= noexist || d_mtime > this_mtime;

      if (!noexist && ISDB (DB_BASIC|DB_VERBOSE))
	{
          const char *fmt = 0;

	  if (d_mtime == (FILE_TIMESTAMP) -1)
            {
              if (ISDB (DB_BASIC))
                fmt = _("Prerequisite `%s' of target `%s' does not exist.\n");
            }
	  else if (d->changed)
            {
              if (ISDB (DB_BASIC))
                fmt = _("Prerequisite `%s' is newer than target `%s'.\n");
            }
          else if (ISDB (DB_VERBOSE))
            fmt = _("Prerequisite `%s' is older than target `%s'.\n");

          if (fmt)
            {
              print_spaces (depth);
              printf (fmt, dep_name (d), file->name);
              fflush (stdout);
            }
	}
    }

  /* Here depth returns to the value it had when we were called.  */
  depth--;

  if (file->double_colon && file->deps == 0)
    {
      must_make = 1;
      DBF (DB_BASIC,
           _("Target `%s' is double-colon and has no prerequisites.\n"));
    }
  else if (!noexist && file->is_target && !deps_changed && file->cmds == 0)
    {
      must_make = 0;
      DBF (DB_VERBOSE,
           _("No commands for `%s' and no prerequisites actually changed.\n"));
    }

  if (!must_make)
    {
      if (ISDB (DB_VERBOSE))
        {
          print_spaces (depth);
          printf (_("No need to remake target `%s'"), file->name);
          if (!streq (file->name, file->hname))
              printf (_("; using VPATH name `%s'"), file->hname);
          puts (".");
          fflush (stdout);
        }

      notice_finished_file (file);

      /* Since we don't need to remake the file, convert it to use the
         VPATH filename if we found one.  hfile will be either the
         local name if no VPATH or the VPATH name if one was found.  */

      while (file)
        {
          file->name = file->hname;
          file = file->prev;
        }

      return 0;
    }

  DBF (DB_BASIC, _("Must remake target `%s'.\n"));

  /* It needs to be remade.  If it's VPATH and not reset via GPATH, toss the
     VPATH.  */
  if (!streq(file->name, file->hname))
    {
      DB (DB_BASIC, (_("  Ignoring VPATH name `%s'.\n"), file->hname));
      file->ignore_vpath = 1;
    }

  /* Now, take appropriate actions to remake the file.  */
  remake_file (file);

  if (file->command_state != cs_finished)
    {
      DBF (DB_VERBOSE, _("Commands of `%s' are being run.\n"));
      return 0;
    }

  switch (file->update_status)
    {
    case 2:
      DBF (DB_BASIC, _("Failed to remake target file `%s'.\n"));
      break;
    case 0:
      DBF (DB_BASIC, _("Successfully remade target file `%s'.\n"));
      break;
    case 1:
      DBF (DB_BASIC, _("Target file `%s' needs remade under -q.\n"));
      break;
    default:
      assert (file->update_status >= 0 && file->update_status <= 2);
      break;
    }

  file->updated = 1;
  return file->update_status;
}

/* Set FILE's `updated' flag and re-check its mtime and the mtime's of all
   files listed in its `also_make' member.  Under -t, this function also
   touches FILE.

   On return, FILE->update_status will no longer be -1 if it was.  */

void
notice_finished_file (file)
     register struct file *file;
{
  struct dep *d;
  int ran = file->command_state == cs_running;

  file->command_state = cs_finished;
  file->updated = 1;

  if (touch_flag
      /* The update status will be:
	 	-1	if this target was not remade;
		0	if 0 or more commands (+ or ${MAKE}) were run and won;
		1	if some commands were run and lost.
	 We touch the target if it has commands which either were not run
	 or won when they ran (i.e. status is 0).  */
      && file->update_status == 0)
    {
      if (file->cmds != 0 && file->cmds->any_recurse)
	{
	  /* If all the command lines were recursive,
	     we don't want to do the touching.  */
	  unsigned int i;
	  for (i = 0; i < file->cmds->ncommand_lines; ++i)
	    if (!(file->cmds->lines_flags[i] & COMMANDS_RECURSE))
	      goto have_nonrecursing;
	}
      else
	{
	have_nonrecursing:
	  if (file->phony)
	    file->update_status = 0;
	  else
	    /* Should set file's modification date and do nothing else.  */
	    file->update_status = touch_file (file);
	}
    }

  if (file->mtime_before_update == 0)
    file->mtime_before_update = file->last_mtime;

  if (ran && !file->phony)
    {
      struct file *f;
      int i = 0;

      /* If -n or -q and all the commands are recursive, we ran them so
         really check the target's mtime again.  Otherwise, assume the target
         would have been updated. */

      if (question_flag || just_print_flag)
        {
          for (i = file->cmds->ncommand_lines; i > 0; --i)
            if (! (file->cmds->lines_flags[i-1] & COMMANDS_RECURSE))
              break;
        }

      /* If there were no commands at all, it's always new. */

      else if (file->is_target && file->cmds == 0)
	i = 1;

      file->last_mtime = i == 0 ? 0 : NEW_MTIME;

      /* Propagate the change of modification time to all the double-colon
	 entries for this file.  */
      for (f = file->double_colon; f != 0; f = f->next)
	f->last_mtime = file->last_mtime;
    }

  if (ran && file->update_status != -1)
    /* We actually tried to update FILE, which has
       updated its also_make's as well (if it worked).
       If it didn't work, it wouldn't work again for them.
       So mark them as updated with the same status.  */
    for (d = file->also_make; d != 0; d = d->next)
      {
	d->file->command_state = cs_finished;
	d->file->updated = 1;
	d->file->update_status = file->update_status;

	if (ran && !d->file->phony)
	  /* Fetch the new modification time.
	     We do this instead of just invalidating the cached time
	     so that a vpath_search can happen.  Otherwise, it would
	     never be done because the target is already updated.  */
	  (void) f_mtime (d->file, 0);
      }
  else if (file->update_status == -1)
    /* Nothing was done for FILE, but it needed nothing done.
       So mark it now as "succeeded".  */
    file->update_status = 0;
}

/* Check whether another file (whose mtime is THIS_MTIME)
   needs updating on account of a dependency which is file FILE.
   If it does, store 1 in *MUST_MAKE_PTR.
   In the process, update any non-intermediate files
   that FILE depends on (including FILE itself).
   Return nonzero if any updating failed.  */

static int
check_dep (file, depth, this_mtime, must_make_ptr)
     struct file *file;
     unsigned int depth;
     FILE_TIMESTAMP this_mtime;
     int *must_make_ptr;
{
  register struct dep *d;
  int dep_status = 0;

  ++depth;
  start_updating (file);

  if (!file->intermediate)
    /* If this is a non-intermediate file, update it and record
       whether it is newer than THIS_MTIME.  */
    {
      FILE_TIMESTAMP mtime;
      dep_status = update_file (file, depth);
      check_renamed (file);
      mtime = file_mtime (file);
      check_renamed (file);
      if (mtime == (FILE_TIMESTAMP) -1 || mtime > this_mtime)
	*must_make_ptr = 1;
    }
  else
    {
      /* FILE is an intermediate file.  */
      FILE_TIMESTAMP mtime;

      if (!file->phony && file->cmds == 0 && !file->tried_implicit)
	{
	  if (try_implicit_rule (file, depth))
	    DBF (DB_IMPLICIT, _("Found an implicit rule for `%s'.\n"));
	  else
	    DBF (DB_IMPLICIT, _("No implicit rule found for `%s'.\n"));
	  file->tried_implicit = 1;
	}
      if (file->cmds == 0 && !file->is_target
	  && default_file != 0 && default_file->cmds != 0)
	{
	  DBF (DB_IMPLICIT, _("Using default commands for `%s'.\n"));
	  file->cmds = default_file->cmds;
	}

      /* If the intermediate file actually exists
	 and is newer, then we should remake from it.  */
      check_renamed (file);
      mtime = file_mtime (file);
      check_renamed (file);
      if (mtime != (FILE_TIMESTAMP) -1 && mtime > this_mtime)
	*must_make_ptr = 1;
	  /* Otherwise, update all non-intermediate files we depend on,
	     if necessary, and see whether any of them is more
	     recent than the file on whose behalf we are checking.  */
      else
	{
	  register struct dep *lastd;

	  lastd = 0;
	  d = file->deps;
	  while (d != 0)
	    {
	      if (is_updating (d->file))
		{
		  error (NILF, _("Circular %s <- %s dependency dropped."),
			 file->name, d->file->name);
		  if (lastd == 0)
		    {
		      file->deps = d->next;
		      free ((char *) d);
		      d = file->deps;
		    }
		  else
		    {
		      lastd->next = d->next;
		      free ((char *) d);
		      d = lastd->next;
		    }
		  continue;
		}

	      d->file->parent = file;
	      dep_status |= check_dep (d->file, depth, this_mtime,
                                       must_make_ptr);
	      check_renamed (d->file);
	      if (dep_status != 0 && !keep_going_flag)
		break;

	      if (d->file->command_state == cs_running
		  || d->file->command_state == cs_deps_running)
		/* Record that some of FILE's deps are still being made.
		   This tells the upper levels to wait on processing it until
		   the commands are finished.  */
		set_command_state (file, cs_deps_running);

	      lastd = d;
	      d = d->next;
	    }
	}
    }

  finish_updating (file);
  return dep_status;
}

/* Touch FILE.  Return zero if successful, one if not.  */

#define TOUCH_ERROR(call) return (perror_with_name (call, file->name), 1)

static int
touch_file (file)
     register struct file *file;
{
  if (!silent_flag)
    message (0, "touch %s", file->name);

#ifndef	NO_ARCHIVES
  if (ar_name (file->name))
    return ar_touch (file->name);
  else
#endif
    {
      int fd = open (file->name, O_RDWR | O_CREAT, 0666);

      if (fd < 0)
	TOUCH_ERROR ("touch: open: ");
      else
	{
	  struct stat statbuf;
	  char buf;
	  int status;

	  do
	    status = fstat (fd, &statbuf);
	  while (status < 0 && EINTR_SET);

	  if (status < 0)
	    TOUCH_ERROR ("touch: fstat: ");
	  /* Rewrite character 0 same as it already is.  */
	  if (read (fd, &buf, 1) < 0)
	    TOUCH_ERROR ("touch: read: ");
	  if (off_t_equal(off_t_to_int(lseek (fd, int_to_off_t(0L), 0)), ZERO_L_off_t) < 0)
	    /*	  if (lseek (fd, 0L, 0) < 0L) */
	    TOUCH_ERROR ("touch: lseek: ");
	  if (write (fd, &buf, 1) < 0)
	    TOUCH_ERROR ("touch: write: ");
	  /* If file length was 0, we just
	     changed it, so change it back.  */
	  if (off_t_equal(off_t_to_int(statbuf.st_size), ZERO_off_t) == 0)
	    /*	  if (statbuf.st_size == 0) */
	    {
	      (void) close (fd);
	      fd = open (file->name, O_RDWR | O_TRUNC, 0666);
	      if (fd < 0)
		TOUCH_ERROR ("touch: open: ");
	    }
	  (void) close (fd);
	}
    }

  return 0;
}

/* Having checked and updated the dependencies of FILE,
   do whatever is appropriate to remake FILE itself.
   Return the status from executing FILE's commands.  */

static void
remake_file (file)
     struct file *file;
{
  if (file->cmds == 0)
    {
      if (file->phony)
	/* Phony target.  Pretend it succeeded.  */
	file->update_status = 0;
      else if (file->is_target)
	/* This is a nonexistent target file we cannot make.
	   Pretend it was successfully remade.  */
	file->update_status = 0;
      else
        {
          const char *msg_noparent
            = _("%sNo rule to make target `%s'%s");
          const char *msg_parent
            = _("%sNo rule to make target `%s', needed by `%s'%s");

          /* This is a dependency file we cannot remake.  Fail.  */
          if (!keep_going_flag && !file->dontcare)
            {
              if (file->parent == 0)
                fatal (NILF, msg_noparent, "", file->name, "");

              fatal (NILF, msg_parent, "", file->name, file->parent->name, "");
            }

          if (!file->dontcare)
            {
              if (file->parent == 0)
                error (NILF, msg_noparent, "*** ", file->name, ".");
              else
                error (NILF, msg_parent, "*** ",
                       file->name, file->parent->name, ".");
            }
          file->update_status = 2;
        }
    }
  else
    {
      chop_commands (file->cmds);

      /* The normal case: start some commands.  */
      if (!touch_flag || file->cmds->any_recurse)
	{
	  execute_file_commands (file);
	  return;
	}

      /* This tells notice_finished_file it is ok to touch the file.  */
      file->update_status = 0;
    }

  /* This does the touching under -t.  */
  notice_finished_file (file);
}

/* Return the mtime of a file, given a `struct file'.
   Caches the time in the struct file to avoid excess stat calls.

   If the file is not found, and SEARCH is nonzero, VPATH searching and
   replacement is done.  If that fails, a library (-lLIBNAME) is tried and
   the library's actual name (/lib/libLIBNAME.a, etc.) is substituted into
   FILE.  */

FILE_TIMESTAMP
f_mtime (file, search)
     register struct file *file;
     int search;
{
  FILE_TIMESTAMP mtime;

  /* File's mtime is not known; must get it from the system.  */

#ifndef	NO_ARCHIVES
  if (ar_name (file->name))
    {
      /* This file is an archive-member reference.  */

      char *arname, *memname;
      struct file *arfile;
      int arname_used = 0;

      /* Find the archive's name.  */
      ar_parse_name (file->name, &arname, &memname);

      /* Find the modification time of the archive itself.
	 Also allow for its name to be changed via VPATH search.  */
      arfile = lookup_file (arname);
      if (arfile == 0)
	{
	  arfile = enter_file (arname);
	  arname_used = 1;
	}
      mtime = f_mtime (arfile, search);
      check_renamed (arfile);
      if (search && strcmp (arfile->hname, arname))
	{
	  /* The archive's name has changed.
	     Change the archive-member reference accordingly.  */

          char *name;
	  unsigned int arlen, memlen;

	  if (!arname_used)
	    {
	      free (arname);
	      arname_used = 1;
	    }

	  arname = arfile->hname;
	  arlen = strlen (arname);
	  memlen = strlen (memname);

	  /* free (file->name); */

	  name = (char *) xmalloc (arlen + 1 + memlen + 2);
	  bcopy (arname, name, arlen);
	  name[arlen] = '(';
	  bcopy (memname, name + arlen + 1, memlen);
	  name[arlen + 1 + memlen] = ')';
	  name[arlen + 1 + memlen + 1] = '\0';

          /* If the archive was found with GPATH, make the change permanent;
             otherwise defer it until later.  */
          if (arfile->name == arfile->hname)
            rename_file (file, name);
          else
            rehash_file (file, name);
          check_renamed (file);
	}

      if (!arname_used)
	free (arname);
      free (memname);

      if (mtime == (FILE_TIMESTAMP) -1)
	/* The archive doesn't exist, so it's members don't exist either.  */
	return (FILE_TIMESTAMP) -1;

      mtime = FILE_TIMESTAMP_FROM_S_AND_NS (ar_member_date (file->hname), 0);
    }
  else
#endif
    {
      mtime = name_mtime (file->name);

      if (mtime == (FILE_TIMESTAMP) -1 && search && !file->ignore_vpath)
	{
	  /* If name_mtime failed, search VPATH.  */
	  char *name = file->name;
	  if (vpath_search (&name, &mtime)
	      /* Last resort, is it a library (-lxxx)?  */
	      || (name[0] == '-' && name[1] == 'l'
		  && library_search (&name, &mtime)))
	    {
	      if (mtime != 0)
		/* vpath_search and library_search store zero in MTIME
		   if they didn't need to do a stat call for their work.  */
		file->last_mtime = mtime;

              /* If we found it in VPATH, see if it's in GPATH too; if so,
                 change the name right now; if not, defer until after the
                 dependencies are updated. */
              if (gpath_search (name, strlen(name) - strlen(file->name) - 1))
                {
                  rename_file (file, name);
                  check_renamed (file);
                  return file_mtime (file);
                }

	      rehash_file (file, name);
	      check_renamed (file);
	      mtime = name_mtime (name);
	    }
	}
    }

  {
    /* Files can have bogus timestamps that nothing newly made will be
       "newer" than.  Updating their dependents could just result in loops.
       So notify the user of the anomaly with a warning.

       We only need to do this once, for now. */

    static FILE_TIMESTAMP now = 0;
    if (!clock_skew_detected
        && mtime != (FILE_TIMESTAMP)-1 && mtime > now
        && !file->updated)
      {
	/* This file's time appears to be in the future.
	   Update our concept of the present, and compare again.  */

	now = file_timestamp_now ();

#ifdef WINDOWS32
	/*
	 * FAT filesystems round time to nearest even second(!). Just
	 * allow for any file (NTFS or FAT) to perhaps suffer from this
	 * braindamage.
	 */
	if (mtime > now && (((mtime % 2) == 0) && ((mtime-1) > now)))
#else
#ifdef __MSDOS__
	/* Scrupulous testing indicates that some Windows
	   filesystems can set file times up to 3 sec into the future!  */
	if (mtime > now + 3)
#else
        if (mtime > now)
#endif
#endif
          {
	    char mtimebuf[FILE_TIMESTAMP_PRINT_LEN_BOUND + 1];
	    char nowbuf[FILE_TIMESTAMP_PRINT_LEN_BOUND + 1];

	    file_timestamp_sprintf (mtimebuf, mtime);
	    file_timestamp_sprintf (nowbuf, now);
            error (NILF, _("*** Warning: File `%s' has modification time in the future (%s > %s)"),
                   file->name, mtimebuf, nowbuf);
            clock_skew_detected = 1;
          }
      }
  }

  /* Store the mtime into all the entries for this file.  */
  if (file->double_colon)
    file = file->double_colon;

  do
    {
      /* If this file is not implicit but it is intermediate then it was
	 made so by the .INTERMEDIATE target.  If this file has never
	 been built by us but was found now, it existed before make
	 started.  So, turn off the intermediate bit so make doesn't
	 delete it, since it didn't create it.  */
      if (mtime != (FILE_TIMESTAMP)-1 && file->command_state == cs_not_started
	  && !file->tried_implicit && file->intermediate)
	file->intermediate = 0;

      file->last_mtime = mtime;
      file = file->prev;
    }
  while (file != 0);

  return mtime;
}


/* Return the mtime of the file or archive-member reference NAME.  */

static FILE_TIMESTAMP
name_mtime (name)
     register char *name;
{
  struct stat st;

  if (stat (name, &st) < 0)
    return (FILE_TIMESTAMP) -1;

  return FILE_TIMESTAMP_STAT_MODTIME (st);
}


/* Search for a library file specified as -lLIBNAME, searching for a
   suitable library file in the system library directories and the VPATH
   directories.  */

static int
library_search (lib, mtime_ptr)
     char **lib;
     FILE_TIMESTAMP *mtime_ptr;
{
  static char *dirs[] =
    {
#ifndef _AMIGA
      "/lib",
      "/usr/lib",
#endif
#if defined(WINDOWS32) && !defined(LIBDIR)
/*
 * This is completely up to the user at product install time. Just define
 * a placeholder.
 */
#define LIBDIR "."
#endif
      LIBDIR,			/* Defined by configuration.  */
      0
    };

  static char *libpatterns = NULL;

  char *libname = &(*lib)[2];	/* Name without the `-l'.  */
  FILE_TIMESTAMP mtime;

  /* Loop variables for the libpatterns value.  */
  char *p, *p2;
  unsigned int len;

  char *file, **dp;

  /* If we don't have libpatterns, get it.  */
  if (!libpatterns)
    {
      int save = warn_undefined_variables_flag;
      warn_undefined_variables_flag = 0;

      libpatterns = xstrdup (variable_expand ("$(strip $(.LIBPATTERNS))"));

      warn_undefined_variables_flag = save;
    }

  /* Loop through all the patterns in .LIBPATTERNS, and search on each one.  */
  p2 = libpatterns;
  while ((p = find_next_token (&p2, &len)) != 0)
    {
      static char *buf = NULL;
      static int buflen = 0;
      static int libdir_maxlen = -1;
      char *libbuf = variable_expand ("");

      /* Expand the pattern using LIBNAME as a replacement.  */
      {
	char c = p[len];
	char *p3, *p4;

	p[len] = '\0';
	p3 = find_percent (p);
	if (!p3)
	  {
	    /* Give a warning if there is no pattern, then remove the
	       pattern so it's ignored next time.  */
	    error (NILF, _(".LIBPATTERNS element `%s' is not a pattern"), p);
	    for (; len; --len, ++p)
	      *p = ' ';
	    *p = c;
	    continue;
	  }
	p4 = variable_buffer_output (libbuf, p, p3-p);
	p4 = variable_buffer_output (p4, libname, strlen (libname));
	p4 = variable_buffer_output (p4, p3+1, len - (p3-p));
	p[len] = c;
      }

      /* Look first for `libNAME.a' in the current directory.  */
      mtime = name_mtime (libbuf);
      if (mtime != (FILE_TIMESTAMP) -1)
	{
	  *lib = xstrdup (libbuf);
	  if (mtime_ptr != 0)
	    *mtime_ptr = mtime;
	  return 1;
	}

      /* Now try VPATH search on that.  */

      file = libbuf;
      if (vpath_search (&file, mtime_ptr))
	{
	  *lib = file;
	  return 1;
	}

      /* Now try the standard set of directories.  */

      if (!buflen)
	{
	  for (dp = dirs; *dp != 0; ++dp)
	    {
	      int l = strlen (*dp);
	      if (l > libdir_maxlen)
		libdir_maxlen = l;
	    }
	  buflen = strlen (libbuf);
	  buf = xmalloc(libdir_maxlen + buflen + 2);
	}
      else if (buflen < strlen (libbuf))
	{
	  buflen = strlen (libbuf);
	  buf = xrealloc (buf, libdir_maxlen + buflen + 2);
	}

      for (dp = dirs; *dp != 0; ++dp)
	{
	  sprintf (buf, "%s/%s", *dp, libbuf);
	  mtime = name_mtime (buf);
	  if (mtime != (FILE_TIMESTAMP) -1)
	    {
	      *lib = xstrdup (buf);
	      if (mtime_ptr != 0)
		*mtime_ptr = mtime;
	      return 1;
	    }
	}
    }

  return 0;
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         rule.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Pattern and suffix rule internals for GNU Make.
Copyright (C) 1988,89,90,91,92,93, 1998 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout

static void freerule PARAMS ((struct rule *rule, struct rule *lastrule));

/* Chain of all pattern rules.  */

struct rule *pattern_rules;

/* Pointer to last rule in the chain, so we can add onto the end.  */

struct rule *last_pattern_rule;

/* Number of rules in the chain.  */

unsigned int num_pattern_rules;

/* Maximum number of target patterns of any pattern rule.  */

unsigned int max_pattern_targets;

/* Maximum number of dependencies of any pattern rule.  */

unsigned int max_pattern_deps;

/* Maximum length of the name of a dependencies of any pattern rule.  */

unsigned int max_pattern_dep_length;

/* Chain of all pattern-specific variables.  */

static struct pattern_var *pattern_vars;

/* Pointer to last struct in the chain, so we can add onto the end.  */

static struct pattern_var *last_pattern_var;

/* Pointer to structure for the file .SUFFIXES
   whose dependencies are the suffixes to be searched.  */

struct file *suffix_file;

/* Maximum length of a suffix.  */

unsigned int maxsuffix;

/* Compute the maximum dependency length and maximum number of
   dependencies of all implicit rules.  Also sets the subdir
   flag for a rule when appropriate, possibly removing the rule
   completely when appropriate.  */

void
count_implicit_rule_limits ()
{
  char *name;
  unsigned int namelen;
  register struct rule *rule, *lastrule;

  num_pattern_rules = max_pattern_targets = max_pattern_deps = 0;
  max_pattern_dep_length = 0;

  name = 0;
  namelen = 0;
  rule = pattern_rules;
  lastrule = 0;
  while (rule != 0)
    {
      unsigned int ndeps = 0;
      register struct dep *dep;
      struct rule *next = rule->next;
      unsigned int ntargets;

      ++num_pattern_rules;

      ntargets = 0;
      while (rule->targets[ntargets] != 0)
	++ntargets;

      if (ntargets > max_pattern_targets)
	max_pattern_targets = ntargets;

      for (dep = rule->deps; dep != 0; dep = dep->next)
	{
	  unsigned int len = strlen (dep->name);

#ifdef VMS
	  char *p = strrchr (dep->name, ']');
          char *p2;
          if (p == 0)
            p = strrchr (dep->name, ':');
          p2 = p != 0 ? strchr (dep->name, '%') : 0;
#else
	  char *p = strrchr (dep->name, '/');
	  char *p2 = p != 0 ? strchr (dep->name, '%') : 0;
#endif
	  ndeps++;

	  if (len > max_pattern_dep_length)
	    max_pattern_dep_length = len;

	  if (p != 0 && p2 > p)
	    {
	      /* There is a slash before the % in the dep name.
		 Extract the directory name.  */
	      if (p == dep->name)
		++p;
	      if (p - dep->name > namelen)
		{
		  if (name != 0)
		    free (name);
		  namelen = p - dep->name;
		  name = (char *) xmalloc (namelen + 1);
		}
	      bcopy (dep->name, name, p - dep->name);
	      name[p - dep->name] = '\0';

	      /* In the deps of an implicit rule the `changed' flag
		 actually indicates that the dependency is in a
		 nonexistent subdirectory.  */

	      dep->changed = !dir_file_exists_p (name, "");
#ifdef VMS
              if (dep->changed && strchr (name, ':') != 0)
#else
	      if (dep->changed && *name == '/')
#endif
		{
		  /* The name is absolute and the directory does not exist.
		     This rule can never possibly match, since this dependency
		     can never possibly exist.  So just remove the rule from
		     the list.  */
		  freerule (rule, lastrule);
		  --num_pattern_rules;
		  goto end_main_loop;
		}
	    }
	  else
	    /* This dependency does not reside in a subdirectory.  */
	    dep->changed = 0;
	}

      if (ndeps > max_pattern_deps)
	max_pattern_deps = ndeps;

      lastrule = rule;
    end_main_loop:
      rule = next;
    }

  if (name != 0)
    free (name);
}

/* Create a pattern rule from a suffix rule.
   TARGET is the target suffix; SOURCE is the source suffix.
   CMDS are the commands.
   If TARGET is nil, it means the target pattern should be `(%.o)'.
   If SOURCE is nil, it means there should be no deps.  */

static void
convert_suffix_rule (target, source, cmds)
     char *target, *source;
     struct commands *cmds;
{
  char *targname, *targpercent, *depname;
  char **names, **percents;
  struct dep *deps;
  unsigned int len;

  if (target == 0)
    /* Special case: TARGET being nil means we are defining a
       `.X.a' suffix rule; the target pattern is always `(%.o)'.  */
    {
#ifdef VMS
      targname = savestring ("(%.obj)", 7);
#else
      targname = savestring ("(%.o)", 5);
#endif
      targpercent = targname + 1;
    }
  else
    {
      /* Construct the target name.  */
      len = strlen (target);
      targname = xmalloc (1 + len + 1);
      targname[0] = '%';
      bcopy (target, targname + 1, len + 1);
      targpercent = targname;
    }

  names = (char **) xmalloc (2 * sizeof (char *));
  percents = (char **) alloca (2 * sizeof (char *));
  names[0] = targname;
  percents[0] = targpercent;
  names[1] = percents[1] = 0;

  if (source == 0)
    deps = 0;
  else
    {
      /* Construct the dependency name.  */
      len = strlen (source);
      depname = xmalloc (1 + len + 1);
      depname[0] = '%';
      bcopy (source, depname + 1, len + 1);
      deps = (struct dep *) xmalloc (sizeof (struct dep));
      deps->next = 0;
      deps->name = depname;
    }

  create_pattern_rule (names, percents, 0, deps, cmds, 0);
}

/* Convert old-style suffix rules to pattern rules.
   All rules for the suffixes on the .SUFFIXES list
   are converted and added to the chain of pattern rules.  */

void
convert_to_pattern ()
{
  register struct dep *d, *d2;
  register struct file *f;
  register char *rulename;
  register unsigned int slen, s2len;

  /* Compute maximum length of all the suffixes.  */

  maxsuffix = 0;
  for (d = suffix_file->deps; d != 0; d = d->next)
    {
      register unsigned int namelen = strlen (dep_name (d));
      if (namelen > maxsuffix)
	maxsuffix = namelen;
    }

  rulename = (char *) alloca ((maxsuffix * 2) + 1);

  for (d = suffix_file->deps; d != 0; d = d->next)
    {
      /* Make a rule that is just the suffix, with no deps or commands.
	 This rule exists solely to disqualify match-anything rules.  */
      convert_suffix_rule (dep_name (d), (char *) 0, (struct commands *) 0);

      f = d->file;
      if (f->cmds != 0)
	/* Record a pattern for this suffix's null-suffix rule.  */
	convert_suffix_rule ("", dep_name (d), f->cmds);

      /* Record a pattern for each of this suffix's two-suffix rules.  */
      slen = strlen (dep_name (d));
      bcopy (dep_name (d), rulename, slen);
      for (d2 = suffix_file->deps; d2 != 0; d2 = d2->next)
	{
	  s2len = strlen (dep_name (d2));

	  if (slen == s2len && streq (dep_name (d), dep_name (d2)))
	    continue;

	  bcopy (dep_name (d2), rulename + slen, s2len + 1);
	  f = lookup_file (rulename);
	  if (f == 0 || f->cmds == 0)
	    continue;

	  if (s2len == 2 && rulename[slen] == '.' && rulename[slen + 1] == 'a')
	    /* A suffix rule `.X.a:' generates the pattern rule `(%.o): %.X'.
	       It also generates a normal `%.a: %.X' rule below.  */
	    convert_suffix_rule ((char *) 0, /* Indicates `(%.o)'.  */
				 dep_name (d),
				 f->cmds);

	  /* The suffix rule `.X.Y:' is converted
	     to the pattern rule `%.Y: %.X'.  */
	  convert_suffix_rule (dep_name (d2), dep_name (d), f->cmds);
	}
    }
}


/* Install the pattern rule RULE (whose fields have been filled in)
   at the end of the list (so that any rules previously defined
   will take precedence).  If this rule duplicates a previous one
   (identical target and dependencies), the old one is replaced
   if OVERRIDE is nonzero, otherwise this new one is thrown out.
   When an old rule is replaced, the new one is put at the end of the
   list.  Return nonzero if RULE is used; zero if not.  */

int
new_pattern_rule (rule, override)
     register struct rule *rule;
     int override;
{
  register struct rule *r, *lastrule;
  register unsigned int i, j;

  rule->in_use = 0;
  rule->terminal = 0;

  rule->next = 0;

  /* Search for an identical rule.  */
  lastrule = 0;
  for (r = pattern_rules; r != 0; lastrule = r, r = r->next)
    for (i = 0; rule->targets[i] != 0; ++i)
      {
	for (j = 0; r->targets[j] != 0; ++j)
	  if (!streq (rule->targets[i], r->targets[j]))
	    break;
	if (r->targets[j] == 0)
	  /* All the targets matched.  */
	  {
	    register struct dep *d, *d2;
	    for (d = rule->deps, d2 = r->deps;
		 d != 0 && d2 != 0; d = d->next, d2 = d2->next)
	      if (!streq (dep_name (d), dep_name (d2)))
		break;
	    if (d == 0 && d2 == 0)
	      {
		/* All the dependencies matched.  */
		if (override)
		  {
		    /* Remove the old rule.  */
		    freerule (r, lastrule);
		    /* Install the new one.  */
		    if (pattern_rules == 0)
		      pattern_rules = rule;
		    else
		      last_pattern_rule->next = rule;
		    last_pattern_rule = rule;

		    /* We got one.  Stop looking.  */
		    goto matched;
		  }
		else
		  {
		    /* The old rule stays intact.  Destroy the new one.  */
		    freerule (rule, (struct rule *) 0);
		    return 0;
		  }
	      }
	  }
      }

 matched:;

  if (r == 0)
    {
      /* There was no rule to replace.  */
      if (pattern_rules == 0)
	pattern_rules = rule;
      else
	last_pattern_rule->next = rule;
      last_pattern_rule = rule;
    }

  return 1;
}


/* Install an implicit pattern rule based on the three text strings
   in the structure P points to.  These strings come from one of
   the arrays of default implicit pattern rules.
   TERMINAL specifies what the `terminal' field of the rule should be.  */

void
install_pattern_rule (p, terminal)
     struct pspec *p;
     int terminal;
{
  register struct rule *r;
  char *ptr;

  r = (struct rule *) xmalloc (sizeof (struct rule));

  r->targets = (char **) xmalloc (2 * sizeof (char *));
  r->suffixes = (char **) xmalloc (2 * sizeof (char *));
  r->lens = (unsigned int *) xmalloc (2 * sizeof (unsigned int));

  r->targets[1] = 0;
  r->suffixes[1] = 0;
  r->lens[1] = 0;

  r->lens[0] = strlen (p->target);
  /* These will all be string literals, but we malloc space for
     them anyway because somebody might want to free them later on.  */
  r->targets[0] = savestring (p->target, r->lens[0]);
  r->suffixes[0] = find_percent (r->targets[0]);
  if (r->suffixes[0] == 0)
    /* Programmer-out-to-lunch error.  */
    abort ();
  else
    ++r->suffixes[0];

  ptr = p->dep;
  r->deps = (struct dep *) multi_glob (parse_file_seq (&ptr, '\0',
                                                       sizeof (struct dep), 1),
				       sizeof (struct dep));

  if (new_pattern_rule (r, 0))
    {
      r->terminal = terminal;
      r->cmds = (struct commands *) xmalloc (sizeof (struct commands));
      r->cmds->fileinfo.filenm = 0;
      r->cmds->fileinfo.lineno = 0;
      /* These will all be string literals, but we malloc space for them
	 anyway because somebody might want to free them later.  */
      r->cmds->commands = xstrdup (p->commands);
      r->cmds->command_lines = 0;
    }
}


/* Free all the storage used in RULE and take it out of the
   pattern_rules chain.  LASTRULE is the rule whose next pointer
   points to RULE.  */

static void
freerule (rule, lastrule)
     register struct rule *rule, *lastrule;
{
  struct rule *next = rule->next;
  register unsigned int i;
  register struct dep *dep;

  for (i = 0; rule->targets[i] != 0; ++i)
    free (rule->targets[i]);

  dep = rule->deps;
  while (dep)
    {
      struct dep *t;

      t = dep->next;
      /* We might leak dep->name here, but I'm not sure how to fix this: I
         think that pointer might be shared (e.g., in the file hash?)  */
      free ((char *) dep);
      dep = t;
    }

  free ((char *) rule->targets);
  free ((char *) rule->suffixes);
  free ((char *) rule->lens);

  /* We can't free the storage for the commands because there
     are ways that they could be in more than one place:
       * If the commands came from a suffix rule, they could also be in
       the `struct file's for other suffix rules or plain targets given
       on the same makefile line.
       * If two suffixes that together make a two-suffix rule were each
       given twice in the .SUFFIXES list, and in the proper order, two
       identical pattern rules would be created and the second one would
       be discarded here, but both would contain the same `struct commands'
       pointer from the `struct file' for the suffix rule.  */

  free ((char *) rule);

  if (pattern_rules == rule)
    if (lastrule != 0)
      abort ();
    else
      pattern_rules = next;
  else if (lastrule != 0)
    lastrule->next = next;
  if (last_pattern_rule == rule)
    last_pattern_rule = lastrule;
}

/* Create a new pattern rule with the targets in the nil-terminated
   array TARGETS.  If TARGET_PERCENTS is not nil, it is an array of
   pointers into the elements of TARGETS, where the `%'s are.
   The new rule has dependencies DEPS and commands from COMMANDS.
   It is a terminal rule if TERMINAL is nonzero.  This rule overrides
   identical rules with different commands if OVERRIDE is nonzero.

   The storage for TARGETS and its elements is used and must not be freed
   until the rule is destroyed.  The storage for TARGET_PERCENTS is not used;
   it may be freed.  */

void
create_pattern_rule (targets, target_percents,
		     terminal, deps, commands, override)
     char **targets, **target_percents;
     int terminal;
     struct dep *deps;
     struct commands *commands;
     int override;
{
  register struct rule *r = (struct rule *) xmalloc (sizeof (struct rule));
  register unsigned int max_targets, i;

  r->cmds = commands;
  r->deps = deps;
  r->targets = targets;

  max_targets = 2;
  r->lens = (unsigned int *) xmalloc (2 * sizeof (unsigned int));
  r->suffixes = (char **) xmalloc (2 * sizeof (char *));
  for (i = 0; targets[i] != 0; ++i)
    {
      if (i == max_targets - 1)
	{
	  max_targets += 5;
	  r->lens = (unsigned int *)
	    xrealloc ((char *) r->lens, max_targets * sizeof (unsigned int));
	  r->suffixes = (char **)
	    xrealloc ((char *) r->suffixes, max_targets * sizeof (char *));
	}
      r->lens[i] = strlen (targets[i]);
      r->suffixes[i] = (target_percents == 0 ? find_percent (targets[i])
			: target_percents[i]) + 1;
      if (r->suffixes[i] == 0)
	abort ();
    }

  if (i < max_targets - 1)
    {
      r->lens = (unsigned int *) xrealloc ((char *) r->lens,
					   (i + 1) * sizeof (unsigned int));
      r->suffixes = (char **) xrealloc ((char *) r->suffixes,
					(i + 1) * sizeof (char *));
    }

  if (new_pattern_rule (r, override))
    r->terminal = terminal;
}

/* Create a new pattern-specific variable struct.  */

struct pattern_var *
create_pattern_var (target, suffix)
     char *target, *suffix;
{
  register struct pattern_var *p = 0;
  unsigned int len = strlen(target);

  /* Look to see if this pattern already exists in the list.  */
  for (p = pattern_vars; p != NULL; p = p->next)
    if (p->len == len && !strcmp(p->target, target))
      break;

  if (p == 0)
    {
      p = (struct pattern_var *) xmalloc (sizeof (struct pattern_var));
      if (last_pattern_var != 0)
        last_pattern_var->next = p;
      else
        pattern_vars = p;
      last_pattern_var = p;
      p->next = 0;
      p->target = target;
      p->len = len;
      p->suffix = suffix + 1;
      p->vars = create_new_variable_set();
    }

  return p;
}

/* Look up a target in the pattern-specific variable list.  */

struct pattern_var *
lookup_pattern_var (target)
     char *target;
{
  struct pattern_var *p;
  unsigned int targlen = strlen(target);

  for (p = pattern_vars; p != 0; p = p->next)
    {
      char *stem;
      unsigned int stemlen;

      if (p->len > targlen)
        /* It can't possibly match.  */
        continue;

      /* From the lengths of the filename and the pattern parts,
         find the stem: the part of the filename that matches the %.  */
      stem = target + (p->suffix - p->target - 1);
      stemlen = targlen - p->len + 1;

      /* Compare the text in the pattern before the stem, if any.  */
      if (stem > target && !strneq (p->target, target, stem - target))
        continue;

      /* Compare the text in the pattern after the stem, if any.
         We could test simply using streq, but this way we compare the
         first two characters immediately.  This saves time in the very
         common case where the first character matches because it is a
         period.  */
      if (*p->suffix == stem[stemlen]
          && (*p->suffix == '\0' || streq (&p->suffix[1], &stem[stemlen+1])))
        break;
    }

  return p;
}

/* Print the data base of rules.  */

static void			/* Useful to call from gdb.  */
print_rule (r)
     struct rule *r;
{
  register unsigned int i;
  register struct dep *d;

  for (i = 0; r->targets[i] != 0; ++i)
    {
      fputs (r->targets[i], stdout);
      if (r->targets[i + 1] != 0)
	putchar (' ');
      else
	putchar (':');
    }
  if (r->terminal)
    putchar (':');

  for (d = r->deps; d != 0; d = d->next)
    printf (" %s", dep_name (d));
  putchar ('\n');

  if (r->cmds != 0)
    print_commands (r->cmds);
}

void
print_rule_data_base ()
{
  register unsigned int rules, terminal;
  register struct rule *r;

  puts ("\n# Implicit Rules");

  rules = terminal = 0;
  for (r = pattern_rules; r != 0; r = r->next)
    {
      ++rules;

      putchar ('\n');
      print_rule (r);

      if (r->terminal)
	++terminal;
    }

  if (rules == 0)
    puts (_("\n# No implicit rules."));
  else
    {
      printf (_("\n# %u implicit rules, %u"), rules, terminal);
#ifndef	NO_FLOAT
      printf (" (%.1f%%)", (double) terminal / (double) rules * 100.0);
#else
      {
	int f = (terminal * 1000 + 5) / rules;
	printf (" (%d.%d%%)", f/10, f%10);
      }
#endif
      puts (_(" terminal."));
    }

  if (num_pattern_rules != rules)
    {
      /* This can happen if a fatal error was detected while reading the
         makefiles and thus count_implicit_rule_limits wasn't called yet.  */
      if (num_pattern_rules != 0)
        fatal (NILF, _("BUG: num_pattern_rules wrong!  %u != %u"),
               num_pattern_rules, rules);
    }

  puts (_("\n# Pattern-specific variable values"));

  {
    struct pattern_var *p;

    rules = 0;
    for (p = pattern_vars; p != 0; p = p->next)
      {
        ++rules;

        printf ("\n%s :\n", p->target);
        print_variable_set (p->vars->set, "# ");
      }

    if (rules == 0)
      puts (_("\n# No pattern-specific variable values."));
    else
      {
        printf (_("\n# %u pattern-specific variable values"), rules);
      }
  }
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         signame.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Convert between signal names and numbers.
   Copyright (C) 1990,92,93,95,96,99 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


/* In the GNU make version, all the headers we need are provided by make.h.  */
/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout


/* Some systems do not define NSIG in <signal.h>.  */
#ifndef	NSIG
#ifdef	_NSIG
#define	NSIG	_NSIG
#else
#define	NSIG	32
#endif
#endif

#if !__STDC__
#define const
#endif

#include "signame.h"
#undef stderr
#define stderr stdout

#ifndef HAVE_SYS_SIGLIST
/* There is too much variation in Sys V signal numbers and names, so
   we must initialize them at runtime.  */

static const char *undoc;

const char *sys_siglist[NSIG];

#else	/* HAVE_SYS_SIGLIST.  */

#ifndef SYS_SIGLIST_DECLARED
extern char *sys_siglist[];
#endif	/* Not SYS_SIGLIST_DECLARED.  */

#endif	/* Not HAVE_SYS_SIGLIST.  */

/* Table of abbreviations for signals.  Note:  A given number can
   appear more than once with different abbreviations.  */
#define SIG_TABLE_SIZE  (NSIG*2)

typedef struct
  {
    int number;
    const char *abbrev;
  } num_abbrev;
static num_abbrev sig_table[SIG_TABLE_SIZE];
/* Number of elements of sig_table used.  */
static int sig_table_nelts = 0;

/* Enter signal number NUMBER into the tables with ABBREV and NAME.  */

static void
init_sig (number, abbrev, name)
     int number;
     const char *abbrev;
     const char *name;
{
#ifndef HAVE_SYS_SIGLIST
  /* If this value is ever greater than NSIG it seems like it'd be a bug in
     the system headers, but... better safe than sorry.  We know, for
     example, that this isn't always true on VMS.  */

  if (number >= 0 && number < NSIG)
    sys_siglist[number] = name;
#endif
  if (sig_table_nelts < SIG_TABLE_SIZE)
    {
      sig_table[sig_table_nelts].number = number;
      sig_table[sig_table_nelts++].abbrev = abbrev;
    }
}

void
signame_init ()
{
#ifndef HAVE_SYS_SIGLIST
  int i;
  char *u = _("unknown signal");

  undoc = xstrdup(u);

  /* Initialize signal names.  */
  for (i = 0; i < NSIG; i++)
    sys_siglist[i] = undoc;
#endif /* !HAVE_SYS_SIGLIST */

  /* Initialize signal names.  */
#if defined (SIGHUP)
  init_sig (SIGHUP, "HUP", _("Hangup"));
#endif
#if defined (SIGINT)
  init_sig (SIGINT, "INT", _("Interrupt"));
#endif
#if defined (SIGQUIT)
  init_sig (SIGQUIT, "QUIT", _("Quit"));
#endif
#if defined (SIGILL)
  init_sig (SIGILL, "ILL", _("Illegal Instruction"));
#endif
#if defined (SIGTRAP)
  init_sig (SIGTRAP, "TRAP", _("Trace/breakpoint trap"));
#endif
  /* If SIGIOT == SIGABRT, we want to print it as SIGABRT because
     SIGABRT is in ANSI and POSIX.1 and SIGIOT isn't.  */
#if defined (SIGABRT)
  init_sig (SIGABRT, "ABRT", _("Aborted"));
#endif
#if defined (SIGIOT)
  init_sig (SIGIOT, "IOT", _("IOT trap"));
#endif
#if defined (SIGEMT)
  init_sig (SIGEMT, "EMT", _("EMT trap"));
#endif
#if defined (SIGFPE)
  init_sig (SIGFPE, "FPE", _("Floating point exception"));
#endif
#if defined (SIGKILL)
  init_sig (SIGKILL, "KILL", _("Killed"));
#endif
#if defined (SIGBUS)
  init_sig (SIGBUS, "BUS", _("Bus error"));
#endif
#if defined (SIGSEGV)
  init_sig (SIGSEGV, "SEGV", _("Segmentation fault"));
#endif
#if defined (SIGSYS)
  init_sig (SIGSYS, "SYS", _("Bad system call"));
#endif
#if defined (SIGPIPE)
  init_sig (SIGPIPE, "PIPE", _("Broken pipe"));
#endif
#if defined (SIGALRM)
  init_sig (SIGALRM, "ALRM", _("Alarm clock"));
#endif
#if defined (SIGTERM)
  init_sig (SIGTERM, "TERM", _("Terminated"));
#endif
#if defined (SIGUSR1)
  init_sig (SIGUSR1, "USR1", _("User defined signal 1"));
#endif
#if defined (SIGUSR2)
  init_sig (SIGUSR2, "USR2", _("User defined signal 2"));
#endif
  /* If SIGCLD == SIGCHLD, we want to print it as SIGCHLD because that
     is what is in POSIX.1.  */
#if defined (SIGCHLD)
  init_sig (SIGCHLD, "CHLD", _("Child exited"));
#endif
#if defined (SIGCLD)
  init_sig (SIGCLD, "CLD", _("Child exited"));
#endif
#if defined (SIGPWR)
  init_sig (SIGPWR, "PWR", _("Power failure"));
#endif
#if defined (SIGTSTP)
  init_sig (SIGTSTP, "TSTP", _("Stopped"));
#endif
#if defined (SIGTTIN)
  init_sig (SIGTTIN, "TTIN", _("Stopped (tty input)"));
#endif
#if defined (SIGTTOU)
  init_sig (SIGTTOU, "TTOU", _("Stopped (tty output)"));
#endif
#if defined (SIGSTOP)
  init_sig (SIGSTOP, "STOP", _("Stopped (signal)"));
#endif
#if defined (SIGXCPU)
  init_sig (SIGXCPU, "XCPU", _("CPU time limit exceeded"));
#endif
#if defined (SIGXFSZ)
  init_sig (SIGXFSZ, "XFSZ", _("File size limit exceeded"));
#endif
#if defined (SIGVTALRM)
  init_sig (SIGVTALRM, "VTALRM", _("Virtual timer expired"));
#endif
#if defined (SIGPROF)
  init_sig (SIGPROF, "PROF", _("Profiling timer expired"));
#endif
#if defined (SIGWINCH)
  /* "Window size changed" might be more accurate, but even if that
     is all that it means now, perhaps in the future it will be
     extended to cover other kinds of window changes.  */
  init_sig (SIGWINCH, "WINCH", _("Window changed"));
#endif
#if defined (SIGCONT)
  init_sig (SIGCONT, "CONT", _("Continued"));
#endif
#if defined (SIGURG)
  init_sig (SIGURG, "URG", _("Urgent I/O condition"));
#endif
#if defined (SIGIO)
  /* "I/O pending" has also been suggested.  A disadvantage is
     that signal only happens when the process has
     asked for it, not everytime I/O is pending.  Another disadvantage
     is the confusion from giving it a different name than under Unix.  */
  init_sig (SIGIO, "IO", _("I/O possible"));
#endif
#if defined (SIGWIND)
  init_sig (SIGWIND, "WIND", _("SIGWIND"));
#endif
#if defined (SIGPHONE)
  init_sig (SIGPHONE, "PHONE", _("SIGPHONE"));
#endif
#if defined (SIGPOLL)
  init_sig (SIGPOLL, "POLL", _("I/O possible"));
#endif
#if defined (SIGLOST)
  init_sig (SIGLOST, "LOST", _("Resource lost"));
#endif
#if defined (SIGDANGER)
  init_sig (SIGDANGER, "DANGER", _("Danger signal"));
#endif
#if defined (SIGINFO)
  init_sig (SIGINFO, "INFO", _("Information request"));
#endif
#if defined (SIGNOFP)
  init_sig (SIGNOFP, "NOFP", _("Floating point co-processor not available"));
#endif
}

/* Return the abbreviation for signal NUMBER.  */

char *
sig_abbrev (number)
     int number;
{
  int i;

  if (sig_table_nelts == 0)
    signame_init ();

  for (i = 0; i < sig_table_nelts; i++)
    if (sig_table[i].number == number)
      return (char *)sig_table[i].abbrev;
  return NULL;
}

/* Return the signal number for an ABBREV, or -1 if there is no
   signal by that name.  */

int
sig_number (abbrev)
     const char *abbrev;
{
  int i;

  if (sig_table_nelts == 0)
    signame_init ();

  /* Skip over "SIG" if present.  */
  if (abbrev[0] == 'S' && abbrev[1] == 'I' && abbrev[2] == 'G')
    abbrev += 3;

  for (i = 0; i < sig_table_nelts; i++)
    if (abbrev[0] == sig_table[i].abbrev[0]
	&& strcmp (abbrev, sig_table[i].abbrev) == 0)
      return sig_table[i].number;
  return -1;
}

#ifndef HAVE_PSIGNAL
/* Print to standard error the name of SIGNAL, preceded by MESSAGE and
   a colon, and followed by a newline.  */

void
psignal (signal, message)
     int signal;
     const char *message;
{
  if (signal <= 0 || signal >= NSIG)
    fprintf (stderr, "%s: unknown signal", message);
  else
    fprintf (stderr, "%s: %s\n", message, sys_siglist[signal]);
}
#endif

#ifndef HAVE_STRSIGNAL
/* Return the string associated with the signal number.  */

char *
strsignal (signal)
     int signal;
{
  static char buf[] = "Signal 12345678901234567890";

  if (signal > 0 || signal < NSIG)
    return (char *) sys_siglist[signal];

  sprintf (buf, "Signal %d", signal);
  return buf;
}
#endif



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         variable.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Internals of variables for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout
#ifdef WINDOWS32
#include "pathstuff.h"
#undef stderr
#define stderr stdout
#endif

/* Hash table of all global variable definitions.  */

#ifndef	VARIABLE_BUCKETS
#define VARIABLE_BUCKETS		523
#endif
#ifndef	PERFILE_VARIABLE_BUCKETS
#define	PERFILE_VARIABLE_BUCKETS	23
#endif
#ifndef	SMALL_SCOPE_VARIABLE_BUCKETS
#define	SMALL_SCOPE_VARIABLE_BUCKETS	13
#endif
static struct variable *variable_table[VARIABLE_BUCKETS];
static struct variable_set global_variable_set
  = { variable_table, VARIABLE_BUCKETS };
static struct variable_set_list global_setlist
  = { 0, &global_variable_set };
struct variable_set_list *current_variable_set_list = &global_setlist;

static struct variable *lookup_variable_in_set PARAMS ((char *name,
                          unsigned int length, struct variable_set *set));

/* Implement variables.  */

/* Define variable named NAME with value VALUE in SET.  VALUE is copied.
   LENGTH is the length of NAME, which does not need to be null-terminated.
   ORIGIN specifies the origin of the variable (makefile, command line
   or environment).
   If RECURSIVE is nonzero a flag is set in the variable saying
   that it should be recursively re-expanded.  */

struct variable *
define_variable_in_set (name, length, value, origin, recursive, set, flocp)
     char *name;
     unsigned int length;
     char *value;
     enum variable_origin origin;
     int recursive;
     struct variable_set *set;
     const struct floc *flocp;
{
  register unsigned int i;
  register unsigned int hashval;
  register struct variable *v;

  hashval = 0;
  for (i = 0; i < length; ++i)
    HASH (hashval, name[i]);
  hashval %= set->buckets;

  for (v = set->table[hashval]; v != 0; v = v->next)
    if (*v->name == *name
	&& strneq (v->name + 1, name + 1, length - 1)
	&& v->name[length] == '\0')
      break;

  if (env_overrides && origin == o_env)
    origin = o_env_override;

  if (v != 0)
    {
      if (env_overrides && v->origin == o_env)
	/* V came from in the environment.  Since it was defined
	   before the switches were parsed, it wasn't affected by -e.  */
	v->origin = o_env_override;

      /* A variable of this name is already defined.
	 If the old definition is from a stronger source
	 than this one, don't redefine it.  */
      if ((int) origin >= (int) v->origin)
	{
	  if (v->value != 0)
	    free (v->value);
	  v->value = xstrdup (value);
          if (flocp != 0)
            v->fileinfo = *flocp;
          else
            v->fileinfo.filenm = 0;
	  v->origin = origin;
	  v->recursive = recursive;
	}
      return v;
    }

  /* Create a new variable definition and add it to the hash table.  */

  v = (struct variable *) xmalloc (sizeof (struct variable));
  v->name = savestring (name, length);
  v->value = xstrdup (value);
  if (flocp != 0)
    v->fileinfo = *flocp;
  else
    v->fileinfo.filenm = 0;
  v->origin = origin;
  v->recursive = recursive;
  v->expanding = 0;
  v->per_target = 0;
  v->append = 0;
  v->export = v_default;
  v->next = set->table[hashval];
  set->table[hashval] = v;
  return v;
}

/* Lookup a variable whose name is a string starting at NAME
   and with LENGTH chars.  NAME need not be null-terminated.
   Returns address of the `struct variable' containing all info
   on the variable, or nil if no such variable is defined.

   If we find a variable which is in the process of being expanded,
   try to find one further up the set_list chain.  If we don't find
   one that isn't being expanded, return a pointer to whatever we
   _did_ find.  */

struct variable *
lookup_variable (name, length)
     char *name;
     unsigned int length;
{
  register struct variable_set_list *setlist;
  struct variable *firstv = 0;

  register unsigned int i;
  register unsigned int rawhash = 0;

  for (i = 0; i < length; ++i)
    HASH (rawhash, name[i]);

  for (setlist = current_variable_set_list;
       setlist != 0; setlist = setlist->next)
    {
      register struct variable_set *set = setlist->set;
      register unsigned int hashval = rawhash % set->buckets;
      register struct variable *v;

      /* Look through this set list.  */
      for (v = set->table[hashval]; v != 0; v = v->next)
	if (*v->name == *name
	    && strneq (v->name + 1, name + 1, length - 1)
	    && v->name[length] == '\0')
          break;

      /* If we didn't find anything, go to the next set list.  */
      if (!v)
        continue;

      /* If it's not being expanded already, we're done.  */
      if (!v->expanding)
        return v;

      /* It is, so try to find another one.  If this is the first one we've
         seen, keep a pointer in case we don't find anything else.  */
      if (!firstv)
        firstv = v;
    }

#ifdef VMS
  /* since we don't read envp[] on startup, try to get the
     variable via getenv() here.  */
  if (!firstv)
    {
      char *vname = alloca (length + 1);
      char *value;
      strncpy (vname, name, length);
      vname[length] = 0;
      value = getenv (vname);
      if (value != 0)
	{
	  char *sptr;
	  int scnt;

	  sptr = value;
	  scnt = 0;

          if (listp)
            *listp = current_variable_set_list;

	  while ((sptr = strchr (sptr, '$')))
	    {
	      scnt++;
	      sptr++;
	    }

	  if (scnt > 0)
	    {
	      char *nvalue;
	      char *nptr;

	      nvalue = alloca (length + scnt + 1);
	      sptr = value;
	      nptr = nvalue;

	      while (*sptr)
		{
		  if (*sptr == '$')
		    {
		      *nptr++ = '$';
		      *nptr++ = '$';
		    }
		  else
		    {
		      *nptr++ = *sptr;
		    }
		  sptr++;
		}

	      return define_variable (vname, length, nvalue, o_env, 1);

	    }

	  return define_variable (vname, length, value, o_env, 1);
	}
    }
#endif /* VMS */

  return firstv;
}

/* Lookup a variable whose name is a string starting at NAME
   and with LENGTH chars in set SET.  NAME need not be null-terminated.
   Returns address of the `struct variable' containing all info
   on the variable, or nil if no such variable is defined.  */

static struct variable *
lookup_variable_in_set (name, length, set)
     char *name;
     unsigned int length;
     struct variable_set *set;
{
  register unsigned int i;
  register unsigned int hash = 0;
  register struct variable *v;

  for (i = 0; i < length; ++i)
    HASH (hash, name[i]);
  hash %= set->buckets;

  for (v = set->table[hash]; v != 0; v = v->next)
    if (*v->name == *name
        && strneq (v->name + 1, name + 1, length - 1)
        && v->name[length] == 0)
      return v;

  return 0;
}

/* Initialize FILE's variable set list.  If FILE already has a variable set
   list, the topmost variable set is left intact, but the the rest of the
   chain is replaced with FILE->parent's setlist.  If we're READing a
   makefile, don't do the pattern variable search now, since the pattern
   variable might not have been defined yet.  */

void
initialize_file_variables (file, reading)
     struct file *file;
     int reading;
{
  register struct variable_set_list *l = file->variables;

  if (l == 0)
    {
      l = (struct variable_set_list *)
	xmalloc (sizeof (struct variable_set_list));
      l->set = (struct variable_set *) xmalloc (sizeof (struct variable_set));
      l->set->buckets = PERFILE_VARIABLE_BUCKETS;
      l->set->table = (struct variable **)
	xmalloc (l->set->buckets * sizeof (struct variable *));
      bzero ((char *) l->set->table,
	     l->set->buckets * sizeof (struct variable *));
      file->variables = l;
    }

  if (file->parent == 0)
    l->next = &global_setlist;
  else
    {
      initialize_file_variables (file->parent, reading);
      l->next = file->parent->variables;
    }

  /* If we're not reading makefiles and we haven't looked yet, see if
     we can find a pattern variable.  */

  if (!reading && !file->pat_searched)
    {
      struct pattern_var *p = lookup_pattern_var (file->name);

      file->pat_searched = 1;
      if (p != 0)
        {
          /* If we found one, insert it between the current target's
             variables and the next set, whatever it is.  */
          file->pat_variables = (struct variable_set_list *)
            xmalloc (sizeof (struct variable_set_list));
          file->pat_variables->set = p->vars->set;
        }
    }

  /* If we have a pattern variable match, set it up.  */

  if (file->pat_variables != 0)
    {
      file->pat_variables->next = l->next;
      l->next = file->pat_variables;
    }
}

/* Pop the top set off the current variable set list,
   and free all its storage.  */

void
pop_variable_scope ()
{
  register struct variable_set_list *setlist = current_variable_set_list;
  register struct variable_set *set = setlist->set;
  register unsigned int i;

  current_variable_set_list = setlist->next;
  free ((char *) setlist);

  for (i = 0; i < set->buckets; ++i)
    {
      register struct variable *next = set->table[i];
      while (next != 0)
	{
	  register struct variable *v = next;
	  next = v->next;

	  free (v->name);
	  if (v->value)
	    free (v->value);
	  free ((char *) v);
	}
    }
  free ((char *) set->table);
  free ((char *) set);
}

struct variable_set_list *
create_new_variable_set ()
{
  register struct variable_set_list *setlist;
  register struct variable_set *set;

  set = (struct variable_set *) xmalloc (sizeof (struct variable_set));
  set->buckets = SMALL_SCOPE_VARIABLE_BUCKETS;
  set->table = (struct variable **)
    xmalloc (set->buckets * sizeof (struct variable *));
  bzero ((char *) set->table, set->buckets * sizeof (struct variable *));

  setlist = (struct variable_set_list *)
    xmalloc (sizeof (struct variable_set_list));
  setlist->set = set;
  setlist->next = current_variable_set_list;

  return setlist;
}

/* Create a new variable set and push it on the current setlist.  */

struct variable_set_list *
push_new_variable_scope ()
{
  return (current_variable_set_list = create_new_variable_set());
}

/* Merge SET1 into SET0, freeing unused storage in SET1.  */

static void
merge_variable_sets (set0, set1)
     struct variable_set *set0, *set1;
{
  register unsigned int bucket1;

  for (bucket1 = 0; bucket1 < set1->buckets; ++bucket1)
    {
      register struct variable *v1 = set1->table[bucket1];
      while (v1 != 0)
	{
	  struct variable *next = v1->next;
	  unsigned int bucket0;
	  register struct variable *v0;

	  if (set1->buckets >= set0->buckets)
	    bucket0 = bucket1;
	  else
	    {
	      register char *n;
	      bucket0 = 0;
	      for (n = v1->name; *n != '\0'; ++n)
		HASH (bucket0, *n);
	    }
	  bucket0 %= set0->buckets;

	  for (v0 = set0->table[bucket0]; v0 != 0; v0 = v0->next)
	    if (streq (v0->name, v1->name))
	      break;

	  if (v0 == 0)
	    {
	      /* There is no variable in SET0 with the same name.  */
	      v1->next = set0->table[bucket0];
	      set0->table[bucket0] = v1;
	    }
	  else
	    {
	      /* The same variable exists in both sets.
		 SET0 takes precedence.  */
	      free (v1->value);
	      free ((char *) v1);
	    }

	  v1 = next;
	}
    }
}

/* Merge SETLIST1 into SETLIST0, freeing unused storage in SETLIST1.  */

void
merge_variable_set_lists (setlist0, setlist1)
     struct variable_set_list **setlist0, *setlist1;
{
  register struct variable_set_list *list0 = *setlist0;
  struct variable_set_list *last0 = 0;

  while (setlist1 != 0 && list0 != 0)
    {
      struct variable_set_list *next = setlist1;
      setlist1 = setlist1->next;

      merge_variable_sets (list0->set, next->set);

      last0 = list0;
      list0 = list0->next;
    }

  if (setlist1 != 0)
    {
      if (last0 == 0)
	*setlist0 = setlist1;
      else
	last0->next = setlist1;
    }
}

/* Define the automatic variables, and record the addresses
   of their structures so we can change their values quickly.  */

void
define_automatic_variables ()
{
#ifdef WINDOWS32
  extern char* default_shell;
#else
  extern char default_shell[];
#endif
  register struct variable *v;
  char buf[200];

  sprintf (buf, "%u", makelevel);
  (void) define_variable ("MAKELEVEL", 9, buf, o_env, 0);

  sprintf (buf, "%s%s%s",
	   version_string,
	   (remote_description == 0 || remote_description[0] == '\0')
	   ? "" : "-",
	   (remote_description == 0 || remote_description[0] == '\0')
	   ? "" : remote_description);
  (void) define_variable ("MAKE_VERSION", 12, buf, o_default, 0);

#ifdef  __MSDOS__
  /* Allow to specify a special shell just for Make,
     and use $COMSPEC as the default $SHELL when appropriate.  */
  {
    static char shell_str[] = "SHELL";
    const int shlen = sizeof (shell_str) - 1;
    struct variable *mshp = lookup_variable ("MAKESHELL", 9);
    struct variable *comp = lookup_variable ("COMSPEC", 7);

    /* Make $MAKESHELL override $SHELL even if -e is in effect.  */
    if (mshp)
      (void) define_variable (shell_str, shlen,
			      mshp->value, o_env_override, 0);
    else if (comp)
      {
	/* $COMSPEC shouldn't override $SHELL.  */
	struct variable *shp = lookup_variable (shell_str, shlen);

	if (!shp)
	  (void) define_variable (shell_str, shlen, comp->value, o_env, 0);
      }
  }
#endif

  /* This won't override any definition, but it
     will provide one if there isn't one there.  */
  v = define_variable ("SHELL", 5, default_shell, o_default, 0);
  v->export = v_export;		/* Always export SHELL.  */

  /* On MSDOS we do use SHELL from environment, since
     it isn't a standard environment variable on MSDOS,
     so whoever sets it, does that on purpose.  */
#ifndef __MSDOS__
  /* Don't let SHELL come from the environment.  */
  if (*v->value == '\0' || v->origin == o_env || v->origin == o_env_override)
    {
      free (v->value);
      v->origin = o_file;
      v->value = xstrdup (default_shell);
    }
#endif

  /* Make sure MAKEFILES gets exported if it is set.  */
  v = define_variable ("MAKEFILES", 9, "", o_default, 0);
  v->export = v_ifset;

  /* Define the magic D and F variables in terms of
     the automatic variables they are variations of.  */

#ifdef VMS
  define_variable ("@D", 2, "$(dir $@)", o_automatic, 1);
  define_variable ("%D", 2, "$(dir $%)", o_automatic, 1);
  define_variable ("*D", 2, "$(dir $*)", o_automatic, 1);
  define_variable ("<D", 2, "$(dir $<)", o_automatic, 1);
  define_variable ("?D", 2, "$(dir $?)", o_automatic, 1);
  define_variable ("^D", 2, "$(dir $^)", o_automatic, 1);
  define_variable ("+D", 2, "$(dir $+)", o_automatic, 1);
#else
  define_variable ("@D", 2, "$(patsubst %/,%,$(dir $@))", o_automatic, 1);
  define_variable ("%D", 2, "$(patsubst %/,%,$(dir $%))", o_automatic, 1);
  define_variable ("*D", 2, "$(patsubst %/,%,$(dir $*))", o_automatic, 1);
  define_variable ("<D", 2, "$(patsubst %/,%,$(dir $<))", o_automatic, 1);
  define_variable ("?D", 2, "$(patsubst %/,%,$(dir $?))", o_automatic, 1);
  define_variable ("^D", 2, "$(patsubst %/,%,$(dir $^))", o_automatic, 1);
  define_variable ("+D", 2, "$(patsubst %/,%,$(dir $+))", o_automatic, 1);
#endif
  define_variable ("@F", 2, "$(notdir $@)", o_automatic, 1);
  define_variable ("%F", 2, "$(notdir $%)", o_automatic, 1);
  define_variable ("*F", 2, "$(notdir $*)", o_automatic, 1);
  define_variable ("<F", 2, "$(notdir $<)", o_automatic, 1);
  define_variable ("?F", 2, "$(notdir $?)", o_automatic, 1);
  define_variable ("^F", 2, "$(notdir $^)", o_automatic, 1);
  define_variable ("+F", 2, "$(notdir $+)", o_automatic, 1);
}

int export_all_variables;

/* Create a new environment for FILE's commands.
   If FILE is nil, this is for the `shell' function.
   The child's MAKELEVEL variable is incremented.  */

char **
target_environment (file)
     struct file *file;
{
  struct variable_set_list *set_list;
  register struct variable_set_list *s;
  struct variable_bucket
    {
      struct variable_bucket *next;
      struct variable *variable;
    };
  struct variable_bucket **table;
  unsigned int buckets;
  register unsigned int i;
  register unsigned nvariables;
  char **result;
  unsigned int mklev_hash;

  if (file == 0)
    set_list = current_variable_set_list;
  else
    set_list = file->variables;

  /* Find the lowest number of buckets in any set in the list.  */
  s = set_list;
  buckets = s->set->buckets;
  for (s = s->next; s != 0; s = s->next)
    if (s->set->buckets < buckets)
      buckets = s->set->buckets;

  /* Find the hash value of the bucket `MAKELEVEL' will fall into.  */
  {
    char *p = "MAKELEVEL";
    mklev_hash = 0;
    while (*p != '\0')
      HASH (mklev_hash, *p++);
  }

  /* Temporarily allocate a table with that many buckets.  */
  table = (struct variable_bucket **)
    alloca (buckets * sizeof (struct variable_bucket *));
  bzero ((char *) table, buckets * sizeof (struct variable_bucket *));

  /* Run through all the variable sets in the list,
     accumulating variables in TABLE.  */
  nvariables = 0;
  for (s = set_list; s != 0; s = s->next)
    {
      register struct variable_set *set = s->set;
      for (i = 0; i < set->buckets; ++i)
	{
	  register struct variable *v;
	  for (v = set->table[i]; v != 0; v = v->next)
	    {
	      unsigned int j = i % buckets;
	      register struct variable_bucket *ov;
	      register char *p = v->name;

	      if (i == mklev_hash % set->buckets
		  && streq (v->name, "MAKELEVEL"))
		/* Don't include MAKELEVEL because it will be
		   added specially at the end.  */
		continue;

              /* If this is a per-target variable and it hasn't been touched
                 already then look up the global version and take its export
                 value.  */
              if (v->per_target && v->export == v_default)
                {
                  struct variable *gv;

                  gv = lookup_variable_in_set(v->name, strlen(v->name),
                                              &global_variable_set);
                  if (gv)
                    v->export = gv->export;
                }

	      switch (v->export)
		{
		case v_default:
		  if (v->origin == o_default || v->origin == o_automatic)
		    /* Only export default variables by explicit request.  */
		    continue;

		  if (! export_all_variables
		      && v->origin != o_command
		      && v->origin != o_env && v->origin != o_env_override)
		    continue;

		  if (*p != '_' && (*p < 'A' || *p > 'Z')
		      && (*p < 'a' || *p > 'z'))
		    continue;
		  for (++p; *p != '\0'; ++p)
		    if (*p != '_' && (*p < 'a' || *p > 'z')
			&& (*p < 'A' || *p > 'Z') && (*p < '0' || *p > '9'))
		      continue;
		  if (*p != '\0')
		    continue;
		  break;

                case v_export:
                  break;

                case v_noexport:
                  continue;

		case v_ifset:
		  if (v->origin == o_default)
		    continue;
		  break;
		}

              /* If this was from a different-sized hash table, then
                 recalculate the bucket it goes in.  */
              if (set->buckets != buckets)
                {
                  register char *np;

                  j = 0;
                  for (np = v->name; *np != '\0'; ++np)
                    HASH (j, *np);
                  j %= buckets;
                }

	      for (ov = table[j]; ov != 0; ov = ov->next)
		if (streq (v->name, ov->variable->name))
		  break;

	      if (ov == 0)
		{
		  register struct variable_bucket *entry;
		  entry = (struct variable_bucket *)
		    alloca (sizeof (struct variable_bucket));
		  entry->next = table[j];
		  entry->variable = v;
		  table[j] = entry;
		  ++nvariables;
		}
	    }
	}
    }

  result = (char **) xmalloc ((nvariables + 2) * sizeof (char *));
  nvariables = 0;
  for (i = 0; i < buckets; ++i)
    {
      register struct variable_bucket *b;
      for (b = table[i]; b != 0; b = b->next)
	{
	  register struct variable *v = b->variable;

	  /* If V is recursively expanded and didn't come from the environment,
	     expand its value.  If it came from the environment, it should
	     go back into the environment unchanged.  */
	  if (v->recursive
	      && v->origin != o_env && v->origin != o_env_override)
	    {
	      char *value = recursively_expand (v);
#ifdef WINDOWS32
              if (strcmp(v->name, "Path") == 0 ||
                  strcmp(v->name, "PATH") == 0)
                convert_Path_to_windows32(value, ';');
#endif
	      result[nvariables++] = concat (v->name, "=", value);
	      free (value);
	    }
	  else
#ifdef WINDOWS32
          {
            if (strcmp(v->name, "Path") == 0 ||
                strcmp(v->name, "PATH") == 0)
              convert_Path_to_windows32(v->value, ';');
            result[nvariables++] = concat (v->name, "=", v->value);
          }
#else
	    result[nvariables++] = concat (v->name, "=", v->value);
#endif
	}
    }
  result[nvariables] = (char *) xmalloc (100);
  (void) sprintf (result[nvariables], "MAKELEVEL=%u", makelevel + 1);
  result[++nvariables] = 0;

  return result;
}

/* Try to interpret LINE (a null-terminated string) as a variable definition.

   ORIGIN may be o_file, o_override, o_env, o_env_override,
   or o_command specifying that the variable definition comes
   from a makefile, an override directive, the environment with
   or without the -e switch, or the command line.

   See the comments for parse_variable_definition().

   If LINE was recognized as a variable definition, a pointer to its `struct
   variable' is returned.  If LINE is not a variable definition, NULL is
   returned.  */

struct variable *
try_variable_definition (flocp, line, origin, target_var)
     const struct floc *flocp;
     char *line;
     enum variable_origin origin;
     int target_var;
{
  register int c;
  register char *p = line;
  register char *beg;
  register char *end;
  enum { f_bogus,
         f_simple, f_recursive, f_append, f_conditional } flavor = f_bogus;
  char *name, *expanded_name, *value, *alloc_value=NULL;
  struct variable *v;
  int append = 0;

  while (1)
    {
      c = *p++;
      if (c == '\0' || c == '#')
	return 0;
      if (c == '=')
	{
	  end = p - 1;
	  flavor = f_recursive;
	  break;
	}
      else if (c == ':')
	if (*p == '=')
	  {
	    end = p++ - 1;
	    flavor = f_simple;
	    break;
	  }
	else
	  /* A colon other than := is a rule line, not a variable defn.  */
	  return 0;
      else if (c == '+' && *p == '=')
	{
	  end = p++ - 1;
	  flavor = f_append;
	  break;
	}
      else if (c == '?' && *p == '=')
        {
          end = p++ - 1;
          flavor = f_conditional;
          break;
        }
      else if (c == '$')
	{
	  /* This might begin a variable expansion reference.  Make sure we
	     don't misrecognize chars inside the reference as =, := or +=.  */
	  char closeparen;
	  int count;
	  c = *p++;
	  if (c == '(')
	    closeparen = ')';
	  else if (c == '{')
	    closeparen = '}';
	  else
	    continue;		/* Nope.  */

	  /* P now points past the opening paren or brace.
	     Count parens or braces until it is matched.  */
	  count = 0;
	  for (; *p != '\0'; ++p)
	    {
	      if (*p == c)
		++count;
	      else if (*p == closeparen && --count < 0)
		{
		  ++p;
		  break;
		}
	    }
	}
    }

  beg = next_token (line);
  while (end > beg && isblank (end[-1]))
    --end;
  p = next_token (p);

  /* Expand the name, so "$(foo)bar = baz" works.  */
  name = (char *) alloca (end - beg + 1);
  bcopy (beg, name, end - beg);
  name[end - beg] = '\0';
  expanded_name = allocated_variable_expand (name);

  if (expanded_name[0] == '\0')
    fatal (flocp, _("empty variable name"));

  /* Calculate the variable's new value in VALUE.  */

  switch (flavor)
    {
    case f_bogus:
      /* Should not be possible.  */
      abort ();
    case f_simple:
      /* A simple variable definition "var := value".  Expand the value.
         We have to allocate memory since otherwise it'll clobber the
	 variable buffer, and we may still need that if we're looking at a
         target-specific variable.  */
      value = alloc_value = allocated_variable_expand (p);
      break;
    case f_conditional:
      /* A conditional variable definition "var ?= value".
         The value is set IFF the variable is not defined yet. */
      v = lookup_variable(expanded_name, strlen(expanded_name));
      if (v)
        {
          free(expanded_name);
          return v;
        }
      flavor = f_recursive;
      /* FALLTHROUGH */
    case f_recursive:
      /* A recursive variable definition "var = value".
	 The value is used verbatim.  */
      value = p;
      break;
    case f_append:
      /* If we have += but we're in a target variable context, defer the
         append until the context expansion.  */
      if (target_var)
        {
          append = 1;
          flavor = f_recursive;
          value = p;
          break;
        }

      /* An appending variable definition "var += value".
	 Extract the old value and append the new one.  */
      v = lookup_variable (expanded_name, strlen (expanded_name));
      if (v == 0)
	{
	  /* There was no old value.
	     This becomes a normal recursive definition.  */
	  value = p;
	  flavor = f_recursive;
	}
      else
	{
	  /* Paste the old and new values together in VALUE.  */

	  unsigned int oldlen, newlen;

	  if (v->recursive)
	    /* The previous definition of the variable was recursive.
	       The new value comes from the unexpanded old and new values.  */
	    flavor = f_recursive;
	  else
	    /* The previous definition of the variable was simple.
	       The new value comes from the old value, which was expanded
	       when it was set; and from the expanded new value.  Allocate
               memory for the expansion as we may still need the rest of the
               buffer if we're looking at a target-specific variable.  */
	    p = alloc_value = allocated_variable_expand (p);

	  oldlen = strlen (v->value);
	  newlen = strlen (p);
	  value = (char *) alloca (oldlen + 1 + newlen + 1);
	  bcopy (v->value, value, oldlen);
	  value[oldlen] = ' ';
	  bcopy (p, &value[oldlen + 1], newlen + 1);
	}
    }

#ifdef __MSDOS__
  /* Many Unix Makefiles include a line saying "SHELL=/bin/sh", but
     non-Unix systems don't conform to this default configuration (in
     fact, most of them don't even have `/bin').  On the other hand,
     $SHELL in the environment, if set, points to the real pathname of
     the shell.
     Therefore, we generally won't let lines like "SHELL=/bin/sh" from
     the Makefile override $SHELL from the environment.  But first, we
     look for the basename of the shell in the directory where SHELL=
     points, and along the $PATH; if it is found in any of these places,
     we define $SHELL to be the actual pathname of the shell.  Thus, if
     you have bash.exe installed as d:/unix/bash.exe, and d:/unix is on
     your $PATH, then SHELL=/usr/local/bin/bash will have the effect of
     defining SHELL to be "d:/unix/bash.exe".  */
  if ((origin == o_file || origin == o_override)
      && strcmp (expanded_name, "SHELL") == 0)
    {
      char shellpath[PATH_MAX];
      extern char * __dosexec_find_on_path (const char *, char *[], char *);

      /* See if we can find "/bin/sh.exe", "/bin/sh.com", etc.  */
      if (__dosexec_find_on_path (value, (char **)0, shellpath))
	{
	  char *p;

	  for (p = shellpath; *p; p++)
	    {
	      if (*p == '\\')
		*p = '/';
	    }
	  v = define_variable_loc (expanded_name, strlen (expanded_name),
                                   shellpath, origin, flavor == f_recursive,
                                   flocp);
	}
      else
	{
	  char *shellbase, *bslash;
	  struct variable *pathv = lookup_variable ("PATH", 4);
	  char *path_string;
	  char *fake_env[2];
	  size_t pathlen = 0;

	  shellbase = strrchr (value, '/');
	  bslash = strrchr (value, '\\');
	  if (!shellbase || bslash > shellbase)
	    shellbase = bslash;
	  if (!shellbase && value[1] == ':')
	    shellbase = value + 1;
	  if (shellbase)
	    shellbase++;
	  else
	    shellbase = value;

	  /* Search for the basename of the shell (with standard
	     executable extensions) along the $PATH.  */
	  if (pathv)
	    pathlen = strlen (pathv->value);
	  path_string = (char *)xmalloc (5 + pathlen + 2 + 1);
	  /* On MSDOS, current directory is considered as part of $PATH.  */
	  sprintf (path_string, "PATH=.;%s", pathv ? pathv->value : "");
	  fake_env[0] = path_string;
	  fake_env[1] = (char *)0;
	  if (__dosexec_find_on_path (shellbase, fake_env, shellpath))
	    {
	      char *p;

	      for (p = shellpath; *p; p++)
		{
		  if (*p == '\\')
		    *p = '/';
		}
	      v = define_variable_loc (expanded_name, strlen (expanded_name),
                                       shellpath, origin,
                                       flavor == f_recursive, flocp);
	    }
	  else
	    v = lookup_variable (expanded_name, strlen (expanded_name));

	  free (path_string);
	}
    }
  else
#endif /* __MSDOS__ */
#ifdef WINDOWS32
  if ((origin == o_file || origin == o_override)
      && strcmp (expanded_name, "SHELL") == 0) {
    extern char* default_shell;

    /*
     * Call shell locator function. If it returns TRUE, then
	 * set no_default_sh_exe to indicate sh was found and
     * set new value for SHELL variable.
	 */
    if (find_and_set_default_shell(value)) {
       v = define_variable_loc (expanded_name, strlen (expanded_name),
                                default_shell, origin, flavor == f_recursive,
                                flocp);
       no_default_sh_exe = 0;
    }
  } else
#endif

  v = define_variable_loc (expanded_name, strlen (expanded_name), value,
                           origin, flavor == f_recursive, flocp);

  v->append = append;

  if (alloc_value)
    free (alloc_value);
  free (expanded_name);

  return v;
}

/* Print information for variable V, prefixing it with PREFIX.  */

static void
print_variable (v, prefix)
     register struct variable *v;
     char *prefix;
{
  const char *origin;

  switch (v->origin)
    {
    case o_default:
      origin = _("default");
      break;
    case o_env:
      origin = _("environment");
      break;
    case o_file:
      origin = _("makefile");
      break;
    case o_env_override:
      origin = _("environment under -e");
      break;
    case o_command:
      origin = _("command line");
      break;
    case o_override:
      origin = _("`override' directive");
      break;
    case o_automatic:
      origin = _("automatic");
      break;
    case o_invalid:
    default:
      abort ();
    }
  fputs ("# ", stdout);
  fputs (origin, stdout);
  if (v->fileinfo.filenm)
    printf (" (from `%s', line %lu)", v->fileinfo.filenm, v->fileinfo.lineno);
  putchar ('\n');
  fputs (prefix, stdout);

  /* Is this a `define'?  */
  if (v->recursive && strchr (v->value, '\n') != 0)
    printf ("define %s\n%s\nendef\n", v->name, v->value);
  else
    {
      register char *p;

      printf ("%s %s= ", v->name, v->recursive ? v->append ? "+" : "" : ":");

      /* Check if the value is just whitespace.  */
      p = next_token (v->value);
      if (p != v->value && *p == '\0')
	/* All whitespace.  */
	printf ("$(subst ,,%s)", v->value);
      else if (v->recursive)
	fputs (v->value, stdout);
      else
	/* Double up dollar signs.  */
	for (p = v->value; *p != '\0'; ++p)
	  {
	    if (*p == '$')
	      putchar ('$');
	    putchar (*p);
	  }
      putchar ('\n');
    }
}


/* Print all the variables in SET.  PREFIX is printed before
   the actual variable definitions (everything else is comments).  */

void
print_variable_set (set, prefix)
     register struct variable_set *set;
     char *prefix;
{
  register unsigned int i, nvariables, per_bucket;
  register struct variable *v;

  per_bucket = nvariables = 0;
  for (i = 0; i < set->buckets; ++i)
    {
      register unsigned int this_bucket = 0;

      for (v = set->table[i]; v != 0; v = v->next)
	{
	  ++this_bucket;
	  print_variable (v, prefix);
	}

      nvariables += this_bucket;
      if (this_bucket > per_bucket)
	per_bucket = this_bucket;
    }

  if (nvariables == 0)
    puts (_("# No variables."));
  else
    {
      printf (_("# %u variables in %u hash buckets.\n"),
	      nvariables, set->buckets);
#ifndef	NO_FLOAT
      printf (_("# average of %.1f variables per bucket, \
max %u in one bucket.\n"),
	      (double) nvariables / (double) set->buckets,
	      per_bucket);
#else
      {
	int f = (nvariables * 1000 + 5) / set->buckets;
	printf (_("# average of %d.%d variables per bucket, \
max %u in one bucket.\n"),
	      f/10, f%10,
	      per_bucket);
      }
#endif
    }
}


/* Print the data base of variables.  */

void
print_variable_data_base ()
{
  puts (_("\n# Variables\n"));

  print_variable_set (&global_variable_set, "");
}


/* Print all the local variables of FILE.  */

void
print_file_variables (file)
     struct file *file;
{
  if (file->variables != 0)
    print_variable_set (file->variables->set, "# ");
}

#ifdef WINDOWS32
void
sync_Path_environment(void)
{
    char* path = allocated_variable_expand("$(Path)");
    static char* environ_path = NULL;

    if (!path)
        return;

    /*
     * If done this before, don't leak memory unnecessarily.
     * Free the previous entry before allocating new one.
     */
    if (environ_path)
        free(environ_path);

    /*
     * Create something WINDOWS32 world can grok
     */
    convert_Path_to_windows32(path, ';');
    environ_path = concat("Path", "=", path);
    putenv(environ_path);
    free(path);
}
#endif



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         vpath.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Implementation of pattern-matching file search paths for GNU Make.
Copyright (C) 1988,89,91,92,93,94,95,96,97 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout
#ifdef WINDOWS32
#include "pathstuff.h"
#undef stderr
#define stderr stdout
#endif


/* Structure used to represent a selective VPATH searchpath.  */

struct vpath
  {
    struct vpath *next;	/* Pointer to next struct in the linked list.  */
    char *pattern;	/* The pattern to match.  */
    char *percent;	/* Pointer into `pattern' where the `%' is.  */
    unsigned int patlen;/* Length of the pattern.  */
    char **searchpath;	/* Null-terminated list of directories.  */
    unsigned int maxlen;/* Maximum length of any entry in the list.  */
  };

/* Linked-list of all selective VPATHs.  */

static struct vpath *vpaths;

/* Structure for the general VPATH given in the variable.  */

static struct vpath *general_vpath;

/* Structure for GPATH given in the variable.  */

static struct vpath *gpaths;

static int selective_vpath_search PARAMS ((struct vpath *path, char **file, FILE_TIMESTAMP *mtime_ptr));

/* Reverse the chain of selective VPATH lists so they
   will be searched in the order given in the makefiles
   and construct the list from the VPATH variable.  */

void
build_vpath_lists ()
{
  register struct vpath *new = 0;
  register struct vpath *old, *nexto;
  register char *p;

  /* Reverse the chain.  */
  for (old = vpaths; old != 0; old = nexto)
    {
      nexto = old->next;
      old->next = new;
      new = old;
    }

  vpaths = new;

  /* If there is a VPATH variable with a nonnull value, construct the
     general VPATH list from it.  We use variable_expand rather than just
     calling lookup_variable so that it will be recursively expanded.  */

  {
    /* Turn off --warn-undefined-variables while we expand SHELL and IFS.  */
    int save = warn_undefined_variables_flag;
    warn_undefined_variables_flag = 0;

    p = variable_expand ("$(strip $(VPATH))");

    warn_undefined_variables_flag = save;
  }

  if (*p != '\0')
    {
      /* Save the list of vpaths.  */
      struct vpath *save_vpaths = vpaths;

      /* Empty `vpaths' so the new one will have no next, and `vpaths'
	 will still be nil if P contains no existing directories.  */
      vpaths = 0;

      /* Parse P.  */
      construct_vpath_list ("%", p);

      /* Store the created path as the general path,
	 and restore the old list of vpaths.  */
      general_vpath = vpaths;
      vpaths = save_vpaths;
    }

  /* If there is a GPATH variable with a nonnull value, construct the
     GPATH list from it.  We use variable_expand rather than just
     calling lookup_variable so that it will be recursively expanded.  */

  {
    /* Turn off --warn-undefined-variables while we expand SHELL and IFS.  */
    int save = warn_undefined_variables_flag;
    warn_undefined_variables_flag = 0;

    p = variable_expand ("$(strip $(GPATH))");

    warn_undefined_variables_flag = save;
  }

  if (*p != '\0')
    {
      /* Save the list of vpaths.  */
      struct vpath *save_vpaths = vpaths;

      /* Empty `vpaths' so the new one will have no next, and `vpaths'
	 will still be nil if P contains no existing directories.  */
      vpaths = 0;

      /* Parse P.  */
      construct_vpath_list ("%", p);

      /* Store the created path as the GPATH,
	 and restore the old list of vpaths.  */
      gpaths = vpaths;
      vpaths = save_vpaths;
    }
}

/* Construct the VPATH listing for the pattern and searchpath given.

   This function is called to generate selective VPATH lists and also for
   the general VPATH list (which is in fact just a selective VPATH that
   is applied to everything).  The returned pointer is either put in the
   linked list of all selective VPATH lists or in the GENERAL_VPATH
   variable.

   If SEARCHPATH is nil, remove all previous listings with the same
   pattern.  If PATTERN is nil, remove all VPATH listings.  Existing
   and readable directories that are not "." given in the searchpath
   separated by the path element separator (defined in make.h) are
   loaded into the directory hash table if they are not there already
   and put in the VPATH searchpath for the given pattern with trailing
   slashes stripped off if present (and if the directory is not the
   root, "/").  The length of the longest entry in the list is put in
   the structure as well.  The new entry will be at the head of the
   VPATHS chain.  */

void
construct_vpath_list (pattern, dirpath)
     char *pattern, *dirpath;
{
  register unsigned int elem;
  register char *p;
  register char **vpath;
  register unsigned int maxvpath;
  unsigned int maxelem;
  char *percent = NULL;

  if (pattern != 0)
    {
      pattern = xstrdup (pattern);
      percent = find_percent (pattern);
    }

  if (dirpath == 0)
    {
      /* Remove matching listings.  */
      register struct vpath *path, *lastpath;

      lastpath = 0;
      path = vpaths;
      while (path != 0)
	{
	  struct vpath *next = path->next;

	  if (pattern == 0
	      || (((percent == 0 && path->percent == 0)
		   || (percent - pattern == path->percent - path->pattern))
		  && streq (pattern, path->pattern)))
	    {
	      /* Remove it from the linked list.  */
	      if (lastpath == 0)
		vpaths = path->next;
	      else
		lastpath->next = next;

	      /* Free its unused storage.  */
	      free (path->pattern);
	      free ((char *) path->searchpath);
	      free ((char *) path);
	    }
	  else
	    lastpath = path;

	  path = next;
	}

      if (pattern != 0)
	free (pattern);
      return;
    }

#ifdef WINDOWS32
    convert_vpath_to_windows32(dirpath, ';');
#endif

  /* Figure out the maximum number of VPATH entries and put it in
     MAXELEM.  We start with 2, one before the first separator and one
     nil (the list terminator) and increment our estimated number for
     each separator or blank we find.  */
  maxelem = 2;
  p = dirpath;
  while (*p != '\0')
    if (*p++ == PATH_SEPARATOR_CHAR || isblank (*p))
      ++maxelem;

  vpath = (char **) xmalloc (maxelem * sizeof (char *));
  maxvpath = 0;

  /* Skip over any initial separators and blanks.  */
  p = dirpath;
  while (*p == PATH_SEPARATOR_CHAR || isblank (*p))
    ++p;

  elem = 0;
  while (*p != '\0')
    {
      char *v;
      unsigned int len;

      /* Find the end of this entry.  */
      v = p;
      while (*p != '\0' && *p != PATH_SEPARATOR_CHAR && !isblank (*p))
	++p;

      len = p - v;
      /* Make sure there's no trailing slash,
	 but still allow "/" as a directory.  */
#ifdef __MSDOS__
      /* We need also to leave alone a trailing slash in "d:/".  */
      if (len > 3 || (len > 1 && v[1] != ':'))
#endif
      if (len > 1 && p[-1] == '/')
	--len;

      if (len > 1 || *v != '.')
	{
	  v = savestring (v, len);

	  /* Verify that the directory actually exists.  */

	  if (dir_file_exists_p (v, ""))
	    {
	      /* It does.  Put it in the list.  */
	      vpath[elem++] = dir_name (v);
	      free (v);
	      if (len > maxvpath)
		maxvpath = len;
	    }
	  else
	    /* The directory does not exist.  Omit from the list.  */
	    free (v);
	}

      /* Skip over separators and blanks between entries.  */
      while (*p == PATH_SEPARATOR_CHAR || isblank (*p))
	++p;
    }

  if (elem > 0)
    {
      struct vpath *path;
      /* ELEM is now incremented one element past the last
	 entry, to where the nil-pointer terminator goes.
	 Usually this is maxelem - 1.  If not, shrink down.  */
      if (elem < (maxelem - 1))
	vpath = (char **) xrealloc ((char *) vpath,
				    (elem + 1) * sizeof (char *));

      /* Put the nil-pointer terminator on the end of the VPATH list.  */
      vpath[elem] = 0;

      /* Construct the vpath structure and put it into the linked list.  */
      path = (struct vpath *) xmalloc (sizeof (struct vpath));
      path->searchpath = vpath;
      path->maxlen = maxvpath;
      path->next = vpaths;
      vpaths = path;

      /* Set up the members.  */
      path->pattern = pattern;
      path->percent = percent;
      path->patlen = strlen (pattern);
    }
  else
    {
      /* There were no entries, so free whatever space we allocated.  */
      free ((char *) vpath);
      if (pattern != 0)
	free (pattern);
    }
}

/* Search the GPATH list for a pathname string that matches the one passed
   in.  If it is found, return 1.  Otherwise we return 0.  */

int
gpath_search (file, len)
     char *file;
     int len;
{
  register char **gp;

  if (gpaths && (len <= gpaths->maxlen))
    for (gp = gpaths->searchpath; *gp != NULL; ++gp)
      if (strneq (*gp, file, len) && (*gp)[len] == '\0')
        return 1;

  return 0;
}

/* Search the VPATH list whose pattern matches *FILE for a directory
   where the name pointed to by FILE exists.  If it is found, we set *FILE to
   the newly malloc'd name of the existing file, *MTIME_PTR (if MTIME_PTR is
   not NULL) to its modtime (or zero if no stat call was done), and return 1.
   Otherwise we return 0.  */

int
vpath_search (file, mtime_ptr)
     char **file;
     FILE_TIMESTAMP *mtime_ptr;
{
  register struct vpath *v;

  /* If there are no VPATH entries or FILENAME starts at the root,
     there is nothing we can do.  */

  if (**file == '/'
#if defined (WINDOWS32) || defined (__MSDOS__)
      || **file == '\\'
      || (*file)[1] == ':'
#endif
      || (vpaths == 0 && general_vpath == 0))
    return 0;

  for (v = vpaths; v != 0; v = v->next)
    if (pattern_matches (v->pattern, v->percent, *file))
      if (selective_vpath_search (v, file, mtime_ptr))
	return 1;

  if (general_vpath != 0
      && selective_vpath_search (general_vpath, file, mtime_ptr))
    return 1;

  return 0;
}


/* Search the given VPATH list for a directory where the name pointed
   to by FILE exists.  If it is found, we set *FILE to the newly malloc'd
   name of the existing file, *MTIME_PTR (if MTIME_PTR is not NULL) to
   its modtime (or zero if no stat call was done), and we return 1.
   Otherwise we return 0.  */

static int
selective_vpath_search (path, file, mtime_ptr)
     struct vpath *path;
     char **file;
     FILE_TIMESTAMP *mtime_ptr;
{
  int not_target;
  char *name, *n;
  char *filename;
  register char **vpath = path->searchpath;
  unsigned int maxvpath = path->maxlen;
  register unsigned int i;
  unsigned int flen, vlen, name_dplen;
  int exists = 0;

  /* Find out if *FILE is a target.
     If and only if it is NOT a target, we will accept prospective
     files that don't exist but are mentioned in a makefile.  */
  {
    struct file *f = lookup_file (*file);
    not_target = f == 0 || !f->is_target;
  }

  flen = strlen (*file);

  /* Split *FILE into a directory prefix and a name-within-directory.
     NAME_DPLEN gets the length of the prefix; FILENAME gets the
     pointer to the name-within-directory and FLEN is its length.  */

  n = strrchr (*file, '/');
#if defined (WINDOWS32) || defined (__MSDOS__)
  /* We need the rightmost slash or backslash.  */
  {
    char *bslash = strrchr(*file, '\\');
    if (!n || bslash > n)
      n = bslash;
  }
#endif
  name_dplen = n != 0 ? n - *file : 0;
  filename = name_dplen > 0 ? n + 1 : *file;
  if (name_dplen > 0)
    flen -= name_dplen + 1;

  /* Allocate enough space for the biggest VPATH entry,
     a slash, the directory prefix that came with *FILE,
     another slash (although this one may not always be
     necessary), the filename, and a null terminator.  */
  name = (char *) xmalloc (maxvpath + 1 + name_dplen + 1 + flen + 1);

  /* Try each VPATH entry.  */
  for (i = 0; vpath[i] != 0; ++i)
    {
      int exists_in_cache = 0;

      n = name;

      /* Put the next VPATH entry into NAME at N and increment N past it.  */
      vlen = strlen (vpath[i]);
      bcopy (vpath[i], n, vlen);
      n += vlen;

      /* Add the directory prefix already in *FILE.  */
      if (name_dplen > 0)
	{
#ifndef VMS
	  *n++ = '/';
#endif
	  bcopy (*file, n, name_dplen);
	  n += name_dplen;
	}

#if defined (WINDOWS32) || defined (__MSDOS__)
      /* Cause the next if to treat backslash and slash alike.  */
      if (n != name && n[-1] == '\\' )
	n[-1] = '/';
#endif
      /* Now add the name-within-directory at the end of NAME.  */
#ifndef VMS
      if (n != name && n[-1] != '/')
	{
	  *n = '/';
	  bcopy (filename, n + 1, flen + 1);
	}
      else
#endif
	bcopy (filename, n, flen + 1);

      /* Check if the file is mentioned in a makefile.  If *FILE is not
	 a target, that is enough for us to decide this file exists.
	 If *FILE is a target, then the file must be mentioned in the
	 makefile also as a target to be chosen.

	 The restriction that *FILE must not be a target for a
	 makefile-mentioned file to be chosen was added by an
	 inadequately commented change in July 1990; I am not sure off
	 hand what problem it fixes.

	 In December 1993 I loosened this restriction to allow a file
	 to be chosen if it is mentioned as a target in a makefile.  This
	 seem logical.  */
      {
	struct file *f = lookup_file (name);
	if (f != 0)
	  exists = not_target || f->is_target;
      }

      if (!exists)
	{
	  /* That file wasn't mentioned in the makefile.
	     See if it actually exists.  */

#ifdef VMS
	  exists_in_cache = exists = dir_file_exists_p (vpath[i], filename);
#else
	  /* Clobber a null into the name at the last slash.
	     Now NAME is the name of the directory to look in.  */
	  *n = '\0';

	  /* We know the directory is in the hash table now because either
	     construct_vpath_list or the code just above put it there.
	     Does the file we seek exist in it?  */
	  exists_in_cache = exists = dir_file_exists_p (name, filename);
#endif
	}

      if (exists)
	{
	  /* The file is in the directory cache.
	     Now check that it actually exists in the filesystem.
	     The cache may be out of date.  When vpath thinks a file
	     exists, but stat fails for it, confusion results in the
	     higher levels.  */

	  struct stat st;

#ifndef VMS
	  /* Put the slash back in NAME.  */
	  *n = '/';
#endif

	  if (!exists_in_cache	/* Makefile-mentioned file need not exist.  */
	      || stat (name, &st) == 0) /* Does it really exist?  */
	    {
	      /* We have found a file.
		 Store the name we found into *FILE for the caller.  */

	      *file = savestring (name, (n + 1 - name) + flen);

	      if (mtime_ptr != 0)
		/* Store the modtime into *MTIME_PTR for the caller.
		   If we have had no need to stat the file here,
		   we record a zero modtime to indicate this.  */
		*mtime_ptr = (exists_in_cache
			      ? FILE_TIMESTAMP_STAT_MODTIME (st)
			      : (FILE_TIMESTAMP) 0);

	      free (name);
	      return 1;
	    }
	  else
	    exists = 0;
	}
    }

  free (name);
  return 0;
}

/* Print the data base of VPATH search paths.  */

void
print_vpath_data_base ()
{
  register unsigned int nvpaths;
  register struct vpath *v;

  puts (_("\n# VPATH Search Paths\n"));

  nvpaths = 0;
  for (v = vpaths; v != 0; v = v->next)
    {
      register unsigned int i;

      ++nvpaths;

      printf ("vpath %s ", v->pattern);

      for (i = 0; v->searchpath[i] != 0; ++i)
	printf ("%s%c", v->searchpath[i],
		v->searchpath[i + 1] == 0 ? '\n' : PATH_SEPARATOR_CHAR);
    }

  if (vpaths == 0)
    puts (_("# No `vpath' search paths."));
  else
    printf (_("\n# %u `vpath' search paths.\n"), nvpaths);

  if (general_vpath == 0)
    puts (_("\n# No general (`VPATH' variable) search path."));
  else
    {
      register char **path = general_vpath->searchpath;
      register unsigned int i;

      fputs (_("\n# General (`VPATH' variable) search path:\n# "), stdout);

      for (i = 0; path[i] != 0; ++i)
	printf ("%s%c", path[i],
		path[i + 1] == 0 ? '\n' : PATH_SEPARATOR_CHAR);
    }
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         default.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Data base of default implicit rules for GNU Make.
Copyright (C) 1988,89,90,91,92,93,94,95,96 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "rule.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "dep.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "variable.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout

/* Define GCC_IS_NATIVE if gcc is the native development environment on
   your system (gcc/bison/flex vs cc/yacc/lex).  */
#ifdef __MSDOS__
#define GCC_IS_NATIVE
#endif


/* This is the default list of suffixes for suffix rules.
   `.s' must come last, so that a `.o' file will be made from
   a `.c' or `.p' or ... file rather than from a .s file.  */

static char default_suffixes[]
#ifdef VMS
  = ".exe .olb .ln .obj .c .cxx .cc .pas .p .for .f .r .y .l .mar \
.s .ss .i .ii .mod .sym .def .h .info .dvi .tex .texinfo .texi .txinfo \
.w .ch .cweb .web .com .sh .elc .el";
#else
  = ".out .a .ln .o .c .cc .C .cpp .p .f .F .r .y .l .s .S \
.mod .sym .def .h .info .dvi .tex .texinfo .texi .txinfo \
.w .ch .web .sh .elc .el";
#endif

static struct pspec default_pattern_rules[] =
  {
    { "(%)", "%",
	"$(AR) $(ARFLAGS) $@ $<" },

    /* The X.out rules are only in BSD's default set because
       BSD Make has no null-suffix rules, so `foo.out' and
       `foo' are the same thing.  */
#ifdef VMS
    { "%.exe", "%",
        "copy $< $@" },
#else
    { "%.out", "%",
	"@rm -f $@ \n cp $< $@" },
#endif
    /* Syntax is "ctangle foo.w foo.ch foo.c".  */
    { "%.c", "%.w %.ch",
	"$(CTANGLE) $^ $@" },
    { "%.tex", "%.w %.ch",
	"$(CWEAVE) $^ $@" },

    { 0, 0, 0 }
  };

static struct pspec default_terminal_rules[] =
  {
#ifdef VMS
    /* RCS.  */
    { "%", "%$$5lv", /* Multinet style */
        "if f$$search($@) .nes. \"\" then +$(CHECKOUT,v)" },
    { "%", "[.$$rcs]%$$5lv", /* Multinet style */
        "if f$$search($@) .nes. \"\" then +$(CHECKOUT,v)" },
    { "%", "%_v", /* Normal style */
        "if f$$search($@) .nes. \"\" then +$(CHECKOUT,v)" },
    { "%", "[.rcs]%_v", /* Normal style */
        "if f$$search($@) .nes. \"\" then +$(CHECKOUT,v)" },

    /* SCCS.  */
	/* ain't no SCCS on vms */
#else
    /* RCS.  */
    { "%", "%,v",
	"$(CHECKOUT,v)" },
    { "%", "RCS/%,v",
	"$(CHECKOUT,v)" },
    { "%", "RCS/%",
	"$(CHECKOUT,v)" },

    /* SCCS.  */
    { "%", "s.%",
	"$(GET) $(GFLAGS) $(SCCS_OUTPUT_OPTION) $<" },
    { "%", "SCCS/s.%",
	"$(GET) $(GFLAGS) $(SCCS_OUTPUT_OPTION) $<" },
#endif /* !VMS */
    { 0, 0, 0 }
  };

static char *default_suffix_rules[] =
  {
#ifdef VMS
    ".obj.exe",
    "$(LINK.obj) $^ $(LOADLIBES) $(LDLIBS) $(CRT0) /exe=$@",
    ".mar.exe",
    "$(COMPILE.mar) $^ \n $(LINK.obj) $(subst .mar,.obj,$^) $(LOADLIBES) $(LDLIBS) $(CRT0) /exe=$@",
    ".s.exe",
    "$(COMPILE.s) $^ \n $(LINK.obj) $(subst .s,.obj,$^) $(LOADLIBES) $(LDLIBS) $(CRT0) /exe=$@",
    ".c.exe",
    "$(COMPILE.c) $^ \n $(LINK.obj) $(subst .c,.obj,$^) $(LOADLIBES) $(LDLIBS) $(CRT0) /exe=$@",
    ".cc.exe",
#ifdef GCC_IS_NATIVE
    "$(COMPILE.cc) $^ \n $(LINK.obj) $(CXXSTARTUP),sys$$disk:[]$(subst .cc,.obj,$^) $(LOADLIBES) $(LXLIBS) $(LDLIBS) $(CXXRT0) /exe=$@",
#else
    "$(COMPILE.cc) $^ \n $(CXXLINK.obj) $(subst .cc,.obj,$^) $(LOADLIBES) $(LXLIBS) $(LDLIBS) $(CXXRT0) /exe=$@",
    ".cxx.exe",
    "$(COMPILE.cxx) $^ \n $(CXXLINK.obj) $(subst .cxx,.obj,$^) $(LOADLIBES) $(LXLIBS) $(LDLIBS) $(CXXRT0) /exe=$@",
#endif
    ".for.exe",
    "$(COMPILE.for) $^ \n $(LINK.obj) $(subst .for,.obj,$^) $(LOADLIBES) $(LDLIBS) /exe=$@",
    ".pas.exe",
    "$(COMPILE.pas) $^ \n $(LINK.obj) $(subst .pas,.obj,$^) $(LOADLIBES) $(LDLIBS) /exe=$@",

    ".com",
    "copy $< >$@",

    ".mar.obj",
    "$(COMPILE.mar) /obj=$@ $<",
    ".s.obj",
    "$(COMPILE.s) /obj=$@ $<",
    ".ss.obj",
    "$(COMPILE.s) /obj=$@ $<",
    ".c.i",
    "$(COMPILE.c)/prep /list=$@ $<",
    ".c.s",
    "$(COMPILE.c)/noobj/machine /list=$@ $<",
    ".i.s",
    "$(COMPILE.c)/noprep/noobj/machine /list=$@ $<",
    ".c.obj",
    "$(COMPILE.c) /obj=$@ $<",
    ".cc.ii",
    "$(COMPILE.cc)/prep /list=$@ $<",
    ".cc.ss",
    "$(COMPILE.cc)/noobj/machine /list=$@ $<",
    ".ii.ss",
    "$(COMPILE.cc)/noprep/noobj/machine /list=$@ $<",
    ".cc.obj",
    "$(COMPILE.cc) /obj=$@ $<",
    ".for.obj",
    "$(COMPILE.for) /obj=$@ $<",
    ".pas.obj",
    "$(COMPILE.pas) /obj=$@ $<",

    ".y.c",
    "$(YACC.y) $< \n rename y_tab.c $@",
    ".l.c",
    "$(LEX.l) $< \n rename lexyy.c $@",

    ".texinfo.info",
    "$(MAKEINFO) $<",

    ".tex.dvi",
    "$(TEX) $<",

#else /* ! VMS */

    ".o",
    "$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".s",
    "$(LINK.s) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".S",
    "$(LINK.S) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".c",
    "$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".cc",
    "$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".C",
    "$(LINK.C) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".cpp",
    "$(LINK.cpp) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".f",
    "$(LINK.f) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".p",
    "$(LINK.p) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".F",
    "$(LINK.F) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".r",
    "$(LINK.r) $^ $(LOADLIBES) $(LDLIBS) -o $@",
    ".mod",
    "$(COMPILE.mod) -o $@ -e $@ $^",

    ".def.sym",
    "$(COMPILE.def) -o $@ $<",

    ".sh",
    "cat $< >$@ \n chmod a+x $@",

    ".s.o",
    "$(COMPILE.s) -o $@ $<",
    ".S.o",
    "$(COMPILE.S) -o $@ $<",
    ".c.o",
    "$(COMPILE.c) $(OUTPUT_OPTION) $<",
    ".cc.o",
    "$(COMPILE.cc) $(OUTPUT_OPTION) $<",
    ".C.o",
    "$(COMPILE.C) $(OUTPUT_OPTION) $<",
    ".cpp.o",
    "$(COMPILE.cpp) $(OUTPUT_OPTION) $<",
    ".f.o",
    "$(COMPILE.f) $(OUTPUT_OPTION) $<",
    ".p.o",
    "$(COMPILE.p) $(OUTPUT_OPTION) $<",
    ".F.o",
    "$(COMPILE.F) $(OUTPUT_OPTION) $<",
    ".r.o",
    "$(COMPILE.r) $(OUTPUT_OPTION) $<",
    ".mod.o",
    "$(COMPILE.mod) -o $@ $<",

    ".c.ln",
    "$(LINT.c) -C$* $<",
    ".y.ln",
#ifndef __MSDOS__
    "$(YACC.y) $< \n $(LINT.c) -C$* y.tab.c \n $(RM) y.tab.c",
#else
    "$(YACC.y) $< \n $(LINT.c) -C$* y_tab.c \n $(RM) y_tab.c",
#endif
    ".l.ln",
    "@$(RM) $*.c\n $(LEX.l) $< > $*.c\n$(LINT.c) -i $*.c -o $@\n $(RM) $*.c",

    ".y.c",
#ifndef __MSDOS__
    "$(YACC.y) $< \n mv -f y.tab.c $@",
#else
    "$(YACC.y) $< \n mv -f y_tab.c $@",
#endif
    ".l.c",
    "@$(RM) $@ \n $(LEX.l) $< > $@",

    ".F.f",
    "$(PREPROCESS.F) $(OUTPUT_OPTION) $<",
    ".r.f",
    "$(PREPROCESS.r) $(OUTPUT_OPTION) $<",

    /* This might actually make lex.yy.c if there's no %R%
       directive in $*.l, but in that case why were you
       trying to make $*.r anyway?  */
    ".l.r",
    "$(LEX.l) $< > $@ \n mv -f lex.yy.r $@",

    ".S.s",
    "$(PREPROCESS.S) $< > $@",

    ".texinfo.info",
    "$(MAKEINFO) $(MAKEINFO_FLAGS) $< -o $@",

    ".texi.info",
    "$(MAKEINFO) $(MAKEINFO_FLAGS) $< -o $@",

    ".txinfo.info",
    "$(MAKEINFO) $(MAKEINFO_FLAGS) $< -o $@",

    ".tex.dvi",
    "$(TEX) $<",

    ".texinfo.dvi",
    "$(TEXI2DVI) $(TEXI2DVI_FLAGS) $<",

    ".texi.dvi",
    "$(TEXI2DVI) $(TEXI2DVI_FLAGS) $<",

    ".txinfo.dvi",
    "$(TEXI2DVI) $(TEXI2DVI_FLAGS) $<",

    ".w.c",
    "$(CTANGLE) $< - $@",	/* The `-' says there is no `.ch' file.  */

    ".web.p",
    "$(TANGLE) $<",

    ".w.tex",
    "$(CWEAVE) $< - $@",	/* The `-' says there is no `.ch' file.  */

    ".web.tex",
    "$(WEAVE) $<",

#endif /* !VMS */

    0, 0,
  };

static char *default_variables[] =
  {
#ifdef VMS
#ifdef __ALPHA
    "ARCH", "ALPHA",
#else
    "ARCH", "VAX",
#endif
    "AR", "library/obj",
    "ARFLAGS", "/replace",
    "AS", "macro",
    "MACRO", "macro",
#ifdef GCC_IS_NATIVE
    "CC", "gcc",
#else
    "CC", "cc",
#endif
    "CD", "builtin_cd",
    "MAKE", "make",
    "ECHO", "write sys$$output \"",
#ifdef GCC_IS_NATIVE
    "C++", "gcc/plus",
    "CXX", "gcc/plus",
#else
    "C++", "cxx",
    "CXX", "cxx",
    "CXXLD", "cxxlink",
#endif
    "CO", "co",
    "CPP", "$(CC) /preprocess_only",
    "FC", "fortran",
    /* System V uses these, so explicit rules using them should work.
       However, there is no way to make implicit rules use them and FC.  */
    "F77", "$(FC)",
    "F77FLAGS", "$(FFLAGS)",
    "LD", "link",
    "LEX", "lex",
    "PC", "pascal",
    "YACC", "bison/yacc",
    "YFLAGS", "/Define/Verbose",
    "BISON", "bison",
    "MAKEINFO", "makeinfo",
    "TEX", "tex",
    "TEXINDEX", "texindex",

    "RM", "delete/nolog",

    "CSTARTUP", "",
#ifdef GCC_IS_NATIVE
    "CRT0", ",sys$$library:vaxcrtl.olb/lib,gnu_cc_library:crt0.obj",
    "CXXSTARTUP", "gnu_cc_library:crtbegin.obj",
    "CXXRT0", ",sys$$library:vaxcrtl.olb/lib,gnu_cc_library:crtend.obj,gnu_cc_library:gxx_main.obj",
    "LXLIBS", ",gnu_cc_library:libstdcxx.olb/lib,gnu_cc_library:libgccplus.olb/lib",
    "LDLIBS", ",gnu_cc_library:libgcc.olb/lib",
#else
    "CRT0", "",
    "CXXSTARTUP", "",
    "CXXRT0", "",
    "LXLIBS", "",
    "LDLIBS", "",
#endif

    "LINK.obj", "$(LD) $(LDFLAGS)",
#ifndef GCC_IS_NATIVE
    "CXXLINK.obj", "$(CXXLD) $(LDFLAGS)",
    "COMPILE.cxx", "$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",
#endif
    "COMPILE.c", "$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",
    "COMPILE.cc", "$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",
    "YACC.y", "$(YACC) $(YFLAGS)",
    "LEX.l", "$(LEX) $(LFLAGS)",
    "COMPILE.for", "$(FC) $(FFLAGS) $(TARGET_ARCH)",
    "COMPILE.pas", "$(PC) $(PFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",
    "COMPILE.mar", "$(MACRO) $(MACROFLAGS)",
    "COMPILE.s", "$(AS) $(ASFLAGS) $(TARGET_MACH)",
    "LINT.c", "$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",

    "MV", "rename/new_version",
    "CP", "copy",

#else /* !VMS */

    "AR", "ar",
    "ARFLAGS", "rv",
    "AS", "as",
#ifdef GCC_IS_NATIVE
    "CC", "gcc",
# ifdef __MSDOS__
    "CXX", "gpp",	/* g++ is an invalid name on MSDOS */
# else
    "CXX", "gcc",
# endif /* __MSDOS__ */
#else
    "CC", "cc",
    "CXX", "g++",
#endif

    /* This expands to $(CO) $(COFLAGS) $< $@ if $@ does not exist,
       and to the empty string if $@ does exist.  */
    "CHECKOUT,v", "+$(if $(wildcard $@),,$(CO) $(COFLAGS) $< $@)",
    "CO", "co",
    "COFLAGS", "",

    "CPP", "$(CC) -E",
#ifdef	CRAY
    "CF77PPFLAGS", "-P",
    "CF77PP", "/lib/cpp",
    "CFT", "cft77",
    "CF", "cf77",
    "FC", "$(CF)",
#else	/* Not CRAY.  */
#ifdef	_IBMR2
    "FC", "xlf",
#else
#ifdef	__convex__
    "FC", "fc",
#else
    "FC", "f77",
#endif /* __convex__ */
#endif /* _IBMR2 */
    /* System V uses these, so explicit rules using them should work.
       However, there is no way to make implicit rules use them and FC.  */
    "F77", "$(FC)",
    "F77FLAGS", "$(FFLAGS)",
#endif	/* Cray.  */
    "GET", SCCS_GET,
    "LD", "ld",
#ifdef GCC_IS_NATIVE
    "LEX", "flex",
#else
    "LEX", "lex",
#endif
    "LINT", "lint",
    "M2C", "m2c",
#ifdef	pyr
    "PC", "pascal",
#else
#ifdef	CRAY
    "PC", "PASCAL",
    "SEGLDR", "segldr",
#else
    "PC", "pc",
#endif	/* CRAY.  */
#endif	/* pyr.  */
#ifdef GCC_IS_NATIVE
    "YACC", "bison -y",
#else
    "YACC", "yacc",	/* Or "bison -y"  */
#endif
    "MAKEINFO", "makeinfo",
    "TEX", "tex",
    "TEXI2DVI", "texi2dvi",
    "WEAVE", "weave",
    "CWEAVE", "cweave",
    "TANGLE", "tangle",
    "CTANGLE", "ctangle",

    "RM", "rm -f",

    "LINK.o", "$(CC) $(LDFLAGS) $(TARGET_ARCH)",
    "COMPILE.c", "$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c",
    "LINK.c", "$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "COMPILE.cc", "$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c",
    "COMPILE.C", "$(COMPILE.cc)",
    "COMPILE.cpp", "$(COMPILE.cc)",
    "LINK.cc", "$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "LINK.C", "$(LINK.cc)",
    "LINK.cpp", "$(LINK.cc)",
    "YACC.y", "$(YACC) $(YFLAGS)",
    "LEX.l", "$(LEX) $(LFLAGS) -t",
    "COMPILE.f", "$(FC) $(FFLAGS) $(TARGET_ARCH) -c",
    "LINK.f", "$(FC) $(FFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "COMPILE.F", "$(FC) $(FFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c",
    "LINK.F", "$(FC) $(FFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "COMPILE.r", "$(FC) $(FFLAGS) $(RFLAGS) $(TARGET_ARCH) -c",
    "LINK.r", "$(FC) $(FFLAGS) $(RFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "COMPILE.def", "$(M2C) $(M2FLAGS) $(DEFFLAGS) $(TARGET_ARCH)",
    "COMPILE.mod", "$(M2C) $(M2FLAGS) $(MODFLAGS) $(TARGET_ARCH)",
    "COMPILE.p", "$(PC) $(PFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c",
    "LINK.p", "$(PC) $(PFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH)",
    "LINK.s", "$(CC) $(ASFLAGS) $(LDFLAGS) $(TARGET_MACH)",
    "COMPILE.s", "$(AS) $(ASFLAGS) $(TARGET_MACH)",
    "LINK.S", "$(CC) $(ASFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_MACH)",
    "COMPILE.S", "$(CC) $(ASFLAGS) $(CPPFLAGS) $(TARGET_MACH) -c",
    "PREPROCESS.S", "$(CC) -E $(CPPFLAGS)",
    "PREPROCESS.F", "$(FC) $(FFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -F",
    "PREPROCESS.r", "$(FC) $(FFLAGS) $(RFLAGS) $(TARGET_ARCH) -F",
    "LINT.c", "$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(TARGET_ARCH)",

#ifndef	NO_MINUS_C_MINUS_O
    "OUTPUT_OPTION", "-o $@",
#endif

#ifdef	SCCS_GET_MINUS_G
    "SCCS_OUTPUT_OPTION", "-G$@",
#endif

#ifdef _AMIGA
    ".LIBPATTERNS", "%.lib",
#else
#ifdef __MSDOS__
    ".LIBPATTERNS", "lib%.a $(DJDIR)/lib/lib%.a",
#else
    ".LIBPATTERNS", "lib%.so lib%.a",
#endif
#endif

#endif /* !VMS */
    0, 0
  };

/* Set up the default .SUFFIXES list.  */

void
set_default_suffixes ()
{
  suffix_file = enter_file (".SUFFIXES");

  if (no_builtin_rules_flag)
    (void) define_variable ("SUFFIXES", 8, "", o_default, 0);
  else
    {
      char *p = default_suffixes;
      suffix_file->deps = (struct dep *)
	multi_glob (parse_file_seq (&p, '\0', sizeof (struct dep), 1),
		    sizeof (struct dep));
      (void) define_variable ("SUFFIXES", 8, default_suffixes, o_default, 0);
    }
}

/* Enter the default suffix rules as file rules.  This used to be done in
   install_default_implicit_rules, but that loses because we want the
   suffix rules installed before reading makefiles, and thee pattern rules
   installed after.  */

void
install_default_suffix_rules ()
{
  register char **s;

  if (no_builtin_rules_flag)
    return;

 for (s = default_suffix_rules; *s != 0; s += 2)
    {
      register struct file *f = enter_file (s[0]);
      /* Don't clobber cmds given in a makefile if there were any.  */
      if (f->cmds == 0)
	{
	  f->cmds = (struct commands *) xmalloc (sizeof (struct commands));
	  f->cmds->fileinfo.filenm = 0;
	  f->cmds->commands = s[1];
	  f->cmds->command_lines = 0;
	}
    }
}


/* Install the default pattern rules.  */

void
install_default_implicit_rules ()
{
  register struct pspec *p;

  if (no_builtin_rules_flag)
    return;

  for (p = default_pattern_rules; p->target != 0; ++p)
    install_pattern_rule (p, 0);

  for (p = default_terminal_rules; p->target != 0; ++p)
    install_pattern_rule (p, 1);
}

void
define_default_variables ()
{
  register char **s;

  if (no_builtin_variables_flag)
    return;

  for (s = default_variables; *s != 0; s += 2)
    (void) define_variable (s[0], strlen (s[0]), s[1], o_default, 1);
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         remote-stub.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Template for the remote job exportation interface to GNU Make.
Copyright (C) 1988, 1989, 1992, 1993, 1996 Free Software Foundation, Inc.
This file is part of GNU Make.

GNU Make is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Make is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Make; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* #include "make.h"  <- modification by J.Ruthruff, 7/28 */
/* #include "filedef.h"  <- modification by J.Ruthruff, 7/28 */
#include "job.h"
/* #include "commands.h"  <- modification by J.Ruthruff, 7/28 */
#undef stderr
#define stderr stdout


char *remote_description = 0;

/* Call once at startup even if no commands are run.  */

void
remote_setup ()
{
}

/* Called before exit.  */

void
remote_cleanup ()
{
}

/* Return nonzero if the next job should be done remotely.  */

int
start_remote_job_p (first_p)
     int first_p;
{
  return 0;
}

/* Start a remote job running the command in ARGV,
   with environment from ENVP.  It gets standard input from STDIN_FD.  On
   failure, return nonzero.  On success, return zero, and set *USED_STDIN
   to nonzero if it will actually use STDIN_FD, zero if not, set *ID_PTR to
   a unique identification, and set *IS_REMOTE to zero if the job is local,
   nonzero if it is remote (meaning *ID_PTR is a process ID).  */

int
start_remote_job (argv, envp, stdin_fd, is_remote, id_ptr, used_stdin)
     char **argv, **envp;
     int stdin_fd;
     int *is_remote;
     int *id_ptr;
     int *used_stdin;
{
  return -1;
}

/* Get the status of a dead remote child.  Block waiting for one to die
   if BLOCK is nonzero.  Set *EXIT_CODE_PTR to the exit status, *SIGNAL_PTR
   to the termination signal or zero if it exited normally, and *COREDUMP_PTR
   nonzero if it dumped core.  Return the ID of the child that died,
   0 if we would have to block and !BLOCK, or < 0 if there were none.  */

int
remote_status (exit_code_ptr, signal_ptr, coredump_ptr, block)
     int *exit_code_ptr, *signal_ptr, *coredump_ptr;
     int block;
{
  errno = ECHILD;
  return -1;
}

/* Block asynchronous notification of remote child death.
   If this notification is done by raising the child termination
   signal, do not block that signal.  */
void
block_remote_children ()
{
  return;
}

/* Restore asynchronous notification of remote child death.
   If this is done by raising the child termination signal,
   do not unblock that signal.  */
void
unblock_remote_children ()
{
  return;
}

/* Send signal SIG to child ID.  Return 0 if successful, -1 if not.  */
int
remote_kill (id, sig)
     int id;
     int sig;
{
  return -1;
}



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         version.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* We use <config.h> instead of "config.h" so that a compilation
   using -I. -I$srcdir will use ./config.h rather than $srcdir/config.h
   (which it would do because make.h was found in $srcdir).  */
#include <config.h>
#undef stderr
#define stderr stdout

#ifndef MAKE_HOST
# define MAKE_HOST "unknown"
#endif

char *version_string = VERSION;
char *make_host = MAKE_HOST;

/*
  Local variables:
  version-control: never
  End:
 */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         getopt1.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* getopt_long and getopt_long_only entry points for GNU getopt.
   Copyright (C) 1987,88,89,90,91,92,93,94,96,97,98
     Free Software Foundation, Inc.

   NOTE: The canonical source of this file is maintained with the GNU C Library.
   Bugs can be reported to bug-glibc@gnu.org.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#undef stderr
#define stderr stdout
#endif

#include "getopt.h"
#undef stderr
#define stderr stdout

#if !defined __STDC__ || !__STDC__
/* This is a separate conditional since some stdc systems
   reject `defined (const)'.  */
#ifndef const
#define const
#endif
#endif

#include <stdio.h>
#undef stderr
#define stderr stdout

/* Comment out all this code if we are using the GNU C Library, and are not
   actually compiling the library itself.  This code is part of the GNU C
   Library, but also included in many other GNU distributions.  Compiling
   and linking in this code is a waste when using the GNU C library
   (especially if it is a shared library).  Rather than having every GNU
   program understand `configure --with-gnu-libc' and omit the object files,
   it is simpler to just do this in the source for each such file.  */

#define GETOPT_INTERFACE_VERSION 2
#if !defined _LIBC && defined __GLIBC__ && __GLIBC__ >= 2
#include <gnu-versions.h>
#undef stderr
#define stderr stdout
#if _GNU_GETOPT_INTERFACE_VERSION == GETOPT_INTERFACE_VERSION
#define ELIDE_CODE
#endif
#endif

#ifndef ELIDE_CODE


/* This needs to come after some library #include
   to get __GNU_LIBRARY__ defined.  */
#ifdef __GNU_LIBRARY__
#include <stdlib.h>
#undef stderr
#define stderr stdout
#endif

#ifndef	NULL
#define NULL 0
#endif

int
getopt_long (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct option *long_options;
     int *opt_index;
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 0);
}

/* Like getopt_long, but '-' as well as '--' can indicate a long option.
   If an option that starts with '-' (not '--') doesn't match a long option,
   but does match a short option, it is parsed as a short option
   instead.  */

int
getopt_long_only (argc, argv, options, long_options, opt_index)
     int argc;
     char *const *argv;
     const char *options;
     const struct option *long_options;
     int *opt_index;
{
  return _getopt_internal (argc, argv, options, long_options, opt_index, 1);
}


#endif	/* Not ELIDE_CODE.  */

#ifdef TEST

#include <stdio.h>
#undef stderr
#define stderr stdout

int
main (argc, argv)
     int argc;
     char **argv;
{
  int c;
  int digit_optind = 0;

  while (1)
    {
      int this_option_optind = optind ? optind : 1;
      int option_index = 0;
      static struct option long_options[] =
      {
	{"add", 1, 0, 0},
	{"append", 0, 0, 0},
	{"delete", 1, 0, 0},
	{"verbose", 0, 0, 0},
	{"create", 0, 0, 0},
	{"file", 1, 0, 0},
	{0, 0, 0, 0}
      };

      c = getopt_long (argc, argv, "abc:d:0123456789",
		       long_options, &option_index);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:
	  printf ("option %s", long_options[option_index].name);
	  if (optarg)
	    printf (" with arg %s", optarg);
	  printf ("\n");
	  break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (digit_optind != 0 && digit_optind != this_option_optind)
	    printf ("digits occur in two different argv-elements.\n");
	  digit_optind = this_option_optind;
	  printf ("option %c\n", c);
	  break;

	case 'a':
	  printf ("option a\n");
	  break;

	case 'b':
	  printf ("option b\n");
	  break;

	case 'c':
	  printf ("option c with value `%s'\n", optarg);
	  break;

	case 'd':
	  printf ("option d with value `%s'\n", optarg);
	  break;

	case '?':
	  break;

	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
	printf ("%s ", argv[optind++]);
      printf ("\n");
    }

  exit (0);
}

#endif /* TEST */



/*************************************************************
  ============================================================
  ************************************************************
  ============================================================
  ************************************************************
                         gettext.c
  ************************************************************
  ============================================================
  ************************************************************
  ============================================================
  *************************************************************/

/* Begin of l10nflist.c */

/* Handle list of needed message catalogs
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#undef stderr
#define stderr stdout
#endif

#ifdef __GNUC__
//# define alloca __builtin_alloca
# define HAVE_ALLOCA 1
#else
# if defined HAVE_ALLOCA_H || defined _LIBC
#  include <alloca.h>
#undef stderr
#define stderr stdout
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca
char *alloca ();
#   endif
#  endif
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE	1
# endif
# include <string.h>
#undef stderr
#define stderr stdout
#else
# include <strings.h>
#undef stderr
#define stderr stdout
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#if defined STDC_HEADERS || defined _LIBC
#else
char *getenv ();
# ifdef HAVE_MALLOC_H
# else
void free ();
# endif
#endif

#if defined _LIBC || defined HAVE_ARGZ_H
# include <argz.h>
#undef stderr
#define stderr stdout
#endif
#include <ctype.h>
#include <sys/types.h>
#undef stderr
#define stderr stdout

#if defined STDC_HEADERS || defined _LIBC
# include <stdlib.h>
#undef stderr
#define stderr stdout
#else
# ifdef HAVE_MEMORY_H
#  include <memory.h>
#undef stderr
#define stderr stdout
# endif
#endif

/* Interrupt of l10nflist.c */

/* Begin of loadinfo.h */

/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.  */

#ifndef PARAMS
# if __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

/* Encoding of locale name parts.  */
#define CEN_REVISION		1
#define CEN_SPONSOR		2
#define CEN_SPECIAL		4
#define XPG_NORM_CODESET	8
#define XPG_CODESET		16
#define TERRITORY		32
#define CEN_AUDIENCE		64
#define XPG_MODIFIER		128

#define CEN_SPECIFIC	(CEN_REVISION|CEN_SPONSOR|CEN_SPECIAL|CEN_AUDIENCE)
#define XPG_SPECIFIC	(XPG_CODESET|XPG_NORM_CODESET|XPG_MODIFIER)

struct loaded_l10nfile
{
  const char *filename;
  int decided;

  const void *data;

  struct loaded_l10nfile *next;
  struct loaded_l10nfile *successor[1];
};

static const char *_nl_normalize_codeset PARAMS ((const unsigned char *codeset,
						  size_t name_len));

static struct loaded_l10nfile *
_nl_make_l10nflist PARAMS ((struct loaded_l10nfile **l10nfile_list,
			    const char *dirlist, size_t dirlist_len, int mask,
			    const char *language, const char *territory,
			    const char *codeset,
			    const char *normalized_codeset,
			    const char *modifier, const char *special,
			    const char *sponsor, const char *revision,
			    const char *filename, int do_allocate));

static const char *_nl_expand_alias PARAMS ((const char *name));

static int _nl_explode_name PARAMS ((char *name, const char **language,
				     const char **modifier,
				     const char **territory,
				     const char **codeset,
				     const char **normalized_codeset,
				     const char **special,
				     const char **sponsor,
				     const char **revision));

/* End of loadinfo.h */

/* Resume of l10nflist.c */

/* On some strange systems still no definition of NULL is found.  Sigh!  */
#ifndef NULL
# if defined __STDC__ && __STDC__
#  define NULL ((void *) 0)
# else
#  define NULL 0
# endif
#endif

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# ifndef stpcpy
#  define stpcpy(dest, src) __stpcpy(dest, src)
# endif
#else
# ifndef HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
# endif
#endif

/* Define function which are usually not available.  */

#if !defined _LIBC && !defined HAVE___ARGZ_COUNT
/* Returns the number of strings in ARGZ.  */
static size_t argz_count__ PARAMS ((const char *argz, size_t len));

static size_t
argz_count__ (argz, len)
     const char *argz;
     size_t len;
{
  size_t count = 0;
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len + 1;
      len -= part_len + 1;
      count++;
    }
  return count;
}
# undef __argz_count
# define __argz_count(argz, len) argz_count__ (argz, len)
#endif	/* !_LIBC && !HAVE___ARGZ_COUNT */

#if !defined _LIBC && !defined HAVE___ARGZ_STRINGIFY
/* Make '\0' separated arg vector ARGZ printable by converting all the '\0's
   except the last into the character SEP.  */
static void argz_stringify__ PARAMS ((char *argz, size_t len, int sep));

static void
argz_stringify__ (argz, len, sep)
     char *argz;
     size_t len;
     int sep;
{
  while (len > 0)
    {
      size_t part_len = strlen (argz);
      argz += part_len;
      len -= part_len + 1;
      if (len > 0)
	*argz++ = sep;
    }
}
# undef __argz_stringify
# define __argz_stringify(argz, len, sep) argz_stringify__ (argz, len, sep)
#endif	/* !_LIBC && !HAVE___ARGZ_STRINGIFY */

#if !defined _LIBC && !defined HAVE___ARGZ_NEXT
static char *argz_next__ PARAMS ((char *argz, size_t argz_len,
				  const char *entry));

static char *
argz_next__ (argz, argz_len, entry)
     char *argz;
     size_t argz_len;
     const char *entry;
{
  if (entry)
    {
      if (entry < argz + argz_len)
        entry = strchr (entry, '\0') + 1;

      return entry >= argz + argz_len ? NULL : (char *) entry;
    }
  else
    if (argz_len > 0)
      return argz;
    else
      return 0;
}
# undef __argz_next
# define __argz_next(argz, len, entry) argz_next__ (argz, len, entry)
#endif	/* !_LIBC && !HAVE___ARGZ_NEXT */

/* Return number of bits set in X.  */
static int pop PARAMS ((int x));

static inline int
pop (x)
     int x;
{
  /* We assume that no more than 16 bits are used.  */
  x = ((x & ~0x5555) >> 1) + (x & 0x5555);
  x = ((x & ~0x3333) >> 2) + (x & 0x3333);
  x = ((x >> 4) + x) & 0x0f0f;
  x = ((x >> 8) + x) & 0xff;

  return x;
}

static struct loaded_l10nfile *
_nl_make_l10nflist (l10nfile_list, dirlist, dirlist_len, mask, language,
		    territory, codeset, normalized_codeset, modifier, special,
		    sponsor, revision, filename, do_allocate)
     struct loaded_l10nfile **l10nfile_list;
     const char *dirlist;
     size_t dirlist_len;
     int mask;
     const char *language;
     const char *territory;
     const char *codeset;
     const char *normalized_codeset;
     const char *modifier;
     const char *special;
     const char *sponsor;
     const char *revision;
     const char *filename;
     int do_allocate;
{
  char *abs_filename;
  struct loaded_l10nfile *last = NULL;
  struct loaded_l10nfile *retval;
  char *cp;
  size_t entries;
  int cnt;

  /* Allocate room for the full file name.  */
  abs_filename = (char *) malloc (dirlist_len
				  + strlen (language)
				  + ((mask & TERRITORY) != 0
				     ? strlen (territory) + 1 : 0)
				  + ((mask & XPG_CODESET) != 0
				     ? strlen (codeset) + 1 : 0)
				  + ((mask & XPG_NORM_CODESET) != 0
				     ? strlen (normalized_codeset) + 1 : 0)
				  + (((mask & XPG_MODIFIER) != 0
				      || (mask & CEN_AUDIENCE) != 0)
				     ? strlen (modifier) + 1 : 0)
				  + ((mask & CEN_SPECIAL) != 0
				     ? strlen (special) + 1 : 0)
				  + (((mask & CEN_SPONSOR) != 0
				      || (mask & CEN_REVISION) != 0)
				     ? (1 + ((mask & CEN_SPONSOR) != 0
					     ? strlen (sponsor) + 1 : 0)
					+ ((mask & CEN_REVISION) != 0
					   ? strlen (revision) + 1 : 0)) : 0)
				  + 1 + strlen (filename) + 1);

  if (abs_filename == NULL)
    return NULL;

  retval = NULL;
  last = NULL;

  /* Construct file name.  */
  memcpy (abs_filename, dirlist, dirlist_len);
  __argz_stringify (abs_filename, dirlist_len, ':');
  cp = abs_filename + (dirlist_len - 1);
  *cp++ = '/';
  cp = stpcpy (cp, language);

  if ((mask & TERRITORY) != 0)
    {
      *cp++ = '_';
      cp = stpcpy (cp, territory);
    }
  if ((mask & XPG_CODESET) != 0)
    {
      *cp++ = '.';
      cp = stpcpy (cp, codeset);
    }
  if ((mask & XPG_NORM_CODESET) != 0)
    {
      *cp++ = '.';
      cp = stpcpy (cp, normalized_codeset);
    }
  if ((mask & (XPG_MODIFIER | CEN_AUDIENCE)) != 0)
    {
      /* This component can be part of both syntaces but has different
	 leading characters.  For CEN we use `+', else `@'.  */
      *cp++ = (mask & CEN_AUDIENCE) != 0 ? '+' : '@';
      cp = stpcpy (cp, modifier);
    }
  if ((mask & CEN_SPECIAL) != 0)
    {
      *cp++ = '+';
      cp = stpcpy (cp, special);
    }
  if ((mask & (CEN_SPONSOR | CEN_REVISION)) != 0)
    {
      *cp++ = ',';
      if ((mask & CEN_SPONSOR) != 0)
	cp = stpcpy (cp, sponsor);
      if ((mask & CEN_REVISION) != 0)
	{
	  *cp++ = '_';
	  cp = stpcpy (cp, revision);
	}
    }

  *cp++ = '/';
  stpcpy (cp, filename);

  /* Look in list of already loaded domains whether it is already
     available.  */
  last = NULL;
  for (retval = *l10nfile_list; retval != NULL; retval = retval->next)
    if (retval->filename != NULL)
      {
	int compare = strcmp (retval->filename, abs_filename);
	if (compare == 0)
	  /* We found it!  */
	  break;
	if (compare < 0)
	  {
	    /* It's not in the list.  */
	    retval = NULL;
	    break;
	  }

	last = retval;
      }

  if (retval != NULL || do_allocate == 0)
    {
      free (abs_filename);
      return retval;
    }

  retval = (struct loaded_l10nfile *)
    malloc (sizeof (*retval) + (__argz_count (dirlist, dirlist_len)
				* (1 << pop (mask))
				* sizeof (struct loaded_l10nfile *)));
  if (retval == NULL)
    return NULL;

  retval->filename = abs_filename;
  retval->decided = (__argz_count (dirlist, dirlist_len) != 1
		     || ((mask & XPG_CODESET) != 0
			 && (mask & XPG_NORM_CODESET) != 0));
  retval->data = NULL;

  if (last == NULL)
    {
      retval->next = *l10nfile_list;
      *l10nfile_list = retval;
    }
  else
    {
      retval->next = last->next;
      last->next = retval;
    }

  entries = 0;
  /* If the DIRLIST is a real list the RETVAL entry corresponds not to
     a real file.  So we have to use the DIRLIST separation mechanism
     of the inner loop.  */
  cnt = __argz_count (dirlist, dirlist_len) == 1 ? mask - 1 : mask;
  for (; cnt >= 0; --cnt)
    if ((cnt & ~mask) == 0
	&& ((cnt & CEN_SPECIFIC) == 0 || (cnt & XPG_SPECIFIC) == 0)
	&& ((cnt & XPG_CODESET) == 0 || (cnt & XPG_NORM_CODESET) == 0))
      {
	/* Iterate over all elements of the DIRLIST.  */
	char *dir = NULL;

	while ((dir = __argz_next ((char *) dirlist, dirlist_len, dir))
	       != NULL)
	  retval->successor[entries++]
	    = _nl_make_l10nflist (l10nfile_list, dir, strlen (dir) + 1, cnt,
				  language, territory, codeset,
				  normalized_codeset, modifier, special,
				  sponsor, revision, filename, 1);
      }
  retval->successor[entries] = NULL;

  return retval;
}

/* Normalize codeset name.  There is no standard for the codeset
   names.  Normalization allows the user to use any of the common
   names.  */
static const char *
_nl_normalize_codeset (codeset, name_len)
     const unsigned char *codeset;
     size_t name_len;
{
  int len = 0;
  int only_digit = 1;
  char *retval;
  char *wp;
  size_t cnt;

  for (cnt = 0; cnt < name_len; ++cnt)
    if (isalnum (codeset[cnt]))
      {
	++len;

	if (isalpha (codeset[cnt]))
	  only_digit = 0;
      }

  retval = (char *) malloc ((only_digit ? 3 : 0) + len + 1);

  if (retval != NULL)
    {
      if (only_digit)
	wp = stpcpy (retval, "iso");
      else
	wp = retval;

      for (cnt = 0; cnt < name_len; ++cnt)
	if (isalpha (codeset[cnt]))
	  *wp++ = tolower (codeset[cnt]);
	else if (isdigit (codeset[cnt]))
	  *wp++ = codeset[cnt];

      *wp = '\0';
    }

  return (const char *) retval;
}

/* End of l10nflist.c */

/* Begin of explodename.c */

/* Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Contributed by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.  */

#if defined STDC_HEADERS || defined _LIBC
#endif

#if defined HAVE_STRING_H || defined _LIBC
#else
#endif

static int
_nl_explode_name (name, language, modifier, territory, codeset,
		  normalized_codeset, special, sponsor, revision)
     char *name;
     const char **language;
     const char **modifier;
     const char **territory;
     const char **codeset;
     const char **normalized_codeset;
     const char **special;
     const char **sponsor;
     const char **revision;
{
  enum { undecided, xpg, cen } syntax;
  char *cp;
  int mask;

  *modifier = NULL;
  *territory = NULL;
  *codeset = NULL;
  *normalized_codeset = NULL;
  *special = NULL;
  *sponsor = NULL;
  *revision = NULL;

  /* Now we determine the single parts of the locale name.  First
     look for the language.  Termination symbols are `_' and `@' if
     we use XPG4 style, and `_', `+', and `,' if we use CEN syntax.  */
  mask = 0;
  syntax = undecided;
  *language = cp = name;
  while (cp[0] != '\0' && cp[0] != '_' && cp[0] != '@'
	 && cp[0] != '+' && cp[0] != ',')
    ++cp;

  if (*language == cp)
    /* This does not make sense: language has to be specified.  Use
       this entry as it is without exploding.  Perhaps it is an alias.  */
    cp = strchr (*language, '\0');
  else if (cp[0] == '_')
    {
      /* Next is the territory.  */
      cp[0] = '\0';
      *territory = ++cp;

      while (cp[0] != '\0' && cp[0] != '.' && cp[0] != '@'
	     && cp[0] != '+' && cp[0] != ',' && cp[0] != '_')
	++cp;

      mask |= TERRITORY;

      if (cp[0] == '.')
	{
	  /* Next is the codeset.  */
	  syntax = xpg;
	  cp[0] = '\0';
	  *codeset = ++cp;

	  while (cp[0] != '\0' && cp[0] != '@')
	    ++cp;

	  mask |= XPG_CODESET;

	  if (*codeset != cp && (*codeset)[0] != '\0')
	    {
	      *normalized_codeset = _nl_normalize_codeset ((const unsigned
                                                            char *)*codeset,
							   cp - *codeset);
	      if (strcmp (*codeset, *normalized_codeset) == 0)
		free ((char *) *normalized_codeset);
	      else
		mask |= XPG_NORM_CODESET;
	    }
	}
    }

  if (cp[0] == '@' || (syntax != xpg && cp[0] == '+'))
    {
      /* Next is the modifier.  */
      syntax = cp[0] == '@' ? xpg : cen;
      cp[0] = '\0';
      *modifier = ++cp;

      while (syntax == cen && cp[0] != '\0' && cp[0] != '+'
	     && cp[0] != ',' && cp[0] != '_')
	++cp;

      mask |= XPG_MODIFIER | CEN_AUDIENCE;
    }

  if (syntax != xpg && (cp[0] == '+' || cp[0] == ',' || cp[0] == '_'))
    {
      syntax = cen;

      if (cp[0] == '+')
	{
 	  /* Next is special application (CEN syntax).  */
	  cp[0] = '\0';
	  *special = ++cp;

	  while (cp[0] != '\0' && cp[0] != ',' && cp[0] != '_')
	    ++cp;

	  mask |= CEN_SPECIAL;
	}

      if (cp[0] == ',')
	{
 	  /* Next is sponsor (CEN syntax).  */
	  cp[0] = '\0';
	  *sponsor = ++cp;

	  while (cp[0] != '\0' && cp[0] != '_')
	    ++cp;

	  mask |= CEN_SPONSOR;
	}

      if (cp[0] == '_')
	{
 	  /* Next is revision (CEN syntax).  */
	  cp[0] = '\0';
	  *revision = ++cp;

	  mask |= CEN_REVISION;
	}
    }

  /* For CEN syntax values it might be important to have the
     separator character in the file name, not for XPG syntax.  */
  if (syntax == xpg)
    {
      if (*territory != NULL && (*territory)[0] == '\0')
	mask &= ~TERRITORY;

      if (*codeset != NULL && (*codeset)[0] == '\0')
	mask &= ~XPG_CODESET;

      if (*modifier != NULL && (*modifier)[0] == '\0')
	mask &= ~XPG_MODIFIER;
    }

  return mask;
}

/* End of explodename.c */

/* Begin of loadmsgcat.c */

/* Load needed message catalogs.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

#include <fcntl.h>
#include <sys/stat.h>
#undef stderr
#define stderr stdout

#if defined STDC_HEADERS || defined _LIBC
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
# include <unistd.h>
#undef stderr
#define stderr stdout
#endif

#if (defined HAVE_MMAP && defined HAVE_MUNMAP) || defined _LIBC
# include <sys/mman.h>
#undef stderr
#define stderr stdout
#endif

/* Interrupt of loadmsgcat.c */

/* Begin of gettext.h */

/* Internal header for GNU gettext internationalization functions.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.  */

#ifndef _GETTEXT_H
#define _GETTEXT_H 1

#include <stdio.h>
#undef stderr
#define stderr stdout

#if HAVE_LIMITS_H || _LIBC
# include <limits.h>
#undef stderr
#define stderr stdout
#endif

/* The magic number of the GNU message catalog format.  */
#define _MAGIC 0x950412de
#define _MAGIC_SWAPPED 0xde120495

/* Revision number of the currently used .mo (binary) file format.  */
#define MO_REVISION_NUMBER 0

/* The following contortions are an attempt to use the C preprocessor
   to determine an unsigned integral type that is 32 bits wide.  An
   alternative approach is to use autoconf's AC_CHECK_SIZEOF macro, but
   doing that would require that the configure script compile and *run*
   the resulting executable.  Locally running cross-compiled executables
   is usually not possible.  */

#if __STDC__
# define UINT_MAX_32_BITS 4294967295U
#else
# define UINT_MAX_32_BITS 0xFFFFFFFF
#endif

/* If UINT_MAX isn't defined, assume it's a 32-bit type.
   This should be valid for all systems GNU cares about because
   that doesn't include 16-bit systems, and only modern systems
   (that certainly have <limits.h>) have 64+-bit integral types.  */

#ifndef UINT_MAX
# define UINT_MAX UINT_MAX_32_BITS
#endif

#if UINT_MAX == UINT_MAX_32_BITS
typedef unsigned nls_uint32;
#else
# if USHRT_MAX == UINT_MAX_32_BITS
typedef unsigned short nls_uint32;
# else
#  if ULONG_MAX == UINT_MAX_32_BITS
typedef unsigned long nls_uint32;
#  else
  /* The following line is intended to throw an error.  Using #error is
     not portable enough.  */
  "Cannot determine unsigned 32-bit data type."
#  endif
# endif
#endif

/* Header for binary .mo file format.  */
struct mo_file_header
{
  /* The magic number.  */
  nls_uint32 magic;
  /* The revision number of the file format.  */
  nls_uint32 revision;
  /* The number of strings pairs.  */
  nls_uint32 nstrings;
  /* Offset of table with start offsets of original strings.  */
  nls_uint32 orig_tab_offset;
  /* Offset of table with start offsets of translation strings.  */
  nls_uint32 trans_tab_offset;
  /* Size of hashing table.  */
  nls_uint32 hash_tab_size;
  /* Offset of first hashing entry.  */
  nls_uint32 hash_tab_offset;
};

struct string_desc
{
  /* Length of addressed string.  */
  nls_uint32 length;
  /* Offset of string in file.  */
  nls_uint32 offset;
};

#endif	/* gettext.h  */

/* End of gettext.h */

/* Resume of loadmsgcat.c */

/* Interrupt of loadmsgcat.c */

/* Begin of gettextP.h */

/* Header describing internals of gettext library
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.  */

#ifndef _GETTEXTP_H
#define _GETTEXTP_H

#ifndef PARAMS
# if __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

#ifndef internal_function
# define internal_function
#endif

#ifndef W
# define W(flag, data) ((flag) ? SWAP (data) : (data))
#endif

#ifdef _LIBC
# include <byteswap.h>
#undef stderr
#define stderr stdout
# define SWAP(i) bswap_32 (i)
#else
static nls_uint32 SWAP PARAMS ((nls_uint32 i));

static inline nls_uint32
SWAP (i)
     nls_uint32 i;
{
  return (i << 24) | ((i & 0xff00) << 8) | ((i >> 8) & 0xff00) | (i >> 24);
}
#endif

struct loaded_domain
{
  const char *data;
  int use_mmap;
  size_t mmap_size;
  int must_swap;
  nls_uint32 nstrings;
  struct string_desc *orig_tab;
  struct string_desc *trans_tab;
  nls_uint32 hash_size;
  nls_uint32 *hash_tab;
};

struct binding
{
  struct binding *next;
  char *domainname;
  char *dirname;
};
/*
static struct loaded_l10nfile *_nl_find_domain PARAMS ((const char *__dirname,
						 char *__locale,
						 const char *__domainname))
     internal_function;
static void _nl_load_domain PARAMS ((struct loaded_l10nfile *__domain))
     internal_function;
static void _nl_unload_domain PARAMS ((struct loaded_domain *__domain))
     internal_function;
*/
#endif /* gettextP.h  */

/* End of gettextP.h */

/* Resume of loadmsgcat.c */

#ifdef _LIBC
/* Rename the non ISO C functions.  This is required by the standard
   because some ISO C functions will require linking with this object
   file and the name space must not be polluted.  */
# define open   __open
# define close  __close
# define read   __read
# define mmap   __mmap
# define munmap __munmap
#endif

/* We need a sign, whether a new catalog was loaded, which can be associated
   with all translations.  This is important if the translations are
   cached by one of GCC's features.  */
int _nl_msg_cat_cntr = 0;

/* Load the message catalogs specified by FILENAME.  If it is no valid
   message catalog do nothing.  */
static void
internal_function
_nl_load_domain (domain_file)
     struct loaded_l10nfile *domain_file;
{
  int fd;
  size_t size;
  struct stat st;
  struct mo_file_header *data = (struct mo_file_header *) -1;
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  int use_mmap = 0;
#endif
  struct loaded_domain *domain;

  domain_file->decided = 1;
  domain_file->data = NULL;

  /* If the record does not represent a valid locale the FILENAME
     might be NULL.  This can happen when according to the given
     specification the locale file name is different for XPG and CEN
     syntax.  */
  if (domain_file->filename == NULL)
    return;

  /* Try to open the addressed file.  */
  fd = open (domain_file->filename, O_RDONLY);
  if (fd == -1)
    return;

  /* We must know about the size of the file.  */
  if (fstat (fd, &st) != 0
      || (size_t_equal((size = int_to_size_t(off_t_to_int(st.st_size))), off_t_to_int(st.st_size)) != 0)
          /* || (size = (size_t) st.st_size) != st.st_size */
      || size < sizeof (struct mo_file_header))
    {
      /* Something went wrong.  */
      close (fd);
      return;
    }

#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  /* Now we are ready to load the file.  If mmap() is available we try
     this first.  If not available or it failed we try to load it.  */
  data = (struct mo_file_header *) mmap (NULL, size, PROT_READ,
					 MAP_PRIVATE, fd, 0);

  if (data != (struct mo_file_header *) -1)
    {
      /* mmap() call was successful.  */
      close (fd);
      use_mmap = 1;
    }
#endif

  /* If the data is not yet available (i.e. mmap'ed) we try to load
     it manually.  */
  if (data == (struct mo_file_header *) -1)
    {
      size_t to_read;
      char *read_ptr;

      data = (struct mo_file_header *) malloc (size);
      if (data == NULL)
	return;

      to_read = size;
      read_ptr = (char *) data;
      do
	{
	  long int nb = (long int) read (fd, read_ptr, to_read);
	  if (nb == -1)
	    {
	      close (fd);
	      return;
	    }

	  read_ptr += nb;
	  to_read -= nb;
	}
      while (to_read > 0);

      close (fd);
    }

  /* Using the magic number we can test whether it really is a message
     catalog file.  */
  if (data->magic != _MAGIC && data->magic != _MAGIC_SWAPPED)
    {
      /* The magic number is wrong: not a message catalog file.  */
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
      if (use_mmap)
	munmap ((caddr_t) data, size);
      else
#endif
	free (data);
      return;
    }

  domain_file->data
    = (struct loaded_domain *) malloc (sizeof (struct loaded_domain));
  if (domain_file->data == NULL)
    return;

  domain = (struct loaded_domain *) domain_file->data;
  domain->data = (char *) data;
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
  domain->use_mmap = use_mmap;
#endif
  domain->mmap_size = size;
  domain->must_swap = data->magic != _MAGIC;

  /* Fill in the information about the available tables.  */
  switch (W (domain->must_swap, data->revision))
    {
    case 0:
      domain->nstrings = W (domain->must_swap, data->nstrings);
      domain->orig_tab = (struct string_desc *)
	((char *) data + W (domain->must_swap, data->orig_tab_offset));
      domain->trans_tab = (struct string_desc *)
	((char *) data + W (domain->must_swap, data->trans_tab_offset));
      domain->hash_size = W (domain->must_swap, data->hash_tab_size);
      domain->hash_tab = (nls_uint32 *)
	((char *) data + W (domain->must_swap, data->hash_tab_offset));
      break;
    default:
      /* This is an illegal revision.  */
#if (defined HAVE_MMAP && defined HAVE_MUNMAP && !defined DISALLOW_MMAP) \
    || defined _LIBC
      if (use_mmap)
	munmap ((caddr_t) data, size);
      else
#endif
	free (data);
      free (domain);
      domain_file->data = NULL;
      return;
    }

  /* Show that one domain is changed.  This might make some cached
     translations invalid.  */
  ++_nl_msg_cat_cntr;
}

#ifdef _LIBC
static void
internal_function
_nl_unload_domain (domain)
     struct loaded_domain *domain;
{
  if (domain->use_mmap)
    munmap ((caddr_t) domain->data, domain->mmap_size);
  else
    free ((void *) domain->data);

  free (domain);
}
#endif

/* End of loadmsgcat.c */

/* Begin of localealias.c */

/* Handle aliases for locale names.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.  */

#if defined STDC_HEADERS || defined _LIBC
#else
char *getenv ();
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
#undef stderr
#define stderr stdout
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE	1
# endif
#else
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# define strcasecmp __strcasecmp

# define mempcpy __mempcpy
# define HAVE_MEMPCPY	1

/* We need locking here since we can be called from different places.  */
# include <bits/libc-lock.h>
#undef stderr
#define stderr stdout

__libc_lock_define_initialized (static, lock);
#endif

/* For those loosing systems which don't have `alloca' we have to add
   some additional code emulating it.  */
#ifdef HAVE_ALLOCA
/* Nothing has to be done.  */
# define ADD_BLOCK(list, address) /* nothing */
# define FREE_BLOCKS(list) /* nothing */
#else
struct block_list
{
  void *address;
  struct block_list *next;
};
# define ADD_BLOCK(list, addr)						      \
  do {									      \
    struct block_list *newp = (struct block_list *) malloc (sizeof (*newp));  \
    /* If we cannot get a free block we cannot add the new element to	      \
       the list.  */							      \
    if (newp != NULL) {							      \
      newp->address = (addr);						      \
      newp->next = (list);						      \
      (list) = newp;							      \
    }									      \
  } while (0)
# define FREE_BLOCKS(list)						      \
  do {									      \
    while (list != NULL) {						      \
      struct block_list *old = list;					      \
      list = list->next;						      \
      free (old);							      \
    }									      \
  } while (0)
# undef alloca
# define alloca(size) (malloc (size))
#endif	/* have alloca */

struct alias_map
{
  const char *alias;
  const char *value;
};

static char *string_space = NULL;
static size_t string_space_act = 0;
static size_t string_space_max = 0;
static struct alias_map *map;
static size_t nmap = 0;
static size_t maxmap = 0;

/* Prototypes for local functions.  */
static size_t read_alias_file PARAMS ((const char *fname, int fname_len))
     internal_function;
static void extend_alias_table PARAMS ((void));
static int alias_compare PARAMS ((const struct alias_map *map1,
				  const struct alias_map *map2));

static const char *
_nl_expand_alias (name)
    const char *name;
{
  static const char *locale_alias_path = ALIASPATH;
  struct alias_map *retval;
  const char *result = NULL;
  size_t added;

#ifdef _LIBC
  __libc_lock_lock (lock);
#endif

  do
    {
      struct alias_map item;

      item.alias = name;

      if (nmap > 0)
	retval = (struct alias_map *) bsearch (&item, map, nmap,
					       sizeof (struct alias_map),
					       (int (*) PARAMS ((const void *,
								 const void *))
						) alias_compare);
      else
	retval = NULL;

      /* We really found an alias.  Return the value.  */
      if (retval != NULL)
	{
	  result = retval->value;
	  break;
	}

      /* Perhaps we can find another alias file.  */
      added = 0;
      while (added == 0 && locale_alias_path[0] != '\0')
	{
	  const char *start;

	  while (locale_alias_path[0] == ':')
	    ++locale_alias_path;
	  start = locale_alias_path;

	  while (locale_alias_path[0] != '\0' && locale_alias_path[0] != ':')
	    ++locale_alias_path;

	  if (start < locale_alias_path)
	    added = read_alias_file (start, locale_alias_path - start);
	}
    }
  while (added != 0);

#ifdef _LIBC
  __libc_lock_unlock (lock);
#endif

  return result;
}

static size_t
internal_function
read_alias_file (fname, fname_len)
     const char *fname;
     int fname_len;
{
#ifndef HAVE_ALLOCA
  struct block_list *block_list = NULL;
#endif
  FILE *fp;
  char *full_fname;
  size_t added;
  static const char aliasfile[] = "/locale.alias";

  full_fname = (char *) alloca (fname_len + sizeof aliasfile);
  ADD_BLOCK (block_list, full_fname);
#ifdef HAVE_MEMPCPY
  mempcpy (mempcpy (full_fname, fname, fname_len),
	   aliasfile, sizeof aliasfile);
#else
  memcpy (full_fname, fname, fname_len);
  memcpy (&full_fname[fname_len], aliasfile, sizeof aliasfile);
#endif

  fp = fopen (full_fname, "r");
  if (fp == NULL)
    {
      FREE_BLOCKS (block_list);
      return 0;
    }

  added = 0;
  while (!feof (fp))
    {
      /* It is a reasonable approach to use a fix buffer here because
	 a) we are only interested in the first two fields
	 b) these fields must be usable as file names and so must not
	    be that long
       */
      unsigned char buf[BUFSIZ];
      unsigned char *alias;
      unsigned char *value;
      unsigned char *cp;

      if (fgets ((char *)buf, sizeof buf, fp) == NULL)
	/* EOF reached.  */
	break;

      /* Possibly not the whole line fits into the buffer.  Ignore
	 the rest of the line.  */
      if (strchr ((char *)buf, '\n') == NULL)
	{
	  char altbuf[BUFSIZ];
	  do
	    if (fgets (altbuf, sizeof altbuf, fp) == NULL)
	      /* Make sure the inner loop will be left.  The outer loop
		 will exit at the `feof' test.  */
	      break;
	  while (strchr (altbuf, '\n') == NULL);
	}

      cp = buf;
      /* Ignore leading white space.  */
      while (isspace (cp[0]))
	++cp;

      /* A leading '#' signals a comment line.  */
      if (cp[0] != '\0' && cp[0] != '#')
	{
	  alias = cp++;
	  while (cp[0] != '\0' && !isspace (cp[0]))
	    ++cp;
	  /* Terminate alias name.  */
	  if (cp[0] != '\0')
	    *cp++ = '\0';

	  /* Now look for the beginning of the value.  */
	  while (isspace (cp[0]))
	    ++cp;

	  if (cp[0] != '\0')
	    {
	      size_t alias_len;
	      size_t value_len;

	      value = cp++;
	      while (cp[0] != '\0' && !isspace (cp[0]))
		++cp;
	      /* Terminate value.  */
	      if (cp[0] == '\n')
		{
		  /* This has to be done to make the following test
		     for the end of line possible.  We are looking for
		     the terminating '\n' which do not overwrite here.  */
		  *cp++ = '\0';
		  *cp = '\n';
		}
	      else if (cp[0] != '\0')
		*cp++ = '\0';

	      if (nmap >= maxmap)
		extend_alias_table ();

	      alias_len = strlen ((char *)alias) + 1;
	      value_len = strlen ((char *)value) + 1;

	      if (string_space_act + alias_len + value_len > string_space_max)
		{
		  /* Increase size of memory pool.  */
		  size_t new_size = (string_space_max
				     + (alias_len + value_len > 1024
					? alias_len + value_len : 1024));
		  char *new_pool = (char *) realloc (string_space, new_size);
		  if (new_pool == NULL)
		    {
		      FREE_BLOCKS (block_list);
		      return added;
		    }
		  string_space = new_pool;
		  string_space_max = new_size;
		}

	      map[nmap].alias = memcpy (&string_space[string_space_act],
					alias, alias_len);
	      string_space_act += alias_len;

	      map[nmap].value = memcpy (&string_space[string_space_act],
					value, value_len);
	      string_space_act += value_len;

	      ++nmap;
	      ++added;
	    }
	}
    }

  /* Should we test for ferror()?  I think we have to silently ignore
     errors.  --drepper  */
  fclose (fp);

  if (added > 0)
    qsort (map, nmap, sizeof (struct alias_map),
	   (int (*) PARAMS ((const void *, const void *))) alias_compare);

  FREE_BLOCKS (block_list);
  return added;
}

static void
extend_alias_table ()
{
  size_t new_size;
  struct alias_map *new_map;

  new_size = maxmap == 0 ? 100 : 2 * maxmap;
  new_map = (struct alias_map *) realloc (map, (new_size
						* sizeof (struct alias_map)));
  if (new_map == NULL)
    /* Simply don't extend: we don't have any more core.  */
    return;

  map = new_map;
  maxmap = new_size;
}

#ifdef _LIBC
static void __attribute__ ((unused))
free_mem (void)
{
  if (string_space != NULL)
    free (string_space);
  if (map != NULL)
    free (map);
}
text_set_element (__libc_subfreeres, free_mem);
#endif

static int
alias_compare (map1, map2)
     const struct alias_map *map1;
     const struct alias_map *map2;
{
#if defined _LIBC || defined HAVE_STRCASECMP
  return strcasecmp (map1->alias, map2->alias);
#else
  const unsigned char *p1 = (const unsigned char *) map1->alias;
  const unsigned char *p2 = (const unsigned char *) map2->alias;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      /* I know this seems to be odd but the tolower() function in
	 some systems libc cannot handle nonalpha characters.  */
      c1 = isupper (*p1) ? tolower (*p1) : *p1;
      c2 = isupper (*p2) ? tolower (*p2) : *p2;
      if (c1 == '\0')
	break;
      ++p1;
      ++p2;
    }
  while (c1 == c2);

  return c1 - c2;
#endif
}

/* End of localealias.c */

/* Begin of finddomain.c */

/* Handle list of needed message catalogs
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.  */

#include <errno.h>
#undef stderr
#define stderr stdout

#if defined STDC_HEADERS || defined _LIBC
#else
# ifdef HAVE_MALLOC_H
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
#else
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
#endif

#ifdef _LIBC
# include <libintl.h>
#undef stderr
#define stderr stdout
#else
# include "gettext.h"
#undef stderr
#define stderr stdout
#endif

/* List of already loaded domains.  */
static struct loaded_l10nfile *_nl_loaded_domains;

/* Return a data structure describing the message catalog described by
   the DOMAINNAME and CATEGORY parameters with respect to the currently
   established bindings.  */
static struct loaded_l10nfile *
internal_function
_nl_find_domain (dirname, locale, domainname)
     const char *dirname;
     char *locale;
     const char *domainname;
{
  struct loaded_l10nfile *retval;
  const char *language;
  const char *modifier;
  const char *territory;
  const char *codeset;
  const char *normalized_codeset;
  const char *special;
  const char *sponsor;
  const char *revision;
  const char *alias_value;
  int mask;

  /* LOCALE can consist of up to four recognized parts for the XPG syntax:

		language[_territory[.codeset]][@modifier]

     and six parts for the CEN syntax:

	language[_territory][+audience][+special][,[sponsor][_revision]]

     Beside the first part all of them are allowed to be missing.  If
     the full specified locale is not found, the less specific one are
     looked for.  The various parts will be stripped off according to
     the following order:
		(1) revision
		(2) sponsor
		(3) special
		(4) codeset
		(5) normalized codeset
		(6) territory
		(7) audience/modifier
   */

  /* If we have already tested for this locale entry there has to
     be one data set in the list of loaded domains.  */
  retval = _nl_make_l10nflist (&_nl_loaded_domains, dirname,
			       strlen (dirname) + 1, 0, locale, NULL, NULL,
			       NULL, NULL, NULL, NULL, NULL, domainname, 0);
  if (retval != NULL)
    {
      /* We know something about this locale.  */
      int cnt;

      if (retval->decided == 0)
	_nl_load_domain (retval);

      if (retval->data != NULL)
	return retval;

      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);

	  if (retval->successor[cnt]->data != NULL)
	    break;
	}
      return cnt >= 0 ? retval : NULL;
      /* NOTREACHED */
    }

  /* See whether the locale value is an alias.  If yes its value
     *overwrites* the alias name.  No test for the original value is
     done.  */
  alias_value = _nl_expand_alias (locale);
  if (alias_value != NULL)
    {
#if defined _LIBC || defined HAVE_STRDUP
      locale = strdup (alias_value);
      if (locale == NULL)
	return NULL;
#else
      size_t len = strlen (alias_value) + 1;
      locale = (char *) malloc (len);
      if (locale == NULL)
	return NULL;

      memcpy (locale, alias_value, len);
#endif
    }

  /* Now we determine the single parts of the locale name.  First
     look for the language.  Termination symbols are `_' and `@' if
     we use XPG4 style, and `_', `+', and `,' if we use CEN syntax.  */
  mask = _nl_explode_name (locale, &language, &modifier, &territory,
			   &codeset, &normalized_codeset, &special,
			   &sponsor, &revision);

  /* Create all possible locale entries which might be interested in
     generalization.  */
  retval = _nl_make_l10nflist (&_nl_loaded_domains, dirname,
			       strlen (dirname) + 1, mask, language, territory,
			       codeset, normalized_codeset, modifier, special,
			       sponsor, revision, domainname, 1);
  if (retval == NULL)
    /* This means we are out of core.  */
    return NULL;

  if (retval->decided == 0)
    _nl_load_domain (retval);
  if (retval->data == NULL)
    {
      int cnt;
      for (cnt = 0; retval->successor[cnt] != NULL; ++cnt)
	{
	  if (retval->successor[cnt]->decided == 0)
	    _nl_load_domain (retval->successor[cnt]);
	  if (retval->successor[cnt]->data != NULL)
	    break;
	}
    }

  /* The room for an alias was dynamically allocated.  Free it now.  */
  if (alias_value != NULL)
    free (locale);

  return retval;
}

#ifdef _LIBC
static void __attribute__ ((unused))
free_mem (void)
{
  struct loaded_l10nfile *runp = _nl_loaded_domains;

  while (runp != NULL)
    {
      struct loaded_l10nfile *here = runp;
      if (runp->data != NULL)
	_nl_unload_domain ((struct loaded_domain *) runp->data);
      runp = runp->next;
      free (here);
    }
}

text_set_element (__libc_subfreeres, free_mem);
#endif

/* End of finddomain.c */

/* Begin of dcgettext.c */

/* Implementation of the dcgettext(3) function.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

#ifndef errno
extern int errno;
#endif
#ifndef __set_errno
# define __set_errno(val) errno = (val)
#endif

#if defined STDC_HEADERS || defined _LIBC
#else
char *getenv ();
# ifdef HAVE_MALLOC_H
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE	1
# endif
#else
#endif
#if !HAVE_STRCHR && !defined _LIBC
# ifndef strchr
#  define strchr index
# endif
#endif

#if defined HAVE_UNISTD_H || defined _LIBC
#endif

#ifdef _LIBC
#else
#endif

/* Interrupt of dcgettext.c */

/* Begin of hash-string.h */

/* Implements a string hashing function.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.  */

#ifndef PARAMS
# if __STDC__
#  define PARAMS(Args) Args
# else
#  define PARAMS(Args) ()
# endif
#endif

/* We assume to have `unsigned long int' value with at least 32 bits.  */
#define HASHWORDBITS 32

/* Defines the so called `hashpjw' function by P.J. Weinberger
   [see Aho/Sethi/Ullman, COMPILERS: Principles, Techniques and Tools,
   1986, 1987 Bell Telephone Laboratories, Inc.]  */
static unsigned long hash_string PARAMS ((const char *__str_param));

static inline unsigned long
hash_string (str_param)
     const char *str_param;
{
  unsigned long int hval, g;
  const char *str = str_param;

  /* Compute the hash value for the given string.  */
  hval = 0;
  while (*str != '\0')
    {
      hval <<= 4;
      hval += (unsigned long) *str++;
      g = hval & ((unsigned long) 0xf << (HASHWORDBITS - 4));
      if (g != 0)
	{
	  hval ^= g >> (HASHWORDBITS - 8);
	  hval ^= g;
	}
    }
  return hval;
}

/* End of hash-string.h */

/* Resume of dcgettext.c */

#ifdef _LIBC
/* Rename the non ANSI C functions.  This is required by the standard
   because some ANSI C functions will require linking with this object
   file and the name space must not be polluted.  */
# define getcwd __getcwd
# ifndef stpcpy
#  define stpcpy __stpcpy
# endif
#else
# if !defined HAVE_GETCWD
char *getwd ();
#  define getcwd(buf, max) getwd (buf)
# else
char *getcwd ();
# endif
# ifndef HAVE_STPCPY
static char *stpcpy PARAMS ((char *dest, const char *src));
# endif
#endif

/* Amount to increase buffer size by in each try.  */
#define PATH_INCR 32

/* The following is from pathmax.h.  */
/* Non-POSIX BSD systems might have gcc's limits.h, which doesn't define
   PATH_MAX but might cause redefinition warnings when sys/param.h is
   later included (as on MORE/BSD 4.3).  */
#if defined(_POSIX_VERSION) || (defined(HAVE_LIMITS_H) && !defined(__GNUC__))
#endif

#ifndef _POSIX_PATH_MAX
# define _POSIX_PATH_MAX 255
#endif

#if !defined(PATH_MAX) && defined(_PC_PATH_MAX)
# define PATH_MAX (pathconf ("/", _PC_PATH_MAX) < 1 ? 1024 : pathconf ("/", _PC_PATH_MAX))
#endif

/* Don't include sys/param.h if it already has been.  */
#if defined(HAVE_SYS_PARAM_H) && !defined(PATH_MAX) && !defined(MAXPATHLEN)
# include <sys/param.h>
#undef stderr
#define stderr stdout
#endif

#if !defined(PATH_MAX) && defined(MAXPATHLEN)
# define PATH_MAX MAXPATHLEN
#endif

#ifndef PATH_MAX
# define PATH_MAX _POSIX_PATH_MAX
#endif

/* XPG3 defines the result of `setlocale (category, NULL)' as:
   ``Directs `setlocale()' to query `category' and return the current
     setting of `local'.''
   However it does not specify the exact format.  And even worse: POSIX
   defines this not at all.  So we can use this feature only on selected
   system (e.g. those using GNU C Library).  */
#ifdef _LIBC
# define HAVE_LOCALE_NULL
#endif

/* Name of the default domain used for gettext(3) prior any call to
   textdomain(3).  The default value for this is "messages".  */
static const char _nl_default_default_domain[] = "messages";

/* Value used as the default domain for gettext(3).  */
static const char *_nl_current_default_domain = _nl_default_default_domain;

/* Contains the default location of the message catalogs.  */
static const char _nl_default_dirname[] = LOCALEDIR;

/* List with bindings of specific domains created by bindtextdomain()
   calls.  */
static struct binding *_nl_domain_bindings;

/* Prototypes for local functions.  */
static char *find_msg PARAMS ((struct loaded_l10nfile *domain_file,
			       const char *msgid)) internal_function;
static const char *category_to_name PARAMS ((int category)) internal_function;
static const char *guess_category_value PARAMS ((int category,
						 const char *categoryname))
     internal_function;

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define DCGETTEXT __dcgettext
#else
# define DCGETTEXT dcgettext__
#endif

/* Look up MSGID in the DOMAINNAME message catalog for the current CATEGORY
   locale.  */
char *
DCGETTEXT (domainname, msgid, category)
     const char *domainname;
     const char *msgid;
     int category;
{
#ifndef HAVE_ALLOCA
  struct block_list *block_list = NULL;
#endif
  struct loaded_l10nfile *domain;
  struct binding *binding;
  const char *categoryname;
  const char *categoryvalue;
  char *dirname, *xdomainname;
  char *single_locale;
  char *retval;
  int saved_errno = errno;

  /* If no real MSGID is given return NULL.  */
  if (msgid == NULL)
    return NULL;

  /* If DOMAINNAME is NULL, we are interested in the default domain.  If
     CATEGORY is not LC_MESSAGES this might not make much sense but the
     defintion left this undefined.  */
  if (domainname == NULL)
    domainname = _nl_current_default_domain;

  /* First find matching binding.  */
  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It is not in the list.  */
	  binding = NULL;
	  break;
	}
    }

  if (binding == NULL)
    dirname = (char *) _nl_default_dirname;
  else if (binding->dirname[0] == '/')
    dirname = binding->dirname;
  else
    {
      /* We have a relative path.  Make it absolute now.  */
      size_t dirname_len = strlen (binding->dirname) + 1;
      size_t path_max;
      char *ret;

      path_max = (unsigned) PATH_MAX;
      path_max += 2;		/* The getcwd docs say to do this.  */

      dirname = (char *) alloca (path_max + dirname_len);
      ADD_BLOCK (block_list, dirname);

      __set_errno (0);
      while ((ret = getcwd (dirname, path_max)) == NULL && errno == ERANGE)
	{
	  path_max += PATH_INCR;
	  dirname = (char *) alloca (path_max + dirname_len);
	  ADD_BLOCK (block_list, dirname);
	  __set_errno (0);
	}

      if (ret == NULL)
	{
	  /* We cannot get the current working directory.  Don't signal an
	     error but simply return the default string.  */
	  FREE_BLOCKS (block_list);
	  __set_errno (saved_errno);
	  return (char *) msgid;
	}

      stpcpy (stpcpy (strchr (dirname, '\0'), "/"), binding->dirname);
    }

  /* Now determine the symbolic name of CATEGORY and its value.  */
  categoryname = category_to_name (category);
  categoryvalue = guess_category_value (category, categoryname);

  xdomainname = (char *) alloca (strlen (categoryname)
				 + strlen (domainname) + 5);
  ADD_BLOCK (block_list, xdomainname);

  stpcpy (stpcpy (stpcpy (stpcpy (xdomainname, categoryname), "/"),
		  domainname),
	  ".mo");

  /* Creating working area.  */
  single_locale = (char *) alloca (strlen (categoryvalue) + 1);
  ADD_BLOCK (block_list, single_locale);

  /* Search for the given string.  This is a loop because we perhaps
     got an ordered list of languages to consider for th translation.  */
  while (1)
    {
      /* Make CATEGORYVALUE point to the next element of the list.  */
      while (categoryvalue[0] != '\0' && categoryvalue[0] == ':')
	++categoryvalue;
      if (categoryvalue[0] == '\0')
	{
	  /* The whole contents of CATEGORYVALUE has been searched but
	     no valid entry has been found.  We solve this situation
	     by implicitly appending a "C" entry, i.e. no translation
	     will take place.  */
	  single_locale[0] = 'C';
	  single_locale[1] = '\0';
	}
      else
	{
	  char *cp = single_locale;
	  while (categoryvalue[0] != '\0' && categoryvalue[0] != ':')
	    *cp++ = *categoryvalue++;
	  *cp = '\0';
	}

      /* If the current locale value is C (or POSIX) we don't load a
	 domain.  Return the MSGID.  */
      if (strcmp (single_locale, "C") == 0
	  || strcmp (single_locale, "POSIX") == 0)
	{
	  FREE_BLOCKS (block_list);
	  __set_errno (saved_errno);
	  return (char *) msgid;
	}

      /* Find structure describing the message catalog matching the
	 DOMAINNAME and CATEGORY.  */
      domain = _nl_find_domain (dirname, single_locale, xdomainname);

      if (domain != NULL)
	{
	  retval = find_msg (domain, msgid);

	  if (retval == NULL)
	    {
	      int cnt;

	      for (cnt = 0; domain->successor[cnt] != NULL; ++cnt)
		{
		  retval = find_msg (domain->successor[cnt], msgid);

		  if (retval != NULL)
		    break;
		}
	    }

	  if (retval != NULL)
	    {
	      FREE_BLOCKS (block_list);
	      __set_errno (saved_errno);
	      return retval;
	    }
	}
    }
  /* NOTREACHED */
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
#endif

static char *
internal_function
find_msg (domain_file, msgid)
     struct loaded_l10nfile *domain_file;
     const char *msgid;
{
  size_t top, act, bottom;
  struct loaded_domain *domain;

  if (domain_file->decided == 0)
    _nl_load_domain (domain_file);

  if (domain_file->data == NULL)
    return NULL;

  domain = (struct loaded_domain *) domain_file->data;

  /* Locate the MSGID and its translation.  */
  if (domain->hash_size > 2 && domain->hash_tab != NULL)
    {
      /* Use the hashing table.  */
      nls_uint32 len = strlen (msgid);
      nls_uint32 hash_val = hash_string (msgid);
      nls_uint32 idx = hash_val % domain->hash_size;
      nls_uint32 incr = 1 + (hash_val % (domain->hash_size - 2));
      nls_uint32 nstr = W (domain->must_swap, domain->hash_tab[idx]);

      if (nstr == 0)
	/* Hash table entry is empty.  */
	return NULL;

      if (W (domain->must_swap, domain->orig_tab[nstr - 1].length) == len
	  && strcmp (msgid,
		     domain->data + W (domain->must_swap,
				       domain->orig_tab[nstr - 1].offset)) == 0)
	return (char *) domain->data + W (domain->must_swap,
					  domain->trans_tab[nstr - 1].offset);

      while (1)
	{
	  if (idx >= domain->hash_size - incr)
	    idx -= domain->hash_size - incr;
	  else
	    idx += incr;

	  nstr = W (domain->must_swap, domain->hash_tab[idx]);
	  if (nstr == 0)
	    /* Hash table entry is empty.  */
	    return NULL;

	  if (W (domain->must_swap, domain->orig_tab[nstr - 1].length) == len
	      && strcmp (msgid,
			 domain->data + W (domain->must_swap,
					   domain->orig_tab[nstr - 1].offset))
	         == 0)
	    return (char *) domain->data
	      + W (domain->must_swap, domain->trans_tab[nstr - 1].offset);
	}
      /* NOTREACHED */
    }

  /* Now we try the default method:  binary search in the sorted
     array of messages.  */
  bottom = 0;
  top = domain->nstrings;
  while (bottom < top)
    {
      int cmp_val;

      act = (bottom + top) / 2;
      cmp_val = strcmp (msgid, domain->data
			       + W (domain->must_swap,
				    domain->orig_tab[act].offset));
      if (cmp_val < 0)
	top = act;
      else if (cmp_val > 0)
	bottom = act + 1;
      else
	break;
    }

  /* If an translation is found return this.  */
  return bottom >= top ? NULL : (char *) domain->data
                                + W (domain->must_swap,
				     domain->trans_tab[act].offset);
}

/* Return string representation of locale CATEGORY.  */
static const char *
internal_function
category_to_name (category)
     int category;
{
  const char *retval;

  switch (category)
  {
#ifdef LC_COLLATE
  case LC_COLLATE:
    retval = "LC_COLLATE";
    break;
#endif
#ifdef LC_CTYPE
  case LC_CTYPE:
    retval = "LC_CTYPE";
    break;
#endif
#ifdef LC_MONETARY
  case LC_MONETARY:
    retval = "LC_MONETARY";
    break;
#endif
#ifdef LC_NUMERIC
  case LC_NUMERIC:
    retval = "LC_NUMERIC";
    break;
#endif
#ifdef LC_TIME
  case LC_TIME:
    retval = "LC_TIME";
    break;
#endif
#ifdef LC_MESSAGES
  case LC_MESSAGES:
    retval = "LC_MESSAGES";
    break;
#endif
#ifdef LC_RESPONSE
  case LC_RESPONSE:
    retval = "LC_RESPONSE";
    break;
#endif
#ifdef LC_ALL
  case LC_ALL:
    /* This might not make sense but is perhaps better than any other
       value.  */
    retval = "LC_ALL";
    break;
#endif
  default:
    /* If you have a better idea for a default value let me know.  */
    retval = "LC_XXX";
  }

  return retval;
}

/* Guess value of current locale from value of the environment variables.  */
static const char *
internal_function
guess_category_value (category, categoryname)
     int category;
     const char *categoryname;
{
  const char *retval;

  /* The highest priority value is the `LANGUAGE' environment
     variable.  This is a GNU extension.  */
  retval = getenv ("LANGUAGE");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* `LANGUAGE' is not set.  So we have to proceed with the POSIX
     methods of looking to `LC_ALL', `LC_xxx', and `LANG'.  On some
     systems this can be done by the `setlocale' function itself.  */
#if defined HAVE_SETLOCALE && defined HAVE_LC_MESSAGES && defined HAVE_LOCALE_NULL
  return setlocale (category, NULL);
#else
  /* Setting of LC_ALL overwrites all other.  */
  retval = getenv ("LC_ALL");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Next comes the name of the desired category.  */
  retval = getenv (categoryname);
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* Last possibility is the LANG environment variable.  */
  retval = getenv ("LANG");
  if (retval != NULL && retval[0] != '\0')
    return retval;

  /* We use C as the default domain.  POSIX says this is implementation
     defined.  */
  return "C";
#endif
}

/* We don't want libintl.a to depend on any other library.  So we
   avoid the non-standard function stpcpy.  In GNU C Library this
   function is available, though.  Also allow the symbol HAVE_STPCPY
   to be defined.  */
#if !_LIBC && !HAVE_STPCPY
static char *
stpcpy (dest, src)
     char *dest;
     const char *src;
{
  while ((*dest++ = *src++) != '\0')
    /* Do nothing. */ ;
  return dest - 1;
}
#endif

#ifdef _LIBC
/* If we want to free all resources we have to do some work at
   program's end.  */
static void __attribute__ ((unused))
free_mem (void)
{
  struct binding *runp;

  for (runp = _nl_domain_bindings; runp != NULL; runp = runp->next)
    {
      free (runp->domainname);
      if (runp->dirname != _nl_default_dirname)
	/* Yes, this is a pointer comparison.  */
	free (runp->dirname);
    }

  if (_nl_current_default_domain != _nl_default_default_domain)
    /* Yes, again a pointer comparison.  */
    free ((char *) _nl_current_default_domain);
}

text_set_element (__libc_subfreeres, free_mem);
#endif

/* End of dcgettext.c */

/* Begin of bindtextdom.c */

/* Implementation of the bindtextdomain(3) function
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

#if defined STDC_HEADERS || defined _LIBC
#else
# ifdef HAVE_MALLOC_H
# else
void free ();
# endif
#endif

#if defined HAVE_STRING_H || defined _LIBC
#else
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif

#ifdef _LIBC
#else
#endif

/* Contains the default location of the message catalogs.  */
/* static const char _nl_default_dirname[]; */

/* List with bindings of specific domains.  */
/* static struct binding *_nl_domain_bindings; */

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define BINDTEXTDOMAIN __bindtextdomain
# ifndef strdup
#  define strdup(str) __strdup (str)
# endif
#else
# define BINDTEXTDOMAIN bindtextdomain__
#endif

/* Specify that the DOMAINNAME message catalog will be found
   in DIRNAME rather than in the system locale data base.  */
static char *
BINDTEXTDOMAIN (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  struct binding *binding;

  /* Some sanity checks.  */
  if (domainname == NULL || domainname[0] == '\0')
    return NULL;

  for (binding = _nl_domain_bindings; binding != NULL; binding = binding->next)
    {
      int compare = strcmp (domainname, binding->domainname);
      if (compare == 0)
	/* We found it!  */
	break;
      if (compare < 0)
	{
	  /* It is not in the list.  */
	  binding = NULL;
	  break;
	}
    }

  if (dirname == NULL)
    /* The current binding has be to returned.  */
    return binding == NULL ? (char *) _nl_default_dirname : binding->dirname;

  if (binding != NULL)
    {
      /* The domain is already bound.  If the new value and the old
	 one are equal we simply do nothing.  Otherwise replace the
	 old binding.  */
      if (strcmp (dirname, binding->dirname) != 0)
	{
	  char *new_dirname;

	  if (strcmp (dirname, _nl_default_dirname) == 0)
	    new_dirname = (char *) _nl_default_dirname;
	  else
	    {
#if defined _LIBC || defined HAVE_STRDUP
	      new_dirname = strdup (dirname);
	      if (new_dirname == NULL)
		return NULL;
#else
	      size_t len = strlen (dirname) + 1;
	      new_dirname = (char *) malloc (len);
	      if (new_dirname == NULL)
		return NULL;

	      memcpy (new_dirname, dirname, len);
#endif
	    }

	  if (binding->dirname != _nl_default_dirname)
	    free (binding->dirname);

	  binding->dirname = new_dirname;
	}
    }
  else
    {
      /* We have to create a new binding.  */
#if !defined _LIBC && !defined HAVE_STRDUP
      size_t len;
#endif
      struct binding *new_binding =
	(struct binding *) malloc (sizeof (*new_binding));

      if (new_binding == NULL)
	return NULL;

#if defined _LIBC || defined HAVE_STRDUP
      new_binding->domainname = strdup (domainname);
      if (new_binding->domainname == NULL)
	return NULL;
#else
      len = strlen (domainname) + 1;
      new_binding->domainname = (char *) malloc (len);
      if (new_binding->domainname == NULL)
	return NULL;
      memcpy (new_binding->domainname, domainname, len);
#endif

      if (strcmp (dirname, _nl_default_dirname) == 0)
	new_binding->dirname = (char *) _nl_default_dirname;
      else
	{
#if defined _LIBC || defined HAVE_STRDUP
	  new_binding->dirname = strdup (dirname);
	  if (new_binding->dirname == NULL)
	    return NULL;
#else
	  len = strlen (dirname) + 1;
	  new_binding->dirname = (char *) malloc (len);
	  if (new_binding->dirname == NULL)
	    return NULL;
	  memcpy (new_binding->dirname, dirname, len);
#endif
	}

      /* Now enqueue it.  */
      if (_nl_domain_bindings == NULL
	  || strcmp (domainname, _nl_domain_bindings->domainname) < 0)
	{
	  new_binding->next = _nl_domain_bindings;
	  _nl_domain_bindings = new_binding;
	}
      else
	{
	  binding = _nl_domain_bindings;
	  while (binding->next != NULL
		 && strcmp (domainname, binding->next->domainname) > 0)
	    binding = binding->next;

	  new_binding->next = binding->next;
	  binding->next = new_binding;
	}

      binding = new_binding;
    }

  return binding->dirname;
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
#endif

/* End of bindtextdom.c */

/* Begin of dgettext.c */

/* Implementation of the dgettext(3) function
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.  */

#if defined HAVE_LOCALE_H || defined _LIBC
# include <locale.h>
#undef stderr
#define stderr stdout
#endif

#ifdef _LIBC
#else
#endif

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define DGETTEXT __dgettext
# define DCGETTEXT __dcgettext
#else
# define DGETTEXT dgettext__
# define DCGETTEXT dcgettext__
#endif

/* Look up MSGID in the DOMAINNAME message catalog of the current
   LC_MESSAGES locale.  */
static char *
DGETTEXT (domainname, msgid)
     const char *domainname;
     const char *msgid;
{
  return DCGETTEXT (domainname, msgid, LC_MESSAGES);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
#endif

/* End of dgettext.c */

/* Begin of gettext.c */

/* Implementation of gettext(3) function.
   Copyright (C) 1995, 1997 Free Software Foundation, Inc.  */

#ifdef _LIBC
# define __need_NULL
# include <stddef.h>
#undef stderr
#define stderr stdout
#else
# ifdef STDC_HEADERS
#  include <stdlib.h>		/* Just for NULL.  */
#undef stderr
#define stderr stdout
# else
#  ifdef HAVE_STRING_H
#  else
#   define NULL ((void *) 0)
#  endif
# endif
#endif

#ifdef _LIBC
#else
#endif

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define GETTEXT __gettext
# define DGETTEXT __dgettext
#else
# define GETTEXT gettext__
# define DGETTEXT dgettext__
#endif

/* Look up MSGID in the current default message catalog for the current
   LC_MESSAGES locale.  If not found, returns MSGID itself (the default
   text).  */
static char *
GETTEXT (msgid)
     const char *msgid;
{
  return DGETTEXT (NULL, msgid);
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
#endif

/* End of gettext.c */

/* Begin of textdomain.c */

/* Implementation of the textdomain(3) function.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, 1995.  */

#if defined STDC_HEADERS || defined _LIBC
#endif

#if defined STDC_HEADERS || defined HAVE_STRING_H || defined _LIBC
#else
# ifndef memcpy
#  define memcpy(Dst, Src, Num) bcopy (Src, Dst, Num)
# endif
#endif

#ifdef _LIBC
#else
#endif

/* Name of the default text domain.  */
/* static const char _nl_default_default_domain[]; */

/* Default text domain in which entries for gettext(3) are to be found.  */
/* static const char *_nl_current_default_domain; */

/* Names for the libintl functions are a problem.  They must not clash
   with existing names and they should follow ANSI C.  But this source
   code is also used in GNU C Library where the names have a __
   prefix.  So we have to make a difference here.  */
#ifdef _LIBC
# define TEXTDOMAIN __textdomain
# ifndef strdup
#  define strdup(str) __strdup (str)
# endif
#else
# define TEXTDOMAIN textdomain__
#endif

/* Set the current default message catalog to DOMAINNAME.
   If DOMAINNAME is null, return the current default.
   If DOMAINNAME is "", reset to the default of "messages".  */
static char *
TEXTDOMAIN (domainname)
     const char *domainname;
{
  char *old;

  /* A NULL pointer requests the current setting.  */
  if (domainname == NULL)
    return (char *) _nl_current_default_domain;

  old = (char *) _nl_current_default_domain;

  /* If domain name is the null string set to default domain "messages".  */
  if (domainname[0] == '\0'
      || strcmp (domainname, _nl_default_default_domain) == 0)
    _nl_current_default_domain = _nl_default_default_domain;
  else
    {
      /* If the following malloc fails `_nl_current_default_domain'
	 will be NULL.  This value will be returned and so signals we
	 are out of core.  */
#if defined _LIBC || defined HAVE_STRDUP
      _nl_current_default_domain = strdup (domainname);
#else
      size_t len = strlen (domainname) + 1;
      char *cp = (char *) malloc (len);
      if (cp != NULL)
	memcpy (cp, domainname, len);
      _nl_current_default_domain = cp;
#endif
    }

  if (old != _nl_default_default_domain)
    free (old);

  return (char *) _nl_current_default_domain;
}

#ifdef _LIBC
/* Alias for function name in GNU C Library.  */
#endif

/* End of textdomain.c */

/* Begin of intl-compat.c */

/* intl-compat.c - Stub functions to call gettext functions from GNU gettext
   Library.
   Copyright (C) 1995 Software Foundation, Inc.  */

#undef gettext
#undef dgettext
#undef dcgettext
#undef textdomain
#undef bindtextdomain

char *
bindtextdomain (domainname, dirname)
     const char *domainname;
     const char *dirname;
{
  return bindtextdomain__ (domainname, dirname);
}

char *
dcgettext (domainname, msgid, category)
     const char *domainname;
     const char *msgid;
     int category;
{
  return dcgettext__ (domainname, msgid, category);
}

char *
dgettext (domainname, msgid)
     const char *domainname;
     const char *msgid;
{
  return dgettext__ (domainname, msgid);
}

char *
gettext (msgid)
     const char *msgid;
{
  return gettext__ (msgid);
}

char *
textdomain (domainname)
     const char *domainname;
{
  return textdomain__ (domainname);
}

/* End of intl-compat.c */


