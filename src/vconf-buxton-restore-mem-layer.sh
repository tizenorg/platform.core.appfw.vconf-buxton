#!/bin/bash
#
#     This script initialize the buxton system layer temp (in memory)
#     with values from the system layer base. It searchs the keys of
#     prefix memory_init/ and copy their value to memory.
#
#     See vconf-buxton-backup-mem-layer.sh
#
# author: jose.bollo@open.eurogiciel.org

layerdb=base
layermem=temp
groupdb=vconf
groupmem=vconf
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
# create the group for in memory vconf
#
if buxtonctl -s list-groups "$layermem" | grep -q "found group $groupmem"
then
    echo "${green}group $groupmem already exists${reset}"
elif buxtonctl -s create-group "$layermem" "$groupmem"
then
    echo "${green}group $groupmem succesfully created${reset}"
else
    echo "${red}ERROR: can't create group '$groupmem' for layer '$layermem'${reset}"
    exit 1
fi

#
# Ensure label for the group
#
if buxtonctl -s set-label "$layermem" "$groupmem" "$label"
then
    echo "${green}group $groupmem succesfully set to label '$label'${reset}"
else
    echo "${red}ERROR: can't set label '$label' to group '$groupmem' for layer '$layermem'${reset}"
    exit 1
fi


buxtonctl -s list-keys "$layerdb" "$groupdb" memory_init/ |
sed 's:^found key ::' |
while read keydb
do
    keymem=${keydb#memory_init/}
    if t=$(buxtonctl -s get "$layermem" "$groupmem" "$keymem")
    then
        value=$(echo -n "$t" | sed 's/.* = [^:]*: \(.*\)/\1/')
        echo "${reset}$keymem is already set as $value"
        if ! buxtonctl -s set-label "$layermem" "$groupmem" "$keymem" "$label"
        then
            echo "${red}ERROR WHILE SETTING LABEL${reset}"
        fi
    elif ! q=$(buxtonctl -s get "$layerdb" "$groupdb" "$keydb")
    then
        echo "${red}ERROR can't get value of $keydb${reset}"
    else
        type=$(echo -n "$q" | sed 's/.* = \([^:]*\): .*/\1/')
        value=$(echo -n "$q" | sed 's/.* = [^:]*: \(.*\)/\1/')
        echo -n "${reset}setting $keymem, $type: $value ..."
        if ! buxtonctl -s -- set-$type "$layermem" "$groupmem" "$keymem" "$value"
        then
            echo "${red}ERROR WHILE SETTING VALUE${reset}"
        elif ! buxtonctl -s set-label "$layermem" "$groupmem" "$keymem" "$label"
        then
            echo "${red}ERROR WHILE SETTING LABEL${reset}"
        else
            echo "${green}done${reset}"
        fi
    fi
done

