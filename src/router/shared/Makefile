#
# Linux router shared code Makefile
#
# Copyright 2001-2003, Broadcom Corporation
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.10 2005/11/30 11:53:42 seg Exp $
#


include $(TOP)/.config

ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

ifeq ($(CONFIG_MSSID),y)
ifeq ($(CONFIG_BUFFALO),y)
CFLAGS	= -I. -I../httpd -I$(SRCBASE)/include.bcm -Wall -I$(SRCBASE)/
else
CFLAGS	= -I. -I../httpd -I$(SRCBASE)/include.v24 -Wall -I$(SRCBASE)/
endif
else
CFLAGS	= -I. -I../httpd -I$(SRCBASE)/include.v23 -Wall -I$(SRCBASE)/
endif

ifeq ($(CONFIG_80211AC),y)
CFLAGS	= -I.  -I../httpd -Wall -I$(SRCBASE)/
CFLAGS +=  -I$(LINUXDIR)/brcm/$(ARCH)/include  -I$(LINUXDIR)/brcm/$(ARCH)/shared/bcmwifi/include
endif

#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -fPIC 
LDFLAGS += -L.
OBJS := nvparse.o utils.o linux_osl.o
#defaults.o
ifeq ($(CONFIG_MAGICBOX),y)
CFLAGS += -fpic

endif
ifeq ($(CONFIG_RB600),y)
CFLAGS += -fpic

endif

ifeq ($(CONFIG_DROPBEAR_SSHD),y)
  CFLAGS += -DHAVE_DROPBEAR -DHAVE_SSHD
endif

ifeq ($(CONFIG_SUPERCHANNEL),y)
CFLAGS += -DHAVE_SUPERCHANNEL
endif
ifeq ($(CONFIG_REGISTER),y)
CFLAGS += -DHAVE_REGISTER
endif
ifeq ($(CONFIG_ALLNETWRT),y)
CFLAGS += -DHAVE_ALLNETWRT
endif

ifeq ($(CONFIG_SILENCE),y)
  CFLAGS += -DHAVE_SILENCE
endif
ifeq ($(CONFIG_BUFFALO),y)
  CFLAGS += -DHAVE_BUFFALO
endif
ifeq ($(CONFIG_DEFAULT_COUNTRYCODE),JP)
  CFLAGS += -DBUFFALO_JP
endif
ifeq ($(CONFIG_SAMBA),y)
  CFLAGS += -DHAVE_SAMBA
endif
ifeq ($(CONFIG_NOTRIAL),y)
CFLAGS += -DHAVE_NOTRIAL
endif
ifeq ($(CONFIG_RT2880),y)
  CFLAGS += -DHAVE_RT2880
endif
ifeq ($(CONFIG_RT61),y)
  CFLAGS += -DHAVE_RT61
endif
ifeq ($(CONFIG_ECB9750),y)
  CFLAGS += -DHAVE_ECB9750
endif
ifeq ($(CONFIG_TECHNAXX3G),y)
  CFLAGS += -DHAVE_TECHNAXX3G
endif
ifeq ($(CONFIG_CORENET),y)
  CFLAGS += -DHAVE_CORENET
endif
ifeq ($(CONFIG_WHRHPGN),y)
CFLAGS += -DHAVE_WHRHPGN
endif
ifeq ($(CONFIG_WZRG300NH),y)
  CFLAGS += -DHAVE_WZRG300NH
endif
ifeq ($(CONFIG_WZR300HP),y)
  CFLAGS += -DHAVE_WZR300HP
endif
ifeq ($(CONFIG_3G),y)
  CFLAGS += -DHAVE_3G
endif
ifeq ($(CONFIG_ERC),y)
CFLAGS += -DHAVE_ERC
endif
ifeq ($(CONFIG_MI424WR),y)
CFLAGS += -DHAVE_MI424WR
endif
ifeq ($(CONFIG_XIOCOM),y)
  CFLAGS += -DHAVE_XIOCOM
endif
ifeq ($(CONFIG_WHRG300N),y)
  CFLAGS += -DHAVE_WHRG300N
endif
ifeq ($(CONFIG_ESR6650),y)
  CFLAGS += -DHAVE_ESR6650
endif
ifeq ($(CONFIG_JJAP93),y)
CFLAGS += -DHAVE_JJAP93
endif
ifeq ($(CONFIG_GEMTEK),y)
  CFLAGS += -DHAVE_GEMTEK
