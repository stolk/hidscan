//
// hidscan.c
//
// (c)2023 Bram Stolk
//

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <wchar.h>		// hidapi uses wide characters.
#include <inttypes.h>
#if defined(_WIN32)
#	include <Windows.h>
#else
#	include <unistd.h>
#	include <sysexits.h>
#endif
#include <string.h>
#include <float.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <hidapi/hidapi.h>

#include "page_names.h"
#include "generic_desktop_usage_names.h"
#include "lighting_and_illumination_names.h"
#include "consumer_usage_names.h"

#if defined(_WIN32)
#	define EX_IOERR	EXIT_FAILURE
#	define EX_NOINPUT EXIT_FAILURE
#endif

// More than 128 HID devices in a single PC would be silly.
#define MAXDEVS			128

static int opt_verbose = 0;

// Howmany hid devices did we find?
static int numdevs = 0;

static int hidscan_finished = 0;

static int hidscan_paused = 0;

#define STRLEN	128


typedef struct _hs_device
{
	char		path[STRLEN];
	wchar_t		manufacturer_string[STRLEN];
	wchar_t		product_string[STRLEN];
	hid_device*	dev;
	uint16_t	usage_page;
	uint16_t	usage;
} hs_device_t;


static hs_device_t	devices[MAXDEVS];


const char* page_name(uint16_t nr)
{
	if (nr>=0xff00)
		return "Vendor-defined";
	if (nr == 0xf1d0)
		return "FIDO Alliance";
	if (nr>=0x93)
		return "Reserved";
	return page_names[nr];
}


const char* usage_name(uint16_t pagenr, uint16_t usagenr)
{
	if (pagenr == 0x01)
	{
		if (usagenr >= 0xe3)
			return "Reserved";
		return generic_desktop_usage_names[usagenr];
	}
	if (pagenr == 0x0c)
	{
		if (usagenr >= 0x2d5)
			return "Reserved";
		return consumer_usage_names[usagenr];
	}
	if (pagenr == 0x59)
	{
		if (usagenr >= 72)
			return "Reserved";
		return lighting_and_illumination_names[usagenr];
	}
	return "";
}


void hidscan_pause_all_devices(void)
{
	hidscan_paused = 1;
	if (opt_verbose)
		fprintf( stderr, "Preparing to go to sleep...\n" );
#if defined(_WIN32)
	const uint8_t rep[8] = { 0x00, 0x40, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, };
	Sleep(40);
#else
	const uint8_t rep[2] = { 0x00, 0x40 };
	usleep(40000);
#endif

	for ( int i=0; i<numdevs; ++i )
	{
		hid_device* hd = devices[i].dev;
		const int written = hid_write( hd, rep, sizeof(rep) );
		if (written<0)
		{
			fprintf( stderr, "hid_write() to %s failed for %zu bytes with: %ls\n", "device", sizeof(rep), hid_error(hd) );
		}
	}
#if defined(_WIN32)
	Sleep(40);
#else
	usleep(40000);
#endif
}


void hidscan_cleanup(void)
{
	if ( !hidscan_paused )
	{
		hidscan_pause_all_devices();
	}

	for ( int i=0; i<numdevs; ++i )
	{
		hid_device* hd = devices[i].dev;
		hid_close(hd);
		devices[i].dev = 0;
	}
	hid_exit();
	numdevs=0;
}


#if !defined(_WIN32)
static int get_permissions( const char* fname )
{
	struct stat statRes;
	const int r = stat( fname, &statRes );
	if ( r )
	{
		fprintf(stderr,"Error: stat() failed with %s on %s\n", strerror(errno), fname);
		exit(EX_NOINPUT);
	}
	const mode_t bits = statRes.st_mode;
	return bits;
}

static int is_accessible(const char* fname)
{
	int accessible = 0;
	const mode_t bits = get_permissions( fname );
	if ( ( bits & S_IROTH ) && ( bits & S_IWOTH ) )
	{
		accessible = 1;
	}
	else
	{
		fprintf
		(
			stderr,
			"Error: No rw-permission for %s which has permission %04o.\n",
			fname, bits
		);
	}
	return accessible;
}
#endif


