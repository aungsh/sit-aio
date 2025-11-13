#!/bin/sh
set -eu
set -o pipefail

sudo iptables -I INPUT -j ACCEPT
podman run -d --rm -p8080:80 vercel-proxy
loginctl enable-linger