endif
ifeq ($(CONFIG_TONZE),y)
  CFLAGS += -DHAVE_TONZE
endif
ifeq ($(CONFIG_WAVESAT),y)
  CFLAGS += -DHAVE_WAVESAT
endif
ifeq ($(CONFIG_GLAUCO),y)
CFLAGS += -DHAVE_GLAUCO
endif

ifeq ($(CONFIG_RAMSKOV),y)
  CFLAGS += -DHAVE_RAMSKOV
endif

ifeq ($(CONFIG_SNMP),y)
  CFLAGS += -DHAVE_SNMP
endif

ifeq ($(REGDOMAIN_OVERRIDE),y)
CFLAGS += -DREGDOMAIN_OVERRIDE
endif

ifeq ($(CONFIG_RB500),y)
  CFLAGS += -DHAVE_RB500
endif
ifeq ($(CONFIG_WG302),y)
CFLAGS += -DHAVE_WG302
endif
ifeq ($(CONFIG_XSCALE),y)
  CFLAGS += -DHAVE_XSCALE
endif
ifeq ($(CONFIG_GATEWORX),y)
  CFLAGS += -DHAVE_GATEWORX
endif
ifeq ($(CONFIG_NOP8670),y)
  CFLAGS += -DHAVE_NOP8670
endif
ifeq ($(CONFIG_X86),y)
  CFLAGS += -DHAVE_X86
endif
ifeq ($(CONFIG_NORTHSTAR),y)
  CFLAGS += -DHAVE_NORTHSTAR
endif
ifeq ($(CONFIG_MAGICBOX),y)
  CFLAGS += -DHAVE_MAGICBOX
endif
ifeq ($(CONFIG_MR3202A),y)
  CFLAGS += -DHAVE_MR3202A
  CFLAGS += -DHAVE_FONERA
endif
ifeq ($(CONFIG_WRK54G),y)
  CFLAGS += -DHAVE_WRK54G
endif
ifeq ($(CONFIG_WRT54GV7),y)
  CFLAGS += -DHAVE_WRT54GV7
endif
ifeq ($(CONFIG_AR430W),y)
  CFLAGS += -DHAVE_AR430W
endif
ifeq ($(CONFIG_WRT300NV2),y)
  CFLAGS += -DHAVE_WRT300NV2
endif
ifeq ($(CONFIG_DIR400),y)
  CFLAGS += -DHAVE_DIR400
  CFLAGS += -DHAVE_FONERA
else
ifeq ($(CONFIG_DIR300),y)
  CFLAGS += -DHAVE_DIR300
  CFLAGS += -DHAVE_FONERA
endif
endif
ifeq ($(CONFIG_MYNETWAY),y)
  CFLAGS += -DHAVE_MYNETWAY
endif
ifeq ($(CONFIG_FONERA),y)
  CFLAGS += -DHAVE_FONERA
endif
ifeq ($(CONFIG_TRIMAX),y)
  CFLAGS += -DHAVE_TRIMAX
endif
ifeq ($(CONFIG_WIKINGS),y)
  CFLAGS += -DHAVE_WIKINGS
endif
ifeq ($(CONFIG_ESPOD),y)
  CFLAGS += -DHAVE_ESPOD
endif
ifeq ($(CONFIG_CARLSONWIRELESS),y)
  CFLAGS += -DHAVE_CARLSONWIRELESS
endif
ifeq ($(CONFIG_IMMERSIVE),y)
  CFLAGS += -DHAVE_IMMERSIVE
endif
ifeq ($(CONFIG_MERAKI),y)
  CFLAGS += -DHAVE_MERAKI
endif
ifeq ($(CONFIG_LS2),y)
  CFLAGS += -DHAVE_LS2
endif
ifeq ($(CONFIG_NS2),y)
  CFLAGS += -DHAVE_NS2
endif
ifeq ($(CONFIG_NS5),y)
  CFLAGS += -DHAVE_NS5
endif
ifeq ($(CONFIG_EOC5610),y)
  CFLAGS += -DHAVE_EOC5610
endif
ifeq ($(CONFIG_NS3),y)
  CFLAGS += -DHAVE_NS3
endif
ifeq ($(CONFIG_PS2),y)
  CFLAGS += -DHAVE_PS2
