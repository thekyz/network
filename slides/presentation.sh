#!/bin/bash

echo "Open '127.0.0.1:8000' in a web browser to see the slides ..."
echo ""

cd "$(dirname "${BASH_SOURCE[0]}")"/slides
python -m SimpleHTTPServer
