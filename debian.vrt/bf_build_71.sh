#!/bin/sh

curl -s https://packagecloud.io/install/repositories/varnishcache/varnish60lts/script.deb.sh | sudo bash
sudo apt-get install varnish=6.0.7-1~`lsb_release -cs` varnish-dev

