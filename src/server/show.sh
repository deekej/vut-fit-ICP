#!/bin/bash

PIDs="$(pidof "./mazed")"

if [[ -z "$PIDs" ]]; then
  exit 0
fi

ps $PIDs

exit 0
