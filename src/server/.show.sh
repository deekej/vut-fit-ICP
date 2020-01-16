#!/bin/bash

PIDs="$(pidof "$1")"

if [[ -z "$PIDs" ]]; then
  exit 0
fi

ps $PIDs

exit 0
