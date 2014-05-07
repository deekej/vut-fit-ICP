#!/bin/bash

kill $(pidof "./mazed") &> /dev/null

SUCCESS="$(./show.sh)"

if [[ -n "$SUCCESS" ]]; then
  kill -9 $(pidof "./mazed") &> /dev/null
fi

exit 0
