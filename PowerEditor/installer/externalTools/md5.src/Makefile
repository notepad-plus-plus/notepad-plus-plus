
PROGRAM = md5

CC = gcc
CFLAGS = -O3 -Wall

RELFILES = Makefile index.html main.c md5.c md5.exe \
	   md5.png md5s.png md5.h \
	   md5.vcproj md5.sln \
	   rfc1321.html rfc1321.txt

all:	$(PROGRAM)

md5:	md5.o main.o
	$(CC) -o md5 md5.o main.o

zip:
	rm -f md5.zip
	zip md5.zip $(RELFILES)
	
tar:
	rm -f md5.tar.gz md5.tar
	tar cfv md5.tar $(RELFILES)
	gzip md5.tar

lint:
	lint main.c md5.c

#	The silly stuff with "tr" is to allow directly cutting and
#	pasting the test cases from RFC 1321.
check:	$(PROGRAM)
	./md5 -d"" -otest.out
	./md5 -d"a" >>test.out
	./md5 -d"abc" >>test.out
	./md5 -d"message digest" >>test.out
	./md5 -d"abcdefghijklmnopqrstuvwxyz" >>test.out
	./md5 -d"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" >>test.out
	./md5 -d"12345678901234567890123456789012345678901234567890123456789012345678901234567890" >>test.out
	@echo "d41d8cd98f00b204e9800998ecf8427e" | tr [a-f] [A-F] >expected.out
	@echo "0cc175b9c0f1b6a831c399e269772661" | tr [a-f] [A-F] >>expected.out
	@echo "900150983cd24fb0d6963f7d28e17f72" | tr [a-f] [A-F] >>expected.out
	@echo "f96b697d7cb7938d525a2f31aaf161d0" | tr [a-f] [A-F] >>expected.out
	@echo "c3fcd3d76192e4007dfb496cca67e13b" | tr [a-f] [A-F] >>expected.out
	@echo "d174ab98d277d9f5a5611c2c9f419d9f" | tr [a-f] [A-F] >>expected.out
	@echo "57edf4a22be3c955ac49da2e2107b67a" | tr [a-f] [A-F] >>expected.out
	@diff test.out expected.out ; if test $$? -ne 0  ; then \
	    echo '** md5:  Verification test failed. **' ; else \
	    echo 'All tests passed.' ; fi

#	Test the Win32 version running under "Wine" (which,
#	obviously, must be installed).
wcheck:	$(PROGRAM)
	wine ./md5.exe -d"" -owtest.out
	wine ./md5.exe -d"a" >>wtest.out
	wine ./md5.exe -d"abc" >>wtest.out
	wine ./md5.exe -d"message digest" >>wtest.out
	wine ./md5.exe -d"abcdefghijklmnopqrstuvwxyz" >>wtest.out
	wine ./md5.exe -d"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" >>wtest.out
	wine ./md5.exe -d"12345678901234567890123456789012345678901234567890123456789012345678901234567890" >>wtest.out
	@echo "d41d8cd98f00b204e9800998ecf8427e" | tr [a-f] [A-F] >expected.out
	@echo "0cc175b9c0f1b6a831c399e269772661" | tr [a-f] [A-F] >>expected.out
	@echo "900150983cd24fb0d6963f7d28e17f72" | tr [a-f] [A-F] >>expected.out
	@echo "f96b697d7cb7938d525a2f31aaf161d0" | tr [a-f] [A-F] >>expected.out
	@echo "c3fcd3d76192e4007dfb496cca67e13b" | tr [a-f] [A-F] >>expected.out
	@echo "d174ab98d277d9f5a5611c2c9f419d9f" | tr [a-f] [A-F] >>expected.out
	@echo "57edf4a22be3c955ac49da2e2107b67a" | tr [a-f] [A-F] >>expected.out
	@diff -b wtest.out expected.out ; if test $$? -ne 0  ; then \
	    echo '** md5:  Verification test failed. **' ; else \
	    echo 'All tests passed.' ; fi

clean:
	rm -f $(PROGRAM) *.bak *.o *.out core
	
md5.o:	md5.c md5.h

main.o: main.c md5.h

