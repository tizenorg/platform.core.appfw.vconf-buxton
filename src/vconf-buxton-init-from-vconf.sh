#!/bin/bash
#
#     This script initialize the buxton database with the values
#     from the vconf database.
#
# author: jose.bollo@open.eurogiciel.org

export LANG=
root=/usr/kdb

layer=base
group=vconf
label=User

#
# prompting
#
if [[ -t 1 ]]
then
  reset=$(printf '\e[0m')
  red=$(printf '\e[1;31m')
  yellow=$(printf '\e[1;33m')
  green=$(printf '\e[1;32m')
else
  reset=
  red=
  yellow=
  green=
fi

#
# create the default group for vconf
#
if buxtonctl list-groups "$layer" | grep -q "found group $group"
then
    echo "${green}group $group already exists${reset}"
elif buxtonctl create-group "$layer" "$group"
then
    echo "${green}group $group succesfully created${reset}"
else
    echo "${red}ERROR: can't create group '$group' for layer '$layer'${reset}"
    exit 1
fi

#
# Ensure label for the group
#
if buxtonctl set-label "$layer" "$group" "$label"
then
    echo "${green}group $group succesfully set to label '$label'${reset}"
else
    echo "${red}ERROR: can't set label '$label' to group '$group' for layer '$layer'${reset}"
    exit 1
fi

#
# Check existing root
#
if [[ ! -d $root ]]
then
  echo "${green}No legacy vconf data.${reset}"
  exit 0
fi

#
# loop on keys of the vconf file system
#
find $root -type f |
while read file
do
    key=${file#$root/}
    #
    # extract type and value of the key
    #
    ktype=$(head -c4 < $file | od -t d4 | awk '{print $2;exit}')
    case $ktype in
    40) # string
        type=string
        value=$(tail -c+5 < $file | sed 's:\(\\\|"\):\\&:g')
        ;;
    41) # integer
        type=int32
        value=$(tail -c+5 < $file | od -t d4 | awk '{print $2;exit}')
        ;;
    42) # double
        type=double
        value=$(tail -c+5 < $file | od -t f8 | awk '{print $2;exit}')
        ;;
    43) # boolean
        type=bool
        value=$(tail -c+5 < $file | od -t d4 | awk '{print $2;exit}')
        ;;
    *) # not a known type
        echo "${red}ERROR: unknown type $ktype for file $file${reset}"
        continue
    esac
    #
    # compute the layer
    #
    case "${key%%/*}" in
    memory_init|db|file)
        ;;
    *)
        echo "${red}ERROR not a valid key prefix $key${reset}"
        continue
    esac
    #
    # set the key to buxton
    #
    echo -n "${reset}setting [$layer.$group:$type] $key = $value ..."
    if ! buxtonctl -- "set-$type" "$layer" "$group" "$key" "$value"
    then
        echo "${red}ERROR WHILE SETTING VALUE${reset}"
    elif ! buxtonctl set-label "$layer" "$group" "$key" "$label"
    then
        echo "${red}ERROR WHILE SETTING LABEL${reset}"
    else
        echo "${green}done${reset}"
    fi
done

