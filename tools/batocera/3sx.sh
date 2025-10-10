#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"
export LD_LIBRARY_PATH="/usr/lib:/lib:$DIR/lib"
export DISPLAY=:0
exec "$DIR/3sx" "$@"
