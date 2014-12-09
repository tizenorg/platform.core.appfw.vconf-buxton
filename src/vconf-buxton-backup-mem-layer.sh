#!/bin/bash
#
#     This script backup the buxton system layer temp that is
#     in memory to the system layer base. All keys are copied
#     prefixed with memory_init/ .
#
#     See vconf-buxton-restore-mem-layer.sh
#
# author: jose.bollo@open.eurogiciel.org

layermem=temp
layerdb=base
groupmem=vconf
groupdb=vconf
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
# check that buxton service is available
#
if ! buxtonctl -s check > /dev/null
then
    echo "${red}ERROR: can not connect to buxton service${reset}"
    exit 1
fi

#
# create the group for vconf
#
if buxtonctl -s list-groups "$layerdb" | grep -q "found group $groupdb"
then
    echo "${green}group $groupdb already exists${reset}"
elif buxtonctl -s create-group "$layerdb" "$groupdb"
then
    echo "${green}group $groupdb succesfully created${reset}"
else
    echo "${red}ERROR: can't create group '$groupdb' for layer '$layerdb'${reset}"
    exit 1
fi

#
# Ensure label for the group
#
if buxtonctl -s set-label "$layerdb" "$groupdb" "$label"
then
    echo "${green}group $groupdb succesfully set to label '$label'${reset}"
else
    echo "${red}ERROR: can't set label '$label' to group '$groupdb' for layer '$layerdb'${reset}"
    exit 1
fi


buxtonctl -s list-keys "$layermem" "$groupmem" |
sed 's:^found key ::' |
while read keymem
do
    keydb="memory_init/$keymem"
    if ! q=$(buxtonctl -s get "$layermem" "$groupmem" "$keymem")
    then
        echo "${red}ERROR can't get value of $keymem${reset}"
    else
        type=$(echo -n "$q" | sed 's/.* = \([^:]*\): .*/\1/')
        value=$(echo -n "$q" | sed 's/.* = [^:]*: \(.*\)/\1/')
        echo -n "${reset}setting $keydb, $type: $value ..."
        if ! buxtonctl -s -- set-$type "$layerdb" "$groupdb" "$keydb" "$value"
        then
            echo "${red}ERROR WHILE SETTING VALUE${reset}"
        elif ! buxtonctl -s set-label "$layerdb" "$groupdb" "$keydb" "$label"
        then
            echo "${red}ERROR WHILE SETTING LABEL${reset}"
        else
            echo "${green}done${reset}"
        fi
    fi
done