endif
ifeq ($(CONFIG_PS5),y)
  CFLAGS += -DHAVE_PS5
endif
ifeq ($(CONFIG_LS5),y)
  CFLAGS += -DHAVE_LS5
endif
ifeq ($(CONFIG_WHRAG108),y)
  CFLAGS += -DHAVE_WHRAG108
endif
ifeq ($(CONFIG_PB42),y)
  CFLAGS += -DHAVE_PB42
endif
ifeq ($(CONFIG_LSX),y)
  CFLAGS += -DHAVE_LSX
endif
ifeq ($(CONFIG_RS),y)
  CFLAGS += -DHAVE_RS
endif
ifeq ($(CONFIG_DANUBE),y)
  CFLAGS += -DHAVE_DANUBE
endif
ifeq ($(CONFIG_STORM),y)
  CFLAGS += -DHAVE_STORM
endif
ifeq ($(CONFIG_ADM5120),y)
  CFLAGS += -DHAVE_ADM5120
endif
ifeq ($(CONFIG_TW6600),y)
  CFLAGS += -DHAVE_TW6600
endif
ifeq ($(CONFIG_CA8),y)
  CFLAGS += -DHAVE_CA8
endif
ifeq ($(CONFIG_CA8PRO),y)
  CFLAGS += -DHAVE_CA8PRO
endif
ifeq ($(CONFIG_RCAA01),y)
  CFLAGS += -DHAVE_RCAA01
endif
ifeq ($(CONFIG_RDAT81),y)
  CFLAGS += -DHAVE_RDAT81
endif

ifeq ($(CONFIG_USR5453),y)
  CFLAGS += -DHAVE_USR5453
endif

ifeq ($(CONFIG_CHILLILOCAL),y)
  CFLAGS += -DHAVE_CHILLILOCAL
endif

ifeq ($(CONFIG_PPPOERELAY),y)
  CFLAGS += -DHAVE_PPPOERELAY
endif

ifeq ($(CONFIG_CHILLISPOT),y)
  CFLAGS += -DHAVE_CHILLI
endif

ifeq ($(CONFIG_RFLOW),y)
CFLAGS += -DHAVE_RFLOW
endif

ifeq ($(CONFIG_DDLAN),y)
  CFLAGS += -DHAVE_DDLAN
endif
ifeq ($(CONFIG_CESAR),y)
  CFLAGS += -DHAVE_CESAR
endif


ifeq ($(CONFIG_34TELECOM),y)
  CFLAGS += -DHAVE_34TELECOM
endif

ifeq ($(CONFIG_NOWIFI),y)
CFLAGS += -DHAVE_NOWIFI
endif
ifeq ($(CONFIG_BOESE),y)
CFLAGS += -DBOESE=1
endif
ifeq ($(CONFIG_WILLIAM),y)
CFLAGS += -DWILLIAM=1
endif
ifeq ($(CONFIG_ALPHA),y)
CFLAGS += -DHAVE_ALPHA
endif
ifeq ($(CONFIG_EOC2610),y)
CFLAGS += -DHAVE_EOC2610
endif
ifeq ($(CONFIG_EOC1650),y)
CFLAGS += -DHAVE_EOC1650
endif

ifeq ($(CONFIG_MADWIFI),y)
CFLAGS += -DHAVE_MADWIFI -I../services/networking -I../madwifi.dev/madwifi.dev -include ../madwifi.dev/madwifi.dev/include/compat.h -I../wireless-tools -DHEADERS_KERNEL
OBJS += ../wireless-tools/libiw.so.29 
endif
#OBJS += countrycode/countrycode_$(ARCH).o

ifeq ($(CONFIG_TELCOM),y)
CFLAGS += -DHAVE_TELCOM
endif

ifeq ($(CONFIG_GGEW),y)
  CFLAGS += -DHAVE_NEWMEDIA
  CFLAGS += -DHAVE_GGEW
endif


ifeq ($(CONFIG_SAGAR),y)
  CFLAGS += -DHAVE_SAGAR
endif

ifeq ($(CONFIG_OPENVPN),y)
  CFLAGS += -DHAVE_OPENVPN
endif

ifeq ($(CONFIG_THOM),y)
  CFLAGS += -DHAVE_THOM
endif

ifeq ($(CONFIG_CDEBUG),y)
  CFLAGS += -DCDEBUG
