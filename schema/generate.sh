#!/usr/bin/env bash

mydir=$(dirname $(realpath $BASH_SOURCE))
srcdir=$(dirname $mydir)

set -x

render () {
    local name="$1" ; shift
    local What="$1" ; shift
    local outdir="${1:-$srcdir/src/${name}}"
    local what="$(echo $What | tr '[:upper:]' '[:lower:]')"
    local tmpl="o${what}.hpp.j2"
    local outhpp="$outdir/${What}.hpp"
    mkdir -p $outdir
    moo -g '/lang:ocpp.jsonnet' -J $mydir  \
        -A path="dunedaq.listrev.${name}" \
        -A ctxpath="dunedaq.listrev" \
        -A os="listrev-${name}-schema.jsonnet" \
           render omodel.jsonnet $tmpl \
           > $outhpp
    echo $outhpp
}

render rdlg Structs
render rdlg Nljs