// We are passed a set of HID devices that matched our enumeration.
// Here, we should examine them, and select one to open.
int hidscan_select_and_open_device( struct hid_device_info* devs )
{
	hid_device* handle = 0;
	struct hid_device_info* cur_dev = devs;
	int count=0;
	const char* filenames[128]={ 0, };
	int rv=0;
	hs_device_t* device = devices;

	while (cur_dev)
	{
		if (cur_dev->manufacturer_string)
			wcpncpy(device->manufacturer_string, cur_dev->manufacturer_string, STRLEN-1);
		if (cur_dev->product_string)
			wcpncpy(device->product_string,      cur_dev->product_string,      STRLEN-1);
		stpncpy(device->path,                cur_dev->path,                STRLEN-1);
		device->usage_page = cur_dev->usage_page;
		device->usage = cur_dev->usage;
		device++;
		count++;
		cur_dev = cur_dev->next;
	}

	numdevs = count;
	if (opt_verbose)
		fprintf( stderr, "Found %d hid devices.\n", count );

#if 0
	for ( int i=0; i<count; ++i )
	{
		// First check if permissions are good on the /dev/rawhidX file.
		const char* fname = devices[i].path;
		if (!is_accessible(fname))
			exit(EX_NOPERM);
		// We can open it.
		handle = hid_open_path( filenames[i] );
		if (!handle)
		{
			fprintf(stderr,"Error: hid_open_path() on %s failed : %ls\n", fname, hid_error(handle));
			exit(EX_IOERR);
		}
		fprintf(stderr,"Opened hid device at %s\n", fname);
		// We like to be blocked.
		hid_set_nonblocking(handle, 0);
		devices[i].dev = handle;
		rv++;
	}
#endif

	return rv;	// return the nr of devices we opened.
}


void hidscan_list(void)
{
	int idx=0;
	while(idx < numdevs)
	{
		if (opt_verbose)
		{
			fprintf(stderr, "%s\n", devices[idx].path);
		}
		fprintf(stdout, "%ls %ls", devices[idx].manufacturer_string, devices[idx].product_string);
		uint16_t prvpag = 0xffff;
		while(1)
		{
			const uint16_t pag = devices[idx].usage_page;
			const uint16_t usa = devices[idx].usage;
			const char* pname = page_name(pag);
			const char* uname = usage_name(pag, usa);
			if (pag != prvpag)
				fprintf(stdout,"\n\t%s/%s", pname, uname);
			else
				fprintf(stdout,",%s", uname);
			idx += 1;
			prvpag = pag;
			if (idx == numdevs || strcmp(devices[idx].path, devices[idx-1].path))
			{
				// Proceed to next raw hid device.
				fprintf(stdout, "\n");
				break;
			}
		}
	}
}


int hidscan_service( void )
{
	return 0;
}


int hidscan_init(void)
{
	hidscan_finished = 0;

	if (hid_init())
	{
		fprintf(stderr, "hid_init() failed: %ls\n", hid_error(0));
		fflush(stderr);
		return 1;
	}
	else
	{
		if (opt_verbose)
		{
			fprintf(stderr, "hid_init() succeeded.\n");
			fflush(stderr);
		}
	}

	struct hid_device_info* devs_hid=0;

	devs_hid  = hid_enumerate( 0x0000, 0x0000 );

	if ( !devs_hid )
	{
		fprintf(stderr,"No HID devices were found.\n");
		fflush(stderr);
		return 1;
	}

	if ( devs_hid )
	{
		if (opt_verbose)
		{
			fprintf(stderr, "Examining hid devices...\n");
			fflush(stderr);
		}
		const int num = hidscan_select_and_open_device( devs_hid );
		(void)num;
		hidscan_list();
		hid_free_enumeration(devs_hid);
	}

	if ( numdevs== 0 )
	{
		fprintf(stderr,"Failed to select and open device.\n");
		fflush(stderr);
		return 1;
	}

	return 0;
}


int main(int argc, char* argv[])
{
	for (int i=1; i<argc; ++i)
	{
		const char* a = argv[i];
		if (!strcmp(a, "-v"))
			opt_verbose = 1;
		if (!strcmp(a, "--versbose"))
			opt_verbose = 1;
	}
	hidscan_init();
	return 0;
}