endif


ifeq ($(CONFIG_BOOT_WAIT_ON),y)
  CFLAGS += -DSET_BOOT_WAIT
endif

ifeq ($(CONFIG_ANTIFLASH),y)
CFLAGS += -DANTI_FLASH
endif


ifeq ($(CONFIG_MACBIND),y)
CFLAGS += -DHAVE_MACBIND
endif

ifeq ($(CONFIG_OMNI),y)
CFLAGS += -DHAVE_OMNI
endif
ifeq ($(CONFIG_MAKSAT),y)
CFLAGS += -DHAVE_MAKSAT
endif

ifeq ($(CONFIG_MAKSAT_BLANK),y)
CFLAGS += -DHAVE_MAKSAT_BLANK
endif

ifeq ($(CONFIG_VILIM),y)
CFLAGS += -DHAVE_VILIM
endif

ifeq ($(CONFIG_ZEROIP),y)
CFLAGS += -DHAVE_ZEROIP
endif

ifeq ($(CONFIG_BRANDING),y)
CFLAGS += -DCONFIG_BRANDING
endif

ifeq ($(CONFIG_MASKMAC),y)
CFLAGS += -DFBNFW
endif

ifeq ($(CONFIG_OPENSSL),y)
CFLAGS += -DHAVE_HTTPS
endif

ifeq ($(CONFIG_MATRIXSSL),y)
CFLAGS += -DHAVE_HTTPS
CFLAGS += -I$(TOP)/matrixssl
CFLAGS += -DHAVE_MATRIXSSL


endif

ifeq ($(CONFIG_WTS),y)
CFLAGS += -DHAVE_WTS
endif

ifeq ($(CONFIG_DLS),y)
CFLAGS += -DHAVE_DLS
endif

ifeq ($(CONFIG_SKYTRON),y)
CFLAGS += -DHAVE_SKYTRON
endif

ifeq ($(CONFIG_POWERNOC),y)
CFLAGS += -DHAVE_POWERNOC
endif

ifeq ($(CONFIG_POWERNOC_WORT54G),y)
CFLAGS += -DHAVE_POWERNOC_WORT54G
endif

ifeq ($(CONFIG_POWERNOC_WOAP54G),y)
CFLAGS += -DHAVE_POWERNOC_WOAP54G
endif

ifeq ($(CONFIG_SKYTEL),y)
CFLAGS += -DHAVE_SKYTEL
endif
ifeq ($(CONFIG_FON),y)
CFLAGS += -DHAVE_FON
endif
ifeq ($(CONFIG_FONBETA),y)
CFLAGS += -DHAVE_FONBETA
endif

ifeq ($(CONFIG_SPUTNIK_APD),y)
CFLAGS += -DHAVE_SPUTNIK_APD
endif

ifeq ($(CONFIG_UPNP),y)
CFLAGS += -DHAVE_UPNP
endif

ifeq ($(CONFIG_SVQOS),y)
CFLAGS += -DHAVE_SVQOS
endif

CFLAGS += -DHAVE_AQOS

ifeq ($(CONFIG_NVRAM_60K),y)
CFLAGS += -DHAVE_NVRAM_60K
endif
ifeq ($(CONFIG_NVRAM_64K),y)
CFLAGS += -DHAVE_NVRAM_64K
endif

include $(TOP)/rules/generic.mk
CFLAGS_EXTRA += $(DD_CFLAGS)

ifeq ($(CONFIG_MATRIXSSL),y)
#CFLAGS += -I$(TOP)/matrixssl
#LDFLAGS += -L$(TOP)/matrixssl/src -lmatrixssl
CFLAGS += -DHAVE_MATRIXSSL
endif
HFLAGS := $(CFLAGS)
CFLAGS += $(COPTS)
all: 
	@true

install: all

clean:
#	rm -f countrycode/countrycode_$(ARCH).o
	rm -f *.o *.so
	rm -f *.a
	rm -f *.h~
	rm -f *.c~

libshared.so: $(OBJS)
#	$(CC) -shared -o $@ $(CFLAGS) $(OBJS) 
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -o $@ $^ -L../nvram -L../libutils -lutils -lcrypt -lnvram

libshared.a: $(OBJS)
	$(AR) arc -o  $@ $^

*.o: $(CY_DEPS)
