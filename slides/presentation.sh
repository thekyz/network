#!/bin/bash

_port=80

echo "Open '127.0.0.1:${_port}' in a web browser to see the slides ..."
echo ""

cd "$(dirname "${BASH_SOURCE[0]}")"
sudo python -m SimpleHTTPServer ${_port}
