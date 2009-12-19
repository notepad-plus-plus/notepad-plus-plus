/*

  Calculate or Check MD5 Signature of File or Command Line Argument

			    by John Walker
		       http://www.fourmilab.ch/

		This program is in the public domain.

*/

#define VERSION     "2.2 (2008-01-14)"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "md5.h"

#define FALSE	0
#define TRUE	1

#define EOS     '\0'

/*  Main program  */

int main(argc, argv)
  int argc; char *argv[];
{
    int i, j, opt, cdata = FALSE, docheck = FALSE, showfile = TRUE, f = 0;
    unsigned int bp;
    char *cp, *clabel, *ifname, *hexfmt = "%02X";
    FILE *in = stdin, *out = stdout;
    unsigned char buffer[16384], signature[16], csig[16];
    struct MD5Context md5c;
    
    /*	Build parameter quality control.  Verify machine
    	properties were properly set in md5.h and refuse
	to run if they're not correct.  */
	
#ifdef CHECK_HARDWARE_PROPERTIES
    /*	Verify unit32 is, in fact, a 32 bit data type.  */
    if (sizeof(uint32) != 4) {
    	fprintf(stderr, "** Configuration error.  Setting for uint32 in file md5.h\n");
	fprintf(stderr, "   is incorrect.  This must be a 32 bit data type, but it\n");
	fprintf(stderr, "   is configured as a %d bit data type.\n", sizeof(uint32) * 8);
	return 2;
    }
    
    /*	If HIGHFIRST is not defined, verify that this machine is,
    	in fact, a little-endian architecture.  */
	
#ifndef HIGHFIRST
    {	uint32 t = 0x12345678;
    
    	if (*((char *) &t) != 0x78) {
    	    fprintf(stderr, "** Configuration error.  Setting for HIGHFIRST in file md5.h\n");
	    fprintf(stderr, "   is incorrect.  This symbol has not been defined, yet this\n");
	    fprintf(stderr, "   machine is a big-endian (most significant byte first in\n");
	    fprintf(stderr, "   memory) architecture.  Please modify md5.h so HIGHFIRST is\n");
	    fprintf(stderr, "   defined when building for this machine.\n");
	    return 2;
	}
    }
#endif
#endif
    
    /*	Process command line options.  */

    for (i = 1; i < argc; i++) {
	cp = argv[i];
        if (*cp == '-') {
	    if (strlen(cp) == 1) {
	    	i++;
	    	break;	    	      /* -  --  Mark end of options; balance are files */
	    }
	    opt = *(++cp);
	    if (islower(opt)) {
		opt = toupper(opt);
	    }

	    switch (opt) {

                case 'C':             /* -Csignature  --  Check signature, set return code */
		    docheck = TRUE;
		    if (strlen(cp + 1) != 32) {
			docheck = FALSE;
		    }
		    memset(csig, 0, 16);
		    clabel = cp + 1;
		    for (j = 0; j < 16; j++) {
			if (isxdigit((int) clabel[0]) && isxdigit((int) clabel[1]) &&
                            sscanf((cp + 1 + (j * 2)), hexfmt, &bp) == 1) {
			    csig[j] = (unsigned char) bp;
			} else {
			    docheck = FALSE;
			    break;
			}
			clabel += 2;
		    }
		    if (!docheck) {
                        fprintf(stderr, "Error in signature specification.  Must be 32 hex digits.\n");
			return 2;
		    }
		    break;

                case 'D':             /* -Dtext  --  Compute signature of given text */
		    MD5Init(&md5c);
		    MD5Update(&md5c, (unsigned char *) (cp + 1), strlen(cp + 1));
		    cdata = TRUE;
		    f++;	      /* Mark no infile argument needed */
		    break;
		    
		case 'L':   	      /* -L  --  Use lower case letters as hex digits */
		    hexfmt = "%02x";
		    break;
		    
		case 'N':   	      /* -N  --  Don't show file name after sum */
		    showfile = FALSE;
		    break;
		    
		case 'O':   	      /* -Ofname  --  Write output to fname (- = stdout) */
		    cp++;
                    if (strcmp(cp, "-") != 0) {
		    	if (out != stdout) {
			    fprintf(stderr, "Redundant output file specification.\n");
			    return 2;
    	    	    	}
                        if ((out = fopen(cp, "w")) == NULL) {
                            fprintf(stderr, "Cannot open output file %s\n", cp);
			    return 2;
			}
		    }
		    break;

                case '?':             /* -U, -? -H  --  Print how to call information. */
                case 'H':
                case 'U':
    printf("\nMD5  --  Calculate MD5 signature of file.  Call");
    printf("\n         with md5 [ options ] [file ...]");
    printf("\n");
    printf("\n         Options:");
    printf("\n              -csig   Check against sig, set exit status 0 = OK");
    printf("\n              -dtext  Compute signature of text argument");
    printf("\n              -l      Use lower case letters for hexadecimal digits");
    printf("\n              -n      Do not show file name after sum");
    printf("\n              -ofname Write output to fname (- = stdout)");
    printf("\n              -u      Print this message");
    printf("\n              -v      Print version information");
    printf("\n");
    printf("\nby John Walker  --  http://www.fourmilab.ch/");
    printf("\nVersion %s\n", VERSION);
    printf("\nThis program is in the public domain.\n");
    printf("\n");
#ifdef CHECK_HARDWARE_PROPERTIES
#ifdef HIGHFIRST
    {	uint32 t = 0x12345678;
    
    	if (*((char *) &t) == 0x78) {
    	    fprintf(stderr, "** Note.  md5 is not optimally configured for use on this\n");
	    fprintf(stderr, "   machine.  This is a little-endian (least significant byte\n");
	    fprintf(stderr, "   first in memory) architecture, yet md5 has been built with the\n");
	    fprintf(stderr, "   symbol HIGHFIRST defined in md5.h, which includes code which\n");
	    fprintf(stderr, "   supports both big- and little-endian machines.  Modifying\n");
	    fprintf(stderr, "   md5.h to undefine HIGHFIRST for this platform will make md5\n");
	    fprintf(stderr, "   run faster on it.\n");
	}
    }
#endif
#endif
		    return 0;
		    
		case 'V':   	      /* -V  --  Print version number */
		    printf("%s\n", VERSION);
		    return 0;
	    }
	} else {
	    break;
	}
    }
    
    if (cdata && (i < argc)) {
    	fprintf(stderr, "Cannot specify both -d option and input file.\n");
	return 2;
    }
    
    if ((i >= argc) && (f == 0)) {
    	f++;
    }
    
    for (; (f > 0) || (i < argc); i++) {
    	if ((!cdata) && (f > 0)) {
	    ifname = "-";
	} else {
    	    ifname = argv[i];
	}
	f = 0;

	if (!cdata) {
	    int opened = FALSE;
	    
	    /* If the data weren't supplied on the command line with
	       the "-d" option, read it now from the input file. */
	
	    if (strcmp(ifname, "-") != 0) {
		if ((in = fopen(ifname, "rb")) == NULL) {
	    	    fprintf(stderr, "Cannot open input file %s\n", ifname);
		    return 2;
		}
		opened = TRUE;
	    } else {
		in = stdin;
	    }
#ifdef _WIN32

	    /** Warning!  On systems which distinguish text mode and
		binary I/O (MS-DOS, Macintosh, etc.) the modes in the open
        	statement for "in" should have forced the input file into
        	binary mode.  But what if we're reading from standard
		input?  Well, then we need to do a system-specific tweak
        	to make sure it's in binary mode.  While we're at it,
        	let's set the mode to binary regardless of however fopen
		set it.

		The following code, conditional on _WIN32, sets binary
		mode using the method prescribed by Microsoft Visual C 7.0
        	("Monkey C"); this may require modification if you're
		using a different compiler or release of Monkey C.	If
        	you're porting this code to a different system which
        	distinguishes text and binary files, you'll need to add
		the equivalent call for that system. */

	    _setmode(_fileno(in), _O_BINARY);
#endif
    
    	    MD5Init(&md5c);
	    while ((j = (int) fread(buffer, 1, sizeof buffer, in)) > 0) {
		MD5Update(&md5c, buffer, (unsigned) j);
	    }
	    
	    if (opened) {
	    	fclose(in);
	    }
	}
	MD5Final(signature, &md5c);

	if (docheck) {
	    docheck = 0;
	    for (j = 0; j < sizeof signature; j++) {
		if (signature[j] != csig[j]) {
		    docheck = 1;
		    break;
		}
	    }
	    if (i < (argc - 1)) {
	    	fprintf(stderr, "Only one file may be tested with the -c option.\n");
		return 2;
	    }
	} else {
	    for (j = 0; j < sizeof signature; j++) {
        	fprintf(out, hexfmt, signature[j]);
	    }
	    if ((!cdata) && showfile) {
		fprintf(out, "  %s", (in == stdin) ? "-" : ifname);
	    }
            fprintf(out, "\n");
	}
    }

    return docheck;
}
