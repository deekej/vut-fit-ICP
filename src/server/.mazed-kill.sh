#!/bin/bash

kill $(pidof "$1") &> /dev/null

SUCCESS="$(./.show.sh)"

if [[ -n "$SUCCESS" ]]; then
  kill -9 $(pidof "$1") &> /dev/null
fi

exit 0
