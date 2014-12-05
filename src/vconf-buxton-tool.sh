#!/bin/bash
#
#     This script emulate the behaviour of vconftool
#
# author: jose.bollo@open.eurogiciel.org

verbose=false
quiet=false

#
# prompting
#
if tty --silent
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

info() {
  $verbose && echo "${green}$*${reset}"
  return 0
}

warning() {
  $quiet || echo "${yellow}WARNING: $*${reset}"
  return 0
}

error() {
  $quiet || echo "${red}ERROR: $*${reset}" >&2
  return 1
}

badargs() {
  error "bad arguments"
  exit
}

#
# calls to buxton
#
buxton() {
  buxtonctl "$@"
}

buxton_is_ready() {
  buxton check > /dev/null
}

buxton_has_group() {
  local layer="$1" group="$2"
  buxton list-groups "${layer}" 2>/dev/null |
  grep -q "found group ${group}\$"
}

buxton_has_key() {
  local layer="$1" group="$2" name="$3"
  buxton list-keys "${layer}" "${group}" 2>/dev/null |
  grep -q "found key ${name}\$"
}

buxton_make_group() {
  local layer="$1" group="$2"
  if buxton create-group "$layer" "$group" > /dev/null
  then
    info "group $group succesfully created for layer $layer"
  else
    error "can not create group $group for layer $layer"
  fi
}

