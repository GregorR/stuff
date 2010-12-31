CURL=curl -O
TAR=tar
CURLVER=7.19.7

all: lddcapture-curl.d/inst/bin/lddcapture-curl

lddcapture-curl.d/inst/bin/lddcapture-curl:
	./lddcapture lddcapture-curl.d lddcapture-curl

lddcapture-curl: curl-$(CURLVER)/src/curl
	cp curl-$(CURLVER)/src/curl lddcapture-curl

curl-$(CURLVER)/src/curl: curl-$(CURLVER)
	cd curl-$(CURLVER) ; \
	    sh ../curlconfigure ; \
	    $(MAKE)

curl-$(CURLVER): curl-$(CURLVER).tar.lzma
	$(TAR) xf curl-$(CURLVER).tar.lzma

curl-$(CURLVER).tar.lzma:
	$(CURL) http://curl.haxx.se/download/curl-7.19.7.tar.lzma
