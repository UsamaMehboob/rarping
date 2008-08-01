#
#       MAKEFILE rarping
#
#       Author : Henri Doreau <henri.doreau@gmail.com>
 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

CC=gcc
ECHO=echo

CFLAGS=-pedantic
DEBUG_CFLAGS=-g -DDEBUG

BASENAME=	rarping
EXEC=$(BASENAME)


.SILENT:

all: release

release: $(EXEC) $(BASENAME).c $(BASENAME).h
	$(ECHO) "    [CC] $@"
	$(CC) -o $(EXEC) $(CFLAGS) $(BASENAME).c

debug: $(EXEC) $(BASENAME).c $(BASENAME).h
	$(ECHO) "    [CC] $@"
	$(CC) -o $(EXEC) $(CFLAGS) $(DEBUG_CFLAGS) $(BASENAME).c

clean:
	$(ECHO) "    [Cleaning]"
	rm -f rarping *.o \#* *~

distclean: clean
