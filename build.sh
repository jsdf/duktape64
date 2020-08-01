#!/bin/bash
set -eu


DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
rm -f *.o

export ROOT="$HOME/code/n64sdk20l/n64sdk"

source "$HOME/code/n64cc/env.sh"


make $@
