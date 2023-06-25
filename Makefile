CFLAGS?=-g -O
CFLAGS+=`pkgconf hidapi-hidraw --cflags`
LDFLAGS+=`pkgconf hidapi-hidraw --libs`

hidscan: hidscan.c
	$(CC) $(CFLAGS) hidscan.c -o hidscan $(LDFLAGS)