buxton_get_label() {
  if [[ $# -eq 2 ]]
  then
    local layer="$1" group="$2" result=
    if ! result=$(buxton get-label "$layer" "$group" 2> /dev/null | 
                  grep "\[$layer] $group:(null) - " |
                  sed 's:.* - ::')
    then
      error "can not get the label $label of group $group of layer $layer"
    elif [[ -z "$result" ]]
    then
      error "invalid label gotten for group $group of layer $layer"
    else
      echo -n "$result"
    fi
  else
    local layer="$1" group="$2" name="$3" result=
    if ! result=$(buxton get-label "$layer" "$group" "$name" 2> /dev/null | 
                  grep "\[$layer] $group:$name - " |
                  sed 's:.* - ::')
    then
      error "can not get the label $label of key $name in group $group of layer $layer"
    elif [[ -z "$result" ]]
    then
      error "invalid label gotten for key $name in group $group of layer $layer"
    else
      echo -n "$result"
    fi
  fi
}

buxton_set_label() {
  if [[ $# -eq 3 ]]
  then
    local layer="$1" group="$2" label="$3"
    if ! buxton set-label "$layer" "$group" "$label" > /dev/null
    then
      error "can not set label $label to group $group of layer $layer"
    elif [[ "$label" != "$(buxton_get_label "$layer" "$group")" ]]
    then
      error "check failed when setting label $label to group $group of layer $layer"
    else
      info "label $label set to group $group of layer $layer"
    fi
  else
    local layer="$1" group="$2" name="$3" label="$4"
    if ! buxton set-label "$layer" "$group" "$name" "$label" > /dev/null
    then
      error "can not set label $label to key $name in group $group of layer $layer"
    elif [[ "$label" != "$(buxton_get_label "$layer" "$group" "$name")" ]]
    then
      error "check failed when setting label $label to key $name in group $group of layer $layer"
    else
      info "label $label set to key $name in group $group of layer $layer"
    fi
  fi
}

buxton_ensure_group() {
  local layer="$1" group="$2" label="$3"
  if buxton_has_group "$layer" "$group"
  then
    info "group $group exists in layer $layer"
  else
    buxton_make_group "$layer" "$group" || return
  fi
  [[ -z "$label" ]] || buxton_set_label "$layer" "$group" "$label"
}

buxton_ensure_ready() {
  if ! buxton_is_ready
  then
    error "can not connect to buxton service"
    exit
  fi
}

buxton_unset() {
  local layer="$1" group="$2" name="$3"
  
  # unset the value
  if ! buxton_has_key "$layer" "$group" "$name"
  then
    info "key $name in group $group of layer $layer is already unset"
  elif ! buxton "unset-value" "$layer" "$group" "$name" > /dev/null
  then
    error "can not unset key $name in group $group of layer $layer"
  elif buxton_has_key "$layer" "$group" "$name"
  then
    error "check failed when unsetting key $name in group $group of layer $layer"
  else
    info "key $name in group $group of layer $layer is unset"
  fi
  exit
}

#############################################################################################

group=vconf


# get the layer of the key
layer_of_key() {
  case "$1/" in
  user/*) echo -n "user";;
  memory/*) echo -n "temp";;
  *) echo -n "base";;
  esac
}

# get the standard value
stdval() {
  local typ="$1" val="$2"
  case "$typ:${val,,}" in
  bool:0|bool:false|bool:off) echo -n "false";;
  bool:1|bool:true|bool:on) echo -n "true";;
  *) echo -n "$val";;
  esac
}

# get buxton-type from vconf-type
v2btype() {
  case "${1,,}" in
  int) echo -n "int32";;
  string) echo -n "string";;
  double) echo -n "double";;
  bool) echo -n "bool";;
  *) error "unknown vconf-type $1"; exit;;
  esac
}

# get vconf-type from buxton-type
b2vtype() {
  case "${1,,}" in
  int32) echo -n "int";;
  string) echo -n "string";;
  double) echo -n "double";;
  bool) echo -n "bool";;
  *) error "unknown buxton-type $1"; exit;;
  esac
}

#
# ensure existing the group for vconf
#
buxton_ensure_group "base" "$group" || exit

# set the value
doset() {
  local typ= name= layer= value= smack= force=false

  # scan arguments
  while [[ $# -ne 0 ]]
  do
    case "$1" in
    -t|--type)
      [[ $# -ge 2 && -z "$typ" ]] || badargs
      typ="$2"
      shift 2
      ;;
    -s|--smack)
      [[ $# -ge 2 && -z "$smack" ]] || badargs
      smack="$2"
      shift 2
      ;;
    -f|--force)
      force=true
      shift
      ;;
    -i|--install)
      warning "option $1 ignored!"
      shift
      ;;
    -u|-g|--user|--group)
      [[ $# -ge 2 ]] || badargs
      warning "option $1 $2 ignored!"
      shift 2
      ;;
    *)
      [[ $# -ge 2 && -z "$name" ]] || badargs
      name="$1"
      value="$2" 
      shift 2
      ;;
    esac
  done
  [[ -n "$typ" && -n "$name" ]] || badargs

  # process
  layer="$(layer_of_key "$name")"
  typ="$(v2btype "$typ")"
  value="$(stdval "$typ" "$value")"
  
  if buxton "set-$typ" "$layer" "$group" "$name" "$value" > /dev/null
  then
    info "key $name in group $group of layer $layer set to $typ: $value"
  else
    error "can not set key $name in group $group of layer $layer with $typ: $value"
  fi
  exit
}

# get the value
doget() {
  local name= layer= rec=false val=

  # scan arguments
  while [[ $# -ne 0 ]]
  do
    case "$1" in
    -r|--recursive)
      rec=true
      shift
      ;;
    *)
      [[ $# -eq 1 && -n "$1" ]] || badargs
      name="$1"
      shift
      ;;
    esac
  done
  [[ -n "$name" ]] || badargs

  # process
  layer="$(layer_of_key "$name")"
  if $rec
  then
    set -- $(buxton list-keys "$layer" "$group" "$name" 2> /dev/null |
             grep "found key" |
             sed 's:.* ::')
  else
    set -- "$name"
  fi
  for name
  do
    val="$(buxton get "$layer" "$group" "$name" 2> /dev/null |
         grep "\[$layer] $group:$name = " |
         sed 's/.* = // ; s/^int32:/int:/ ; s/^\(.*\): \(.*\)$/\2 (\1)/')"
    if [[ -z "$val" ]]
    then
      error "key $name not found in group $group of layer $layer"
    else
      echo "$name, value = $val"
    fi
  done
  exit
}

# unset the value
dounset() {
  local name= layer=

  # scan arguments
  [[ $# -eq 1 && -n "$name" ]] || badargs
  name="$1"
  layer="$(layer_of_key "$name")"

  # process
  buxton_unset "$layer" "$group" "$name"
  exit
}

# set the label
dolabel() {
  local name= smack= layer=
  
  # scan arguments
  [[ $# -eq 2 && -n "$name" && -n "$smack" ]] || badargs
  name="$1"
  smack="$2"
  layer="$(layer_of_key "$name")"

  # process
  buxton_set_label "$layer" "$group" "$name"
  exit
}






exe="$(basename "$0")"
cmd="${1,,}"
shift

case "${cmd}" in
-v|--verbose) verbose=true; cmd="${1,,}"; shift;;
-q|--quiet) quiet=true; cmd="${1,,}"; shift;;
esac

case "${cmd}" in
get) doget "$@";;
set) doset "$@";;
unset) dounset "$@";;
label) dolabel "$@";;
help|-h|--help) cat << EOC

Usage: $exe [-v|--verbose|-q|--quiet] command ...

Command set: set a value (create or update)

   $exe set -t <TYPE> <KEY-NAME> <VALUE> <OPTIONS>

       <TYPE> = int | bool | double | string

       <OPTIONS>

          -u, --user    <UID>    ignored! (compatibility)
          -g, --group   <GID>    ignored! (compatibility)
          -i, --install          ignored! (compatibility)
          -s, --smack   <LABEL>  tells to set the security label <LABEL>
          -f, --force            tells force updating the value

       Ex) $exe set -t string db/testapp/key1 "This is test" 

Command get: get a value

   $exe get <OPTIONS> <KEY-NAME>

       <OPTIONS>

          -r, --recursive        retrieve all keys having the given prefix

       Ex) $exe get db/testapp/key1
           $exe get -r db/testapp/

Command unset: remove a value

   $exe unset <KEY-NAME>

       Ex) $exe unset db/testapp/key1

Command label: set the security label

   $exe label <KEY-NAME> <SMACK-LABEL>

       Ex) $exe label db/testapp/key1 User::Share

EOC
esac

