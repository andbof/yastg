#!/bin/sh

git log -1 | awk '/^commit\ /{print $NF}'
